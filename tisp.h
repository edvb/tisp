/* zlib License
 *
 * Copyright (c) 2017-2018 Ed van Bruggen
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

#define warnf(M, ...) do {                          \
	fprintf(stderr, "tisp:%d: error: " M "\n",  \
	                  __LINE__, ##__VA_ARGS__); \
	return NULL;                                \
} while(0)

#define warn(M) do {                                \
	fprintf(stderr, "tisp:%d: error: " M "\n",  \
	                 __LINE__);                 \
	return NULL;                                \
} while(0)

#define car(P)  ((P)->v.p.car)
#define cdr(P)  ((P)->v.p.cdr)
#define nilp(P) ((P)->t == NIL)

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
	NIL,
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

struct Env {
	Val nil, t;
	Hash h;
	void **libh;
	size_t libhc;
};

void skip_spaces(Str str);
char *type_str(Type t);
int issym(char c);
int list_len(Val v);

void hash_add(Hash ht, char *key, Val val);

Val mk_int(int i);
Val mk_str(char *s);
Val mk_prim(Prim prim);
Val mk_rat(int num, int den);
Val mk_sym(char *s);
Val mk_func(Val args, Val body, Env env);
Val mk_pair(Val a, Val b);
Val mk_list(Env env, int n, Val *a);

Val eval_list(Env env, Val v);

Val tisp_read(Env env, Str str);
void tisp_print(Val v);
Val tisp_eval(Env env, Val v);

Env  tisp_env_init(size_t cap);
