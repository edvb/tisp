/* zlib License
 *
 * Copyright (c) 2017-2019 Ed van Bruggen
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
#include <ctype.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tisp.h"

#define BETWEEN(X, A, B)  ((A) <= (X) && (X) <= (B))
#define LEN(X)            (sizeof(X) / sizeof((X)[0]))

/* functions */
static void hash_add(Hash ht, char *key, Val val);

/* general utility wrappers */

static void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;
	if (!(p = calloc(nmemb, size))) {
		fprintf(stderr, "calloc: ");
		perror(NULL);
		exit(1);
	}
	return p;
}

static void *
emalloc(size_t size)
{
	void *p;
	if (!(p = malloc(size))) {
		fprintf(stderr, "malloc: ");
		perror(NULL);
		exit(1);
	}
	return p;
}

static void *
erealloc(void *p, size_t size)
{
	if (!(p = realloc(p, size))) {
		fprintf(stderr, "realloc: ");
		perror(NULL);
		exit(1);
	}
	return p;
}

/* utility functions */

/* return named string for each type */
/* TODO loop through each type bit to print */
char *
type_str(Type t)
{
	switch (t) {
	case NONE:      return "void";
	case NIL:       return "nil";
	case INTEGER:   return "integer";
	case DECIMAL:   return "decimal";
	case RATIO:     return "ratio";
	case STRING:    return "string";
	case SYMBOL:    return "symbol";
	case PRIMITIVE: return "primitive";
	case FUNCTION:  return "function";
	case MACRO:     return "macro";
	case PAIR:      return "pair";
	default:
		if (t == EXPRESSION)
			return "expression";
		if (t == RATIONAL)
			return "rational";
		if (t & NUMBER)
			return "number";
		return "invalid";
	}
}

/* check if character can be a part of a symbol */
static int
issym(char c)
{
	return BETWEEN(c, 'a', 'z') || BETWEEN(c, 'A', 'Z') ||
	       BETWEEN(c, '0', '9') || strchr("_+-*/=<>!?$&^#@:~", c);
}

/* check if character is start of a number */
static int
isnum(char *str)
{
	return isdigit(*str) || (*str == '.' && isdigit(str[1])) ||
	       ((*str == '-' || *str == '+') && (isdigit(str[1]) || str[1] == '.'));
}

/* check if character is a symbol delimiter */
static char
isdelim(int c)
{
	return isspace(c) || c == '(' || c == ')' || c == '"' || c == ';';
}

/* skip over comments and white space */
static void
skip_ws(Str str, int skipnl)
{
	const char *s = skipnl ? " \t\n" : " \t";
	while (*str->d && (strchr(s, *str->d) || *str->d == ';')) {
		str->d += strspn(str->d, s);     /* skip white space */
		for (; *str->d == ';'; str->d++) /* skip comments until newline */
			str->d += strcspn(str->d, "\n") - !skipnl;
	}
}

/* count number of parenthesis */
static int
count_parens(char *s, int len)
{
	int count = 0;
	for (int i = 0; i < len && s[i]; i++)
		if (s[i] == '(')
			count++;
		else if (s[i] == ')')
			count--;
	return count;
}

/* get length of list, if improper list return -1 */
int
list_len(Val v)
{
	int len = 0;
	for (; v->t == PAIR; v = cdr(v))
		len++;
	return nilp(v) ? len : -1;
}

/* return last element in list */
static Val
list_last(Val v)
{
	while (cdr(v)->t == PAIR)
		v = cdr(v);
	return nilp(cdr(v)) ? car(v) : cdr(v);
}

/* check if two values are equal */
static int
vals_eq(Val a, Val b)
{
	if (a->t & NUMBER && b->t & NUMBER) { /* NUMBERs */
		if (num(a) != num(b) || den(a) != den(b))
			return 0;
		return 1;
	}
	if (a->t != b->t)
		return 0;
	if (a->t == PAIR) /* PAIR */
		return vals_eq(car(a), car(b)) && vals_eq(cdr(a), cdr(b));
	if (a != b) /* PROCEDUREs, STRING, SYMBOL, NIL, VOID */
		return 0;
	return 1;
}

