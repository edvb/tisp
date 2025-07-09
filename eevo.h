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
#ifndef EEVO_H
#define EEVO_H

#include <stdlib.h>
#include <stdio.h>

#define EEVO_REC_FACTOR 2

#define EEVO_OP_CHARS "_+-*/\\|=^<>.:"
#define EEVO_SYM_CHARS "_!?" "@#$%&~" "*-"

#define eevo_warnf(M, ...) do {                                    \
	fprintf(stderr, "; eevo: error: " M "\n", ##__VA_ARGS__); \
	return NULL;                                              \
} while(0)
#define eevo_warn(M) do {                           \
	fprintf(stderr, "; eevo: error: " M "\n"); \
	return NULL;                               \
} while(0)

/* TODO test general condition */
#define eevo_arg_min(ARGS, NAME, NARGS) do {                                      \
	if (eevo_lstlen(ARGS) < NARGS)                                            \
		eevo_warnf("%s: expected at least %d argument%s, received %d",    \
		           NAME, NARGS, NARGS > 1 ? "s" : "", eevo_lstlen(ARGS)); \
} while(0)
#define eevo_arg_max(ARGS, NAME, NARGS) do {                                          \
	if (eevo_lstlen(ARGS) > NARGS)                                                \
		eevo_warnf("%s: expected at no more than %d argument%s, received %d", \
		           NAME, NARGS, NARGS > 1 ? "s" : "", eevo_lstlen(ARGS));     \
} while(0)
#define eevo_arg_num(ARGS, NAME, NARGS) do {                                      \
	if (NARGS > -1 && eevo_lstlen(ARGS) != NARGS)                             \
		eevo_warnf("%s: expected %d argument%s, received %d",             \
		           NAME, NARGS, NARGS > 1 ? "s" : "", eevo_lstlen(ARGS)); \
} while(0)
#define eevo_arg_type(ARG, NAME, TYPE) do {                                     \
	if (!(ARG->t & (TYPE)))                                                \
		eevo_warnf(NAME ": expected %s, received %s",                   \
		                eevo_type_str(TYPE), eevo_type_str(ARG->t));     \
} while(0)

#define eevo_env_name_prim(NAME, FN) eevo_env_add(st, #NAME, eevo_prim(EEVO_PRIM, prim_##FN, #NAME))
#define eevo_env_prim(NAME)          eevo_env_name_prim(NAME, NAME)
#define eevo_env_name_form(NAME, FN) eevo_env_add(st, #NAME, eevo_prim(EEVO_FORM, form_##FN, #NAME))
#define eevo_env_form(NAME)          eevo_env_name_form(NAME, NAME)

#define eevo_fgetat(ST, O) ST->file[ST->filec+O]
#define eevo_fget(ST) eevo_fgetat(ST,0)
#define eevo_finc(ST) ST->filec++
#define eevo_fincn(ST, N) ST->filec += N

struct Eevo_;
typedef struct Eevo_ *Eevo;
typedef struct EevoSt_ *EevoSt;

typedef struct EevoEntry_ *EevoEntry;

typedef struct EevoRec_ {
	int size, cap;
	struct EevoEntry_ {
		char *key;
		Eevo val;
	} *items;
	struct EevoRec_ *next;
} *EevoRec;

