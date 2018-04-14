/* See LICENSE file for copyright and license details. */

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

struct Val nil;
struct Val t;

void skip_spaces(Str str);
char *type_str(Type t);

Val hash_add(Hash ht, char *key, Val val);

Val mk_int(int i);
Val mk_str(char *s);
Val mk_prim(Prim prim);
Val mk_rat(int num, int den);
Val mk_sym(char *s);
Val mk_func(Val args, Val body, Hash env);
Val mk_pair(Val a, Val b);
Val mk_list(int n, Val *a);

Val eval_list(Hash env, Val v);

Val tisp_read(Str str);
void tisp_print(Val v);
Val tisp_eval(Hash env, Val v);

Hash tisp_init_env(size_t cap);