/* reduce fraction by modifying supplied numerator and denominator */
static void
frac_reduce(int *num, int *den)
{
	int a = abs(*num), b = abs(*den), c = a % b;
	while (c > 0) {
		a = b;
		b = c;
		c = a % b;
	}
	*num = *num / b;
	*den = *den / b;
}

/* hash map */

/* return hashed number based on key */
static uint32_t
hash(char *key)
{
	uint32_t h = 0;
	char c;
	/* TODO ULONG_MAX is always bigger than uint32_t */
	while (h < ULONG_MAX && (c = *key++))
		h = h * 33 + c;
	return h;
}

/* create new empty hash table with given capacity */
static Hash
hash_new(size_t cap, Hash next)
{
	if (cap < 1) return NULL;
	Hash ht = emalloc(sizeof(struct Hash));
	ht->size = 0;
	ht->cap = cap;
	ht->items = ecalloc(cap, sizeof(struct Entry));
	ht->next = next;
	return ht;
}

/* get entry in one hash table for the key */
static Entry
entry_get(Hash ht, char *key)
{
	int i = hash(key) % ht->cap;
	char *s;
	while ((s = ht->items[i].key)) {
		if (!strcmp(s, key))
			break;
		if (++i == ht->cap)
			i = 0;
	}
	return &ht->items[i]; /* returns entry if found or empty one to be filled */
}

/* get value of given key in each hash table */
static Val
hash_get(Hash ht, char *key)
{
	Entry e;
	for (; ht; ht = ht->next) {
		e = entry_get(ht, key);
		if (e->key)
			return e->val;
	}
	return NULL;
}

/* enlarge the hash table to ensure algorithm's efficiency */
static void
hash_grow(Hash ht)
{
	int i, ocap = ht->cap;
	Entry oitems = ht->items;
	ht->cap *= 2;
	ht->items = ecalloc(ht->cap, sizeof(struct Entry));
	for (i = 0; i < ocap; i++) /* repopulate new hash table with old values */
		if (oitems[i].key)
			hash_add(ht, oitems[i].key, oitems[i].val);
	free(oitems);
}

/* create new key and value pair to the hash table */
static void
hash_add(Hash ht, char *key, Val val)
{
	Entry e = entry_get(ht, key);
	e->val = val;
	if (!e->key) {
		e->key = key;
		ht->size++;
		if (ht->size > ht->cap / 2) /* grow table if it is more than half full */
			hash_grow(ht);
	}
}

/* add each binding args[i] -> vals[i] */
/* args and vals are both lists */
/* TODO don't return anything in hash_extend since it's modifying ht */
static Hash
hash_extend(Hash ht, Val args, Val vals)
{
	Val arg, val;
	for (; !nilp(args); args = cdr(args), vals = cdr(vals)) {
		if (args->t == PAIR) {
			arg = car(args);
			val = car(vals);
		} else {
			arg = args;
			val = vals;
		}
		if (arg->t != SYMBOL)
			tsp_warnf("expected symbol for argument of function"
				  " definition, recieved %s", type_str(arg->t));
		hash_add(ht, arg->v.s, val);
		if (args->t != PAIR)
			break;
	}
	return ht;
}

/* clean up hash table */
static void
hash_free(Hash ht)
{
	for (Hash h = ht; h; h = h->next) {
		for (int i = 0; i < h->cap; i++)
			if (h->items[i].key)
				free(h->items[i].val);
		free(h->items);
	}
	free(ht);
}

/* make types */

Val
mk_int(int i)
{
	Val ret = emalloc(sizeof(struct Val));
	ret->t = INTEGER;
	num(ret) = i;
	den(ret) = 1;
	return ret;
}

Val
mk_dec(double d)
{
	Val ret = emalloc(sizeof(struct Val));
	ret->t = DECIMAL;
	num(ret) = d;
	den(ret) = 1;
	return ret;
}

Val
mk_rat(int num, int den)
{
	if (den == 0)
		tsp_warn("division by zero");
	frac_reduce(&num, &den);
	if (den < 0) { /* simplify so only numerator is negative */
		den = abs(den);
		num = -num;
	}
	if (den == 1) /* simplify into integer if denominator is 1 */
		return mk_int(num);
	Val ret = emalloc(sizeof(struct Val));
	ret->t = RATIO;
	ret->v.n = (Ratio){ num, den };
	return ret;
}

