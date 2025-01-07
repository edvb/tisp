/* zlib License
 *
 * Copyright (c) 2017-2024 Ed van Bruggen
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
static void rec_add(Rec ht, char *key, Val val);
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

/* count number of parenthesis */
/* FIXME makes reading O(n^2) replace w/ better counting sys */
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
tsp_lstlen(Val v)
{
	int len = 0;
	for (; v->t == TSP_PAIR; v = cdr(v))
		len++;
	return nilp(v) ? len : -1; /* TODO -len */
}

/* check if two values are equal */
static int
vals_eq(Val a, Val b)
{
	if (a->t & TSP_NUM && b->t & TSP_NUM) { /* NUMBERs */
		if (a->v.n.num != b->v.n.num || a->v.n.den != b->v.n.den)
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
	Rec ht = malloc(sizeof(struct Rec));
	if (!ht) perror("; malloc"), exit(1);
	ht->size = 0;
	ht->cap = cap;
	ht->items = calloc(cap, sizeof(struct Entry));
	if (!ht->items) perror("; calloc"), exit(1);
	ht->next = next;
	return ht;
}

/* get entry in one record for the key */
static Entry
entry_get(Rec ht, char *key)
{
	int i = hash(key) % ht->cap;
	char *s;
	/* look for key starting at hash until empty entry is found */
	while ((s = ht->items[i].key)) {
		if (!strcmp(s, key))
			break;
		if (++i == ht->cap) /* loop back around if end is reached */
			i = 0;
	}
	return &ht->items[i]; /* returns entry if found or empty one to be filled */
}

/* get value of given key in each record */
static Val
rec_get(Rec ht, char *key)
{
	Entry e;
	for (; ht; ht = ht->next) {
		e = entry_get(ht, key);
		if (e->key)
			return e->val;
	}
	return NULL;
}

/* enlarge the record to ensure algorithm's efficiency */
static void
rec_grow(Rec ht)
{
	int i, ocap = ht->cap;
	Entry oitems = ht->items;
	ht->cap *= 2;
	ht->items = calloc(ht->cap, sizeof(struct Entry));
	if (!ht->items) perror("; calloc"), exit(1);
	for (i = 0; i < ocap; i++) /* repopulate new record with old values */
		if (oitems[i].key)
			rec_add(ht, oitems[i].key, oitems[i].val);
	free(oitems);
}

/* create new key and value pair to the record */
static void
rec_add(Rec ht, char *key, Val val)
{
	Entry e = entry_get(ht, key);
	e->val = val;
	if (!e->key) {
		e->key = key;
		if (++ht->size > ht->cap / 2) /* grow record if it is more than half full */
			rec_grow(ht);
	}
}

/* add each binding args[i] -> vals[i] */
/* args and vals are both lists */
static Rec
rec_extend(Rec ht, Val args, Val vals)
{
	Val arg, val;
	int argnum = tsp_lstlen(args);
	Rec ret = rec_new(argnum > 0 ? 2*argnum : 8, ht);
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
				  " definition, recieved %s", tsp_type_str(arg->t));
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
	Val ret = malloc(sizeof(struct Val));
	if (!ret) perror("; malloc"), exit(1);
	ret->t = t;
	return ret;
}

Val
mk_int(int i)
{
	Val ret = mk_val(TSP_INT);
	ret->v.n.num = i;
	ret->v.n.den = 1;
	return ret;
}

