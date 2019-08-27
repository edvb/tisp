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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../tisp.h"

#define EVAL_CHECK(A, V, NAME, TYPE) do { \
	if (!(A = tisp_eval(env, V)))     \
		return NULL;              \
	tsp_arg_type(A, NAME, TYPE);      \
} while(0)

static Val
prim_numerator(Env env, Val args)
{
	Val a;
	tsp_arg_num(args, "numerator", 1);
	EVAL_CHECK(a, car(args), "numerator", RATIONAL);
	return mk_int(num(a));
}

static Val
prim_denominator(Env env, Val args)
{
	Val a;
	tsp_arg_num(args, "denominator", 1);
	EVAL_CHECK(a, car(args), "denominator", RATIONAL);
	return mk_int(den(a));
}

static Val
prim_dec(Env env, Val args)
{
	Val a;
	tsp_arg_num(args, "den", 1);
	EVAL_CHECK(a, car(args), "den", NUMBER);
	return mk_dec(num(a)/den(a));
}

/* wrapper functions to be returned by mk_num, all need same arguments */
static Val
create_int(double num, double den)
{
	assert(den == 1);
	return mk_int(num);
}

static Val
create_dec(double num, double den)
{
	assert(den == 1);
	return mk_dec(num);
}

static Val
create_rat(double num, double den)
{
	return mk_rat(num, den);
}

/* return pointer to one of the preceding functions depending on what sort
 * number should be created by the following arithmetic functions */
static Val
(*mk_num(Type a, Type b, int isfrac))(double, double)
{
	if (a & DECIMAL || b & DECIMAL)
		return &create_dec;
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
		return mk_dec((num(a)/den(a)) + (num(b)/den(b)));
	return (mk_num(a->t, b->t, 0))
		(num(a) * den(b) + den(a) * num(b),
		 den(a) * den(b));
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
		return mk_dec((num(a)/den(a)) - (num(b)/den(b)));
	return (mk_num(a->t, b->t, 0))
		(num(a) * den(b) - den(a) * num(b),
		 den(a) * den(b));
}

static Val
prim_mul(Env env, Val args)
{
	Val a, b;
	tsp_arg_num(args, "*", 2);
	EVAL_CHECK(a, car(args), "*", NUMBER);
	EVAL_CHECK(b, car(cdr(args)), "*", NUMBER);
	if (a->t & DECIMAL || b->t & DECIMAL)
		return mk_dec((num(a)/den(a)) * (num(b)/den(b)));
	return (mk_num(a->t, b->t, 0))(num(a) * num(b), den(a) * den(b));

}

static Val
prim_div(Env env, Val args)
{
	Val a, b;
	int len = list_len(args);
	if (len != 2 && len != 1)
		tsp_warnf("/: expected 1 or 2 arguments, recieved %d", len);
	EVAL_CHECK(a, car(args), "/", NUMBER);
	if (len == 1) {
		b = a;
		a = mk_int(1);
	} else {
		EVAL_CHECK(b, car(cdr(args)), "/", NUMBER);
	}
	if (a->t & DECIMAL || b->t & DECIMAL)
		return mk_dec((num(a)/den(a)) / (num(b)/den(b)));
	return (mk_num(a->t, b->t, 1))(num(a) * den(b), den(a) * num(b));
}

static Val
prim_mod(Env env, Val args)
{
	Val a, b;
	tsp_arg_num(args, "mod", 2);
	EVAL_CHECK(a, car(args), "mod", INTEGER);
	EVAL_CHECK(b, car(cdr(args)), "mod", INTEGER);
	if (num(b) == 0)
		tsp_warn("division by zero");
	return mk_int((int)num(a) % abs((int)num(b)));
}

static Val
prim_pow(Env env, Val args)
{
	double bnum, bden;
	Val b, p;
	tsp_arg_num(args, "pow", 2);
	EVAL_CHECK(b, car(args), "pow", EXPRESSION);
	EVAL_CHECK(p, car(cdr(args)), "pow", EXPRESSION);
	bnum = pow(num(b), num(p)/den(p));
	bden = pow(den(b), num(p)/den(p));
	if (bnum == (int)bnum && bden == (int)bden &&
	    b->t & NUMBER && p->t & NUMBER)
		return mk_num(b->t, p->t, 0)(bnum, bden);
	return mk_pair(mk_sym(env, "^"), mk_pair(b, mk_pair(p, env->nil)));
}

#define PRIM_COMPARE(NAME, OP)                    \
static Val                                        \
prim_##NAME(Env env, Val args)                    \
{                                                 \
	Val v;                                    \
	if (!(v = tisp_eval_list(env, args)))     \
		return NULL;                      \
	if (list_len(v) != 2)                     \
		return env->t;                    \
	tsp_arg_type(car(v), #OP, NUMBER);        \
	tsp_arg_type(car(cdr(v)), #OP, NUMBER);   \
	return ((num(car(v))*den(car(cdr(v)))) OP \
		(num(car(cdr(v)))*den(car(v)))) ? \
		env->t : env->nil;                \
}

PRIM_COMPARE(lt,  <)
PRIM_COMPARE(gt,  >)
PRIM_COMPARE(lte, <=)
PRIM_COMPARE(gte, >=)

#define PRIM_TRIG(NAME)                                           \
static Val                                                        \
prim_##NAME(Env env, Val args)                                    \
{                                                                 \
	Val v;                                                    \
	tsp_arg_num(args, #NAME, 1);                              \
	EVAL_CHECK(v, car(args), #NAME, EXPRESSION);              \
	if (v->t & DECIMAL)                                       \
		return mk_dec(NAME(num(v)));                      \
	return mk_pair(mk_sym(env, #NAME), mk_pair(v, env->nil)); \
}

PRIM_TRIG(sin)
PRIM_TRIG(cos)
PRIM_TRIG(tan)
PRIM_TRIG(sinh)
PRIM_TRIG(cosh)
PRIM_TRIG(tanh)
PRIM_TRIG(asin)
PRIM_TRIG(acos)
PRIM_TRIG(atan)
PRIM_TRIG(asinh)
PRIM_TRIG(acosh)
PRIM_TRIG(atanh)
PRIM_TRIG(exp)
PRIM_TRIG(log)

void
tib_env_math(Env env)
{
	tsp_env_fn(numerator);
	tsp_env_fn(denominator);

	tsp_env_fn(dec);

	tsp_env_name_fn(+, add);
	tsp_env_name_fn(-, sub);
	tsp_env_name_fn(*, mul);
	tsp_env_name_fn(/, div);
	tsp_env_fn(mod);
	tsp_env_name_fn(^, pow);

	tsp_env_name_fn(<,  lt);
	tsp_env_name_fn(>,  gt);
	tsp_env_name_fn(<=, lte);
	tsp_env_name_fn(>=, gte);

	tsp_env_fn(sin);
	tsp_env_fn(cos);
	tsp_env_fn(tan);
	tsp_env_fn(sinh);
	tsp_env_fn(cosh);
	tsp_env_fn(tanh);
	tsp_env_name_fn(arcsin,  asin);
	tsp_env_name_fn(arccos,  acos);
	tsp_env_name_fn(arctan,  atan);
	tsp_env_name_fn(arcsinh, asinh);
	tsp_env_name_fn(arccosh, acosh);
	tsp_env_name_fn(arctanh, atanh);
	tsp_env_fn(exp);
	tsp_env_fn(log);
}