Val
mk_str(Env env, char *s)
{
	Val ret;
	if ((ret = hash_get(env->strs, s)))
		return ret;
	ret = emalloc(sizeof(struct Val));
	ret->t = STRING;
	ret->v.s = emalloc((strlen(s)+1) * sizeof(char));
	strcpy(ret->v.s, s);
	hash_add(env->strs, s, ret);
	return ret;
}

Val
mk_sym(Env env, char *s)
{
	Val ret;
	if ((ret = hash_get(env->syms, s)))
		return ret;
	ret = emalloc(sizeof(struct Val));
	ret->t = SYMBOL;
	ret->v.s = emalloc((strlen(s)+1) * sizeof(char));
	strcpy(ret->v.s, s);
	hash_add(env->syms, s, ret);
	return ret;
}

Val
mk_prim(Prim pr)
{
	Val ret = emalloc(sizeof(struct Val));
	ret->t = PRIMITIVE;
	ret->v.pr = pr;
	return ret;
}

Val
mk_func(Type t, Val args, Val body, Env env)
{
	Val ret = emalloc(sizeof(struct Val));
	ret->t = t;
	ret->v.f.args = args;
	ret->v.f.body = body;
	ret->v.f.env  = env;
	return ret;
}

Val
mk_pair(Val a, Val b)
{
	Val ret = emalloc(sizeof(struct Val));
	ret->t = PAIR;
	car(ret) = a;
	cdr(ret) = b;
	return ret;
}

Val
mk_list(Env env, int n, Val *a)
{
	int i;
	Val b = env->nil;
	for (i = n-1; i >= 0; i--)
		b = mk_pair(a[i], b);
	return b;
}

/* read */

/* read first character of number to determine sign */
static int
read_sign(Str str)
{
	switch (*str->d) {
	case '-': ++str->d; return -1;
	case '+': ++str->d; return 1;
	default: return 1;
	}
}

/* return read integer */
static int
read_int(Str str)
{
	int ret;
	for (ret = 0; isdigit(*str->d); str->d++)
		ret = ret * 10 + *str->d - '0';
	return ret;
}

/* return read scientific notation */
static Val
read_sci(Str str, double val, int isint)
{
	if (tolower(*str->d) != 'e')
		goto finish;

	str->d++;
	double sign = read_sign(str) == 1 ? 10.0 : 0.1;
	for (int expo = read_int(str); expo--; val *= sign) ;

finish:
	if (isint)
		return mk_int(val);
	return mk_dec(val);
}

/* return read number */
static Val
read_num(Str str)
{
	int sign = read_sign(str);
	int num = read_int(str);
	Str s;
	switch (*str->d) {
	case '/':
		str->d++;
		if (!isnum(str->d))
			tsp_warn("incorrect ratio format, no denominator found");
		return mk_rat(sign * num, read_sign(str) * read_int(str));
	case '.':
		s = emalloc(sizeof(Str));
		s->d = ++str->d;
		double d = (double) read_int(s);
		int size = s->d - str->d;
		str->d = s->d;
		free(s);
		while (size--)
			d /= 10.0;
		return read_sci(str, sign * (num+d), 0);
	default:
		return read_sci(str, sign * num, 1);
	}
}

/* return character for escape */
static char
esc_char(char c)
{
	switch (c) {
	case 'n': return '\n';
	case 't': return '\t';
	case '\\':
	case '"':
	default:
		return c;
	}
}

/* replace all encoded escape characters in string with their actual character */
static char *
esc_str(char *s)
{
	char *c, *ret = emalloc((strlen(s)+1) * sizeof(char));
	for (c = ret; *s != '\0'; c++, s++)
		if (*s == '\\')
			*c = esc_char(*(++s));
		else
			*c = *s;
	*c = '\0';
	return ret;
}

/* return read string */
static Val
read_str(Env env, Str str)
{
	int len = 0;
	char *s = ++str->d; /* skip starting open quote */
	for (; *str->d != '"'; str->d++, len++)
		if (!*str->d)
			tsp_warn("reached end before closing double quote");
		else if (*str->d == '\\' && str->d[1] == '"')
			str->d++, len++;
	str->d++; /* skip last closing quote */
	s[len] = '\0'; /* TODO remember string length */
	return mk_str(env, esc_str(s));
}

