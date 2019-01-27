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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../tisp.h"

#define EVAL_CHECK(A, V, NAME, TYPE) do { \
	if (!(A = tisp_eval(env, V)))     \
		return NULL;              \
	tsp_arg_type(A, NAME, TYPE);      \
} while(0)

/* Wrapper functions to be returned by mk_num, all need same arguments */
static Val
create_int(double num, double den)
{
	assert(den == 1);
	return mk_int(num);
}

static Val
create_dub(double num, double den)
{
	assert(den == 1);
	return mk_dub(num);
}

static Val
create_rat(double num, double den)
{
	return mk_rat(num, den);
}

/* Return pointer to one of the preceding functions depending on what sort
 * number should be created by the following arithmetic functions */
static Val
(*mk_num(Type a, Type b, int isfrac))(double, double)
{
	if (a & DECIMAL || b & DECIMAL)
		return &create_dub;
	if (isfrac || a & RATIO || b & RATIO)
		return &create_rat;
	return &create_int;
}

static Val
prim_add(Env env, Val args)
{
	Val a, b;
	tsp_arg_num(args, "+", 2);
	EVAL_CHECK(a, car(args), "+", NUMBER);
	EVAL_CHECK(b, car(cdr(args)), "+", NUMBER);
	if (a->t & DECIMAL || b->t & DECIMAL)
		return mk_dub((a->v.n.num/a->v.n.den) + (b->v.n.num/b->v.n.den));
	return (mk_num(a->t, b->t, 0))
		(a->v.n.num * b->v.n.den + a->v.n.den * b->v.n.num,
		 a->v.n.den * b->v.n.den);
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
	if (a->t & DECIMAL || b->t & DECIMAL)
		return mk_dub((a->v.n.num/a->v.n.den) - (b->v.n.num/b->v.n.den));
	return (mk_num(a->t, b->t, 0))
		(a->v.n.num * b->v.n.den - a->v.n.den * b->v.n.num,
		 a->v.n.den * b->v.n.den);
}

static Val
prim_mul(Env env, Val args)
{
	Val a, b;
	tsp_arg_num(args, "*", 2);
	EVAL_CHECK(a, car(args), "*", NUMBER);
	EVAL_CHECK(b, car(cdr(args)), "*", NUMBER);
	if (a->t & DECIMAL || b->t & DECIMAL)
		return mk_dub((a->v.n.num/a->v.n.den) * (b->v.n.num/b->v.n.den));
	return (mk_num(a->t, b->t, 0))
		(a->v.n.num * b->v.n.num, a->v.n.den * b->v.n.den);

}

static Val
prim_div(Env env, Val args)
{
	Val a, b;
	tsp_arg_num(args, "/", 2);
	EVAL_CHECK(a, car(args), "/", NUMBER);
	EVAL_CHECK(b, car(cdr(args)), "/", NUMBER);
	if (a->t & DECIMAL || b->t & DECIMAL)
		return mk_dub((a->v.n.num/a->v.n.den) / (b->v.n.num/b->v.n.den));
	return (mk_num(a->t, b->t, 1))
		(a->v.n.num * b->v.n.den, a->v.n.den * b->v.n.num);
}

static Val
prim_mod(Env env, Val args)
{
	Val a, b;
	tsp_arg_num(args, "mod", 2);
	EVAL_CHECK(a, car(args), "mod", INTEGER);
	EVAL_CHECK(b, car(cdr(args)), "mod", INTEGER);
	if (b->v.n.num == 0)
		tsp_warn("division by zero");
	return mk_int((int)a->v.n.num % abs((int)b->v.n.num));
}

#define PRIM_COMPARE(NAME, OP, FUNC)                                          \
static Val                                                                    \
prim_##NAME(Env env, Val args)                                                \
{                                                                             \
	Val v;                                                                \
	if (!(v = tisp_eval_list(env, args)))                                 \
		return NULL;                                                  \
	if (list_len(v) != 2)                                                 \
		return env->t;                                                \
	tsp_arg_type(car(v), FUNC, INTEGER);                                  \
	tsp_arg_type(car(cdr(v)), FUNC, INTEGER);                             \
	return (car(v)->v.n.num OP car(cdr(v))->v.n.num) ? env->t : env->nil; \
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
