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
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tisp.h"

#define BETWEEN(X, A, B)  ((A) <= (X) && (X) <= (B))

/* functions */
static void hash_add(Hash ht, char *key, Val val);
static Hash hash_extend(Hash ht, Val args, Val vals);
static void hash_merge(Hash ht, Hash ht2);

static void
die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}

	exit(1);
}

static void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;
	if (!(p = calloc(nmemb, size)))
		die("calloc:");
	return p;
}

static void *
emalloc(size_t size)
{
	void *p;
	if (!(p = malloc(size)))
		die("malloc:");
	return p;
}

static void *
erealloc(void *p, size_t size)
{
	if (!(p = realloc(p, size)))
		die("realloc:");
	return p;
}

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
	case PAIR:      return "pair";
	default:
		if (t & NUMBER)
			return "number";
		return "invalid";
	}
}

static int
issym(char c)
{
	return BETWEEN(c, 'a', 'z') || BETWEEN(c, 'A', 'Z') ||
	       BETWEEN(c, '0', '9') || strchr("_+-*/=<>?", c);
}

static int
isnum(char *str)
{
	return isdigit(*str) || (*str == '.' && isdigit(str[1])) ||
	       ((*str == '-' || *str == '+') && (isdigit(str[1]) || str[1] == '.'));
}

void
skip_ws(Str str)
{
	str->d += strspn(str->d, " \t\n"); /* skip white space */
	for (; *str->d == ';'; str->d++) /* skip comments until newline */
		str->d += strcspn(str->d, "\n");
}

int
list_len(Val v)
{
	int len = 0;
	Val a;
	for (a = v; a->t == PAIR; a = cdr(a))
		len++;
	return a->t == NIL ? len : -1;
}

static Val
list_last(Val v)
{
	while (!(cdr(v)->t & NIL))
		v = cdr(v);
	return car(v);
}

static int
vals_eq(Val a, Val b)
{
	if (a->t != b->t)
		return 0;
	switch (a->t) {
	case INTEGER:
	case DECIMAL:
	case RATIO:
		if (a->v.n.num != b->v.n.num || a->v.n.den != b->v.n.den)
			return 0;
		break;
	default: /* PRIMITIVE, STRING, SYMBOL */
		if (a != b)
			return 0;
	}
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
	if (cap < 1) return NULL;
	Hash ht = emalloc(sizeof(struct Hash));
	ht->size = 0;
	ht->cap = cap;
	ht->items = ecalloc(cap, sizeof(struct Entry));
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

/* get value of given key in hash table */
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
	if (ht->size < ht->cap / 2)
		return; /* only need to grow table if it is more than half full */
	int i;
	int ocap = ht->cap;
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
		hash_grow(ht);
	}
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
			tsp_warn("hash_extend: argument not a symbol");
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

Val
mk_int(int i)
{
	Val ret = emalloc(sizeof(struct Val));
	ret->t = INTEGER;
	ret->v.n.num = i;
	ret->v.n.den = 1;
	return ret;
}