/* return read symbol */
static Val
read_sym(Env env, Str str)
{
	int n = 1, i = 0;
	char *sym = emalloc(n);
	for (; *str->d && issym(*str->d); str->d++) {
		sym[i++] = *str->d;
		if (i == n) {
			n *= 2;
			sym = erealloc(sym, n);
		}
	}
	sym[i] = '\0';
	return mk_sym(env, sym);
}

/* return read string containing a list */
static Val
read_pair(Env env, Str str)
{
	Val a, b;
	skip_ws(str, 1);
	if (*str->d == ')') {
		str->d++;
		skip_ws(str, 1);
		return env->nil;
	}
	/* TODO simplify read_pair by supporting (. x) => x */
	if (!(a = tisp_read(env, str)))
		return NULL;
	skip_ws(str, 1);
	if (*str->d == '.' && isdelim(str->d[1])) {
		str->d++;
		if (!(b = tisp_read(env, str)))
			return NULL;
		skip_ws(str, 1);
		if (*str->d != ')')
			tsp_warn("did not find closing ')'");
		str->d++;
		skip_ws(str, 1);
	} else {
		if (!(b = read_pair(env, str)))
			return NULL;
	}
	return mk_pair(a, b);
}

/* reads given string returning its tisp value */
Val
tisp_read(Env env, Str str)
{
	char *shorthands[] = {
		"'", "quote",
		"`", "quasiquote",
		",", "unquote",
	};
	skip_ws(str, 1);
	if (strlen(str->d) == 0)
		return env->none;
	if (isnum(str->d))
		return read_num(str);
	if (*str->d == '"')
		return read_str(env, str);
	for (int i = 0; i < LEN(shorthands); i += 2) {
		if (*str->d == *shorthands[i]) {
			Val v;
			str->d++;
			if (!(v = tisp_read(env, str)))
				return NULL;
			return mk_pair(mk_sym(env, shorthands[i+1]),
			               mk_pair(v, env->nil));
		}
	}
	if (issym(*str->d))
		return read_sym(env, str);
	if (*str->d == '(') {
		str->d++;
		return read_pair(env, str);
	}
	tsp_warnf("could not read given input '%s'", str->d);
}

/* return string containing contents of file name */
char *
tisp_read_file(char *fname)
{
	char buf[BUFSIZ], *file = NULL;
	int len = 0, n, fd, parens = 0;
	if (!fname)
		fd = 0;
	else if ((fd = open(fname, O_RDONLY)) < 0)
		tsp_warnf("could not load file '%s'", fname);
	while ((n = read(fd, buf, sizeof(buf))) > 0) {
		file = erealloc(file, len + n + 1);
		memcpy(file + len, buf, n);
		len += n;
		file[len] = '\0';
		if (fd == 0 && !(parens += count_parens(buf, n)))
			break;
	}
	if (fd)
		close(fd);
	if (n < 0)
		tsp_warnf("could not read file '%s'", fname);
	return file;
}

/* read given file name returning its tisp value */
Val
tisp_parse_file(Env env, char *fname)
{
	struct Str str = { NULL };
	Val ret = mk_pair(NULL, env->nil);
	Val v, last = ret;
	char *file;
	if (!(file = tisp_read_file(fname)))
		return ret;
	for (str.d = file; *str.d && (v = tisp_read(env, &str)); last = cdr(last))
		cdr(last) = mk_pair(v, env->nil);
	free(file);
	return cdr(ret);
}

/* eval */

/* evaluate each element of list */
Val
tisp_eval_list(Env env, Val v)
{
	Val cur = mk_pair(NULL, env->none);
	Val ret = cur, ev;
	for (; !nilp(v); v = cdr(v), cur = cdr(cur)) {
		if (v->t != PAIR) {
			if (!(ev = tisp_eval(env, v)))
				return NULL;
			cdr(cur) = ev;
			return cdr(ret);
		}
		if (!(ev = tisp_eval(env, car(v))))
			return NULL;
		cdr(cur) = mk_pair(ev, env->none);
	}
	cdr(cur) = env->nil;
	return cdr(ret);
}

