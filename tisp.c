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

/* utility functions */

/* return named string for each type */
/* TODO loop through each type bit to print */
char *
type_str(TspType t)
{
	switch (t) {
	case TSP_NONE:  return "Void";
	case TSP_NIL:   return "Nil";
	case TSP_INT:   return "Int";
	case TSP_DEC:   return "Dec";
	case TSP_RATIO: return "Ratio";
	case TSP_STR:   return "Str";
	case TSP_SYM:   return "Sym";
	case TSP_PRIM:  return "Prim";
	case TSP_FORM:  return "Form";
	case TSP_FUNC:  return "Func";
	case TSP_MACRO: return "Macro";
	case TSP_PAIR:  return "Pair";
	default:
		if (t == TSP_EXPR)
			return "Expr";
		if (t == TSP_RATIONAL)
			return "Rational";
		if (t & TSP_NUM)
			return "Num";
		return "Invalid";
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
	for (; v->t == TSP_PAIR; v = cdr(v))
		len++;
	return nilp(v) ? len : -1;
}

/* check if two values are equal */
static int
vals_eq(Val a, Val b)
{
	if (a->t & TSP_NUM && b->t & TSP_NUM) { /* NUMBERs */
		if (num(a) != num(b) || den(a) != den(b))
			return 0;
		return 1;
	}
	if (a->t != b->t)
		return 0;
	if (a->t == TSP_PAIR) /* PAIR */
		return vals_eq(car(a), car(b)) && vals_eq(cdr(a), cdr(b));
	/* TODO function var names should not matter in comparison */
	if (a->t & (TSP_FUNC | TSP_MACRO)) /* FUNCTION, MACRO */
		return vals_eq(a->v.f.args, b->v.f.args) &&
		       vals_eq(a->v.f.body, b->v.f.body);
	if (a != b) /* PRIMITIVE, STRING, SYMBOL, NIL, VOID */
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
	while (h < UINT32_MAX && (c = *key++))
		h = h * 33 + c;
	return h;
}

/* create new empty hash table with given capacity */
static Hash
hash_new(size_t cap, Hash next)
{
	if (cap < 1) return NULL;
	Hash ht = malloc(sizeof(struct Hash));
	if (!ht) perror("; malloc"), exit(1);
	ht->size = 0;
	ht->cap = cap;
	ht->items = calloc(cap, sizeof(struct Entry));
	if (!ht->items) perror("; calloc"), exit(1);
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
	ht->items = calloc(ht->cap, sizeof(struct Entry));
	if (!ht->items) perror("; calloc"), exit(1);
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
		if (args->t == TSP_PAIR) {
			arg = car(args);
			val = car(vals);
		} else {
			arg = args;
			val = vals;
		}
		if (arg->t != TSP_SYM)
			tsp_warnf("expected symbol for argument of function"
				  " definition, recieved %s", type_str(arg->t));
		hash_add(ht, arg->v.s, val);
		if (args->t != TSP_PAIR)
			break;
	}
	return ht;
}

/* make types */

Val
mk_val(TspType t)
{
	Val ret = malloc(sizeof(struct Val));
	if (!ret) perror("; malloc"), exit(1);
	ret->t = t;
	return ret;
}

Val
mk_int(int i)
{
	Val ret = mk_val(TSP_INT);
	num(ret) = i;
	den(ret) = 1;
	return ret;
}

Val
mk_dec(double d)
{
	Val ret = mk_val(TSP_DEC);
	num(ret) = d;
	den(ret) = 1;
	return ret;
}

Val
mk_rat(int num, int den)
{
	Val ret;
	if (den == 0)
		tsp_warn("division by zero");
	frac_reduce(&num, &den);
	if (den < 0) { /* simplify so only numerator is negative */
		den = abs(den);
		num = -num;
	}
	if (den == 1) /* simplify into integer if denominator is 1 */
		return mk_int(num);
	ret = mk_val(TSP_RATIO);
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
	ret = mk_val(TSP_STR);
	ret->v.s = strndup(s, strlen(s));
	if (!ret->v.s) perror("; strndup"), exit(1);
	hash_add(st->strs, s, ret);
	return ret;
}

Val
mk_sym(Tsp st, char *s)
{
	Val ret;
	if ((ret = hash_get(st->syms, s)))
		return ret;
	ret = mk_val(TSP_SYM);
	ret->v.s = strndup(s, strlen(s));
	if (!ret->v.s) perror("; strndup"), exit(1);
	hash_add(st->syms, s, ret);
	return ret;
}

