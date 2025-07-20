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

#include "eevo.h"

struct Eevo_ eevo_nil  = { .t = EEVO_NIL };
struct Eevo_ eevo_true = { .t = EEVO_SYM, .v = { .s = "True" } };
struct Eevo_ eevo_void = { .t = EEVO_NONE };

#define Nil &eevo_nil
#define True &eevo_true
#define Void &eevo_void

#define fst(P)  ((P)->v.p.fst)
#define rst(P)  ((P)->v.p.rst)
#define snd(P)  fst(rst(P))
#define ffst(P) fst(fst(P))
#define rfst(P) rst(fst(P))
#define rrst(P) rst(rst(P))
#define nilp(V) ((V)->t == EEVO_NIL)
#define num(N) ((N)->v.n.num)
#define den(N) ((N)->v.n.den)

#define BETWEEN(X, A, B)  ((A) <= (X) && (X) <= (B))
#define LEN(X)            (sizeof(X) / sizeof((X)[0]))

/* functions */
static void rec_add(EevoRec rec, char *key, Eevo val);
static Eevo eval_proc(EevoSt st, EevoRec env, Eevo f, Eevo args);

/* utility functions */

/* return type of eevo value */
static Eevo
eevo_typeof(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "Type", 1);
	int id = 0;
	for (int i = fst(args)->t; i > 1; i = i >> 1)
		id++;
	return st->types[id];
}

/* return named string for each type */
/* TODO loop through each type bit to print */
char *
eevo_type_str(EevoType t)
{
	switch (t) {
	case EEVO_NONE:  return "Void";
	case EEVO_NIL:   return "Nil";
	case EEVO_INT:   return "Int";
	case EEVO_DEC:   return "Dec";
	case EEVO_RATIO: return "Ratio";
	case EEVO_STR:   return "Str";
	case EEVO_SYM:   return "Sym";
	case EEVO_PRIM:  return "Prim";
	case EEVO_FORM:  return "Form";
	case EEVO_FUNC:  return "Func";
	case EEVO_MACRO: return "Macro";
	case EEVO_PAIR:  return "Pair";
	case EEVO_REC:   return "Rec";
	case EEVO_TYPE:  return "Type";
	case EEVO_RATIONAL: return "Rational";
	case EEVO_NUM:      return "Num";
	case EEVO_EXPR:     return "Expr";
	case EEVO_TEXT:     return "Text";
	case EEVO_PROC:     return "Proc";
	case EEVO_LIT:      return "Lit";
	case EEVO_LIST:     return "List";
	case EEVO_CALLABLE: return "Callable";
	case EEVO_FUNCTOR:  return "Functor";
	default:
		if (t &  EEVO_NUM) return "Num";
		return "Invalid";
	}
}

/* check if character can be a part of a symbol */
static int
is_sym(char c)
{
	/* TODO expand for all UTF: if c > 255 */
	return BETWEEN(c, 'a', 'z') || BETWEEN(c, 'A', 'Z') ||
	       BETWEEN(c, '0', '9') || strchr(EEVO_SYM_CHARS, c);
}