/* evaluate procedure f of name v with arguments */
static Val
eval_proc(Env env, Val v, Val f, Val args)
{
	Val ret;
	switch (f->t) {
	case PRIMITIVE:
		return (*f->v.pr)(env, args);
	case FUNCTION:
		/* tail call into the function body with the extended env */
		if (!(args = tisp_eval_list(env, args)))
			return NULL;
		/* FALLTHROUGH */
	case MACRO:
		tsp_arg_num(args, v->t == SYMBOL ? v->v.s : "lambda", list_len(f->v.f.args));
		env->h = hash_new(32, env->h);
		if (!(hash_extend(env->h, f->v.f.args, args)))
			return NULL;
		ret = list_last(tisp_eval_list(env, f->v.f.body));
		if (f->t == MACRO)
			ret = tisp_eval(env, ret);
		env->h = env->h->next;
		return ret;
	default:
		tsp_warnf("attempt to evaluate non procedural type %s", type_str(f->t));
	}
}

/* evaluate given value */
Val
tisp_eval(Env env, Val v)
{
	Val f;
	switch (v->t) {
	case SYMBOL:
		if (!(f = hash_get(env->h, v->v.s)))
#ifdef TSP_SYM_RETURN
			return v;
#else
			tsp_warnf("could not find symbol %s", v->v.s);
#endif
		return f;
	case PAIR:
		if (!(f = tisp_eval(env, car(v))))
			return NULL;
		return eval_proc(env, car(v), f, cdr(v));
	default:
		return v;
	}
}

/* print */

/* print value of a list, display as #<void> if element inside returns void */
/* TODO remove need for list_print by using repl to skip printing */
static void
list_print(FILE *f, Val v)
{
	if (v->t == NONE)
		fprintf(f, "#<void>");
	else
		tisp_print(f, v);
}

/* main print function */
void
tisp_print(FILE *f, Val v)
{
	switch (v->t) {
	case NONE:
		break;
	case NIL:
		fprintf(f, "()");
		break;
	case INTEGER:
		fprintf(f, "%d", (int)num(v));
		break;
	case DECIMAL:
		fprintf(f, "%.15g", num(v));
		if (num(v) == (int)num(v))
			fprintf(f, ".0");
		break;
	case RATIO:
		fprintf(f, "%d/%d", (int)num(v), (int)den(v));
		break;
	case STRING:
		fprintf(f, "\"%s\"", v->v.s);
		break;
	case SYMBOL:
		fputs(v->v.s, f);
		break;
	case PRIMITIVE:
		fprintf(f, "#<primitive>");
		break;
	case FUNCTION:
		fprintf(f, "#<function>");
		break;
	case MACRO:
		fprintf(f, "#<macro>");
		break;
	case PAIR:
		putc('(', f);
		list_print(f, car(v));
		for (v = cdr(v); !nilp(v); v = cdr(v)) {
			if (v->t == PAIR) {
				putc(' ', f);
				list_print(f, car(v));
			} else {
				fprintf(f, " . ");
				list_print(f, v);
				break;
			}
		}
		putc(')', f);
		break;
	default:
		fprintf(stderr, "tisp: could not print value type %s\n", type_str(v->t));
	}
}

/* primitives */

/* return first element of list */
static Val
prim_car(Env env, Val args)
{
	Val v;
	tsp_arg_num(args, "car", 1);
	if (!(v = tisp_eval_list(env, args)))
		return NULL;
	tsp_arg_type(car(v), "car", PAIR);
	return caar(v);
}

/* return elements of a list after the first */
static Val
prim_cdr(Env env, Val args)
{
	Val v;
	tsp_arg_num(args, "cdr", 1);
	if (!(v = tisp_eval_list(env, args)))
		return NULL;
	tsp_arg_type(car(v), "cdr", PAIR);
	return cdar(v);
}

/* return new pair */
static Val
prim_cons(Env env, Val args)
{
	Val v;
	tsp_arg_num(args, "cons", 2);
	if (!(v = tisp_eval_list(env, args)))
		return NULL;
	return mk_pair(car(v), cadr(v));
}

/* do not evaluate argument */
static Val
prim_quote(Env env, Val args)
{
	tsp_arg_num(args, "quote", 1);
	return car(args);
}

/* returns nothing */
static Val
prim_void(Env env, Val args)
{
	return env->none;
}