Val
mk_prim(TspType t, Prim pr, char *name)
{
	Val ret = mk_val(t);
	ret->v.pr.name = name;
	ret->v.pr.pr = pr;
	return ret;
}

Val
mk_func(TspType t, char *name, Val args, Val body, Hash env)
{
	Val ret = mk_val(t);
	ret->v.f.name = name;
	ret->v.f.args = args;
	ret->v.f.body = body;
	ret->v.f.env  = env;
	return ret;
}

Val
mk_pair(Val a, Val b)
{
	Val ret = mk_val(TSP_PAIR);
	car(ret) = a;
	cdr(ret) = b;
	return ret;
}

Val
mk_list(Tsp st, int n, ...)
{
	Val lst, cur;
	va_list argp;
	va_start(argp, n);
	lst = mk_pair(va_arg(argp, Val), st->nil);
	for (cur = lst; n > 1; n--, cur = cdr(cur))
		cdr(cur) = mk_pair(va_arg(argp, Val), st->nil);
	va_end(argp);
	return lst;
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
	char *c, *ret = malloc((strlen(s)+1) * sizeof(char));
	if (!ret) perror("; malloc"), exit(1);
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
	char *sym = malloc(n);
	for (; tsp_fget(st) && issym(tsp_fget(st)); tsp_finc(st)) {
		if (!sym) perror("; alloc"), exit(1);
		sym[i++] = tsp_fget(st);
		if (i == n) {
			n *= 2;
			sym = realloc(sym, n);
		}
	}
	sym[i] = '\0';
	return mk_sym(st, sym);
}

/* return read string containing a list */
/* TODO read pair after as well, allow lambda((x) (* x 2))(4) */
static Val
read_pair(Tsp st, char endchar)
{
	Val a, b;
	skip_ws(st, 1);
	if (tsp_fget(st) == endchar)
		return tsp_finc(st), st->nil;
	/* TODO simplify read_pair by supporting (. x) => x */
	if (!(a = tisp_read(st)))
		return NULL;
	skip_ws(st, 1);
	if (!tsp_fget(st))
		tsp_warnf("reached end before closing '%c'", endchar);
	if (tsp_fget(st) == '.' && isdelim(tsp_fgetat(st,1))) {
		tsp_finc(st);
		if (!(b = tisp_read(st)))
			return NULL;
		skip_ws(st, 1);
		if (tsp_fget(st) != endchar)
			tsp_warnf("did not find closing '%c'", endchar);
		tsp_finc(st);
		skip_ws(st, 1);
	} else {
		if (!(b = read_pair(st, endchar)))
			return NULL;
	}
	return mk_pair(a, b);
}