Val
mk_dub(double d)
{
	Val ret = emalloc(sizeof(struct Val));
	ret->t = DECIMAL;
	ret->v.n.num = d;
	ret->v.n.den = 1;
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
mk_str(Env env, char *s) {
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
	ret->v.s = s;
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
mk_func(Val args, Val body, Env env)
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
mk_list(Env env, int n, Val *a)
{
	int i;
	Val b = env->nil;
	for (i = n-1; i >= 0; i--)
		b = mk_pair(a[i], b);
	return b;
}

static int
read_sign(Str str)
{
	switch (*str->d) {
	case '-': ++str->d; return -1;
	case '+': ++str->d; return 1;
	default: return 1;
	}
}

static int
read_int(Str str)
{
	int ret;
	for (ret = 0; isdigit(*str->d); str->d++)
		ret = ret * 10 + *str->d - '0';
	return ret;
}

static Val
read_sci(Str str, double val, int isint) {
	if (tolower(*str->d) != 'e')
		goto finish;

	str->d++;
	double sign = read_sign(str) == 1 ? 10.0 : 0.1;
	for (int expo = read_int(str); expo--; val *= sign) ;

finish:
	if (isint)
		return mk_int(val);
	return mk_dub(val);
}

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
		s = emalloc(sizeof(str));
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

static Val
read_str(Env env, Str str)
{
	int len = 0;
	char *s = ++str->d;
	for (; *str->d++ != '"'; len++)
		if (!*str->d)
			tsp_warn("reached end before closing double quote");
	s[len] = '\0';
	return mk_str(env, s);
}

static Val
read_sym(Env env, Str str)
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
	return mk_sym(env, sym);
}

static Val
read_list(Env env, Str str)
{
	int n = 0;
	Val *a = emalloc(sizeof(Val)), b;
	str->d++;
	skip_ws(str);
	while (*str->d != ')') {
		if (!str->d[1])
			tsp_warn("reached end before closing ')'");
		a = erealloc(a, (n+1) * sizeof(Val)); /* TODO realloc less */
		if (!(a[n++] = tisp_read(env, str)))
			return NULL;
		skip_ws(str);
	}
	b = mk_list(env, n, a);
	free(a);
	str->d++;
	skip_ws(str);
	return b;
}

Val
tisp_read(Env env, Str str)
{
	skip_ws(str);
	if (strlen(str->d) == 0)
		return env->none;
	if (isnum(str->d))
		return read_num(str);
	if (*str->d == '"')
		return read_str(env, str);
	if (*str->d == '\'') {
		str->d++;
		return mk_pair(mk_sym(env, "quote"), mk_pair(tisp_read(env, str), env->nil));
	}
	if (issym(*str->d))
		return read_sym(env, str);
	if (*str->d == '(')
		return read_list(env, str);
	tsp_warn("could not read given input");
}

Val
tisp_eval_list(Env env, Val v)
{
	int cap = 1, size = 0;
	Val ret, *new = emalloc(sizeof(Val));
	for (; !nilp(v); v = cdr(v)) {
		if (!(new[size] = tisp_eval(env, car(v))))
			new[size] = env->none;
		if (++size == cap) {
			cap *= 2;
			new = erealloc(new, cap*sizeof(Val));
		}
	}
	ret = mk_list(env, size, new);
	free(new);
	return ret;
}

Val
tisp_eval(Env env, Val v)
{
	Val f, args;
	switch (v->t) {
	case NONE:
	case NIL:
	case INTEGER:
	case DECIMAL:
	case RATIO:
	case STRING:
		return v;
	case SYMBOL:
		if (!(f = hash_get(env->h, v->v.s)))
			tsp_warnf("could not find symbol %s", v->v.s);
		return f;
	case PAIR:
		if (!(f = tisp_eval(env, car(v))))
			return NULL;
		args = cdr(v);
		switch (f->t) {
		case PRIMITIVE:
			return (*f->v.pr)(env, args);
		case FUNCTION:
			/* tail call into the function body with the extended env */
			if (!(args = tisp_eval_list(env, args)))
				return NULL;
			if (!(hash_extend(env->h, f->v.f.args, args)))
				return NULL;
			hash_merge(env->h, f->v.f.env->h);
			return list_last(tisp_eval_list(env, f->v.f.body));
		default:
			tsp_warnf("attempt to evaluate non procedural type %s", type_str(f->t));
		}
	default: break;
	}
	return v;
}

static void
list_print(FILE *f, Val v)
{
	if (v->t == NONE)
		fprintf(f, "#<void>");
	else
		tisp_print(f, v);
}

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
		fprintf(f, "%d", (int)v->v.n.num);
		break;
	case DECIMAL:
		fprintf(f, "%.15g", v->v.n.num);
		if (v->v.n.num == (int)v->v.n.num)
			fprintf(f, ".0");
		break;
	case RATIO:
		fprintf(f, "%d/%d", (int)v->v.n.num, (int)v->v.n.den);
		break;
	case STRING:
		fprintf(f, "\"%s\"", v->v.s);
		break;
	case SYMBOL:
		fprintf(f, v->v.s);
		break;
	case PRIMITIVE:
		fprintf(f, "#<primitive>");
		break;
	case FUNCTION:
		fprintf(f, "#<function>");
		break;
	case PAIR:
		putc('(', f);
		list_print(f, car(v));
		v = cdr(v);
		while (!nilp(v)) {
			if (v->t == PAIR) {
				putc(' ', f);
				list_print(f, car(v));
				v = cdr(v);
			} else {
				fprintf(f, " . ");
				list_print(f, v);
				break;
			}
		}
		putc(')', f);
		break;
	default:
		fprintf(stderr, "tisp: could not print value type %s", type_str(v->t));
	}
}

static Val
prim_car(Env env, Val args)
{
	Val v;
	tsp_arg_num(args, "car", 1);
	if (!(v = tisp_eval_list(env, args)))
		return NULL;
	tsp_arg_type(car(v), "car", PAIR);
	return car(car(v));
}

static Val
prim_cdr(Env env, Val args)
{
	Val v;
	tsp_arg_num(args, "cdr", 1);
	if (!(v = tisp_eval_list(env, args)))
		return NULL;
	tsp_arg_type(car(v), "cdr", PAIR);
	return cdr(car(v));
}

static Val
prim_cons(Env env, Val args)
{
	Val v;
	tsp_arg_num(args, "cons", 2);
	if (!(v = tisp_eval_list(env, args)))
		return NULL;
	return mk_pair(car(v), car(cdr(v)));
}

static Val
prim_quote(Env env, Val args)
{
	tsp_arg_num(args, "quote", 1);
	return car(args);
}

static Val
prim_void(Env env, Val args)
{
	return env->none;
}

static Val
prim_begin(Env env, Val args)
{
	Val v;
	if (!(v = tisp_eval_list(env, args)))
		return NULL;
	if (nilp(v))
		return env->none;
	return list_last(v);
}