/* evaluate argument given */
static Val
prim_eval(Env env, Val args)
{
	Val v;
	tsp_arg_num(args, "eval", 1);
	if (!(v = tisp_eval(env, car(args))))
		return NULL;
	return (v = tisp_eval(env, v)) ? v : env->none;
}

/* test equality of all values given */
static Val
prim_eq(Env env, Val args)
{
	Val v;
	if (!(v = tisp_eval_list(env, args)))
		return NULL;
	if (nilp(v))
		return env->t;
	for (; !nilp(cdr(v)); v = cdr(v))
		if (!vals_eq(car(v), cadr(v)))
			return env->nil;
	return env->t;
}

/* evaluates all expressions if their conditions are met */
static Val
prim_cond(Env env, Val args)
{
	Val v, cond;
	for (v = args; !nilp(v); v = cdr(v))
		if (!(cond = tisp_eval(env, caar(v))))
			return NULL;
		else if (!nilp(cond)) /* TODO incorporate else directly into cond */
			return tisp_eval(env, car(cdar(v)));
	return env->none;
}

/* return type of tisp value */
static Val
prim_type(Env env, Val args)
{
	Val v;
	tsp_arg_num(args, "type", 1);
	if (!(v = tisp_eval(env, car(args))))
		return NULL;
	return mk_str(env, type_str(v->t));
}

/* creates new tisp lambda function */
static Val
prim_lambda(Env env, Val args)
{
	tsp_arg_min(args, "lambda", 2);
	return mk_func(FUNCTION, car(args), cdr(args), env);
}

/* creates new tisp defined macro */
static Val
prim_macro(Env env, Val args)
{
	tsp_arg_min(args, "macro", 2);
	return mk_func(MACRO, car(args), cdr(args), env);
}

/* creates new variable of given name and value
 * if pair is given as name of variable, creates function with the car as the
 * function name and the cdr the function arguments */
static Val
prim_define(Env env, Val args)
{
	Val sym, val;
	Hash h;
	tsp_arg_min(args, "define", 2);
	if (car(args)->t == PAIR) {
		sym = caar(args);
		if (sym->t != SYMBOL)
			tsp_warnf("define: incorrect format,"
			          " expected symbol for function name, received %s",
			          type_str(sym->t));
		val = mk_func(FUNCTION, cdar(args), cdr(args), env);
	} else if (car(args)->t == SYMBOL) {
		sym = car(args);
		val = tisp_eval(env, cadr(args));
	} else
		tsp_warn("define: incorrect format, no variable name found");
	if (!val)
		return NULL;
	/* last linked hash is global namespace */
	for (h = env->h; h->next; h = h->next) ;
	hash_add(h, sym->v.s, val);
	return env->none;
}

/* set symbol to new value */
static Val
prim_set(Env env, Val args)
{
	Val val;
	Hash h;
	Entry e = NULL;
	tsp_arg_num(args, "set!", 2);
	tsp_arg_type(car(args), "set!", SYMBOL);
	if (!(val = tisp_eval(env, cadr(args))))
		return NULL;
	/* find first occurrence of symbol */
	for (h = env->h; h; h = h->next) {
		e = entry_get(h, car(args)->v.s);
		if (e->key)
			break;
	}
	if (!e || !e->key)
		tsp_warnf("set!: variable %s is not defined", car(args)->v.s);
	e->val = val;
	return env->none;
}

