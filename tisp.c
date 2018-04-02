/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tisp.h"
#include "util.h"

/* functions */
static Hash hash_extend(Hash ht, Val args, Val vals);
static void hash_merge(Hash ht, Hash ht2);

void
skip_spaces(Str str)
{
	str->d += strspn(str->d, " \t\n"); /* skip white space */
	for (; *str->d == ';'; str->d++) /* skip comments until newline */
		str->d += strcspn(str->d, "\n");
}

static int
issym(char c)
{
	return BETWEEN(c, 'a', 'z') || strchr("+-*/=?", c);
}

static int
list_len(Val v)
{
	int len = 0;
	Val a;
	for (a = v; a->t == PAIR; a = cdr(a))
		len++;
	return a->t == NIL ? len : -1;
}

static int
vals_eq(Val a, Val b)
{
	if (a->t != b->t)
		return 0;
	switch (a->t) {
	case INTEGER:
		if (a->v.i != b->v.i)
			return 0;
		break;
	case SYMBOL:
	case STRING:
		if (strcmp(a->v.s, b->v.s))
			return 0;
		break;
	default:
		if (a != b)
			return 0;
	}
	return 1;
}

/* reduce fraction by modifying supplied numerator and denominator */
static void
frac_reduce(int *num, int *den)
{
	int a = *num, b = *den, c = a % b;
	while(c > 0) {
		a = b;
		b = c;
		c = a % b;
	}
	*num = *num / b;
	*den = *den / b;
}

/* return hashed number based on key */
static uint32_t
hash(char *key)
{
	uint32_t h = 0;
	char c;
	while (h < ULONG_MAX && (c = *key++))
		h = h * 33 + c;
	return h;
}

/* create new empty hash table with given capacity */
static Hash
hash_new(size_t cap)
{
	int i;
	if (cap < 1) return NULL;
	Hash ht = emalloc(sizeof(struct Hash));
	ht->size = 0;
	ht->cap = cap;
	ht->items = ecalloc(cap, sizeof(struct Entry));
	for (i = 0; i < cap; i++)
		ht->items[i].key = NULL;
	ht->next = NULL;
	return ht;
}

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
	return &ht->items[i];
}

/* get vale of given key in hash table  */
static Val
hash_get(Hash ht, char *key)
{
	Entry e;
	for (; ht; ht = ht->next) {
		e = entry_get(ht, key);
		if (e->key)
			return e->val;
	}
	warnf("hash_get: could not find symbol [%s]", key);
}

/* enlarge the hash table to ensure algorithm's efficiency */
static void
hash_grow(Hash ht)
{
	if (ht->size < ht->cap / 2)
		return; /* only need to grow table if it is more than half full */
	int i;
	int ocap = ht->cap;
	Entry oitems = ht->items;
	ht->cap *= 2;
	ht->items = ecalloc(ht->cap, sizeof(struct Entry));
	for (i = 0; i < ht->cap; i++) /* empty the new larger hash table */
		ht->items[i].key = NULL;
	for (i = 0; i < ocap; i++) /* repopulate new hash table with old values */
		if (oitems[i].key)
			hash_add(ht, oitems[i].key, oitems[i].val);
	free(oitems);
}

/* create new key and value pair to the hash table */
Val
hash_add(Hash ht, char *key, Val val)
{
	Entry e = entry_get(ht, key);
	e->val = val;
	if (!e->key) {
		e->key = key;
		ht->size++;
		hash_grow(ht);
	}
	return val;
}

/* add each binding args[i] -> vals[i] */
/* args and vals are both scheme lists */
static Hash
hash_extend(Hash ht, Val args, Val vals)
{
	Val arg, val;

	for (; !nilp(args) && !nilp(vals); args = cdr(args), vals = cdr(vals)) {
		arg = car(args);
		val = car(vals);
		if (arg->t != SYMBOL)
			warn("hash_extend: argument not a symbol");
		hash_add(ht, arg->v.s, val);
	}
	return ht;
}

/* add everything from ht2 into ht */
static void
hash_merge(Hash ht, Hash ht2)
{
	int i;

	for (; ht2; ht2 = ht2->next)
		for (i = 0; i < ht2->cap; i++)
			if (ht2->items[i].key)
				hash_add(ht, ht2->items[i].key, ht2->items[i].val);
}

