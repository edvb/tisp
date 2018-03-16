/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <libgen.h>
#include <limits.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern/arg.h"
#include "extern/linenoise.h"
#include "util.h"

/* defines */
#define car(P)  ((P)->v.p.car)
#define cdr(P)  ((P)->v.p.cdr)
#define nilp(P) ((P)->t == NIL)

/* typedefs */
struct Val;
typedef struct Val *Val;

/* improved interface for a pointer to a string */
typedef struct Str {
	char *d;
} *Str;

typedef enum { false, true } bool;

typedef enum {
	ERROR_OK,
	ERROR_SYNTAX
} Error;

/* fraction */
typedef struct {
	int num, den;
} Ratio;

typedef struct Entry *Entry;

typedef struct Hash {
	int size, cap;
	struct Entry {
		char *key;
		Val val;
	} *items;
	struct Hash *next;
} *Hash;

/* basic function written in C, not lisp */
typedef Val (*Prim)(Hash, Val);

/* function written directly in lisp instead of C */
typedef struct {
	Val args;
	Val body;
	Hash env;
} Func;

typedef struct {
	Val car, cdr;
} Pair;

typedef enum {
	NIL,
	BOOLEAN,
	INTEGER,
	RATIONAL,
	STRING,
	SYMBOL,
	PRIMITIVE,
	FUNCTION,
	PAIR,
} Type;

struct Val {
	Type t;
	union {
		int i;
		Ratio r;
		char *s;
		Prim pr;
		Func f;
		Pair p;
	} v;
};

/* functions */
static void hash_add(Hash ht, char *key, Val val);
static void hash_extend(Hash ht, Val args, Val vals);
static void hash_merge(Hash ht, Hash ht2);

Val read_val(Str str);
Val read_list(Str str);

static void tisp_print(Val v);
static Val tisp_eval(Hash env, Val v);

/* variables */
char *argv0;

struct Val nil;

#include "config.h"

static int
issymbol(char c)
{
	return BETWEEN(c, 'a', 'z') || strchr("+-*/", c);
}

