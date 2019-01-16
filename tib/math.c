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
#include <stdio.h>
#include <stdlib.h>

#include "../tisp.h"

#define EVAL_CHECK(A, V, NAME, TYPE) do { \
	if (!(A = tisp_eval(env, V)))     \
		return NULL;              \
	tsp_arg_type(A, NAME, TYPE);      \
} while(0)

#define RATIO_DUB(A, OP, B) return mk_dub(((double)A->v.r.num/A->v.r.den) OP B->v.n)
#define RATIO_INT(A, OP, B) return mk_rat(A->v.r.num OP (B->v.n * A->v.r.den), A->v.r.den)
#define RATIO_INT2(A, OP, B) return mk_rat(A->v.r.num OP B->v.n, A->v.r.den)
#define COMBINE_FIN(A, OP, B) do {               \
	if (A->t & DOUBLE || B->t & DOUBLE)      \
		return mk_dub(A->v.n OP B->v.n); \
	return mk_int(A->v.n OP B->v.n);         \
} while(0)

static Val
prim_add(Env env, Val args)
{
	Val a, b;
	tsp_arg_num(args, "+", 2);
	EVAL_CHECK(a, car(args), "+", NUMBER);
	EVAL_CHECK(b, car(cdr(args)), "+", NUMBER);
	switch (a->t) {
	case RATIO:
		switch (b->t) {
		case RATIO:
			return mk_rat(a->v.r.num * b->v.r.den + a->v.r.den * b->v.r.num,
			              a->v.r.den * b->v.r.den);
		case INTEGER:
			RATIO_INT(a, +, b);
		case DOUBLE:
			RATIO_DUB(a, +, b);
		default: break;
		}
	case INTEGER:
		if (b->t & RATIO)
			RATIO_INT(b, +, a);
	case DOUBLE:
		if (b->t & RATIO)
			RATIO_DUB(b, +, a);
	default: break;
	}
	COMBINE_FIN(a, +, b);
}

static Val
prim_sub(Env env, Val args)
{
	Val a, b;
	int len = list_len(args);
	if (len != 2 && len != 1)
		tsp_warnf("-: expected 1 or 2 arguments, recieved %d", len);
	EVAL_CHECK(a, car(args), "-", NUMBER);
	if (len == 1) {
		b = a;
		a = mk_int(0);
	} else {
		EVAL_CHECK(b, car(cdr(args)), "-", NUMBER);
	}
	switch (a->t) {
	case RATIO:
		switch (b->t) {
		case RATIO:
			return mk_rat(a->v.r.num * b->v.r.den - a->v.r.den * b->v.r.num,
			              a->v.r.den * b->v.r.den);
		case INTEGER:
			RATIO_INT(a, -, b);
		case DOUBLE:
			RATIO_DUB(a, -, b);
		default: break;
		}
	case INTEGER:
		if (b->t & RATIO)
			return mk_rat((a->v.n * b->v.r.den) - b->v.r.num, b->v.r.den);
	case DOUBLE:
		if (b->t & RATIO)
			return mk_dub(a->v.n - ((double)b->v.r.num/b->v.r.den));
	default: break;
	}
	COMBINE_FIN(a, -, b);
}

static Val
prim_mul(Env env, Val args)
{
	Val a, b;
	tsp_arg_num(args, "*", 2);
	EVAL_CHECK(a, car(args), "*", NUMBER);
	EVAL_CHECK(b, car(cdr(args)), "*", NUMBER);
	switch (a->t) {
	case RATIO:
		switch (b->t) {
		case RATIO:
			return mk_rat(a->v.r.num * b->v.r.num, a->v.r.den * b->v.r.den);
		case INTEGER:
			RATIO_INT2(a, *, b);
		case DOUBLE:
			RATIO_DUB(a, *, b);
		default: break;
		}
	case INTEGER:
		if (b->t & RATIO)
			RATIO_INT2(b, *, a);
	case DOUBLE:
		if (b->t & RATIO)
			RATIO_DUB(b, *, a);
	default: break;
	}
	COMBINE_FIN(a, *, b);
}

static Val
prim_div(Env env, Val args)
{
	Val a, b;
	tsp_arg_num(args, "/", 2);
	EVAL_CHECK(a, car(args), "/", NUMBER);
	EVAL_CHECK(b, car(cdr(args)), "/", NUMBER);
	switch (a->t) {
	case RATIO:
		switch (b->t) {
		case RATIO:
			return mk_rat(a->v.r.num * b->v.r.den, a->v.r.den * b->v.r.num);
		case INTEGER:
			return mk_rat(a->v.r.num, a->v.r.den * b->v.n);
		case DOUBLE:
			RATIO_DUB(a, /, b);
		default: break;
		}
	case INTEGER:
		if (b->t & RATIO)
			return mk_rat(b->v.r.den * a->v.n, b->v.r.num);
	case DOUBLE:
		if (b->t & RATIO)
			return mk_dub(a->v.n / ((double)b->v.r.num/b->v.r.den));
	default: break;
	}
	if (a->t & DOUBLE || b->t & DOUBLE)
		return mk_dub(a->v.n / b->v.n);
	return mk_rat(a->v.n, b->v.n);
}

static Val
prim_mod(Env env, Val args)
{
	Val a, b;
	tsp_arg_num(args, "mod", 2);
	EVAL_CHECK(a, car(args), "mod", INTEGER);
	EVAL_CHECK(b, car(cdr(args)), "mod", INTEGER);
	if (b->v.n == 0)
		tsp_warn("division by zero");
	return mk_int((int)a->v.n % abs((int)b->v.n));
}

#define PRIM_COMPARE(NAME, OP, FUNC)                                  \
static Val                                                            \
prim_##NAME(Env env, Val args)                                        \
{                                                                     \
	Val v;                                                        \
	if (!(v = tisp_eval_list(env, args)))                         \
		return NULL;                                          \
	if (list_len(v) != 2)                                         \
		return env->t;                                        \
	tsp_arg_type(car(v), FUNC, INTEGER);                          \
	tsp_arg_type(car(cdr(v)), FUNC, INTEGER);                     \
	return (car(v)->v.n OP car(cdr(v))->v.n) ? env->t : env->nil; \
}

PRIM_COMPARE(lt,  <,  "<")
PRIM_COMPARE(gt,  >,  ">")
PRIM_COMPARE(lte, <=, "<=")
PRIM_COMPARE(gte, >=, ">=")

void
tib_env_math(Env env)
{
	tisp_env_add(env, "pi",  mk_dub(3.141592653589793));
	tisp_env_add(env, "e",   mk_dub(2.718281828459045));

	tisp_env_add(env, "+",   mk_prim(prim_add));
	tisp_env_add(env, "-",   mk_prim(prim_sub));
	tisp_env_add(env, "*",   mk_prim(prim_mul));
	tisp_env_add(env, "/",   mk_prim(prim_div));
	tisp_env_add(env, "mod", mk_prim(prim_mod));

	tisp_env_add(env, "<",   mk_prim(prim_lt));
	tisp_env_add(env, ">",   mk_prim(prim_gt));
	tisp_env_add(env, "<=",  mk_prim(prim_lte));
	tisp_env_add(env, ">=",  mk_prim(prim_gte));
}