/* reads given string returning its tisp value */
Val
tisp_read(Tsp st)
{
	char *prefix[] = {
		"'",  "quote",
		"`",  "quasiquote",
		",@", "unquote-splice", /* always check before , */
		",",  "unquote",
		"@",  "Func",
	};
	skip_ws(st, 1);
	if (strlen(st->file+st->filec) == 0) /* empty list */
		return st->none;
	if (isnum(st->file+st->filec)) /* number */
		return read_num(st);
	/* TODO support | for symbols */
	if (tsp_fget(st) == '"') /* strings */
		return read_str(st);
	for (int i = 0; i < LEN(prefix); i += 2) { /* character prefix */
		if (!strncmp(st->file+st->filec, prefix[i], strlen(prefix[i]))) {
			Val v;
			tsp_fincn(st, strlen(prefix[i]));
			if (!(v = tisp_read(st)))
				return NULL;
			return mk_list(st, 2, mk_sym(st, prefix[i+1]), v);
		}
	}
	if (issym(tsp_fget(st))) /* symbols */
		return read_sym(st);
	if (tsp_fget(st) == '(') /* list */
		return tsp_finc(st), read_pair(st, ')');
	tsp_warnf("could not read given input '%c'", st->file[st->filec]);
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
		file = realloc(file, len + n + 1);
		if (!file) perror("; realloc"), exit(1);
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
		if (v->t != TSP_PAIR) {
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
	for (; v->t == TSP_PAIR; v = cdr(v))
		if (!(ret = tisp_eval(st, env, car(v))))
			return NULL;
	return nilp(v) ? ret : tisp_eval(st, env, v);
}

static void
prepend_bt(Tsp st, Hash env, Val f)
{
	if (!f->v.f.name) /* no need to record anonymous functions */
		return;
	for (; env->next; env = env->next) ; /* bt var located at base env */
	Entry e = entry_get(env, "bt");
	if (e->val->t == TSP_PAIR && car(e->val)->t == TSP_SYM &&
	    !strncmp(f->v.f.name, car(e->val)->v.s, strlen(car(e->val)->v.s)))
		return; /* don't record same function on recursion */
	e->val = mk_pair(mk_sym(st, f->v.f.name), e->val);
}

/* evaluate procedure f with arguments */
static Val
eval_proc(Tsp st, Hash env, Val f, Val args)
{
	Val ret;
	Hash e;
	/* evaluate function and primitive arguments before being passed */
	if (f->t & (TSP_FUNC|TSP_PRIM))
		if (!(args = tisp_eval_list(st, env, args)))
			return NULL;
	switch (f->t) {
	case TSP_FORM:
	case TSP_PRIM:
		return (*f->v.pr.pr)(st, env, args);
	case TSP_FUNC:
	case TSP_MACRO:
		tsp_arg_num(args, f->v.f.name ? f->v.f.name : "anon",
		            list_len(f->v.f.args));
		e = hash_new(8, f->v.f.env);
		/* TODO call hash_extend in hash_new to know new hash size */
		if (!(hash_extend(e, f->v.f.args, args)))
			return NULL;
		if (!(ret = tisp_eval_seq(st, e, f->v.f.body)))
			return prepend_bt(st, env, f), NULL;
		if (f->t == TSP_MACRO)
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
	case TSP_SYM:
		if (!(f = hash_get(env, v->v.s)))
			tsp_warnf("could not find symbol %s", v->v.s);
		return f;
	case TSP_PAIR:
		if (!(f = tisp_eval(st, env, car(v))))
			return NULL;
		return eval_proc(st, env, f, cdr(v));
	case TSP_STR: /* TODO string interpolation */
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
	case TSP_NONE:
		fputs("(Void)", f);
		break;
	case TSP_NIL:
		fputs("Nil", f);
		break;
	case TSP_INT:
		fprintf(f, "%d", (int)num(v));
		break;
	case TSP_DEC:
		fprintf(f, "%.15g", num(v));
		if (num(v) == (int)num(v))
			fprintf(f, ".0");
		break;
	case TSP_RATIO:
		fprintf(f, "%d/%d", (int)num(v), (int)den(v));
		break;
	case TSP_STR:
	case TSP_SYM:
		fputs(v->v.s, f);
		break;
	case TSP_FUNC:
	case TSP_MACRO:
		fprintf(f, "#<%s%s%s>", /* if proc name is not null print it */
		            v->t == TSP_FUNC ? "function" : "macro",
		            v->v.pr.name ? ":" : "",
		            v->v.pr.name ? v->v.pr.name : "");
		break;
	case TSP_PRIM:
		fprintf(f, "#<primitive:%s>", v->v.pr.name);
		break;
	case TSP_FORM:
		fprintf(f, "#<form:%s>", v->v.pr.name);
		break;
	case TSP_PAIR:
		putc('(', f);
		tisp_print(f, car(v));
		for (v = cdr(v); !nilp(v); v = cdr(v)) {
			if (v->t == TSP_PAIR) {
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
		fprintf(stderr, "; tisp: could not print value type %s\n", type_str(v->t));
	}
}

/* primitives */

/* return first element of list */
static Val
prim_car(Tsp st, Hash env, Val args)
{
	tsp_arg_num(args, "car", 1);
	tsp_arg_type(car(args), "car", TSP_PAIR);
	return caar(args);
}

/* return elements of a list after the first */
static Val
prim_cdr(Tsp st, Hash env, Val args)
{
	tsp_arg_num(args, "cdr", 1);
	tsp_arg_type(car(args), "cdr", TSP_PAIR);
	return cdar(args);
}

/* return new pair */
static Val
prim_cons(Tsp st, Hash env, Val args)
{
	tsp_arg_num(args, "cons", 2);
	return mk_pair(car(args), cadr(args));
}

/* do not evaluate argument */
static Val
form_quote(Tsp st, Hash env, Val args)
{
	tsp_arg_num(args, "quote", 1);
	return car(args);
}

/* TODO make Void variable like Nil, True, False or like Str, Int ? */
/* returns nothing */
static Val
prim_Void(Tsp st, Hash env, Val args)
{
	return st->none;
}

/* evaluate argument given */
static Val
prim_eval(Tsp st, Hash env, Val args)
{
	Val v;
	tsp_arg_num(args, "eval", 1);
	return (v = tisp_eval(st, st->global, car(args))) ? v : st->none;
}

/* test equality of all values given */
static Val
prim_eq(Tsp st, Hash env, Val args)
{
	if (nilp(args))
		return st->t;
	for (; !nilp(cdr(args)); args = cdr(args))
		if (!vals_eq(car(args), cadr(args)))
			return st->nil;
	return st->t;
}

/* evaluates all expressions if their conditions are met */
static Val
form_cond(Tsp st, Hash env, Val args)
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
prim_typeof(Tsp st, Hash env, Val args)
{
	tsp_arg_num(args, "typeof", 1);
	return mk_str(st, type_str(car(args)->t));
}

/* TODO rename get to getattr like python ? */
/* get a property of given value */
static Val
prim_get(Tsp st, Hash env, Val args)
{
	Val v, prop;
	tsp_arg_num(args, "get", 2);
	v = car(args), prop = cadr(args);
	tsp_arg_type(prop, "get", TSP_SYM);
	switch (v->t) {
	case TSP_FORM:
	case TSP_PRIM:
		if (!strncmp(prop->v.s, "name", 4))
			return mk_sym(st, v->v.pr.name);
		break;
	case TSP_FUNC:
	case TSP_MACRO:
		if (!strncmp(prop->v.s, "name", 4))
			return mk_sym(st, v->v.f.name ? v->v.f.name : "anon");
		if (!strncmp(prop->v.s, "body", 4))
			return v->v.f.body;
		if (!strncmp(prop->v.s, "args", 4))
			return v->v.f.args;
		break;
	case TSP_INT:
	case TSP_RATIO:
		if (!strncmp(prop->v.s, "numerator", 3))
			return mk_int(v->v.n.num);
		if (!strncmp(prop->v.s, "denominator", 3))
			return mk_int(v->v.n.den);
		break;
	case TSP_PAIR: /* TODO get nth element if number */
		if (!strncmp(prop->v.s, "car", 3))
			return v->v.p.car;
		if (!strncmp(prop->v.s, "cdr", 3))
			return v->v.p.cdr;
		break;
	case TSP_STR:
	case TSP_SYM:
		if (!strncmp(prop->v.s, "len", 3))
			return mk_int(strlen(v->v.s));
	default: break;
	}
	tsp_warnf("get: can not access %s from type %s",
		   prop->v.s, type_str(v->t));
}

/* creates new tisp function */
static Val
form_Func(Tsp st, Hash env, Val args)
{
	Val params, body;
	tsp_arg_min(args, "Func", 1);
	if (nilp(cdr(args))) { /* if only 1 argument is given, auto fill func parameters */
		params = mk_pair(mk_sym(st, "it"), st->nil);
		body = args;
	} else {
		params = car(args);
		body = cdr(args);
	}
	return mk_func(TSP_FUNC, NULL, params, body, env);
}

/* creates new tisp defined macro */
static Val
form_Macro(Tsp st, Hash env, Val args)
{
	tsp_arg_min(args, "Macro", 1);
	Val ret = form_Func(st, env, args);
	ret->t = TSP_MACRO;
	return ret;
}

/* creates new variable of given name and value
 * if pair is given as name of variable, creates function with the car as the
 * function name and the cdr the function arguments */
/* TODO if var not func error if more than 2 args */
static Val
form_def(Tsp st, Hash env, Val args)
{
	Val sym, val;
	tsp_arg_min(args, "def", 1);
	if (car(args)->t == TSP_PAIR) { /* create function if given argument list */
		sym = caar(args); /* first element of argument list is function name */
		if (sym->t != TSP_SYM)
			tsp_warnf("def: incorrect format,"
			          " expected symbol for function name, received %s",
			          type_str(sym->t));
		val = mk_func(TSP_FUNC, sym->v.s, cdar(args), cdr(args), env);
	} else if (car(args)->t == TSP_SYM) { /* create variable */
		sym = car(args); /* if only symbol given, make it self evaluating */
		val = nilp(cdr(args)) ? sym : tisp_eval(st, env, cadr(args));
	} else tsp_warn("def: incorrect format, no variable name found");
	if (!val)
		return NULL;
	/* set procedure name if it was previously anonymous */
	if (val->t & (TSP_FUNC|TSP_MACRO) && !val->v.f.name)
		val->v.f.name = sym->v.s;
	hash_add(env, sym->v.s, val);
	return st->none;
}

/* set symbol to new value */
static Val
form_set(Tsp st, Hash env, Val args)
{
	Val val;
	Hash h;
	Entry e = NULL;
	tsp_arg_num(args, "set!", 2);
	tsp_arg_type(car(args), "set!", TSP_SYM);
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

static Val
form_definedp(Tsp st, Hash env, Val args)
{
	Entry e = NULL;
	tsp_arg_min(args, "defined?", 1);
	tsp_arg_type(car(args), "defined?", TSP_SYM);
	for (Hash h = env; h; h = h->next) {
		e = entry_get(h, car(args)->v.s);
		if (e->key)
			break;
	}
	return (!e || !e->key) ? st->nil : st->t;
}

/* loads tisp file or C dynamic library */
/* TODO lua like error listing places load looked */
/* TODO only use dlopen if -ldl is given with TIB_DYNAMIC */
/* TODO define load in lisp which calls load-dl */
static Val
prim_load(Tsp st, Hash env, Val args)
{
	Val tib;
	void (*tibenv)(Tsp);
	char name[PATH_MAX];
	const char *paths[] = {
		"/usr/local/share/tisp/", "/usr/share/tisp/", "./", NULL
	};

	tsp_arg_num(args, "load", 1);
	tib = car(args);
	tsp_arg_type(tib, "load", TSP_STR);

	for (int i = 0; paths[i]; i++) {
		strcpy(name, paths[i]);
		strcat(name, tib->v.s);
		strcat(name, ".tsp");
		if (access(name, R_OK) != -1) {
			tisp_eval_seq(st, env, tisp_parse_file(st, name));
			return st->none;
		}
	}

	/* If not tisp file, try loading shared object library */
	st->libh = realloc(st->libh, (st->libhc+1)*sizeof(void*));
	if (!st->libh) perror("; realloc"), exit(1);

	memset(name, 0, sizeof(name));
	strcpy(name, "libtib");
	strcat(name, tib->v.s);
	strcat(name, ".so");
	if (!(st->libh[st->libhc] = dlopen(name, RTLD_LAZY)))
		tsp_warnf("load: could not load '%s':\n%s", tib->v.s, dlerror());
	dlerror();

	memset(name, 0, sizeof(name));
	strcpy(name, "tib_env_");
	strcat(name, tib->v.s);
	tibenv = dlsym(st->libh[st->libhc], name);
	if (dlerror())
		tsp_warnf("load: could not run '%s':\n%s", tib->v.s, dlerror());
	(*tibenv)(st);

	st->libhc++;
	return st->none;
}

/* display message and return error */
static Val
prim_error(Tsp st, Hash env, Val args)
{
	/* TODO have error auto print function name that was pre-defined */
	tsp_arg_min(args, "error", 2);
	tsp_arg_type(car(args), "error", TSP_SYM);
	/* TODO specify error raised by error func */
	fprintf(stderr, "; tisp: error: %s: ", car(args)->v.s);
	for (args = cdr(args); !nilp(args); args = cdr(args))
		tisp_print(stderr, car(args));
	fputc('\n', stderr);
	return NULL;
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
	Tsp st = malloc(sizeof(struct Tsp));
	if (!st) perror("; malloc"), exit(1);

	st->file = NULL;
	st->filec = 0;

	st->strs = hash_new(cap, NULL);
	st->syms = hash_new(cap, NULL);

	st->nil = mk_val(TSP_NIL);
	st->none = mk_val(TSP_NONE);
	st->t = mk_val(TSP_SYM);
	st->t->v.s = "True";

	st->global = hash_new(cap, NULL);
	tisp_env_add(st, "True", st->t);
	tisp_env_add(st, "Nil", st->nil);
	tisp_env_add(st, "bt", st->nil);
	tisp_env_add(st, "version", mk_str(st, "0.0.0"));
	tsp_env_prim(car);
	tsp_env_prim(cdr);
	tsp_env_prim(cons);
	tsp_env_form(quote);
	tsp_env_prim(Void);
	tsp_env_prim(eval);
	tsp_env_name_prim(=, eq);
	tsp_env_form(cond);
	tsp_env_prim(typeof);
	tsp_env_prim(get);
	tsp_env_form(Func);
	tsp_env_form(Macro);
	tsp_env_form(def);
	tsp_env_name_form(set!, set);
	tsp_env_prim(load);
	tsp_env_name_form(defined?, definedp);
	tsp_env_prim(error);

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