/* TODO skip comments */
static void
skip_spaces(Str str) {
	for (; *str->d && isspace(*str->d); str->d++);
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
	return &nil;
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
static void
hash_extend(Hash ht, Val args, Val vals)
{
	Val arg, val;

	for (; !nilp(args) && !nilp(vals); args = cdr(args), vals = cdr(vals)) {
		arg = car(args);
		val = car(vals);
		if (arg->t != SYMBOL)
			die(1, "%s: hash_extend: Argument not a symbol", argv0);
		hash_add(ht, arg->v.s, val);
	}
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

Val
mk_val(Type t)
{
	Val v = emalloc(sizeof(struct Val));
	v->t = t;
	return v;
}

Val
mk_bool(bool b)
{
	Val v = mk_val(BOOLEAN);
	v->v.i = b;
	return v;
}

Val
mk_int(int i)
{
	Val v = mk_val(INTEGER);
	v->v.i = i;
	return v;
}

Val
mk_string(char *s)
{
	Val v = mk_val(STRING);
	v->v.s = s;
	return v;
}

Val
mk_symbol(char *s)
{
	Val v = mk_val(SYMBOL);
	v->v.s = s;
	return v;
}

Val
mk_prim(Prim pr)
{
	Val v = mk_val(PRIMITIVE);
	v->v.pr = pr;
	return v;
}

Val
mk_pair(Val a, Val b)
{
	Val ret = mk_val(PAIR);
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
	int i = 0;
	for (; isdigit(*str->d); str->d++)
		i = i * 10 + *str->d - '0';
	return mk_int(i);
}

Val
read_string(Str str)
{
	int len = 0;
	char *s = ++str->d;
	for (; *str->d && *str->d++ != '"'; len++);
	s[len] = '\0';
	return mk_string(estrdup(s));
}

Val
read_symbol(Str str)
{
	int len = 0;
	char *sym = str->d;
	for (; *str->d && issymbol(*str->d++); len++);
	sym[len] = '\0';
	return mk_symbol(estrdup(sym));
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
		a[n++] = read_val(str);
		skip_spaces(str);
	}
	b = mk_list(n, a);
	free(a);
	skip_spaces(str);
	return b;
}

Val
read_val(Str str)
{
	skip_spaces(str);
	if (*str->d == '#') /* TODO check 2nd char */
		return mk_bool(*(++str->d) == 't' ? true : false);
	if (isdigit(*str->d)) /* TODO negitive numbers */
		return read_int(str);
	if (*str->d == '"') /* TODO fix */
		return read_string(str);
	if (issymbol(*str->d))
		return read_symbol(str);
	if (*str->d == '(')
		return read_list(str);
	return NULL;
}

char *
hints(const char *buf, int *color, int *bold)
{
	struct { char *match, *hint; int color, bold; } hint[] = {
		{ "(lambda", " (args) (body))", 35, 0 },
		{ "(define", " var exp)",       35, 0 },
		{ NULL, NULL, 0, 0 }
	};
	for (int i = 0; hint[i].match; i++) {
		if (!strcasecmp(buf, hint[i].match)){
			*color = hint[i].color;
			*bold = hint[i].bold;
			return hint[i].hint;
		}
	}
	return NULL;
}

static Val
tisp_read(char *cmd)
{
	char *line;
	struct Str str;
	Val ast;

	if (cmd) {
		line = estrdup(cmd);
	} else {
		if (SHOW_HINTS)
			linenoiseSetHintsCallback(hints);
		if (!(line = linenoise("> ")))
			return NULL;
		linenoiseHistoryAdd(line);
	}

	str.d = line;
	ast = read_val(&str);
	free(line);

	return ast;
}

static Val
eval_pair(Hash env, Val v)
{
	int cap = 0, size = 0;
	Val *new = emalloc(sizeof(Val));
	while (!nilp(v)) {
		new[cap++] = tisp_eval(env, car(v));
		if (cap == size) {
			size *= 2;
			v = erealloc(v, size*sizeof(Val));
		}
		v = cdr(v);
	}
	Val ret = mk_list(cap, new);
	free(new);
	return ret;
}

static Val
tisp_trampoline(Hash env, Val v, int *continu)
{
	Val f, args;
	switch (v->t) {
	case NIL:
	case BOOLEAN:
	case INTEGER:
	case RATIONAL:
	case STRING:
		*continu = 0;
		return v;
	case SYMBOL:
		*continu = 0;
		return hash_get(env, v->v.s);
	case PAIR:
		f = tisp_eval(env, car(v));
		args = eval_pair(env, cdr(v));
		switch (f->t) {
		case PRIMITIVE:
			*continu = 0;
			return (*f->v.pr)(env, args);
		case FUNCTION:
			/* tail call into the function body with the extended env */
			hash_extend(env, f->v.f.args, args);
			hash_merge(env, f->v.f.env);
			*continu = 1;
			return f->v.f.body;
		default:
			die(1, "%s: Attempt to eval non primitive", argv0);
		}
	default: break;
	}
	return v;
}

static Val
tisp_eval(Hash env, Val v)
{
	int continu = 1;
	while(continu)
		v = tisp_trampoline(env, v, &continu);
	return v;
}

/* TODO return str for error msgs? */
static void
tisp_print(Val v)
{
	switch (v->t) {
	case NIL:
		printf("()");
		break;
	case BOOLEAN:
		printf(v->v.i ? "#t" : "#f");
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
	case FUNCTION:
		printf("#<closure>");
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
				puts(" . ");
				tisp_print(v);
				break;
			}
		}
		putchar(')');
		break;
	default:
		printf("%s: Could not print value type [%d]", argv0, v->t);
	}
}

static Val
add(Hash env, Val args)
{
	Val v;
	int i = 0;
	for (v = args; !nilp(v); v = cdr(v))
		i += car(v)->v.i;
	return mk_int(i);
}

static Hash
init_env(void)
{
	Hash h = hash_new(64);
	hash_add(h, "+", mk_prim(add));
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

static void
usage(const int eval)
{
	die(eval, "usage: %s [-hv] [CODE]", argv0);
}

int
main(int argc, char *argv[])
{
	ARGBEGIN {
	case 'h':
		usage(0);
	case 'v':
		printf("%s v%s (c) 2017-2018 Ed van Bruggen\n", argv0, VERSION);
		return 0;
	default:
		usage(1);
	} ARGEND;

	char *str = NULL;
	Val v;
	Hash env = init_env();

	nil.t = NIL;

	if (argc > 0 && argv[0][0] == '(')
		str = argv[0];

	while ((v = tisp_read(str))) {
		v = tisp_eval(env, v);

		tisp_print(v);
		putchar('\n');
		val_free(v);
		if (str) break;
	}

	for (; env; env = env->next) {
		for (int i = 0; i < env->cap; i++)
			if (env->items[i].key)
				val_free(env->items[i].val);
		free(env->items);
		free(env);
	}

	return 0;
}
