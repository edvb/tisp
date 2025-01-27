/* zlib License
 *
 * Copyright (c) 2017-2025 Ed van Bruggen
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
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tisp.h"

#define car(P)  ((P)->v.p.car)
#define cdr(P)  ((P)->v.p.cdr)
#define caar(P) car(car(P))
#define cadr(P) car(cdr(P))
#define cdar(P) cdr(car(P))
#define cddr(P) cdr(cdr(P))
#define nilp(V) ((V)->t == TSP_NIL)
#define num(N) ((N)->v.n.num)
#define den(N) ((N)->v.n.den)

#define BETWEEN(X, A, B)  ((A) <= (X) && (X) <= (B))
#define LEN(X)            (sizeof(X) / sizeof((X)[0]))

/* functions */
static void rec_add(Rec rec, char *key, Val val);
static Val eval_proc(Tsp st, Rec env, Val f, Val args);

/* utility functions */

/* return named string for each type */
/* TODO loop through each type bit to print */
char *
tsp_type_str(TspType t)
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
	case TSP_REC:   return "Rec";
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
is_sym(char c)
{
	/* TODO expand for all UTF: if c > 255 */
	return BETWEEN(c, 'a', 'z') || BETWEEN(c, 'A', 'Z') ||
	       BETWEEN(c, '0', '9') || strchr(TSP_SYM_CHARS, c);
}

/* check if character can be a part of an operator */
static int
is_op(char c)
{
	return strchr(TSP_OP_CHARS, c) != NULL;
}

/* check if character is start of a number */
/* TODO only if -/+ is not followed by a delim */
/* TODO use XOR '0' < 10 */
static int
isnum(char *str)
{
	return isdigit(*str) || (*str == '.' &&  isdigit(str[1])) ||
	       ((*str == '-' || *str == '+') && (isdigit(str[1]) || str[1] == '.'));
}

/* skip over comments and white space */
/* TODO support mac/windows line endings */
static void
skip_ws(Tsp st, int skipnl)
{
	const char *s = skipnl ? " \t\n\r" : " \t";
	while (tsp_fget(st) && (strchr(s, tsp_fget(st)) || tsp_fget(st) == ';')) {
		st->filec += strspn(st->file+st->filec, s); /* skip white space */
		for (; tsp_fget(st) == ';'; tsp_finc(st)) /* skip comments until newline */
			st->filec += strcspn(st->file+st->filec, "\n") - !skipnl;
	}
}