/* loads tisp file or C dynamic library */
/* TODO lua like error listing places load looked */
/* TODO only use dlopen if -ldl is given with TIB_DYNAMIC */
/* TODO define load in lisp which calls load-dl */
static Val
prim_load(Env env, Val args)
{
	Val v;
	void (*tibenv)(Env);
	char *name;
	const char *paths[] = {
#ifdef DEBUG
		"./", /* check working directory first when in debug mode */
#endif
		"/usr/local/share/tisp/", "/usr/share/tisp/", "./", NULL
	};

	tsp_arg_num(args, "load", 1);
	if (!(v = tisp_eval(env, car(args))))
		return NULL;
	tsp_arg_type(v, "load", STRING);

	name = emalloc(PATH_MAX * sizeof(char));
	for (int i = 0; paths[i]; i++) {
		strcpy(name, paths[i]);
		strcat(name, v->v.s);
		strcat(name, ".tsp");
		if (access(name, R_OK) != -1) {
			tisp_eval_list(env, tisp_parse_file(env, name));
			free(name);
			return env->none;
		}
	}

	/* If not tisp file, try loading shared object library */
	env->libh = erealloc(env->libh, (env->libhc+1)*sizeof(void*));

	name = erealloc(name, (strlen(v->v.s)+10) * sizeof(char));
	strcpy(name, "libtib");
	strcat(name, v->v.s);
	strcat(name, ".so");
	if (!(env->libh[env->libhc] = dlopen(name, RTLD_LAZY))) {
		free(name);
		tsp_warnf("load: could not load '%s':\n%s", v->v.s, dlerror());
	}
	dlerror();

	name = erealloc(name, (strlen(v->v.s)+9) * sizeof(char));
	strcpy(name, "tib_env_");
	strcat(name, v->v.s);
	tibenv = dlsym(env->libh[env->libhc], name);
	if (dlerror()) {
		free(name);
		tsp_warnf("load: could not run '%s':\n%s", v->v.s, dlerror());
	}
	(*tibenv)(env);
	free(name);

	env->libhc++;
	return env->none;
}

/* display message and return error */
static Val
prim_error(Env env, Val args)
{
	Val v;
	if (!(v = tisp_eval_list(env, args)))
		return NULL;
	/* TODO have error auto print function name that was pre-defined */
	tsp_arg_min(v, "error", 2);
	tsp_arg_type(car(v), "error", SYMBOL);
	fprintf(stderr, "tisp: error: %s: ", car(v)->v.s);
	for (v = cdr(v); !nilp(v); v = cdr(v))
		if (car(v)->t & STRING) /* don't print quotes around string */
			fprintf(stderr, "%s", car(v)->v.s);
		else
			tisp_print(stderr, car(v));
	fputc('\n', stderr);
	return NULL;
}

/* list tisp version */
static Val
prim_version(Env env, Val args)
{
	return mk_str(env, "0.0");
}

/* environment */

/* add new variable of name key and value v to the given environment */
void
tisp_env_add(Env e, char *key, Val v)
{
	hash_add(e->h, key, v);
}

/* initialise tisp's environment */
Env
tisp_env_init(size_t cap)
{
	Env env = emalloc(sizeof(struct Env));

	env->nil = emalloc(sizeof(struct Val));
	env->nil->t = NIL;
	env->none = emalloc(sizeof(struct Val));
	env->none->t = NONE;
	env->t = emalloc(sizeof(struct Val));
	env->t->t = SYMBOL;
	env->t->v.s = "t";

	env->h = hash_new(cap, NULL);
	tisp_env_add(env, "t", env->t);
	tsp_env_fn(car);
	tsp_env_fn(cdr);
	tsp_env_fn(cons);
	tsp_env_fn(quote);
	tsp_env_fn(void);
	tsp_env_fn(eval);
	tsp_env_name_fn(=, eq);
	tsp_env_fn(cond);
	tsp_env_fn(type);
	tsp_env_fn(lambda);
	tsp_env_fn(macro);
	tsp_env_fn(define);
	tsp_env_name_fn(set!, set);
	tsp_env_fn(load);
	tsp_env_fn(error);
	tsp_env_fn(version);

	env->strs = hash_new(cap, NULL);
	env->syms = hash_new(cap, NULL);

	env->libh = NULL;
	env->libhc = 0;

	return env;
}

void
tisp_env_lib(Env env, char* lib)
{
	struct Str s;
	Val v;
	if (!(s.d = strndup(lib, strlen(lib))))
		return;
	if ((v = tisp_read(env, &s)))
		tisp_eval_list(env, v);
}

void
tisp_env_free(Env env)
{
	int i;

	hash_free(env->h);
	hash_free(env->strs);
	hash_free(env->syms);
	for (i = 0; i < env->libhc; i++)
		dlclose(env->libh[i]);
	free(env->nil);
	free(env->none);
	free(env);
}

void
val_free(Val v)
{
	if (v->t == PAIR) {
		val_free(car(v));
		val_free(cdr(v));
	}
	if (v->t != NIL)
		free(v);
}
