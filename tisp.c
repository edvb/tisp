/* zlib License
 *
 * Copyright (c) 2017-2020 Ed van Bruggen
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

/* TODO remove ealloc funcs */
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
/* TODO only if -/+ is not followed by a delim */
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
	/* add .{}[]`', */
	return isspace(c) || c == '(' || c == ')' || c == '"' || c == ';';
}

/* skip over comments and white space */
/* TODO support mac/windows line endings */
static void
skip_ws(Tsp st, int skipnl)
{
	const char *s = skipnl ? " \t\n" : " \t";
	while (tsp_fget(st) && (strchr(s, tsp_fget(st)) || tsp_fget(st) == ';')) {
		st->filec += strspn(st->file+st->filec, s); /* skip white space */
		for (; tsp_fget(st) == ';'; tsp_finc(st)) /* skip comments until newline */
			st->filec += strcspn(st->file+st->filec, "\n") - !skipnl;
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
	ret->v.n.num = num;
	ret->v.n.den = den;
	return ret;
}

/* TODO combine mk_str and mk_sym */
Val
mk_str(Tsp st, char *s)
{
	Val ret;
	if ((ret = hash_get(st->strs, s)))
		return ret;
	ret = emalloc(sizeof(struct Val));
	ret->t = STRING;
	ret->v.s = emalloc((strlen(s)+1) * sizeof(char));
	strcpy(ret->v.s, s);
	hash_add(st->strs, s, ret);
	return ret;
}

Val
mk_sym(Tsp st, char *s)
{
	Val ret;
	if ((ret = hash_get(st->syms, s)))
		return ret;
	ret = emalloc(sizeof(struct Val));
	ret->t = SYMBOL;
	ret->v.s = emalloc((strlen(s)+1) * sizeof(char));
	strcpy(ret->v.s, s);
	hash_add(st->syms, s, ret);
	return ret;
}

Val
mk_prim(Prim pr, char *name)
{
	Val ret = emalloc(sizeof(struct Val));
	ret->t = PRIMITIVE;
	ret->v.pr.name = name;
	ret->v.pr.pr = pr;
	return ret;
}

Val
mk_func(Type t, char *name, Val args, Val body, Hash env)
{
	Val ret = emalloc(sizeof(struct Val));
	ret->t = t;
	ret->v.f.name = name;
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
mk_list(Tsp st, int n, Val *a)
{
	int i;
	Val b = st->nil;
	for (i = n-1; i >= 0; i--)
		b = mk_pair(a[i], b);
	return b;
}

/* read */

/* read first character of number to determine sign */
static int
read_sign(Tsp st)
{
	switch (tsp_fget(st)) {
	case '-': tsp_finc(st); return -1;
	case '+': tsp_finc(st); return 1;
	default: return 1;
	}
}

/* return read integer */
static int
read_int(Tsp st)
{
	int ret = 0;
	for (; tsp_fget(st) && isdigit(tsp_fget(st)); tsp_finc(st))
		ret = ret * 10 + tsp_fget(st) - '0';
	return ret;
}

/* return read scientific notation */
static Val
read_sci(Tsp st, double val, int isint)
{
	if (tolower(tsp_fget(st)) != 'e')
		goto finish;

	tsp_finc(st);
	double sign = read_sign(st) == 1 ? 10.0 : 0.1;
	for (int expo = read_int(st); expo--; val *= sign) ;

finish:
	if (isint)
		return mk_int(val);
	return mk_dec(val);
}

/* return read number */
static Val
read_num(Tsp st)
{
	int sign = read_sign(st);
	int num = read_int(st);
	size_t oldc;
	switch (tsp_fget(st)) {
	case '/':
		if (!isnum(st->file + ++st->filec))
			tsp_warn("incorrect ratio format, no denominator found");
		return mk_rat(sign * num, read_sign(st) * read_int(st));
	case '.':
		tsp_finc(st);
		oldc = st->filec;
		double d = (double) read_int(st);
		int size = st->filec - oldc;
		while (size--)
			d /= 10.0;
		return read_sci(st, sign * (num+d), 0);
	default:
		return read_sci(st, sign * num, 1);
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
read_str(Tsp st)
{
	int len = 0;
	char *s = st->file + ++st->filec; /* skip starting open quote */
	for (; tsp_fget(st) != '"'; tsp_finc(st), len++)
		if (!tsp_fget(st))
			tsp_warn("reached end before closing double quote");
		else if (tsp_fget(st) == '\\' && tsp_fgetat(st, 1) == '"')
			tsp_finc(st), len++;
	tsp_finc(st); /* skip last closing quote */
	s[len] = '\0'; /* TODO remember string length */
	return mk_str(st, esc_str(s));
}

/* return read symbol */
static Val
read_sym(Tsp st)
{
	int n = 1, i = 0;
	char *sym = emalloc(n);
	for (; tsp_fget(st) && issym(tsp_fget(st)); tsp_finc(st)) {
		sym[i++] = tsp_fget(st);
		if (i == n) {
			n *= 2;
			sym = erealloc(sym, n);
		}
	}
	sym[i] = '\0';
	return mk_sym(st, sym);
}

/* return read string containing a list */
/* TODO read pair after as well, allow lambda((x) (* x 2))(4) */
static Val
read_pair(Tsp st)
{
	Val a, b;
	skip_ws(st, 1);
	if (tsp_fget(st) == ')') {
		tsp_finc(st);
		skip_ws(st, 1);
		return st->nil;
	}
	/* TODO simplify read_pair by supporting (. x) => x */
	if (!(a = tisp_read(st)))
		return NULL;
	skip_ws(st, 1);
	if (!tsp_fget(st))
		tsp_warn("reached end before closing ')'");
	if (tsp_fget(st) == '.' && isdelim(tsp_fgetat(st,1))) {
		tsp_finc(st);
		if (!(b = tisp_read(st)))
			return NULL;
		skip_ws(st, 1);
		if (tsp_fget(st) != ')')
			tsp_warn("did not find closing ')'");
		tsp_finc(st);
		skip_ws(st, 1);
	} else {
		if (!(b = read_pair(st)))
			return NULL;
	}
	return mk_pair(a, b);
}

/* reads given string returning its tisp value */
Val
tisp_read(Tsp st)
{
	char *shorthands[] = {
		"'", "quote",
		"`", "quasiquote",
		",", "unquote",
	};
	skip_ws(st, 1);
	if (strlen(st->file+st->filec) == 0)
		return st->none;
	if (isnum(st->file+st->filec))
		return read_num(st);
	/* TODO support | for symbols */
	if (tsp_fget(st) == '"')
		return read_str(st);
	for (int i = 0; i < LEN(shorthands); i += 2) {
		if (tsp_fget(st) == *shorthands[i]) {
			Val v;
			tsp_finc(st);
			if (!(v = tisp_read(st)))
				return NULL;
			return mk_pair(mk_sym(st, shorthands[i+1]),
			               mk_pair(v, st->nil));
		}
	}
	if (issym(tsp_fget(st)))
		return read_sym(st);
	if (tsp_fget(st) == '(') {
		tsp_finc(st);
		return read_pair(st);
	}
	tsp_warnf("could not read given input '%s'", st->file+st->filec);
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
tisp_parse_file(Tsp st, char *fname)
{
	Val ret = mk_pair(st->none, st->nil);
	Val v, last = ret;
	char *file = st->file;
	size_t filec = st->filec;
	if (!(st->file = tisp_read_file(fname)))
		return ret;
	for (st->filec = 0; tsp_fget(st) && (v = tisp_read(st)); last = cdr(last))
		cdr(last) = mk_pair(v, st->nil);
	free(st->file);
	st->file = file;
	st->filec = filec;
	return cdr(ret);
}

/* eval */

/* evaluate each element of list */
Val
tisp_eval_list(Tsp st, Hash env, Val v)
{
	Val cur = mk_pair(NULL, st->none);
	Val ret = cur, ev;
	for (; !nilp(v); v = cdr(v), cur = cdr(cur)) {
		if (v->t != PAIR) {
			if (!(ev = tisp_eval(st, env, v)))
				return NULL;
			cdr(cur) = ev;
			return cdr(ret);
		}
		if (!(ev = tisp_eval(st, env, car(v))))
			return NULL;
		cdr(cur) = mk_pair(ev, st->none);
	}
	cdr(cur) = st->nil;
	return cdr(ret);
}

/* evaluate all elements of list returning last */
Val
tisp_eval_seq(Tsp st, Hash env, Val v)
{
	Val ret = st->none;
	for (; v->t == PAIR; v = cdr(v))
		if (!(ret = tisp_eval(st, env, car(v))))
			return NULL;
	return nilp(v) ? ret : tisp_eval(st, env, v);
}

/* evaluate procedure f of name v with arguments */
static Val
eval_proc(Tsp st, Hash env, Val f, Val args)
{
	Val ret;
	Hash e;
	switch (f->t) {
	case PRIMITIVE:
		return (*f->v.pr.pr)(st, env, args);
	case FUNCTION:
		/* tail call into the function body with the extended env */
		if (!(args = tisp_eval_list(st, env, args)))
			return NULL;
		/* FALLTHROUGH */
	case MACRO:
		tsp_arg_num(args, f->v.f.name ? f->v.f.name : "anonymous",
		            list_len(f->v.f.args));
		e = hash_new(8, f->v.f.env);
		/* TODO call hash_extend in hash_new to know new hash size */
		if (!(hash_extend(e, f->v.f.args, args)))
			return NULL;
		if (!(ret = tisp_eval_seq(st, e, f->v.f.body)))
			return NULL;
		if (f->t == MACRO)
			ret = tisp_eval(st, env, ret);
		return ret;
	default:
		tsp_warnf("attempt to evaluate non procedural type %s", type_str(f->t));
	}
}

/* evaluate given value */
Val
tisp_eval(Tsp st, Hash env, Val v)
{
	Val f;
	switch (v->t) {
	case SYMBOL:
		if (!(f = hash_get(env, v->v.s)))
#ifdef TSP_SYM_RETURN
			return v;
#else
			tsp_warnf("could not find symbol %s", v->v.s);
#endif
		return f;
	case PAIR:
		if (!(f = tisp_eval(st, env, car(v))))
			return NULL;
		return eval_proc(st, env, f, cdr(v));
	default:
		return v;
	}
}

/* print */

/* main print function */
void
tisp_print(FILE *f, Val v)
{
	switch (v->t) {
	case NONE:
		fputs("#<void>", f);
		break;
	case NIL:
		fputs("nil", f);
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
	case FUNCTION:
	case MACRO:
		fprintf(f, "#<%s%s%s>", /* if proc name is not null print it */
		            v->t == FUNCTION ? "function" : "macro",
		            v->v.pr.name ? ":" : "",
		            v->v.pr.name ? v->v.pr.name : "");
		break;
	case PRIMITIVE:
		fprintf(f, "#<primitive:%s>", v->v.pr.name);
		break;
	case PAIR:
		putc('(', f);
		tisp_print(f, car(v));
		for (v = cdr(v); !nilp(v); v = cdr(v)) {
			if (v->t == PAIR) {
				putc(' ', f);
				tisp_print(f, car(v));
			} else {
				fputs(" . ", f);
				tisp_print(f, v);
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
prim_car(Tsp st, Hash env, Val args)
{
	Val v;
	tsp_arg_num(args, "car", 1);
	if (!(v = tisp_eval_list(st, env, args)))
		return NULL;
	tsp_arg_type(car(v), "car", PAIR);
	return caar(v);
}

/* return elements of a list after the first */
static Val
prim_cdr(Tsp st, Hash env, Val args)
{
	Val v;
	tsp_arg_num(args, "cdr", 1);
	if (!(v = tisp_eval_list(st, env, args)))
		return NULL;
	tsp_arg_type(car(v), "cdr", PAIR);
	return cdar(v);
}

/* return new pair */
static Val
prim_cons(Tsp st, Hash env, Val args)
{
	Val v;
	tsp_arg_num(args, "cons", 2);
	if (!(v = tisp_eval_list(st, env, args)))
		return NULL;
	return mk_pair(car(v), cadr(v));
}

/* do not evaluate argument */
static Val
prim_quote(Tsp st, Hash env, Val args)
{
	tsp_arg_num(args, "quote", 1);
	return car(args);
}

/* returns nothing */
static Val
prim_void(Tsp st, Hash env, Val args)
{
	return st->none;
}

/* evaluate argument given */
static Val
prim_eval(Tsp st, Hash env, Val args)
{
	Val v;
	tsp_arg_num(args, "eval", 1);
	if (!(v = tisp_eval(st, env, car(args))))
		return NULL;
	return (v = tisp_eval(st, st->global, v)) ? v : st->none;
}

/* test equality of all values given */
static Val
prim_eq(Tsp st, Hash env, Val args)
{
	Val v;
	if (!(v = tisp_eval_list(st, env, args)))
		return NULL;
	if (nilp(v))
		return st->t;
	for (; !nilp(cdr(v)); v = cdr(v))
		if (!vals_eq(car(v), cadr(v)))
			return st->nil;
	return st->t;
}

/* evaluates all expressions if their conditions are met */
static Val
prim_cond(Tsp st, Hash env, Val args)
{
	Val v, cond;
	for (v = args; !nilp(v); v = cdr(v))
		if (!(cond = tisp_eval(st, env, caar(v))))
			return NULL;
		else if (!nilp(cond)) /* TODO incorporate else directly into cond */
			return tisp_eval_seq(st, env, cdar(v));
	return st->none;
}

/* return type of tisp value */
static Val
prim_type(Tsp st, Hash env, Val args)
{
	Val v;
	tsp_arg_num(args, "type", 1);
	if (!(v = tisp_eval(st, env, car(args))))
		return NULL;
	return mk_str(st, type_str(v->t));
}

/* get a property of given value */
static Val
prim_get(Tsp st, Hash env, Val args)
{
	Val v, prop;
	tsp_arg_num(args, "get", 2);
	if (!(v = tisp_eval(st, env, car(args))))
		return NULL;
	if (!(prop = tisp_eval(st, env, cadr(args))))
		return NULL;
	tsp_arg_type(prop, "get", SYMBOL);
	switch (v->t) {
	case PRIMITIVE:
		if (!strncmp(prop->v.s, "name", 4))
			return mk_str(st, v->v.pr.name);
		break;
	case FUNCTION:
	case MACRO:
		if (!strncmp(prop->v.s, "name", 4))
			return mk_str(st, v->v.f.name);
		if (!strncmp(prop->v.s, "body", 4))
			return v->v.f.body;
		if (!strncmp(prop->v.s, "args", 4))
			return v->v.f.args;
		break;
	case INTEGER:
	case RATIO:
		if (!strncmp(prop->v.s, "num", 3))
			return mk_int(v->v.n.num);
		if (!strncmp(prop->v.s, "den", 3))
			return mk_int(v->v.n.den);
		break;
	case PAIR:
		if (!strncmp(prop->v.s, "car", 3))
			return v->v.p.car;
		if (!strncmp(prop->v.s, "cdr", 3))
			return v->v.p.cdr;
		break;
	case STRING:
	case SYMBOL:
		if (!strncmp(prop->v.s, "len", 3))
			return mk_int(strlen(v->v.s));
	default: break;
	}
	tsp_warnf("get: can not access %s from type %s",
		   prop->v.s, type_str(v->t));
}

/* creates new tisp lambda function */
static Val
prim_lambda(Tsp st, Hash env, Val args)
{
	tsp_arg_min(args, "lambda", 2);
	return mk_func(FUNCTION, NULL, car(args), cdr(args), env);
}

/* creates new tisp defined macro */
static Val
prim_macro(Tsp st, Hash env, Val args)
{
	tsp_arg_min(args, "macro", 2);
	return mk_func(MACRO, NULL, car(args), cdr(args), env);
}

/* creates new variable of given name and value
 * if pair is given as name of variable, creates function with the car as the
 * function name and the cdr the function arguments */
/* TODO if var not func error if more than 2 args */
static Val
prim_define(Tsp st, Hash env, Val args)
{
	Val sym, val;
	tsp_arg_min(args, "define", 2);
	if (car(args)->t == PAIR) { /* create function if given argument list */
		sym = caar(args); /* first element of argument list is function name */
		if (sym->t != SYMBOL)
			tsp_warnf("define: incorrect format,"
			          " expected symbol for function name, received %s",
			          type_str(sym->t));
		val = mk_func(FUNCTION, sym->v.s, cdar(args), cdr(args), env);
	} else if (car(args)->t == SYMBOL) { /* create variable */
		sym = car(args);
		val = tisp_eval(st, env, cadr(args));
	} else tsp_warn("define: incorrect format, no variable name found");
	if (!val)
		return NULL;
	/* set procedure name if it was previously anoymous */
	if (val->t & (FUNCTION|MACRO) && !val->v.f.name)
		val->v.f.name = sym->v.s;
	hash_add(env, sym->v.s, val);
	return st->none;
}

/* set symbol to new value */
static Val
prim_set(Tsp st, Hash env, Val args)
{
	Val val;
	Hash h;
	Entry e = NULL;
	tsp_arg_num(args, "set!", 2);
	tsp_arg_type(car(args), "set!", SYMBOL);
	if (!(val = tisp_eval(st, env, cadr(args))))
		return NULL;
	/* find first occurrence of symbol */
	for (h = env; h; h = h->next) {
		e = entry_get(h, car(args)->v.s);
		if (e->key)
			break;
	}
	if (!e || !e->key)
		tsp_warnf("set!: variable %s is not defined", car(args)->v.s);
	e->val = val;
	return val;
}

/* loads tisp file or C dynamic library */
/* TODO lua like error listing places load looked */
/* TODO only use dlopen if -ldl is given with TIB_DYNAMIC */
/* TODO define load in lisp which calls load-dl */
static Val
prim_load(Tsp st, Hash env, Val args)
{
	Val v;
	void (*tibenv)(Tsp);
	char *name;
	const char *paths[] = {
		"/usr/local/share/tisp/", "/usr/share/tisp/", "./", NULL
	};

	tsp_arg_num(args, "load", 1);
	if (!(v = tisp_eval(st, env, car(args))))
		return NULL;
	tsp_arg_type(v, "load", STRING);

	name = emalloc(PATH_MAX * sizeof(char));
	for (int i = 0; paths[i]; i++) {
		strcpy(name, paths[i]);
		strcat(name, v->v.s);
		strcat(name, ".tsp");
		if (access(name, R_OK) != -1) {
			tisp_eval_seq(st, env, tisp_parse_file(st, name));
			free(name);
			return st->none;
		}
	}

	/* If not tisp file, try loading shared object library */
	st->libh = erealloc(st->libh, (st->libhc+1)*sizeof(void*));

	name = erealloc(name, (strlen(v->v.s)+10) * sizeof(char));
	strcpy(name, "libtib");
	strcat(name, v->v.s);
	strcat(name, ".so");
	if (!(st->libh[st->libhc] = dlopen(name, RTLD_LAZY))) {
		free(name);
		tsp_warnf("load: could not load '%s':\n%s", v->v.s, dlerror());
	}
	dlerror();

	name = erealloc(name, (strlen(v->v.s)+9) * sizeof(char));
	strcpy(name, "tib_env_");
	strcat(name, v->v.s);
	tibenv = dlsym(st->libh[st->libhc], name);
	if (dlerror()) {
		free(name);
		tsp_warnf("load: could not run '%s':\n%s", v->v.s, dlerror());
	}
	(*tibenv)(st);
	free(name);

	st->libhc++;
	return st->none;
}

/* display message and return error */
static Val
prim_error(Tsp st, Hash env, Val args)
{
	Val v;
	if (!(v = tisp_eval_list(st, env, args)))
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
prim_version(Tsp st, Hash env, Val args)
{
	return mk_str(st, "0.0");
}

/* environment */

/* add new variable of name key and value v to the given environment */
void
tisp_env_add(Tsp st, char *key, Val v)
{
	hash_add(st->global, key, v);
}

/* initialise tisp's state and global environment */
Tsp
tisp_env_init(size_t cap)
{
	Tsp st = emalloc(sizeof(struct Tsp));

	st->file = NULL;
	st->filec = 0;

	st->nil = emalloc(sizeof(struct Val));
	st->nil->t = NIL;
	st->none = emalloc(sizeof(struct Val));
	st->none->t = NONE;
	st->t = emalloc(sizeof(struct Val));
	st->t->t = SYMBOL;
	st->t->v.s = "t";

	st->global = hash_new(cap, NULL);
	tisp_env_add(st, "t", st->t);
	tsp_env_fn(car);
	tsp_env_fn(cdr);
	tsp_env_fn(cons);
	tsp_env_fn(quote);
	tsp_env_fn(void);
	tsp_env_fn(eval);
	tsp_env_name_fn(=, eq);
	tsp_env_fn(cond);
	tsp_env_fn(type);
	tsp_env_fn(get);
	tsp_env_fn(lambda);
	tsp_env_fn(macro);
	tsp_env_fn(define);
	tsp_env_name_fn(set!, set);
	tsp_env_fn(load);
	tsp_env_fn(error);
	tsp_env_fn(version);

	st->strs = hash_new(cap, NULL);
	st->syms = hash_new(cap, NULL);

	st->libh = NULL;
	st->libhc = 0;

	return st;
}

void
tisp_env_lib(Tsp st, char* lib)
{
	Val v;
	char *file = st->file;
	size_t filec = st->filec;
	st->file = lib;
	st->filec = 0;
	if ((v = tisp_read(st)))
		tisp_eval_seq(st, st->global, v);
	st->file = file;
	st->filec = filec;
}