/* get length of list, if improper list return negative length */
int
tsp_lstlen(Val v)
{
	int len = 0;
	for (; v->t == TSP_PAIR; v = cdr(v))
		len++;
	return nilp(v) ? len : -(len + 1);
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

/* records */

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

/* create new empty rec with given capacity */
static Rec
rec_new(size_t cap, Rec next)
{
	Rec rec;
	if (!(rec = malloc(sizeof(struct Rec))))
		perror("; malloc"), exit(1);
	rec->size = 0;
	rec->cap = cap;
	if (!(rec->items = calloc(cap, sizeof(struct Entry))))
		perror("; calloc"), exit(1);
	rec->next = next;
	return rec;
}

/* get entry in one record for the key */
static Entry
entry_get(Rec rec, char *key)
{
	int i = hash(key) % rec->cap;
	char *s;
	/* look for key starting at hash until empty entry is found */
	while ((s = rec->items[i].key)) {
		if (!strcmp(s, key))
			break;
		if (++i == rec->cap) /* loop back around if end is reached */
			i = 0;
	}
	return &rec->items[i]; /* returns entry if found or empty one to be filled */
}

/* get value of given key in each record */
static Val
rec_get(Rec rec, char *key)
{
	Entry e;
	for (; rec; rec = rec->next) {
		e = entry_get(rec, key);
		if (e->key)
			return e->val;
	}
	return NULL;
}

/* enlarge the record to ensure algorithm's efficiency */
static void
rec_grow(Rec rec)
{
	int i, ocap = rec->cap;
	Entry oitems = rec->items;
	rec->cap *= TSP_REC_FACTOR;
	if (!(rec->items = calloc(rec->cap, sizeof(struct Entry))))
		perror("; calloc"), exit(1);
	for (i = 0; i < ocap; i++) /* repopulate new record with old values */
		if (oitems[i].key)
			rec_add(rec, oitems[i].key, oitems[i].val);
	free(oitems);
}

/* create new key and value pair to the record */
static void
rec_add(Rec rec, char *key, Val val)
{
	Entry e = entry_get(rec, key);
	e->val = val;
	if (!e->key) {
		e->key = key;
		/* grow record if it is more than half full */
		if (++rec->size > rec->cap / TSP_REC_FACTOR)
			rec_grow(rec);
	}
}

/* add each vals[i] to rec with key args[i] */
static Rec
rec_extend(Rec rec, Val args, Val vals)
{
	Val arg, val;
	int argnum = TSP_REC_FACTOR * tsp_lstlen(args);
	/* HACK need extra +1 for when argnum = 0 */
	Rec ret = rec_new(argnum > 0 ? argnum : -argnum + 1, rec);
	for (; !nilp(args); args = cdr(args), vals = cdr(vals)) {
		if (args->t == TSP_PAIR) {
			arg = car(args);
			val = car(vals);
		} else {
			arg = args;
			val = vals;
		}
		if (arg->t != TSP_SYM)
			tsp_warnf("expected symbol for argument of function definition, recieved '%s'",
			          tsp_type_str(arg->t));
		rec_add(ret, arg->v.s, val);
		if (args->t != TSP_PAIR)
			break;
	}
	return ret;
}

/* make types */

Val
mk_val(TspType t)
{
	Val ret;
	if (!(ret = malloc(sizeof(struct Val))))
		perror("; malloc"), exit(1);
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
	num(ret) = num;
	den(ret) = den;
	return ret;
}

/* TODO combine mk_str and mk_sym, replace st with intern hash */
Val
mk_str(Tsp st, char *s)
{
	Val ret;
	if ((ret = rec_get(st->strs, s)))
		return ret;
	ret = mk_val(TSP_STR);
	ret->v.s = s;
	rec_add(st->strs, s, ret);
	return ret;
}

Val
mk_sym(Tsp st, char *s)
{
	Val ret;
	if ((ret = rec_get(st->syms, s)))
		return ret;
	ret = mk_val(TSP_SYM);
	ret->v.s = s;
	rec_add(st->syms, s, ret);
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
mk_func(TspType t, char *name, Val args, Val body, Rec env)
{
	Val ret = mk_val(t);
	ret->v.f.name = name;
	ret->v.f.args = args;
	ret->v.f.body = body;
	ret->v.f.env  = env;
	return ret;
}

Val
mk_rec(Tsp st, Rec env, Val assoc)
{
	int cap;
	Val v, ret = mk_val(TSP_REC);
	if (!assoc)
		return ret->v.r = env, ret;
	cap = TSP_REC_FACTOR * tsp_lstlen(assoc);
	ret->v.r = rec_new(cap > 0 ? cap : -cap + 1, NULL);
	Rec r = rec_new(4, env);
	rec_add(r, "this", ret);
	for (Val cur = assoc; cur->t == TSP_PAIR; cur = cdr(cur))
		if (car(cur)->t == TSP_PAIR && caar(cur)->t & (TSP_SYM|TSP_STR)) {
			if (!(v = tisp_eval(st, r, cdar(cur)->v.p.car)))
				return NULL;
			rec_add(ret->v.r, caar(cur)->v.s, v);
		} else if (car(cur)->t == TSP_SYM) {
			if (!(v = tisp_eval(st, r, car(cur))))
				return NULL;
			rec_add(ret->v.r, car(cur)->v.s, v);
		} else tsp_warn("Rec: missing key symbol or string");
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
	Val lst;
	va_list argp;
	va_start(argp, n);
	lst = mk_pair(va_arg(argp, Val), st->nil);
	for (Val cur = lst; n > 1; n--, cur = cdr(cur))
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
	/* TODO skip commas */
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
	case 'r': return '\r';
	case 't': return '\t';
	case '\n': return ' ';
	case '\\':
	case '"':
	default:  return c;
	}
}

/* replace all encoded escape characters in string with their actual character */
static char *
esc_str(char *s, int len, int do_esc)
{
	char *pos, *ret;
	if (!(ret = malloc((len+1) * sizeof(char))))
		perror("; malloc"), exit(1);
	for (pos = ret; pos-ret < len; pos++, s++)
		*pos = (*s == '\\' && do_esc) ? esc_char(*(++s)) : *s;
	*pos = '\0';
	return ret;
}

/* return read string or symbol, depending on mk_fn */
static Val
read_str(Tsp st, Val (*mk_fn)(Tsp, char*))
{
	int len = 0;
	char *s = st->file + ++st->filec; /* skip starting open quote */
	char endchar = mk_fn == &mk_str ? '"' : '~';
	for (; tsp_fget(st) != endchar; tsp_finc(st), len++) /* get length of new escaped string */
		if (!tsp_fget(st))
			tsp_warnf("reached end before closing %c", endchar);
		else if (tsp_fget(st) == '\\' && tsp_fgetat(st, -1) != '\\')
			tsp_finc(st); /* skip over break condition since it is escaped */
	tsp_finc(st); /* skip last closing quote */
	return mk_fn(st, esc_str(s, len, mk_fn == &mk_str)); /* only escape strings */
}

/* return read symbol */
static Val
read_sym(Tsp st, int (*is_char)(char))
{
	int len = 0;
	char *s = st->file + st->filec;
	for (; tsp_fget(st) && is_char(tsp_fget(st)); tsp_finc(st))
		len++; /* get length of new symbol */
	return mk_sym(st, esc_str(s, len, 0));
}

/* return read list, pair, or improper list */
Val
read_pair(Tsp st, char endchar)
{
	Val v, ret = mk_pair(NULL, st->nil);
	int skipnl = endchar != '\n';
	skip_ws(st, skipnl);
	for (Val pos = ret; tsp_fget(st) && tsp_fget(st) != endchar; pos = cdr(pos)) {
		if (!(v = tisp_read(st)))
			return NULL;
		/* pair cdr, end with non-nil (improper list) */
		if (v->t == TSP_SYM && !strncmp(v->v.s, ".", 2)) {
			skip_ws(st, skipnl);
			if (!(v = tisp_read(st)))
				return NULL;
			cdr(pos) = v;
			break;
		}
		cdr(pos) = mk_pair(v, st->nil);
		skip_ws(st, skipnl);
	}
	skip_ws(st, skipnl);
	if (skipnl && tsp_fget(st) != endchar)
		tsp_warnf("did not find closing '%c'", endchar);
	tsp_finc(st);
	return cdr(ret);
}

/* reads given string returning its tisp value */
Val
tisp_read_sexpr(Tsp st)
{
	/* TODO merge w/ infix */
	/* TODO mk const global */
	static char *prefix[] = {
		"'",  "quote",
		"`",  "quasiquote",
		",@", "unquote-splice", /* always check before , */
		",",  "unquote",
		"@",  "Func",
		"f\"",  "strformat",
		/* "?",  "try?", */
		/* "$",  "system!", */
		/* "-",  "negative", */
		/* "!",  "not?", */
	};
	skip_ws(st, 1);
	if (strlen(st->file+st->filec) == 0) /* empty list */
		return st->none;
	if (isnum(st->file+st->filec)) /* number */
		return read_num(st);
	if (tsp_fget(st) == '"') /* string */
		return read_str(st, mk_str);
	if (tsp_fget(st) == '~') /* explicit symbol */
		return read_str(st, mk_sym);
	for (int i = 0; i < LEN(prefix); i += 2) { /* character prefix */
		if (!strncmp(st->file+st->filec, prefix[i], strlen(prefix[i]))) {
			Val v;
			tsp_fincn(st, strlen(prefix[i]) - (prefix[i][1] == '"'));
			if (!(v = tisp_read(st))) return NULL;
			return mk_list(st, 2, mk_sym(st, prefix[i+1]), v);
		}
	}
	if (is_op(tsp_fget(st))) /* operators */
		return read_sym(st, &is_op);
	if (is_sym(tsp_fget(st))) /* symbols */
		return read_sym(st, &is_sym);
	if (tsp_fget(st) == '(') /* list */
		return tsp_finc(st), read_pair(st, ')');
	if (tsp_fget(st) == '[') /* list */
		return tsp_finc(st), mk_pair(mk_sym(st, "list"), read_pair(st, ']'));
	if (tsp_fget(st) == '{') { /* record */
		Val v; tsp_finc(st);
		if (!(v = read_pair(st, '}'))) return NULL;
		return mk_pair(mk_sym(st, "Rec"), v);
	}
	tsp_warnf("could not read given input '%c' (%d)",
	          st->file[st->filec], (int)st->file[st->filec]);
}

/* read single value, made up of s-expression and optional syntax sugar */
Val
tisp_read(Tsp st)
{
	Val v;
	if (!(v = tisp_read_sexpr(st)))
		return NULL;
	/* HACK find more general way to do this */
	while (tsp_fget(st) == '(' || tsp_fget(st) == ':' || tsp_fget(st) == '>' ||
	       tsp_fget(st) == '{')
		v = tisp_read_sugar(st, v);
	return v;
}

/* read extra syntax sugar on top of s-expressions */
Val
tisp_read_sugar(Tsp st, Val v)
{
	Val lst, w;
	if (tsp_fget(st) == '(') { /* func(x y) => (func x y) */
		/* FIXME @it(3) */
		tsp_finc(st);
		if (!(lst = read_pair(st, ')'))) return NULL;
		return mk_pair(v, lst);
	} else if (tsp_fget(st) == '{') { /* rec{ key: value } => (recmerge rec { key: value }) */
		tsp_finc(st);
		if (!(lst = read_pair(st, '}'))) return NULL;
		return mk_list(st, 3, mk_sym(st, "recmerge"), v,
		                      mk_pair(mk_sym(st, "Rec"), lst));
	} else if (tsp_fget(st) == ':') {
		tsp_finc(st);
		switch (tsp_fget(st)) {
		case '(': /* proc:(lst) => (map proc lst) */
			tsp_finc(st);
			if (!(w = read_pair(st, ')'))) return NULL;
			return mk_pair(mk_sym(st, "map"), mk_pair(v, w));
		case ':': /* var::prop => (var 'prop) */
			tsp_finc(st);
			if (!(w = read_sym(st, &is_sym))) return NULL;
			return mk_list(st, 2, v, mk_list(st, 2, mk_sym(st, "quote"), w));
		default: /* key: val => (key val) */
			skip_ws(st, 1);
			if (!(w = tisp_read(st))) return NULL;
			return mk_list(st, 2, v, w);
		}
	} else if (tsp_fget(st) == '>' && tsp_fgetat(st, 1) == '>') {
		tsp_finc(st), tsp_finc(st);
		if (!(w = tisp_read(st)) || w->t != TSP_PAIR)
			tsp_warn("invalid UFCS");
		return mk_pair(car(w), mk_pair(v, cdr(w)));
	}
	/* return mk_pair(v, tisp_read(st)); */
	return v;
}

Val
tisp_read_line(Tsp st, int level)
{
	Val pos, ret;
	if (!(ret = read_pair(st, '\n'))) /* read line */
		return NULL;
	if (ret->t != TSP_PAIR) /* force to be pair */
		ret = mk_pair(ret, st->nil);
	for (pos = ret; cdr(pos)->t == TSP_PAIR; pos = cdr(pos)) ; /* get last pair */
	for (; tsp_fget(st); pos = cdr(pos)) { /* read indented lines as sub expressions */
		int newlevel = strspn(st->file+st->filec, "\t ");
		if (newlevel <= level)
			break;
		st->filec += newlevel;
		cdr(pos) = mk_pair(tisp_read_line(st, newlevel), cdr(pos));
	}
	return nilp(cdr(ret)) ? car(ret) : ret; /* if only 1 element in list, return just it */
}

/* eval */

/* evaluate each element of list */
/* TODO arg for tisp_eval or expand_macro */
Val
tisp_eval_list(Tsp st, Rec env, Val v)
{
	Val ret = mk_pair(NULL, st->nil), ev;
	for (Val cur = ret; !nilp(v); v = cdr(v), cur = cdr(cur)) {
		if (v->t != TSP_PAIR) { /* last element in improper list */
			if (!(ev = tisp_eval(st, env, v)))
				return NULL;
			cdr(cur) = ev;
			return cdr(ret);
		}
		if (!(ev = tisp_eval(st, env, car(v))))
			return NULL;
		cdr(cur) = mk_pair(ev, st->nil);
	}
	return cdr(ret);
}

/* evaluate all elements of list returning last */
Val
tisp_eval_body(Tsp st, Rec env, Val v)
{
	Val ret = st->none;
	for (; v->t == TSP_PAIR; v = cdr(v))
		if (nilp(cdr(v)) && car(v)->t == TSP_PAIR) { /* func call is last, do tail call */
			Val f, args;
			if (!(f = tisp_eval(st, env, caar(v))))
				return NULL;
			if (f->t != TSP_FUNC)
				return eval_proc(st, env, f, cdar(v));
			tsp_arg_num(cdar(v), f->v.f.name ? f->v.f.name : "anon",
			            tsp_lstlen(f->v.f.args));
			if (!(args = tisp_eval_list(st, env, cdar(v))))
				return NULL;
			if (!(env = rec_extend(f->v.f.env, f->v.f.args, args)))
				return NULL;
			v = mk_pair(NULL, f->v.f.body); /* continue loop from body of func call */
		} else if (!(ret = tisp_eval(st, env, car(v))))
			return NULL;
	return ret;
}

static void
prepend_bt(Tsp st, Rec env, Val f)
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
eval_proc(Tsp st, Rec env, Val f, Val args)
{
	Val ret;
	Rec fenv;
	/* evaluate function and primitive arguments before being passed */
	switch (f->t) {
	case TSP_PRIM:
		if (!(args = tisp_eval_list(st, env, args)))
			return NULL;
		/* FALLTHROUGH */
	case TSP_FORM:
		return (*f->v.pr.pr)(st, env, args);
	case TSP_FUNC:
		if (!(args = tisp_eval_list(st, env, args)))
			return NULL;
		/* FALLTHROUGH */
	case TSP_MACRO:
		tsp_arg_num(args, f->v.f.name ? f->v.f.name : "anon", tsp_lstlen(f->v.f.args));
		if (!(fenv = rec_extend(f->v.f.env, f->v.f.args, args)))
			return NULL;
		if (!(ret = tisp_eval_body(st, fenv, f->v.f.body)))
			return prepend_bt(st, env, f), NULL;
		if (f->t == TSP_MACRO) /* TODO remove w/ expand_macro */
			ret = tisp_eval(st, env, ret);
		return ret;
	case TSP_REC:
		if (!(args = tisp_eval_list(st, env, args)))
			return NULL;
		tsp_arg_num(args, "record", 1);
		tsp_arg_type(car(args), "record", TSP_SYM);
		if (!(ret = rec_get(f->v.r, car(args)->v.s)) &&
		    !(ret = rec_get(f->v.r, "else")))
			tsp_warnf("could not find element '%s' in record", car(args)->v.s);
		return ret;
	default:
		tsp_warnf("attempt to evaluate non procedural type %s", tsp_type_str(f->t));
	}
}

/* evaluate given value */
Val
tisp_eval(Tsp st, Rec env, Val v)
{
	Val f;
	switch (v->t) {
	case TSP_SYM:
		if (!(f = rec_get(env, v->v.s)))
			tsp_warnf("could not find symbol '%s'", v->v.s);
		return f;
	case TSP_PAIR:
		if (!(f = tisp_eval(st, env, car(v))))
			return NULL;
		return eval_proc(st, env, f, cdr(v));
	default:
		return v;
	}
}

/* print */

/* main print function */
/* TODO return Str, unify w/ val_string */
void
tisp_print(FILE *f, Val v)
{
	switch (v->t) {
	case TSP_NONE:
		fputs("Void", f);
		break;
	case TSP_NIL:
		fputs("Nil", f);
		break;
	case TSP_INT:
		fprintf(f, "%d", (int) num(v));
		break;
	case TSP_DEC:
		/* TODO fix 1.e-5 print as int 1e-05 */
		fprintf(f, "%.15g", num(v));
		if (num(v) == (int)num(v))
			fprintf(f, ".0");
		break;
	case TSP_RATIO:
		fprintf(f, "%d/%d", (int)num(v), (int)den(v));
		break;
	case TSP_STR:
	case TSP_SYM:
		fputs(v->v.s, f); /* TODO fflush? */
		break;
	case TSP_FUNC:
	case TSP_MACRO:
		/* TODO replace with printing function name with type annotaction */
		fprintf(f, "#<%s%s%s>", /* if proc name is not null print it */
		            v->t == TSP_FUNC ? "function" : "macro",
		            v->v.f.name ? ":" : "", /* TODO dont work for anon funcs */
		            v->v.f.name ? v->v.f.name : "");
		break;
	case TSP_PRIM:
		fprintf(f, "#<primitive:%s>", v->v.pr.name);
		break;
	case TSP_FORM:
		fprintf(f, "#<form:%s>", v->v.pr.name);
		break;
	case TSP_REC:
		putc('{', f);
		for (Rec r = v->v.r; r; r = r->next)
			for (int i = 0, c = 0; c < r->size; i++)
				if (r->items[i].key) {
					c++;
					fprintf(f, " %s: ", r->items[i].key);
					tisp_print(f, r->items[i].val);
				} else if (c == TSP_REC_MAX_PRINT) {
					fputs(" ...", f);
					break;
				}
		fputs(" }", f);
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
		fprintf(stderr, "; tisp: could not print value type %s\n", tsp_type_str(v->t));
	}
	/* fflush(f); */
}

/* environment */

/* add new variable of name key and value v to the given environment */
void
tisp_env_add(Tsp st, char *key, Val v)
{
	rec_add(st->env, key, v);
}

/* initialise tisp's state and global environment */
Tsp
tisp_env_init(size_t cap)
{
	Tsp st;
	if (!(st = malloc(sizeof(struct Tsp))))
		perror("; malloc"), exit(1);

	st->file = NULL;
	st->filec = 0;

	/* TODO intern (memorize) all types, including stateless func calls */
	st->strs = rec_new(cap, NULL);
	st->syms = rec_new(cap, NULL);

	/* TODO make globals */
	st->nil = mk_val(TSP_NIL);
	st->none = mk_val(TSP_NONE);
	st->t = mk_val(TSP_SYM);
	st->t->v.s = "True";

	st->env = rec_new(cap, NULL);
	tisp_env_add(st, "True", st->t);
	tisp_env_add(st, "Nil", st->nil);
	tisp_env_add(st, "Void", st->none);
	tisp_env_add(st, "bt", st->nil);
	tisp_env_add(st, "version", mk_str(st, "0.1"));

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
	skip_ws(st, 1);
	if ((v = tisp_read(st)))
		tisp_eval_body(st, st->env, v);
	st->file = file;
	st->filec = filec;
}


#include "tib/core.c"
#include "tib/string.c"
#include "tib/math.c"
#include "tib/io.c"
#include "tib/os.c"