Val
mk_dec(double d)
{
	Val ret = mk_val(TSP_DEC);
	ret->v.n.num = d;
	ret->v.n.den = 1;
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
	Val v, ret = mk_val(TSP_REC);
	if (!assoc) return ret->v.r = env, ret;
	ret->v.r = rec_new(64, NULL);
	Rec h = rec_new(4, env);
	rec_add(h, "this", ret);
	for (Val cur = assoc; cur->t == TSP_PAIR; cur = cdr(cur))
		if (car(cur)->t == TSP_PAIR && caar(cur)->t & (TSP_SYM|TSP_STR)) {
			if (!(v = tisp_eval(st, h, cdar(cur)->v.p.car)))
				return NULL;
			rec_add(ret->v.r, caar(cur)->v.s, v);
		} else if (car(cur)->t == TSP_SYM) {
			if (!(v = tisp_eval(st, h, car(cur))))
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
	char *pos, *ret = malloc((len+1) * sizeof(char));
	if (!ret) perror("; malloc"), exit(1);
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
	for (; is_char(tsp_fget(st)); tsp_finc(st))
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
	static char *prefix[] = {
		"'",  "quote",
		"`",  "quasiquote",
		",@", "unquote-splice", /* always check before , */
		",",  "unquote",
		"@",  "Func",
		"f\"",  "strformat",
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
	tsp_warnf("could not read given input '%c'", st->file[st->filec]);
}

/* read extra syntax sugar on top of s-expressions */
Val
tisp_read(Tsp st)
{
	Val v, lst, w;
	if (!(v = tisp_read_sexpr(st))) return NULL;
	if (tsp_fget(st) == '[') { /* func[x y] => (func x y) */
		/* TODO fix @it[3] */
		tsp_finc(st);
		if (!(lst = read_pair(st, ']'))) return NULL;
		return mk_pair(v, lst);
	} else if (tsp_fget(st) == '{') { /* rec{ key: value } => (recmerge rec { key: value }) */
		tsp_finc(st);
		if (!(lst = read_pair(st, '}'))) return NULL;
		return mk_list(st, 3, mk_sym(st, "recmerge"), v,
		                      mk_pair(mk_sym(st, "Rec"), lst));
	} else if (tsp_fget(st) == ':') {
		tsp_finc(st);
		switch (tsp_fget(st)) {
		case '[': /* proc:[lst] => (map proc lst) */
			tsp_finc(st);
			if (!(w = read_pair(st, ']'))) return NULL;
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

/* return string containing contents of file name */
char *
tisp_read_file(char *fname)
{
	char buf[BUFSIZ], *file = NULL;
	int len = 0, n, fd, parens = 0;
	if (!fname) /* read from stdin if no file given */
		fd = 0;
	else if ((fd = open(fname, O_RDONLY)) < 0)
		tsp_warnf("could not find file '%s'", fname);
	while ((n = read(fd, buf, sizeof(buf))) > 0) {
		file = realloc(file, len + n + 1);
		if (!file) perror("; realloc"), exit(1);
		memcpy(file + len, buf, n);
		len += n;
		file[len] = '\0';
		if (fd == 0 && !(parens += count_parens(buf, n)))
			break;
	}
	if (fd) /* close file if not stdin */
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
/* TODO Val f -> Func f */
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
		fprintf(f, "%d", (int)v->v.n.num);
		break;
	case TSP_DEC:
		/* TODO fix 1.e-5 print as int 1e-05 */
		fprintf(f, "%.15g", v->v.n.num);
		if (v->v.n.num == (int)v->v.n.num)
			fprintf(f, ".0");
		break;
	case TSP_RATIO:
		fprintf(f, "%d/%d", (int)v->v.n.num, (int)v->v.n.den);
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
		for (Rec h = v->v.r; h; h = h->next)
			for (int i = 0, c = 0; c < h->size; i++)
				if (h->items[i].key) {
					c++;
					fprintf(f, " %s: ", h->items[i].key);
					tisp_print(f, h->items[i].val);
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

/* primitives */

/* return first element of list */
static Val
prim_car(Tsp st, Rec env, Val args)
{
	tsp_arg_num(args, "car", 1);
	tsp_arg_type(car(args), "car", TSP_PAIR);
	return caar(args);
}

/* return elements of a list after the first */
static Val
prim_cdr(Tsp st, Rec env, Val args)
{
	tsp_arg_num(args, "cdr", 1);
	tsp_arg_type(car(args), "cdr", TSP_PAIR);
	return cdar(args);
}

/* return new pair */
static Val
prim_cons(Tsp st, Rec env, Val args)
{
	tsp_arg_num(args, "cons", 2);
	return mk_pair(car(args), cadr(args));
}

/* do not evaluate argument */
static Val
form_quote(Tsp st, Rec env, Val args)
{
	tsp_arg_num(args, "quote", 1);
	return car(args);
}

/* evaluate argument given */
static Val
prim_eval(Tsp st, Rec env, Val args)
{
	Val v;
	tsp_arg_num(args, "eval", 1);
	return (v = tisp_eval(st, st->env, car(args))) ? v : st->none;
}

/* test equality of all values given */
static Val
prim_eq(Tsp st, Rec env, Val args)
{
	if (nilp(args))
		return st->t;
	for (; !nilp(cdr(args)); args = cdr(args))
		if (!vals_eq(car(args), cadr(args)))
			return st->nil;
	/* return nilp(car(args)) ? st->t : car(args); */
	return st->t;
}

/* evaluates all expressions if their conditions are met */
static Val
form_cond(Tsp st, Rec env, Val args)
{
	Val v, cond;
	for (v = args; !nilp(v); v = cdr(v))
		if (!(cond = tisp_eval(st, env, caar(v))))
			return NULL;
		else if (!nilp(cond)) /* TODO incorporate else directly into cond */
			return tisp_eval_body(st, env, cdar(v));
	return st->none;
}

/* return type of tisp value */
static Val
prim_typeof(Tsp st, Rec env, Val args)
{
	tsp_arg_num(args, "typeof", 1);
	return mk_str(st, tsp_type_str(car(args)->t));
}

/* return record of properties for given procedure */
static Val
prim_procprops(Tsp st, Rec env, Val args)
{
	tsp_arg_num(args, "procprops", 1);
	Val proc = car(args);
	Rec ret = rec_new(6, NULL);
	switch (proc->t) {
	case TSP_FORM:
	case TSP_PRIM:
		rec_add(ret, "name", mk_sym(st, proc->v.pr.name));
		break;
	case TSP_FUNC:
	case TSP_MACRO:
		rec_add(ret, "name", mk_sym(st, proc->v.f.name ? proc->v.f.name : "anon"));
		rec_add(ret, "args", proc->v.f.args);
		rec_add(ret, "body", proc->v.f.body);
		/* rec_add(ret, "env", proc->v.f.env); */
		break;
	default:
		tsp_warnf("procprops: expected Proc, received '%s'", tsp_type_str(proc->t));
	}
	return mk_rec(st, ret, NULL);
}

/* creates new tisp function */
static Val
form_Func(Tsp st, Rec env, Val args)
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
form_Macro(Tsp st, Rec env, Val args)
{
	tsp_arg_min(args, "Macro", 1);
	Val ret = form_Func(st, env, args);
	ret->t = TSP_MACRO;
	return ret;
}

/** Records **/

/* merge second record into first record, without mutation */
static Val
prim_recmerge(Tsp st, Rec env, Val args)
{
	Val ret = mk_val(TSP_REC);;
	tsp_arg_num(args, "recmerge", 2);
	tsp_arg_type(car(args), "recmerge", TSP_REC);
	tsp_arg_type(cadr(args), "recmerge", TSP_REC);
	ret->v.r = rec_new(cadr(args)->v.r->size*2, car(args)->v.r);
	for (Rec h = cadr(args)->v.r; h; h = h->next)
		for (int i = 0, c = 0; c < h->size; i++)
			if (h->items[i].key)
				c++, rec_add(ret->v.r, h->items[i].key, h->items[i].val);
	return ret;
}

/* retrieve list of every entry in given record */
static Val
prim_records(Tsp st, Rec env, Val args)
{
	Val ret = st->nil;
	tsp_arg_num(args, "records", 1);
	tsp_arg_type(car(args), "records", TSP_REC);
	for (Rec h = car(args)->v.r; h; h = h->next)
		for (int i = 0, c = 0; c < h->size; i++)
			if (h->items[i].key) {
				Val entry = mk_pair(mk_sym(st, h->items[i].key),
				                               h->items[i].val);
				ret = mk_pair(entry, ret);
				c++;
			}
	return ret;
}

/* creates new variable of given name and value
 * if pair is given as name of variable, creates function with the car as the
 * function name and the cdr the function arguments */
/* TODO if var not func error if more than 2 args */
static Val
form_def(Tsp st, Rec env, Val args)
{
	Val sym, val;
	tsp_arg_min(args, "def", 1);
	if (car(args)->t == TSP_PAIR) { /* create function if given argument list */
		sym = caar(args); /* first element of argument list is function name */
		if (sym->t != TSP_SYM)
			tsp_warnf("def: incorrect format,"
			          " expected symbol for function name, received %s",
			          tsp_type_str(sym->t));
		val = mk_func(TSP_FUNC, sym->v.s, cdar(args), cdr(args), env);
	} else if (car(args)->t == TSP_SYM) { /* create variable */
		sym = car(args); /* if only symbol given, make it self evaluating */
		val = nilp(cdr(args)) ? sym : tisp_eval(st, env, cadr(args));
	} else tsp_warn("def: incorrect format, no variable name found");
	if (!val)
		return NULL;
	/* set procedure name if it was previously anonymous */
	if (val->t & (TSP_FUNC|TSP_MACRO) && !val->v.f.name)
		val->v.f.name = sym->v.s; /* TODO some bug here */
	rec_add(env, sym->v.s, val);
	return st->none;
}

/* TODO fix crashing if try to undefine builtin */
static Val
form_undefine(Tsp st, Rec env, Val args)
{
	tsp_arg_min(args, "undefine!", 1);
	tsp_arg_type(car(args), "undefine!", TSP_SYM);
	for (Rec h = env; h; h = h->next) {
		Entry e = entry_get(h, car(args)->v.s);
		if (e->key) {
			e->key = NULL;
			/* TODO tsp_free(e->val); */
			return st->none;
		}
	}
	tsp_warnf("undefine!: could not find symbol %s to undefine", car(args)->v.s);
}

static Val
form_definedp(Tsp st, Rec env, Val args)
{
	Entry e = NULL;
	tsp_arg_min(args, "defined?", 1);
	tsp_arg_type(car(args), "defined?", TSP_SYM);
	for (Rec h = env; h; h = h->next) {
		e = entry_get(h, car(args)->v.s);
		if (e->key)
			break;
	}
	return (e && e->key) ? st->t : st->nil;
}

/* loads tisp file or C dynamic library */
/* TODO lua like error listing places load looked */
/* TODO only use dlopen if -ldl is given with TIB_DYNAMIC */
/* TODO define load in lisp which calls load-dl */
static Val
prim_load(Tsp st, Rec env, Val args)
{
	Val tib;
	void (*tibenv)(Tsp);
	char name[PATH_MAX];
	const char *paths[] = {
		"/usr/local/lib/tisp/pkgs/", "/usr/lib/tisp/pkgs/", "./", NULL
	};

	tsp_arg_num(args, "load", 1);
	tib = car(args);
	tsp_arg_type(tib, "load", TSP_STR);

	for (int i = 0; paths[i]; i++) {
		strcpy(name, paths[i]);
		strcat(name, tib->v.s);
		strcat(name, ".tsp");
		if (access(name, R_OK) != -1) {
			tisp_eval_body(st, env, tisp_parse_file(st, name));
			return st->none;
		}
	}

	/* If not tisp file, try loading shared object library */
	if (!(st->libh = realloc(st->libh, (st->libhc+1)*sizeof(void*))))
		perror("; realloc"), exit(1);

	memset(name, 0, sizeof(name));
	strcpy(name, "libtib");
	strcat(name, tib->v.s);
	strcat(name, ".so");
	if (!(st->libh[st->libhc] = dlopen(name, RTLD_LAZY)))
		tsp_warnf("load: could not load '%s':\n; %s", tib->v.s, dlerror());
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
prim_error(Tsp st, Rec env, Val args)
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
	rec_add(st->env, key, v);
}

/* initialise tisp's state and global environment */
Tsp
tisp_env_init(size_t cap)
{
	Tsp st = malloc(sizeof(struct Tsp));
	if (!st) perror("; malloc"), exit(1);

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
	tsp_env_prim(car);
	tsp_env_prim(cdr);
	tsp_env_prim(cons);
	tsp_env_form(quote);
	tsp_env_prim(eval);
	tsp_env_name_prim(=, eq);
	tsp_env_form(cond);
	tsp_env_prim(typeof);
	tsp_env_prim(procprops);
	tsp_env_form(Func);
	tsp_env_form(Macro);
	tisp_env_add(st, "Rec", mk_prim(TSP_FORM, mk_rec, "Rec"));
	tsp_env_prim(recmerge);
	tsp_env_prim(records);
	tsp_env_form(def);
	tsp_env_name_form(undefine!, undefine);
	tsp_env_name_form(defined?, definedp);
	tsp_env_prim(load);
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
	skip_ws(st, 1);
	if ((v = tisp_read(st)))
		tisp_eval_body(st, st->env, v);
	st->file = file;
	st->filec = filec;
}
