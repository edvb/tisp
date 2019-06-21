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

#ifdef DEBUG
#  define tsp_warnf(M, ...) do {                              \
	fprintf(stderr, "tisp:%s:%d: error: " M "\n",         \
	                  __FILE__, __LINE__, ##__VA_ARGS__); \
	return NULL;                                          \
} while(0)
#  define tsp_warn(M) do {                                                 \
	fprintf(stderr, "tisp:%s:%d: error: " M "\n", __FILE__, __LINE__); \
	return NULL;                                                       \
} while(0)
#else
#  define tsp_warnf(M, ...) do {                                \
	fprintf(stderr, "tisp: error: " M "\n", ##__VA_ARGS__); \
	return NULL;                                            \
} while(0)
#  define tsp_warn(M) do {                       \
	fprintf(stderr, "tisp: error: " M "\n"); \
	return NULL;                             \
} while(0)
#endif
#define tsp_arg_num(ARGS, NAME, NARGS) do {                                    \
	if (list_len(ARGS) != NARGS && NARGS != -1)                            \
		tsp_warnf("%s: expected %d argument%s, received %d",           \
		           NAME, NARGS, NARGS > 1 ? "s" : "", list_len(ARGS)); \
} while(0)
#define tsp_arg_type(ARG, NAME, TYPE) do {                         \
	if (!(ARG->t & (TYPE)))                                    \
		tsp_warnf(NAME ": expected %s, received %s",       \
		                type_str(TYPE), type_str(ARG->t)); \
} while(0)

#define tsp_env_name_fn(NAME, FN) tisp_env_add(env, #NAME, mk_prim(prim_##FN))
#define tsp_env_fn(NAME)          tsp_env_name_fn(NAME, NAME)

#define car(P)  ((P)->v.p.car)
#define cdr(P)  ((P)->v.p.cdr)
#define caar(P) car(car(P))
#define cadr(P) car(cdr(P))
#define cdar(P) cdr(car(P))
#define cddr(P) cdr(cdr(P))
#define nilp(P) ((P)->t == NIL)
#define num(P)  ((P)->v.n.num)
#define den(P)  ((P)->v.n.den)

struct Val;
typedef struct Val *Val;
typedef struct Env *Env;

/* improved interface for a pointer to a string */
typedef struct Str {
	char *d;
} *Str;

typedef enum {
	ERROR_OK,
	ERROR_SYNTAX
} Error;

/* fraction */
typedef struct {
	double num, den;
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
typedef Val (*Prim)(Env, Val);

/* function written directly in lisp instead of C */
typedef struct {
	Val args;
	Val body;
	Env env;
} Func;

typedef struct {
	Val car, cdr;
} Pair;

typedef enum {
	NONE      = 1 << 0,
	NIL       = 1 << 1,
	INTEGER   = 1 << 2,
	DECIMAL   = 1 << 3,
	RATIO     = 1 << 4,
	STRING    = 1 << 5,
	SYMBOL    = 1 << 6,
	PRIMITIVE = 1 << 7,
	FUNCTION  = 1 << 8,
	MACRO     = 1 << 9,
	PAIR      = 1 << 10,
} Type;
#define RATIONAL   (INTEGER | RATIO)
#define NUMBER     (RATIONAL | DECIMAL)
#define EXPRESSION (NUMBER | SYMBOL | PAIR)

struct Val {
	Type t; /* NONE, NIL */
	union {
		Ratio n; /* INTEGER, DECIMAL, RATIO */
		char *s; /* STRING, SYMBOL */
		Prim pr; /* PRIMITIVE */
		Func f;  /* FUNCTION */
		Pair p;  /* PAIR */
	} v;
};

struct Env {
	Val none, nil, t;
	Hash h, strs, syms;
	void **libh;
	size_t libhc;
};

char *type_str(Type t);
int list_len(Val v);
void skip_ws(Str str);

Val mk_int(int i);
Val mk_str(Env env, char *s);
Val mk_prim(Prim prim);
Val mk_dec(double d);
Val mk_rat(int num, int den);
Val mk_sym(Env env, char *s);
Val mk_func(Type t, Val args, Val body, Env env);
Val mk_pair(Val a, Val b);
Val mk_list(Env env, int n, Val *a);

Val tisp_read(Env env, Str str);
Val tisp_eval_list(Env env, Val v);
Val tisp_eval(Env env, Val v);
void tisp_print(FILE *f, Val v);

char *tisp_read_file(char *fname);
Val tisp_parse_file(Env env, char *fname);

void tisp_env_add(Env e, char *key, Val v);
Env  tisp_env_init(size_t cap);
void tisp_env_free(Env env);
