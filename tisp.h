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

#define tsp_warnf(M, ...) do {                                  \
	fprintf(stderr, "; tisp: error: " M "\n", ##__VA_ARGS__); \
	return NULL;                                            \
} while(0)
#define tsp_warn(M) do {                         \
	fprintf(stderr, "; tisp: error: " M "\n"); \
	return NULL;                             \
} while(0)

/* TODO test general condition */
#define tsp_arg_min(ARGS, NAME, NARGS) do {                                    \
	if (list_len(ARGS) < NARGS)                                            \
		tsp_warnf("%s: expected at least %d argument%s, received %d",  \
		           NAME, NARGS, NARGS > 1 ? "s" : "", list_len(ARGS)); \
} while(0)
#define tsp_arg_max(ARGS, NAME, NARGS) do {                                          \
	if (list_len(ARGS) > NARGS)                                                  \
		tsp_warnf("%s: expected at no more than %d argument%s, received %d", \
		           NAME, NARGS, NARGS > 1 ? "s" : "", list_len(ARGS));       \
} while(0)
#define tsp_arg_num(ARGS, NAME, NARGS) do {                                    \
	if (list_len(ARGS) != NARGS && NARGS != -1)                            \
		tsp_warnf("%s: expected %d argument%s, received %d",           \
		           NAME, NARGS, NARGS > 1 ? "s" : "", list_len(ARGS)); \
} while(0)
#define tsp_arg_type(ARG, NAME, TYPE) do {                                     \
	if (!(ARG->t & (TYPE)))                                                \
		tsp_warnf(NAME ": expected %s, received %s",                   \
		                type_str(TYPE), type_str(ARG->t));             \
} while(0)

#define tsp_env_name_prim(NAME, FN) tisp_env_add(st, #NAME, mk_prim(TSP_PRIM, prim_##FN, #NAME))
#define tsp_env_prim(NAME)          tsp_env_name_prim(NAME, NAME)
#define tsp_env_name_form(NAME, FN) tisp_env_add(st, #NAME, mk_prim(TSP_FORM, form_##FN, #NAME))
#define tsp_env_form(NAME)          tsp_env_name_form(NAME, NAME)
#define tsp_include_tib(NAME)       void tib_env_##NAME(Tsp)

#define tsp_finc(ST) ST->filec++
#define tsp_fincn(ST, N) ST->filec += N
#define tsp_fgetat(ST, O) ST->file[ST->filec+O]
#define tsp_fget(ST) tsp_fgetat(ST,0)

#define car(P)  ((P)->v.p.car)
#define cdr(P)  ((P)->v.p.cdr)
#define caar(P) car(car(P))
#define cadr(P) car(cdr(P))
#define cdar(P) cdr(car(P))
#define cddr(P) cdr(cdr(P))
#define nilp(P) ((P)->t == TSP_NIL)
#define num(P)  ((P)->v.n.num)
#define den(P)  ((P)->v.n.den)

struct Val;
typedef struct Val *Val;
typedef struct Tsp *Tsp;

typedef struct Entry *Entry;

typedef struct Hash {
	int size, cap;
	struct Entry {
		char *key;
		Val val;
	} *items;
	struct Hash *next;
} *Hash;

/* possible tisp object types */
typedef enum {
	TSP_NONE  = 1 << 0,  /* void */
	TSP_NIL   = 1 << 1,  /* nil: false, empty list */
	TSP_INT   = 1 << 2,  /* integer: whole number */
	TSP_DEC   = 1 << 3,  /* decimal: floating point number */
	TSP_RATIO = 1 << 4,  /* ratio: numerator/denominator */
	TSP_STR   = 1 << 5,  /* string: immutable characters */
	TSP_SYM   = 1 << 6,  /* symbol: variable names */
	TSP_PRIM  = 1 << 7,  /* primitive: built-in function */
	TSP_FORM  = 1 << 8,  /* special form: built-in macro */
	TSP_FUNC  = 1 << 9,  /* function: procedure written is tisp */
	TSP_MACRO = 1 << 10, /* macro: function without evaluated arguments */
	TSP_PAIR  = 1 << 11, /* pair: building block for lists */
} TspType;
#define TSP_RATIONAL (TSP_INT | TSP_RATIO)
#define TSP_NUM      (TSP_RATIONAL | TSP_DEC)
/* TODO rename to expr type to math ? */
#define TSP_EXPR     (TSP_NUM | TSP_SYM | TSP_PAIR)

/* bultin function written in C, not tisp */
typedef Val (*Prim)(Tsp, Hash, Val);

/* tisp object */
struct Val {
	TspType t; /* NONE, NIL */
	union {
		char *s;                                            /* STRING, SYMBOL */
		struct { double num, den; } n;                      /* NUMBER */
		struct { char *name; Prim pr; } pr;                 /* PRIMITIVE, FORM */
		struct { char *name; Val args, body; Hash env; } f; /* FUNCTION, MACRO */
		struct { Val car, cdr; } p;                         /* PAIR */
	} v;
};

/* tisp state and global environment */
struct Tsp {
	char *file;
	size_t filec;
	Val none, nil, t;
	Hash global, strs, syms;
	void **libh;
	size_t libhc;
};

char *type_str(TspType t);
int list_len(Val v);

Val mk_int(int i);
Val mk_dec(double d);
Val mk_rat(int num, int den);
Val mk_str(Tsp st, char *s);
Val mk_sym(Tsp st, char *s);
Val mk_prim(TspType t, Prim prim, char *name);
Val mk_func(TspType t, char *name, Val args, Val body, Hash env);
Val mk_pair(Val a, Val b);
Val mk_list(Tsp st, int n, ...);

Val tisp_read(Tsp st);
Val tisp_read_line(Tsp st);
Val tisp_eval_list(Tsp st, Hash env, Val v);
Val tisp_eval_seq(Tsp st, Hash env, Val v);
Val tisp_eval(Tsp st, Hash env, Val v);
void tisp_print(FILE *f, Val v);

char *tisp_read_file(char *fname);
Val tisp_parse_file(Tsp st, char *fname);

void tisp_env_add(Tsp st, char *key, Val v);
Tsp  tisp_env_init(size_t cap);
void tisp_env_lib(Tsp st, char* lib);