/* possible eevo value types */
typedef enum {
	EEVO_NONE  = 1 << 0,  /* void */
	EEVO_NIL   = 1 << 1,  /* nil: false, empty list */
	EEVO_INT   = 1 << 2,  /* integer: whole number */
	EEVO_DEC   = 1 << 3,  /* decimal: floating point number */
	EEVO_RATIO = 1 << 4,  /* ratio: numerator/denominator */
	EEVO_STR   = 1 << 5,  /* string: immutable characters */
	EEVO_SYM   = 1 << 6,  /* symbol: variable names */
	EEVO_PRIM  = 1 << 7,  /* primitive: built-in function */
	EEVO_FORM  = 1 << 8,  /* special form: built-in macro */
	EEVO_FUNC  = 1 << 9,  /* function: procedure written is eevo */
	EEVO_MACRO = 1 << 10, /* macro: function without evaluated arguments */
	EEVO_PAIR  = 1 << 11, /* pair: building block for lists */
	EEVO_REC   = 1 << 12, /* record: hash table */
	EEVO_TYPE  = 1 << 13, /* type: kind of eevo value */
	EEVO_RATIONAL = EEVO_INT | EEVO_RATIO,
	EEVO_NUM      = EEVO_RATIONAL | EEVO_DEC,
	/* TODO rename to expr type to math ? */
	EEVO_EXPR     = EEVO_NUM | EEVO_SYM | EEVO_PAIR,
	EEVO_TEXT     = EEVO_STR | EEVO_SYM,
	EEVO_PROC     = EEVO_FUNC | EEVO_PRIM | EEVO_MACRO | EEVO_FORM,
	EEVO_LIT      = EEVO_NONE | EEVO_NIL | EEVO_NUM | EEVO_STR | EEVO_PROC,
	EEVO_LIST     = EEVO_PAIR | EEVO_NIL,
	EEVO_CALLABLE = EEVO_PROC | EEVO_REC | EEVO_TYPE, // | EEVO_PAIR
	EEVO_FUNCTOR  = EEVO_PAIR | EEVO_REC | EEVO_TYPE,
} EevoType;

typedef struct EevoTypeVal_ {
	EevoType t;
	char *name;
	Eevo func;
	/* Eevo cond; /1* refinement condition *1/ */
} EevoTypeVal;

/* bultin function written in C, not eevo */
typedef Eevo (*EevoPrim)(EevoSt, EevoRec, Eevo);

/* eevo object */
struct Eevo_ {
	EevoType t; /* NONE, NIL */
	union {
		char *s;                                                /* STRING, SYMBOL */
		struct { double num, den; } n;                          /* NUMBER */
		struct { char *name; EevoPrim pr; } pr;                 /* PRIMITIVE, FORM */
		struct { char *name; Eevo args, body; EevoRec env; } f; /* FUNCTION, MACRO */
		struct { Eevo fst, rst; } p;                            /* PAIR */
		EevoRec r;                                              /* REC */
		EevoTypeVal t;                                          /* TYPE */
	} v;
};

/* eevo state and global environment */
struct EevoSt_ {
	char *file;
	size_t filec;
	Eevo none, nil, t;
	Eevo types[14];
	EevoRec env, strs, syms;
	void **libh;
	size_t libhc;
};

char *eevo_type_str(EevoType t);
int eevo_lstlen(Eevo v);

Eevo eevo_int(int i);
Eevo eevo_dec(double d);
Eevo eevo_rat(int num, int den);
Eevo eevo_str(EevoSt st, char *s);
Eevo eevo_sym(EevoSt st, char *s);
Eevo eevo_prim(EevoType t, EevoPrim prim, char *name);
Eevo eevo_func(EevoType t, char *name, Eevo args, Eevo body, EevoRec env);
Eevo eevo_rec(EevoSt st, EevoRec prev, Eevo records);
Eevo eevo_pair(Eevo a, Eevo b);
Eevo eevo_list(EevoSt st, int n, ...);

Eevo eevo_read_sexpr(EevoSt st);
Eevo eevo_read(EevoSt st);
Eevo eevo_read_sugar(EevoSt st, Eevo v);
Eevo eevo_read_line(EevoSt st, int level);
Eevo eevo_eval_list(EevoSt st, EevoRec env, Eevo v);
Eevo eevo_eval_body(EevoSt st, EevoRec env, Eevo v);
Eevo eevo_eval(EevoSt st, EevoRec env, Eevo v);
char *eevo_print(Eevo v);

void   eevo_env_add(EevoSt st, char *key, Eevo v);
EevoSt eevo_env_init(size_t cap);
Eevo   eevo_env_lib(EevoSt st, char* lib);

void eevo_env_core(EevoSt);
void eevo_env_string(EevoSt);
void eevo_env_math(EevoSt);
void eevo_env_io(EevoSt);
void eevo_env_os(EevoSt);
#endif // EEVO_H
