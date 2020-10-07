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
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../tisp.h"

#define EVAL_CHECK(A, V, NAME, TYPE) do {  \
	if (!(A = tisp_eval(st, vars, V))) \
		return NULL;               \
	tsp_arg_type(A, NAME, TYPE);       \
} while(0)

static Val
prim_numerator(Tsp st, Hash vars, Val args)
{
	Val a;
	tsp_arg_num(args, "numerator", 1);
	EVAL_CHECK(a, car(args), "numerator", TSP_RATIONAL);
	return mk_int(num(a));
}

static Val
prim_denominator(Tsp st, Hash vars, Val args)
{
	Val a;
	tsp_arg_num(args, "denominator", 1);
	EVAL_CHECK(a, car(args), "denominator", TSP_RATIONAL);
	return mk_int(den(a));
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

/* return pointer to one of the preceding functions depending on what
 * number should be created by the following arithmetic functions
 * force arg is used to force number to one type:
 *   0 -> no force, 1 -> force ratio/int, 2 -> force decimal */
static Val
(*mk_num(TspType a, TspType b, int force))(double, double)
{
	if (force == 1)
		return &create_rat;
	if (force == 2)
		return &create_dec;
	if (a & TSP_DEC || b & TSP_DEC)
		return &create_dec;
	if (a & TSP_RATIO || b & TSP_RATIO)
		return &create_rat;
	return &create_int;
}

#define PRIM_ROUND(NAME, FORCE)                                      \
static Val                                                           \
prim_##NAME(Tsp st, Hash vars, Val args)                             \
{                                                                    \
	Val a;                                                       \
	tsp_arg_num(args, #NAME, 1);                                 \
	EVAL_CHECK(a, car(args), #NAME, TSP_NUM);                    \
	return (mk_num(a->t, a->t, FORCE))(NAME(num(a)/den(a)), 1.); \
}

/* define int and dec as identity functions to use them in the same macro */
#define Int(X) (X)
#define Dec(X) (X)
PRIM_ROUND(Int,   1)
PRIM_ROUND(Dec,   2)
#undef Int
#undef Dec
PRIM_ROUND(round, 0)
PRIM_ROUND(floor, 0)
PRIM_ROUND(ceil,  0)

static Val
prim_add(Tsp st, Hash vars, Val args)
{
	Val a, b;
	tsp_arg_num(args, "+", 2);
	EVAL_CHECK(a, car(args), "+", TSP_NUM);
	EVAL_CHECK(b, car(cdr(args)), "+", TSP_NUM);
	if (a->t & TSP_DEC || b->t & TSP_DEC)
		return mk_dec((num(a)/den(a)) + (num(b)/den(b)));
	return (mk_num(a->t, b->t, 0))
		(num(a) * den(b) + den(a) * num(b),
		 den(a) * den(b));
}

static Val
prim_sub(Tsp st, Hash vars, Val args)
{
	Val a, b;
	int len = list_len(args);
	if (len != 2 && len != 1)
		tsp_warnf("-: expected 1 or 2 arguments, recieved %d", len);
	EVAL_CHECK(a, car(args), "-", TSP_NUM);
	if (len == 1) {
		b = a;
		a = mk_int(0);
	} else {
		EVAL_CHECK(b, car(cdr(args)), "-", TSP_NUM);
	}
	if (a->t & TSP_DEC || b->t & TSP_DEC)
		return mk_dec((num(a)/den(a)) - (num(b)/den(b)));
	return (mk_num(a->t, b->t, 0))
		(num(a) * den(b) - den(a) * num(b),
		 den(a) * den(b));
}

static Val
prim_mul(Tsp st, Hash vars, Val args)
{
	Val a, b;
	tsp_arg_num(args, "*", 2);
	EVAL_CHECK(a, car(args), "*", TSP_NUM);
	EVAL_CHECK(b, car(cdr(args)), "*", TSP_NUM);
	if (a->t & TSP_DEC || b->t & TSP_DEC)
		return mk_dec((num(a)/den(a)) * (num(b)/den(b)));
	return (mk_num(a->t, b->t, 0))(num(a) * num(b), den(a) * den(b));

}

static Val
prim_div(Tsp st, Hash vars, Val args)
{
	Val a, b;
	int len = list_len(args);
	if (len != 2 && len != 1)
		tsp_warnf("/: expected 1 or 2 arguments, recieved %d", len);
	EVAL_CHECK(a, car(args), "/", TSP_NUM);
	if (len == 1) {
		b = a;
		a = mk_int(1);
	} else {
		EVAL_CHECK(b, car(cdr(args)), "/", TSP_NUM);
	}
	if (a->t & TSP_DEC || b->t & TSP_DEC)
		return mk_dec((num(a)/den(a)) / (num(b)/den(b)));
	return (mk_num(a->t, b->t, 1))(num(a) * den(b), den(a) * num(b));
}

static Val
prim_mod(Tsp st, Hash vars, Val args)
{
	Val a, b;
	tsp_arg_num(args, "mod", 2);
	EVAL_CHECK(a, car(args), "mod", TSP_INT);
	EVAL_CHECK(b, car(cdr(args)), "mod", TSP_INT);
	if (num(b) == 0)
		tsp_warn("division by zero");
	return mk_int((int)num(a) % abs((int)num(b)));
}

/* TODO if given function as 2nd arg run it on first arg */
static Val
prim_pow(Tsp st, Hash vars, Val args)
{
	double bnum, bden;
	Val b, p;
	tsp_arg_num(args, "pow", 2);
	EVAL_CHECK(b, car(args), "pow", TSP_EXPR);
	EVAL_CHECK(p, car(cdr(args)), "pow", TSP_EXPR);
	bnum = pow(num(b), num(p)/den(p));
	bden = pow(den(b), num(p)/den(p));
	if ((bnum == (int)bnum && bden == (int)bden) ||
	     b->t & TSP_DEC || p->t & TSP_DEC)
		return mk_num(b->t, p->t, 0)(bnum, bden);
	return mk_pair(mk_sym(st, "^"), mk_pair(b, mk_pair(p, st->nil)));
}

#define PRIM_COMPARE(NAME, OP)                     \
static Val                                         \
prim_##NAME(Tsp st, Hash vars, Val args)           \
{                                                  \
	Val v;                                     \
	if (!(v = tisp_eval_list(st, vars, args))) \
		return NULL;                       \
	if (list_len(v) != 2)                      \
		return st->t;                      \
	tsp_arg_type(car(v), #OP, TSP_NUM);        \
	tsp_arg_type(car(cdr(v)), #OP, TSP_NUM);   \
	return ((num(car(v))*den(car(cdr(v)))) OP  \
		(num(car(cdr(v)))*den(car(v)))) ?  \
		st->t : st->nil;                   \
}

PRIM_COMPARE(lt,  <)
PRIM_COMPARE(gt,  >)
PRIM_COMPARE(lte, <=)
PRIM_COMPARE(gte, >=)

#define PRIM_TRIG(NAME)                                         \
static Val                                                      \
prim_##NAME(Tsp st, Hash vars, Val args)                        \
{                                                               \
	Val v;                                                  \
	tsp_arg_num(args, #NAME, 1);                            \
	EVAL_CHECK(v, car(args), #NAME, TSP_EXPR);              \
	if (v->t & TSP_DEC)                                     \
		return mk_dec(NAME(num(v)));                    \
	return mk_pair(mk_sym(st, #NAME), mk_pair(v, st->nil)); \
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
tib_env_math(Tsp st)
{
	tsp_env_fn(numerator);
	tsp_env_fn(denominator);

	tsp_env_fn(Int);
	tsp_env_fn(Dec);
	tsp_env_fn(floor);
	tsp_env_fn(ceil);
	tsp_env_fn(round);

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