#define MK_TYPE(TYPE, TYPE_NAME, TYPE_FULL, FN_NAME) \
Val FN_NAME(TYPE TYPE_NAME) { \
	Val ret = emalloc(sizeof(struct Val)); \
	ret->t = TYPE_FULL; \
	ret->v.TYPE_NAME = TYPE_NAME; \
	return ret; \
}

MK_TYPE(int, i, INTEGER, mk_int)
MK_TYPE(char *, s, STRING, mk_str)
MK_TYPE(char *, s, SYMBOL, mk_sym)
MK_TYPE(Prim, pr, PRIMITIVE, mk_prim)

Val
mk_rat(int num, int den)
{
	if (den == 0)
		warn("division by zero");
	frac_reduce(&num, &den);
	if (den == 1) /* simplify fraction into integer if denominator is 1 */
		return mk_int(num);
	Val ret = emalloc(sizeof(struct Val));
	ret->t = RATIONAL;
	ret->v.r = (Ratio){ num, den };
	return ret;
}

Val
mk_func(Val args, Val body, Hash env)
{
	Val ret = emalloc(sizeof(struct Val));
	ret->t = FUNCTION;
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
mk_list(int n, Val *a)
{
	int i;
	Val b = &nil;
	for (i = n-1; i >= 0; i--)
		b = mk_pair(a[i], b);
	return b;
}

Val
read_int(Str str)
{
	int i, j;
	for (i = 0; isdigit(*str->d); str->d++)
		i = i * 10 + *str->d - '0';
	if (*str->d != '/')
		return mk_int(i);
	for (j = 0, *str->d++; isdigit(*str->d); str->d++)
		j = j * 10 + *str->d - '0';
	return mk_rat(i, j);
}

Val
read_str(Str str)
{
	int len = 0;
	char *s = ++str->d;
	for (; *str->d && *str->d++ != '"'; len++);
	s[len] = '\0';
	return mk_str(estrdup(s));
}

Val
read_sym(Str str)
{
	int n = 1;
	int i = 0;
	char *sym = emalloc(n);
	for (; issym(*str->d); str->d++) {
		sym[i++] = *str->d;
		if (i == n) {
			n *= 2;
			sym = erealloc(sym, n);
		}
	}
	sym[i] = '\0';
	return mk_sym(sym);
}

Val
read_list(Str str)
{
	int n = 0;
	Val *a = emalloc(sizeof(Val)), b;
	str->d++;
	skip_spaces(str);
	while (*str->d && *str->d != ')') {
		a = erealloc(a, (n+1) * sizeof(Val)); /* TODO realloc less */
		a[n++] = tisp_read(str);
		skip_spaces(str);
	}
	b = mk_list(n, a);
	free(a);
	str->d++;
	skip_spaces(str);
	return b;
}

Val
tisp_read(Str str)
{
	skip_spaces(str);
	if (isdigit(*str->d)) /* TODO negative numbers */
		return read_int(str);
	if (*str->d == '"') /* TODO fix */
		return read_str(str);
	if (issym(*str->d))
		return read_sym(str);
	if (*str->d == '(')
		return read_list(str);
	return NULL;
}

Val
eval_list(Hash env, Val v)
{
	int cap = 1, size = 0;
	Val *new = emalloc(sizeof(Val));
	for (; !nilp(v); v = cdr(v)) {
		if (!(new[size++] = tisp_eval(env, car(v))))
			return NULL;
		if (size == cap) {
			cap *= 2;
			new = erealloc(new, cap*sizeof(Val));
		}
	}
	Val ret = mk_list(size, new);
	free(new);
	return ret;
}

Val
tisp_eval(Hash env, Val v)
{
	Val f, args;
	switch (v->t) {
	case NIL:
	case INTEGER:
	case RATIONAL:
	case STRING:
		return v;
	case SYMBOL:
		return hash_get(env, v->v.s);
	case PAIR:
		if (!(f = tisp_eval(env, car(v))))
			return NULL;
		args = cdr(v);
		switch (f->t) {
		case PRIMITIVE:
			return (*f->v.pr)(env, args);
		case FUNCTION:
			/* tail call into the function body with the extended env */
			if (!(args = eval_list(env, args)))
				return NULL;
			if (!(hash_extend(env, f->v.f.args, args)))
				return NULL;
			hash_merge(env, f->v.f.env);
			return tisp_eval(env, f->v.f.body);
		default:
			warn("attempt to evaluate non primitive");
		}
	default: break;
	}
	return v;
}

/* TODO return str for error msgs? */
void
tisp_print(Val v)
{
	switch (v->t) {
	case NIL:
		printf("()");
		break;
	case INTEGER:
		printf("%d", v->v.i);
		break;
	case RATIONAL:
		printf("%d/%d", v->v.r.num, v->v.r.den);
		break;
	case STRING:
		printf("\"%s\"", v->v.s);
		break;
	case SYMBOL:
		printf(v->v.s);
		break;
	case PRIMITIVE:
		printf("#<primitive>");
		break;
	case FUNCTION:
		printf("#<function>");
		break;
	case PAIR:
		putchar('(');
		tisp_print(car(v));
		v = cdr(v);
		while (!nilp(v)) {
			if (v->t == PAIR) {
				putchar(' ');
				tisp_print(car(v));
				v = cdr(v);
			} else {
				printf(" . ");
				tisp_print(v);
				break;
			}
		}
		putchar(')');
		break;
	default:
		fprintf(stderr, "tisp: could not print value type [%d]", v->t);
	}
}

static Val
prim_car(Hash env, Val args)
{
	Val v;
	if (list_len(args) != 1)
		warnf("car: expected 1 argument, received [%d]", list_len(args));
	if (!(v = eval_list(env, args)))
		return NULL;
	if (car(v)->t != PAIR)
		warnf("car: expected list, received type [%d]", car(v)->t);
	return car(car(v));
}

static Val
prim_cdr(Hash env, Val args)
{
	Val v;
	if (list_len(args) != 1)
		warnf("cdr: expected 1 argument, received [%d]", list_len(args));
	if (!(v = eval_list(env, args)))
		return NULL;
	if (car(v)->t != PAIR)
		warnf("cdr: expected list, received type [%d]", car(v)->t);
	return cdr(car(v));
}

static Val
prim_cons(Hash env, Val args)
{
	Val v;
	if (list_len(args) != 2)
		warnf("cons: expected 2 arguments, received [%d]", list_len(args));
	if (!(v = eval_list(env, args)))
		return NULL;
	return mk_pair(car(v), car(cdr(v)));
}

static Val
prim_eq(Hash env, Val args)
{
	Val v;
	if (!(v = eval_list(env, args)))
		return NULL;
	if (nilp(v))
		return &t;
	for (; !nilp(cdr(v)); v = cdr(v))
		if (!vals_eq(car(v), car(cdr(v))))
			return &nil;
	return &t;
}

static Val
prim_quote(Hash env, Val args)
{
	if (list_len(args) != 1)
		warnf("quote: expected 1 argument, received [%d]", list_len(args));
	return car(args);
}

static Val
prim_cond(Hash env, Val args)
{
	Val v, cond;
	for (v = args; !nilp(v); v = cdr(v))
		if (!(cond = tisp_eval(env, car(car(v)))))
			return NULL;
		else if (!nilp(cond))
			return tisp_eval(env, car(cdr(car(v))));
	return &nil;
}

static Val
prim_lambda(Hash env, Val args)
{
	if (list_len(args) < 2 || (car(args)->t != PAIR && !nilp(car(args))))
		warn("lambda: incorrect format");
	return mk_func(car(args), car(cdr(args)), env);
}

static Val
prim_define(Hash env, Val args)
{
	Val v;
	if (list_len(args) != 2 || car(args)->t != SYMBOL)
		warn("define: incorrect format");
	if (!(v = tisp_eval(env, car(cdr(args)))))
		return NULL;
	hash_add(env, car(args)->v.s, v);
	return NULL;
}

Hash
tisp_init_env(size_t cap)
{
	nil.t = NIL;
	t.t = SYMBOL;
	t.v.s = estrdup("t");

	Hash h = hash_new(cap);
	hash_add(h, "t", &t);
	hash_add(h, "car",    mk_prim(prim_car));
	hash_add(h, "cdr",    mk_prim(prim_cdr));
	hash_add(h, "cons",   mk_prim(prim_cons));
	hash_add(h, "=",      mk_prim(prim_eq));
	hash_add(h, "quote",  mk_prim(prim_quote));
	hash_add(h, "cond",   mk_prim(prim_cond));
	hash_add(h, "lambda", mk_prim(prim_lambda));
	hash_add(h, "define", mk_prim(prim_define));
	return h;
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