/* check if character can be a part of an operator */
static int
is_op(char c)
{
	return strchr(EEVO_OP_CHARS, c) != NULL;
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

/* skips:
 *  - spaces/tabs
 *  - newlines, if `skipnl` is true
 *  - comments, starting with ';' until end of line */
static void
skip_ws(EevoSt st, int skipnl)
{
	const char *s = skipnl ? " \t\n\r" : " \t";
	while (eevo_fget(st) && (strchr(s, eevo_fget(st)) || eevo_fget(st) == ';')) {
		/* skip contiguous white space */
		st->filec += strspn(st->file+st->filec, s);
		/* skip comments until newline */
		for (; eevo_fget(st) == ';'; eevo_finc(st))
			st->filec += strcspn(st->file+st->filec, "\n") - !skipnl;
	}
}

/* get length of list, if improper list return negative length */
int
eevo_lstlen(Eevo v)
{
	int len = 0;
	for (; v->t == EEVO_PAIR; v = rst(v))
		len++;
	return nilp(v) ? len : -(len + 1);
}

/* check if two values are equal */
static int
vals_eq(Eevo a, Eevo b)
{
	if (a->t & EEVO_NUM && b->t & EEVO_NUM) { /* NUMBERs */
		if (num(a) != num(b) || den(a) != den(b))
			return 0;
		return 1;
	}
	if (a->t != b->t)
		return 0;
	if (a->t == EEVO_PAIR) /* PAIR */
		return vals_eq(fst(a), fst(b)) && vals_eq(rst(a), rst(b));
	/* TODO function var names should not matter in comparison */
	if (a->t & (EEVO_FUNC | EEVO_MACRO)) /* FUNCTION, MACRO */
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
static EevoRec
rec_new(size_t cap, EevoRec next)
{
	EevoRec rec;
	if (!(rec = malloc(sizeof(struct EevoRec_))))
		perror("; malloc"), exit(1);
	rec->size = 0;
	rec->cap = cap;
	if (!(rec->items = calloc(cap, sizeof(struct EevoEntry_))))
		perror("; calloc"), exit(1);
	rec->next = next;
	return rec;
}

/* get entry in one record for the key */
static EevoEntry
entry_get(EevoRec rec, char *key)
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
static Eevo
rec_get(EevoRec rec, char *key)
{
	EevoEntry e;
	for (; rec; rec = rec->next) {
		e = entry_get(rec, key);
		if (e->key)
			return e->val;
	}
	return NULL;
}

/* enlarge the record to ensure algorithm's efficiency */
static void
rec_grow(EevoRec rec)
{
	int i, ocap = rec->cap;
	EevoEntry oitems = rec->items;
	rec->cap *= EEVO_REC_FACTOR;
	if (!(rec->items = calloc(rec->cap, sizeof(struct EevoEntry_))))
		perror("; calloc"), exit(1);
	for (i = 0; i < ocap; i++) /* repopulate new record with old values */
		if (oitems[i].key)
			rec_add(rec, oitems[i].key, oitems[i].val);
	free(oitems);
}

/* create new key and value pair to the record */
static void
rec_add(EevoRec rec, char *key, Eevo val)
{
	EevoEntry e = entry_get(rec, key);
	e->val = val;
	if (!e->key) {
		e->key = key;
		/* grow record if it is more than half full */
		if (++rec->size > rec->cap / EEVO_REC_FACTOR)
			rec_grow(rec);
	}
}

/* add each vals[i] to rec with key args[i] */
static EevoRec
rec_extend(EevoRec next, Eevo args, Eevo vals)
{
	Eevo arg, val;
	int argnum = EEVO_REC_FACTOR * eevo_lstlen(args);
	/* HACK need extra +1 for when argnum = 0 */
	EevoRec ret = rec_new(argnum > 0 ? argnum : -argnum + 1, next);
	for (; !nilp(args); args = rst(args), vals = rst(vals)) {
		if (args->t == EEVO_PAIR) {
			arg = fst(args);
			val = fst(vals);
		} else {
			arg = args;
			val = vals;
		}
		if (arg->t != EEVO_SYM)
			eevo_warnf("expected symbol for argument of function definition, "
			           "recieved '%s'",
			          eevo_type_str(arg->t));
		rec_add(ret, arg->v.s, val);
		if (args->t != EEVO_PAIR)
			break;
	}
	return ret;
}

/* make types */

Eevo
eevo_val(EevoType t)
{
	Eevo ret;
	if (!(ret = malloc(sizeof(struct Eevo_))))
		perror("; malloc"), exit(1);
	ret->t = t;
	return ret;
}

Eevo
eevo_type(EevoSt st, EevoType t, char *name, Eevo func)
{
	Eevo ret = eevo_val(EEVO_INT);
	ret->t = EEVO_TYPE;
	ret->v.t = (EevoTypeVal){ .t = t, .name = name, .func = func };
	/* ret->t = EEVO_TYPE & t; */
	/* ret->v.f = (Func){ .name = name, .func = func }; */
	return ret;
}

Eevo
eevo_int(int i)
{
	Eevo ret = eevo_val(EEVO_INT);
	num(ret) = i;
	den(ret) = 1;
	return ret;
}

Eevo
eevo_dec(double d)
{
	Eevo ret = eevo_val(EEVO_DEC);
	num(ret) = d;
	den(ret) = 1;
	return ret;
}

Eevo
eevo_rat(int num, int den)
{
	Eevo ret;
	if (den == 0)
		eevo_warn("division by zero");
	frac_reduce(&num, &den);
	if (den < 0) { /* simplify so only numerator is negative */
		den = abs(den);
		num = -num;
	}
	if (den == 1) /* simplify into integer if denominator is 1 */
		return eevo_int(num);
	ret = eevo_val(EEVO_RATIO);
	num(ret) = num;
	den(ret) = den;
	return ret;
}

/* TODO combine eevo_str and eevo_sym, replace st with intern hash */
Eevo
eevo_str(EevoSt st, char *s)
{
	Eevo ret;
	if ((ret = rec_get(st->strs, s)))
		return ret;
	ret = eevo_val(EEVO_STR);
	ret->v.s = s;
	rec_add(st->strs, s, ret);
	return ret;
}

Eevo
eevo_sym(EevoSt st, char *s)
{
	Eevo ret;
	if ((ret = rec_get(st->syms, s)))
		return ret;
	ret = eevo_val(EEVO_SYM);
	ret->v.s = s;
	rec_add(st->syms, s, ret);
	return ret;
}

Eevo
eevo_prim(EevoType t, EevoPrim pr, char *name)
{
	Eevo ret = eevo_val(t);
	ret->v.pr.name = name;
	ret->v.pr.pr = pr;
	return ret;
}

Eevo
eevo_func(EevoType t, char *name, Eevo args, Eevo body, EevoRec env)
{
	Eevo ret = eevo_val(t);
	ret->v.f.name = name;
	ret->v.f.args = args;
	ret->v.f.body = body;
	ret->v.f.env  = env;
	return ret;
}

/* TODO swap eevo_rec and rec_new */
Eevo
eevo_rec(EevoSt st, EevoRec prev, Eevo records)
{
	int cap;
	Eevo v, ret = eevo_val(EEVO_REC);
	if (!records)
		return ret->v.r = prev, ret;
	cap = EEVO_REC_FACTOR * eevo_lstlen(records);
	ret->v.r = rec_new(cap > 0 ? cap : -cap + 1, NULL);
	EevoRec r = rec_new(4, prev);
	rec_add(r, "this", ret);
	for (Eevo cur = records; cur->t == EEVO_PAIR; cur = rst(cur))
		if (fst(cur)->t == EEVO_PAIR && ffst(cur)->t & (EEVO_SYM|EEVO_STR)) {
			if (!(v = eevo_eval(st, r, fst(rfst(cur)))))
				return NULL;
			rec_add(ret->v.r, ffst(cur)->v.s, v);
		} else if (fst(cur)->t == EEVO_SYM) {
			if (!(v = eevo_eval(st, r, fst(cur))))
				return NULL;
			rec_add(ret->v.r, fst(cur)->v.s, v);
		} else eevo_warn("Rec: missing key symbol or string");
	return ret;
}

Eevo
eevo_pair(Eevo a, Eevo b)
{
	Eevo ret = eevo_val(EEVO_PAIR);
	fst(ret) = a;
	rst(ret) = b;
	return ret;
}

Eevo
eevo_list(EevoSt st, int n, ...)
{
	Eevo lst;
	va_list argp;
	va_start(argp, n);
	lst = eevo_pair(va_arg(argp, Eevo), Nil);
	for (Eevo cur = lst; n > 1; n--, cur = rst(cur))
		rst(cur) = eevo_pair(va_arg(argp, Eevo), Nil);
	va_end(argp);
	return lst;
}

/* read */

/* read first character of number to determine sign */
static int
read_sign(EevoSt st)
{
	switch (eevo_fget(st)) {
	case '-': eevo_finc(st); return -1;
	case '+': eevo_finc(st); return 1;
	default: return 1;
	}
}

/* return read integer */
static int
read_int(EevoSt st)
{
	char c;
	int ret = 0;
	for (; (c = eevo_fget(st)) && (isdigit(c) || c == '_'); eevo_finc(st))
		if (c != '_')
			ret = ret * 10 + (c - '0');
	return ret;
}

/* return integer read in any base: binary, octal, hexadecimal, etc */
/* TODO error on numbers higher than base (0b2, 0o9, etc) */
static Eevo
read_base(EevoSt st, int base)
{
	char c;
	int ret = 0;
	eevo_fincn(st, 2); /* skip the base signifier prefix (0b, 0o, 0x) */
	for (; (c = eevo_fget(st)) && (isxdigit(c) || c == '_'); eevo_finc(st))
		if (isdigit(c))
			ret = ret * base + (c - '0');
		else if (c != '_')
			ret = ret * base + (tolower(c) - 'a' + 10);
	return eevo_int(ret);
}

/* return read scientific notation */
static Eevo
read_sci(EevoSt st, double val, int isint)
{
	if (tolower(eevo_fget(st)) != 'e')
		goto finish;

	eevo_finc(st);
	double sign = read_sign(st) == 1 ? 10.0 : 0.1;
	for (int expo = read_int(st); expo--; val *= sign) ;

finish:
	if (isint)
		return eevo_int(val);
	return eevo_dec(val);
}

/* return read number */
static Eevo
read_num(EevoSt st)
{
	if (eevo_fget(st) == '0')
		switch (tolower(eevo_fgetat(st, 1))) {
		case 'b': return read_base(st, 2);
		case 'o': return read_base(st, 8);
		case 'x': return read_base(st, 16);
		}
	int sign = read_sign(st);
	int num = read_int(st);
	size_t oldc;
	switch (eevo_fget(st)) {
	case '/':
		if (!isnum(st->file + ++st->filec))
			eevo_warn("incorrect ratio format, no denominator found");
		return eevo_rat(sign * num, read_sign(st) * read_int(st));
	case '.':
		eevo_finc(st);
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

/* return read string or symbol, depending on eevo_fn */
static Eevo
read_str(EevoSt st, Eevo (*eevo_fn)(EevoSt, char*))
{
	int len = 0;
	char *s = st->file + ++st->filec; /* skip starting open quote */
	char endchar = eevo_fn == &eevo_str ? '"' : '~';
	/* get length of new escaped string */
	for (; eevo_fget(st) != endchar; eevo_finc(st), len++)
		if (!eevo_fget(st))
			eevo_warnf("reached end before closing %c", endchar);
		else if (eevo_fget(st) == '\\' && eevo_fgetat(st, -1) != '\\')
			eevo_finc(st); /* skip over break condition since it is escaped */
	eevo_finc(st); /* skip last closing quote */
	return eevo_fn(st, esc_str(s, len, eevo_fn == &eevo_str)); /* only escape strings */
}

/* return read symbol */
static Eevo
read_sym(EevoSt st, int (*is_char)(char))
{
	int len = 0;
	char *s = st->file + st->filec;
	for (; eevo_fget(st) && is_char(eevo_fget(st)); eevo_finc(st))
		len++; /* get length of new symbol */
	return eevo_sym(st, esc_str(s, len, 0));
}

/* return read list, pair, or improper list */
Eevo
read_pair(EevoSt st, char endchar)
{
	Eevo v, ret = eevo_pair(NULL, Nil);
	int skipnl = endchar != '\n';
	skip_ws(st, 1);
	/* if (!eevo_fget(st)) */
	/* 	return Nil; */
		/* eevo_warnf("reached end before closing '%c'", endchar); */
	/* TODO replace w/ strchr to also check for NULL and allow }} */
	/* !strchr(endchars, eevo_fget(st)) */
	for (Eevo pos = ret; eevo_fget(st) && eevo_fget(st) != endchar; pos = rst(pos)) {
		if (!(v = eevo_read(st)))
			return NULL;
		/* pair rest, end with non-nil (improper list) */
		if (v->t == EEVO_SYM && !strncmp(v->v.s, "...", 4)) {
			skip_ws(st, skipnl);
			if (!(v = eevo_read(st)))
				return NULL;
			rst(pos) = v;
			break;
		}
		rst(pos) = eevo_pair(v, Nil);
		/* if (v->t == EEVO_SYM && is_op(v->v.s[0])) { */
		/* 	is_infix = 1; */
		/* 	skip_ws(st, 1); */
		/* } else */
		skip_ws(st, skipnl);
	}
	skip_ws(st, skipnl);
	if (skipnl && eevo_fget(st) != endchar)
		eevo_warnf("did not find closing '%c'", endchar);
		/* eevo_warnf("found more than one element before closing '%c'", endchar); */
	eevo_finc(st);
	/* if (is_infix) */
	/* 	return fst(ret) = eevo_sym(st, "infix"), ret; */
	return rst(ret);
}

/* reads given string returning its eevo value */
Eevo
eevo_read_sexpr(EevoSt st)
{
	/* TODO merge w/ infix */
	/* TODO mk const global */
	static char *prefix[] = {
		"'",   "quote",
		"`",   "quasiquote",
		",@",  "unquote-splice", /* always check before , */
		",",   "unquote",
		"@",   "Func",
		"f\"", "strfmt",
		/* "?",   "try?", */
		/* "$",   "system!", */
		/* "-",   "negative", */
		/* "!",   "not?", */
	};
	skip_ws(st, 1); /* TODO dont skip nl in read */
	/* TODO replace w/ fget? */
	/* if == ] } ) etc say expected value before */
	if (strlen(st->file+st->filec) == 0) /* empty list */
		return Void;
	if (isnum(st->file+st->filec)) /* number */
		return read_num(st);
	if (eevo_fget(st) == '"') /* string */
		return read_str(st, eevo_str);
	if (eevo_fget(st) == '~') /* explicit symbol */
		return read_str(st, eevo_sym);
	for (int i = 0; i < LEN(prefix); i += 2) { /* character prefix */
		if (!strncmp(st->file+st->filec, prefix[i], strlen(prefix[i]))) {
			Eevo v;
			eevo_fincn(st, strlen(prefix[i]) - (prefix[i][1] == '"'));
			if (!(v = eevo_read(st))) return NULL;
			return eevo_list(st, 2, eevo_sym(st, prefix[i+1]), v);
		}
	}
	if (is_op(eevo_fget(st))) /* operators */
		return read_sym(st, &is_op);
	if (is_sym(eevo_fget(st))) /* symbols */
		return read_sym(st, &is_sym);
	if (eevo_fget(st) == '(') /* list */
		return eevo_finc(st), read_pair(st, ')');
	if (eevo_fget(st) == '[') /* list */
		return eevo_finc(st), eevo_pair(eevo_sym(st, "list"), read_pair(st, ']'));
	if (eevo_fget(st) == '{') { /* record */
		Eevo v; eevo_finc(st);
		if (!(v = read_pair(st, '}'))) return NULL;
		return eevo_pair(eevo_sym(st, "Rec"), v);
	}
	eevo_warnf("could not parse given input '%c' (%d)",
	          st->file[st->filec], (int)st->file[st->filec]);
}

/* read single value, made up of s-expression and optional syntax sugar */
Eevo
eevo_read(EevoSt st)
{
	Eevo v;
	if (!(v = eevo_read_sexpr(st)))
		return NULL;
	/* HACK find more general way to do this */
	while (eevo_fget(st) == '(' || eevo_fget(st) == ':' || eevo_fget(st) == '>' ||
	       eevo_fget(st) == '{')
		v = eevo_read_sugar(st, v);
	return v;
}

/* read extra syntax sugar on top of s-expressions */
Eevo
eevo_read_sugar(EevoSt st, Eevo v)
{
	Eevo lst, w;
	if (eevo_fget(st) == '(') { /* func(x y) => (func x y) */
		/* FIXME @it(3) */
		eevo_finc(st);
		if (!(lst = read_pair(st, ')'))) return NULL;
		return eevo_pair(v, lst);
	} else if (eevo_fget(st) == '{') { /* rec{ key: value } => (recmerge rec { key: value }) */
		eevo_finc(st);
		if (!(lst = read_pair(st, '}'))) return NULL;
		return eevo_list(st, 3, eevo_sym(st, "recmerge"), v,
		                      eevo_pair(eevo_sym(st, "Rec"), lst));
	} else if (eevo_fget(st) == ':') {
		eevo_finc(st);
		switch (eevo_fget(st)) {
		case '(': /* proc:(a b c) => (map proc [a b c]) */
			eevo_finc(st);
			if (!(w = read_pair(st, ')'))) return NULL;
			return eevo_list(st, 3, eevo_sym(st, "map"), v,
			                      eevo_pair(eevo_sym(st, "list"), w));
		case ':': /* var::prop => (var 'prop) */
			eevo_finc(st);
			if (!(w = read_sym(st, &is_sym))) return NULL;
			return eevo_list(st, 2, v, eevo_list(st, 2, eevo_sym(st, "quote"), w));
		default: /* key: val => (key val) */
			skip_ws(st, 1);
			if (!(w = eevo_read(st))) return NULL;
			return eevo_list(st, 2, v, w);
		}
	} else if (eevo_fget(st) == '>' && eevo_fgetat(st, 1) == '>') {
		eevo_finc(st), eevo_finc(st);
		if (!(w = eevo_read(st)))
			eevo_warn("invalid UFCS");
		if (w->t != EEVO_PAIR)
			w = eevo_pair(w, Nil);
		return eevo_pair(fst(w), eevo_pair(v, rst(w)));
	}
	/* return eevo_pair(v, eevo_read(st)); */
	return v;
}

/* line reading synax sugar:
 *  - imply parenthesis around new lines
 *  - indented lines are sub-expressions
 *  - lines with single expression return just that expression */
Eevo
eevo_read_line(EevoSt st, int level)
{
	Eevo pos, ret;
	if (!(ret = read_pair(st, '\n'))) /* read line */
		return NULL;
	if (ret->t != EEVO_PAIR) /* force to be pair */
		ret = eevo_pair(ret, Nil);
	for (pos = ret; rst(pos)->t == EEVO_PAIR; pos = rst(pos)) ; /* get last pair */
	for (; eevo_fget(st); pos = rst(pos)) { /* read indented lines as sub-expressions */
		Eevo v;
		int newlevel = strspn(st->file+st->filec, "\t ");
		if (newlevel <= level)
			break;
		st->filec += newlevel;
		/* skip_ws(st, 1); */
		if (!(v = eevo_read_line(st, newlevel)))
			return NULL;
		if (!nilp(v))
			rst(pos) = eevo_pair(v, rst(pos));
	}
	return nilp(rst(ret)) ? fst(ret) : ret; /* if only 1 element in list, return just it */
}

/* eval */

/* evaluate each element of list */
/* TODO arg for eevo_eval or expand_macro */
Eevo
eevo_eval_list(EevoSt st, EevoRec env, Eevo v)
{
	Eevo ret = eevo_pair(NULL, Nil), ev;
	for (Eevo cur = ret; !nilp(v); v = rst(v), cur = rst(cur)) {
		if (v->t != EEVO_PAIR) { /* last element in improper list */
			if (!(ev = eevo_eval(st, env, v)))
				return NULL;
			rst(cur) = ev;
			return rst(ret);
		}
		if (!(ev = eevo_eval(st, env, fst(v))))
			return NULL;
		rst(cur) = eevo_pair(ev, Nil);
	}
	return rst(ret);
}

/* evaluate all elements of list returning last */
Eevo
eevo_eval_body(EevoSt st, EevoRec env, Eevo body)
{
	Eevo ret = Void;
	for (; body->t == EEVO_PAIR; body = rst(body))
		if (nilp(rst(body)) && fst(body)->t == EEVO_PAIR) { /* func call is last, do tail call */
			Eevo f, args;
			if (!(f = eevo_eval(st, env, ffst(body))))
				return NULL;
			if (f->t != EEVO_FUNC)
				return eval_proc(st, env, f, rfst(body));
			eevo_arg_num(rfst(body), f->v.f.name ? f->v.f.name : "anon",
			            eevo_lstlen(f->v.f.args));
			if (!(args = eevo_eval_list(st, env, rfst(body))))
				return NULL;
			if (!(env = rec_extend(f->v.f.env, f->v.f.args, args)))
				return NULL;
			/* continue loop from body of func call */
			body = eevo_pair(NULL, f->v.f.body);
		} else if (!(ret = eevo_eval(st, env, fst(body))))
			return NULL;
	return ret;
}

static void
prepend_bt(EevoSt st, EevoRec env, Eevo f)
{
	if (!f->v.f.name) /* no need to record anonymous functions */
		return;
	for (; env->next; env = env->next) ; /* bt var located at base env */
	EevoEntry e = entry_get(env, "bt");
	if (e->val->t == EEVO_PAIR && fst(e->val)->t == EEVO_SYM &&
	    !strncmp(f->v.f.name, fst(e->val)->v.s, strlen(fst(e->val)->v.s)))
		return; /* don't record same function on recursion */
	e->val = eevo_pair(eevo_sym(st, f->v.f.name), e->val);
}

/* evaluate procedure f with arguments */
static Eevo
eval_proc(EevoSt st, EevoRec env, Eevo f, Eevo args)
{
	Eevo ret;
	EevoRec fenv;
	/* evaluate function and primitive arguments before being passed */
	switch (f->t) {
	case EEVO_PRIM:
		if (!(args = eevo_eval_list(st, env, args)))
			return NULL;
		/* FALLTHROUGH */
	case EEVO_FORM:
		return (*f->v.pr.pr)(st, env, args);
	case EEVO_FUNC:
		if (!(args = eevo_eval_list(st, env, args)))
			return NULL;
		/* FALLTHROUGH */
	case EEVO_MACRO:
		eevo_arg_num(args, f->v.f.name ? f->v.f.name : "anon", eevo_lstlen(f->v.f.args));
		if (!(fenv = rec_extend(f->v.f.env, f->v.f.args, args)))
			return NULL;
		if (!(ret = eevo_eval_body(st, fenv, f->v.f.body)))
			return prepend_bt(st, env, f), NULL;
		if (f->t == EEVO_MACRO) /* TODO remove w/ expand_macro */
			ret = eevo_eval(st, env, ret);
		return ret;
	case EEVO_REC:
		if (!(args = eevo_eval_list(st, env, args)))
			return NULL;
		eevo_arg_num(args, "record", 1);
		eevo_arg_type(fst(args), "record", EEVO_SYM);
		if (!(ret = rec_get(f->v.r, fst(args)->v.s)) &&
		    !(ret = rec_get(f->v.r, "else")))
			eevo_warnf("could not find element '%s' in record", fst(args)->v.s);
		return ret;
	case EEVO_TYPE:
		if (f->v.t.func)
			return eval_proc(st, env, f->v.t.func, args);
		eevo_warnf("could not convert to type '%s'", f->v.t.name);
	case EEVO_PAIR: // TODO eval each element as func w/ args: body
	default:
		eevo_warnf("attempt to evaluate non procedural type '%s' (%s)",
				eevo_type_str(f->t), eevo_print(f));
	}
}

/* evaluate given value */
Eevo
eevo_eval(EevoSt st, EevoRec env, Eevo v)
{
	Eevo f;
	switch (v->t) {
	case EEVO_SYM:
		if (!(f = rec_get(env, v->v.s)))
			eevo_warnf("could not find symbol '%s'", v->v.s);
		return f;
	case EEVO_PAIR:
		if (!(f = eevo_eval(st, env, fst(v))))
			return NULL;
		return eval_proc(st, env, f, rst(v));
	default:
		return v;
	}
}

/* print */

/* Resize string if its length is larger or equal to its size
 *   modify size to new max size after grown */
static char *
str_grow(char *str, int len, int *size)
{
	char *ret = str;
	if (len >= *size)
		if (!(ret = realloc(str, *size *= 2)))
			return free(str), NULL;
	return ret;
}

/* Convert record to string */
static char *
print_rec(EevoRec rec)
{
	int len = 0;
	int size = 64;
	char *ret = calloc(size, sizeof(char));
	for (EevoRec r = rec; r; r = r->next)
		for (int i = 0, c = 0; c < r->size; i++)
			if (r->items[i].key) {
				int olen = len;
				char *key = r->items[i].key;
				char *val = eevo_print(r->items[i].val);
				len += strlen(key) + strlen(val) + 2;
				if (!(ret = str_grow(ret, len, &size)))
					return NULL;
				snprintf(ret + strlen(ret), len-olen, "%s:%s", key, val);
				c++;
			}
	return ret;
}

/* Convert eevo value to string to be printed
 *   returned string needs to be freed after use */
char *
eevo_print(Eevo v)
{
	int len;
	int size = 64;
	char *head, *tail, *ret = calloc(size, sizeof(char));
	switch (v->t) {
	case EEVO_NONE:
		strcat(ret, "Void");
		break;
	case EEVO_NIL:
		strcat(ret, "Nil");
		break;
	case EEVO_INT:
		len = snprintf(NULL, 0, "%d", (int)num(v)) + 1;
		if (!(ret = str_grow(ret, len, &size)))
			return NULL;
		snprintf(ret, len, "%d", (int)num(v));
		break;
	case EEVO_DEC:
		len = snprintf(NULL, 0, "%.15G", num(v)) + 3;
		if (!(ret = str_grow(ret, len, &size)))
			return NULL;
		snprintf(ret, len, "%.15G", num(v));
		if (num(v) == (int)num(v))
			strcat(ret, ".0");
		break;
	case EEVO_RATIO:
		len = snprintf(NULL, 0, "%d/%d", (int)num(v), (int)den(v)) + 1;
		if (!(ret = str_grow(ret, len, &size)))
			return NULL;
		snprintf(ret, len, "%d/%d", (int)num(v), (int)den(v));
		break;
	case EEVO_STR:
	case EEVO_SYM:
		len = strlen(v->v.s);
		if (!(ret = str_grow(ret, len, &size)))
			return NULL;
		strcat(ret, v->v.s);
		break;
	case EEVO_FUNC:
	case EEVO_MACRO:
		if (!v->v.f.name) {
			strcat(ret, "anon");
			break;
		}
		len = strlen(v->v.f.name);
		if (!(ret = str_grow(ret, len, &size)))
			return NULL;
		strcat(ret, v->v.f.name);
		break;
	case EEVO_PRIM:
	case EEVO_FORM:
		len = strlen(v->v.pr.name);
		if (!(ret = str_grow(ret, len, &size)))
			return NULL;
		strcat(ret, v->v.pr.name);
		break;
	case EEVO_TYPE:
		len = strlen(v->v.t.name);
		if (!(ret = str_grow(ret, len, &size)))
			return NULL;
		strcat(ret, v->v.t.name);
		break;
	case EEVO_REC:
		ret = print_rec(v->v.r);
		break;
	case EEVO_PAIR:
		head = eevo_print(fst(v));
		tail = nilp(rst(v)) ? "" : eevo_print(rst(v));
		len = strlen(head) + strlen(tail) + 1;
		if (!(ret = str_grow(ret, len, &size)))
			return NULL;
		snprintf(ret, len, "%s%s", head, tail);
		break;
	default:
		free(ret);
		eevo_warnf("could not print type '%s'", eevo_type_str(v->t));
	}
	return ret;
}


/* environment */

/* add new variable of name key and value v to the given environment */
void
eevo_env_add(EevoSt st, char *key, Eevo v)
{
	rec_add(st->env, key, v);
}

/* initialise eevo's state and global environment */
EevoSt
eevo_env_init(size_t cap)
{
	EevoSt st;
	if (!(st = malloc(sizeof(struct EevoSt_))))
		perror("; malloc"), exit(1);

	st->file = NULL;
	st->filec = 0;

	/* TODO intern (memorize) all types, including stateless func calls */
	st->strs = rec_new(cap, NULL);
	st->syms = rec_new(cap, NULL);

	st->env = rec_new(cap, NULL);
	eevo_env_add(st, "True", True);
	eevo_env_add(st, "Nil", Nil);
	eevo_env_add(st, "Void", Void);
	eevo_env_add(st, "bt", Nil);
	eevo_env_add(st, "version", eevo_str(st, "0.1"));

	/* Types */
	st->types[0]  = eevo_type(st, EEVO_NONE,  "TVoid", NULL);
	st->types[1]  = eevo_type(st, EEVO_NIL,   "TNil",  NULL);
	st->types[2]  = eevo_type(st, EEVO_INT,   "Int",   NULL);
	st->types[3]  = eevo_type(st, EEVO_DEC,   "Dec",   NULL);
	st->types[4]  = eevo_type(st, EEVO_RATIO, "Ratio", NULL);
	st->types[5]  = eevo_type(st, EEVO_STR,   "Str",   NULL);
	st->types[6]  = eevo_type(st, EEVO_SYM,   "Sym",   NULL);
	st->types[7]  = eevo_type(st, EEVO_PRIM,  "Prim",  NULL);
	st->types[8]  = eevo_type(st, EEVO_FORM,  "Form",  NULL);
	st->types[9]  = eevo_type(st, EEVO_FUNC,  "Func",  NULL);
	st->types[10] = eevo_type(st, EEVO_MACRO, "Macro", NULL);
	st->types[11] = eevo_type(st, EEVO_PAIR,  "Pair",  NULL);
	/* Eevo lst = eevo_sym(st, "lst"); */
	/* Eevo List = eevo_func(EEVO_FUNC, "List", lst, eevo_list(st, 1, lst), st->env); */
	/* st->types[11] = eevo_type(st, EEVO_PAIR | EEVO_NONE,  "List",  List); */
	st->types[12] = eevo_type(st, EEVO_REC,   "Rec",   eevo_prim(EEVO_FORM, eevo_rec,    "Rec"));
	st->types[13] = eevo_type(st, EEVO_TYPE,  "Type",  eevo_prim(EEVO_PRIM, eevo_typeof, "Type"));
	for (int i = 0; i < LEN(st->types); i++)
		eevo_env_add(st, st->types[i]->v.t.name, st->types[i]);
		/* TODO define type predicate functions here (nil?, string?, etc) */

	st->libh = NULL;
	st->libhc = 0;

	/* eevo_env_lib(st, libs); */

	return st;
}

/* load lib from string into environment */
Eevo
eevo_env_lib(EevoSt st, char* lib)
{
	Eevo parsed, expr, ret;
	char *file = st->file;
	size_t filec = st->filec;
	st->file = lib;
	st->filec = 0;
	skip_ws(st, 1);
	parsed = eevo_pair(eevo_sym(st, "do"), Nil);
	for (Eevo pos = parsed; eevo_fget(st) && (expr = eevo_read_line(st, 0)); pos = rst(pos))
		rst(pos) = eevo_pair(expr, Nil);
	ret = eevo_eval_body(st, st->env, rst(parsed));
	st->file = file;
	st->filec = filec;
	return ret;
}


#include "core/core.c"
#include "core/string.c"
#include "core/math.c"
#include "core/io.c"
#include "core/os.c"