static Val
prim_eval(Env env, Val args)
{
	Val v;
	if (!(v = tisp_eval(env, car(args))))
		return NULL;
	return tisp_eval(env, v);
}

static Val
prim_eq(Env env, Val args)
{
	Val v;
	if (!(v = tisp_eval_list(env, args)))
		return NULL;
	if (nilp(v))
		return env->t;
	for (; !nilp(cdr(v)); v = cdr(v))
		if (!vals_eq(car(v), car(cdr(v))))
			return env->nil;
	return env->t;
}

static Val
prim_cond(Env env, Val args)
{
	Val v, cond;
	for (v = args; !nilp(v); v = cdr(v))
		if (!(cond = tisp_eval(env, car(car(v)))))
			return NULL;
		else if (!nilp(cond))
			return tisp_eval(env, car(cdr(car(v))));
	return env->nil;
}

static Val
prim_type(Env env, Val args)
{
	Val v;
	tsp_arg_num(args, "type", 1);
	if (!(v = tisp_eval(env, car(args))))
		return NULL;
	return mk_str(env, type_str(v->t));
}

static Val
prim_lambda(Env env, Val args)
{
	if (list_len(args) < 2)
		tsp_warnf("lambda: expected 2 or more arguments, received %d", list_len(args));
	if (car(args)->t != PAIR && !nilp(car(args)))
		tsp_warn("lambda: incorrect format, no argument list found");
	return mk_func(car(args), cdr(args), env);
}

static Val
prim_define(Env env, Val args)
{
	Val sym, val;
	if (list_len(args) < 2)
		tsp_warnf("define: expected 2 or more arguments, received %d", list_len(args));
	if (car(args)->t == PAIR) {
		sym = car(car(args));
		val = mk_func(cdr(car(args)), cdr(args), env);
	} else if (car(args)->t == SYMBOL) {
		sym = car(args);
		val = tisp_eval(env, car(cdr(args)));
	} else
		tsp_warn("define: incorrect format, no variable name found");
	if (!val)
		return NULL;
	hash_add(env->h, sym->v.s, val);
	return env->none;
}

static Val
prim_load(Env env, Val args)
{
	Val v;
	void (*tibenv)(Env);
	char *lib, *func;
	tsp_arg_num(args, "load", 1);
	if (!(v = tisp_eval(env, car(args))))
		return NULL;
	tsp_arg_type(v, "load", STRING);

	env->libh = erealloc(env->libh, (env->libhc+1)*sizeof(void*));

	lib = emalloc((strlen(v->v.s)+10) * sizeof(char));
	strcat(lib, "libtib");
	strcat(lib, v->v.s);
	strcat(lib, ".so");
	if (!(env->libh[env->libhc] = dlopen(lib, RTLD_LAZY)))
		tsp_warnf("load: could not load '%s':\n%s", v->v.s, dlerror());
	dlerror();
	free(lib);

	func = emalloc((strlen(v->v.s)+9) * sizeof(char));
	strcat(func, "tib_env_");
	strcat(func, v->v.s);
	tibenv = dlsym(env->libh[env->libhc], func);
	if (dlerror())
		tsp_warnf("load: could not run '%s':\n%s", v->v.s, dlerror());
	(*tibenv)(env);
	free(func);

	env->libhc++;
	return env->none;
}

void
tisp_env_add(Env e, char *key, Val v)
{
	hash_add(e->h, key, v);
}

Env
tisp_env_init(size_t cap)
{
	Env e = emalloc(sizeof(struct Env));
	e->nil = emalloc(sizeof(struct Val));
	e->nil->t = NIL;
	e->none = emalloc(sizeof(struct Val));
	e->none->t = NONE;
	e->t = emalloc(sizeof(struct Val));
	e->t->t = SYMBOL;
	e->t->v.s = "t";

	e->h = hash_new(cap);
	hash_add(e->h, "t", e->t);
	hash_add(e->h, "car",    mk_prim(prim_car));
	hash_add(e->h, "cdr",    mk_prim(prim_cdr));
	hash_add(e->h, "cons",   mk_prim(prim_cons));
	hash_add(e->h, "quote",  mk_prim(prim_quote));
	hash_add(e->h, "void",   mk_prim(prim_void));
	hash_add(e->h, "begin",  mk_prim(prim_begin));
	hash_add(e->h, "eval",   mk_prim(prim_eval));
	hash_add(e->h, "=",      mk_prim(prim_eq));
	hash_add(e->h, "cond",   mk_prim(prim_cond));
	hash_add(e->h, "type",   mk_prim(prim_type));
	hash_add(e->h, "lambda", mk_prim(prim_lambda));
	hash_add(e->h, "define", mk_prim(prim_define));
	hash_add(e->h, "load",   mk_prim(prim_load));

	e->strs = hash_new(cap);
	e->syms = hash_new(cap);

	e->libh = NULL;
	e->libhc = 0;
	return e;
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
