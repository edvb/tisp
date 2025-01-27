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
#include <assert.h>
#include <math.h>

#define EVAL_CHECK(A, V, NAME, TYPE) do {  \
	if (!(A = tisp_eval(st, vars, V))) \
		return NULL;               \
	tsp_arg_type(A, NAME, TYPE);       \
} while(0)

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
prim_##NAME(Tsp st, Rec vars, Val args)                              \
{                                                                    \
	Val n;                                                       \
	tsp_arg_num(args, #NAME, 1);                                 \
	n = car(args);                                               \
	tsp_arg_type(n, #NAME, TSP_NUM);                             \
	return (mk_num(n->t, n->t, FORCE))(NAME(num(n)/den(n)), 1.); \
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
prim_add(Tsp st, Rec vars, Val args)
{
	Val a, b;
	tsp_arg_num(args, "+", 2);
	a = car(args), b = cadr(args);
	tsp_arg_type(a, "+", TSP_NUM);
	tsp_arg_type(b, "+", TSP_NUM);
	if (a->t & TSP_DEC || b->t & TSP_DEC)
		return mk_dec((num(a)/den(a)) + (num(b)/den(b)));
	return (mk_num(a->t, b->t, 0))
		(num(a) * den(b) + den(a) * num(b),
		 den(a) * den(b));
}

static Val
prim_sub(Tsp st, Rec vars, Val args)
{
	Val a, b;
	int len = tsp_lstlen(args);
	if (len != 2 && len != 1)
		tsp_warnf("-: expected 1 or 2 arguments, recieved %d", len);
	a = car(args);
	tsp_arg_type(a, "-", TSP_NUM);
	if (len == 1) {
		b = a;
		a = mk_int(0);
	} else {
		b = cadr(args);
		tsp_arg_type(b, "-", TSP_NUM);
	}
	if (a->t & TSP_DEC || b->t & TSP_DEC)
		return mk_dec((num(a)/den(a)) - (num(b)/den(b)));
	return (mk_num(a->t, b->t, 0))
		(num(a) * den(b) - den(a) * num(b),
		 den(a) * den(b));
}

static Val
prim_mul(Tsp st, Rec vars, Val args)
{
	Val a, b;
	tsp_arg_num(args, "*", 2);
	a = car(args), b = cadr(args);
	tsp_arg_type(a, "*", TSP_NUM);
	tsp_arg_type(b, "*", TSP_NUM);
	if (a->t & TSP_DEC || b->t & TSP_DEC)
		return mk_dec((num(a)/den(a)) * (num(b)/den(b)));
	return (mk_num(a->t, b->t, 0))(num(a) * num(b), den(a) * den(b));

}

static Val
prim_div(Tsp st, Rec vars, Val args)
{
	Val a, b;
	int len = tsp_lstlen(args);
	if (len != 2 && len != 1)
		tsp_warnf("/: expected 1 or 2 arguments, recieved %d", len);
	a = car(args);
	tsp_arg_type(a, "/", TSP_NUM);
	if (len == 1) {
		b = a;
		a = mk_int(1);
	} else {
		b = cadr(args);
		tsp_arg_type(b, "/", TSP_NUM);
	}
	if (a->t & TSP_DEC || b->t & TSP_DEC)
		return mk_dec((num(a)/den(a)) / (num(b)/den(b)));
	return (mk_num(a->t, b->t, 1))(num(a) * den(b), den(a) * num(b));
}

static Val
prim_mod(Tsp st, Rec vars, Val args)
{
	Val a, b;
	tsp_arg_num(args, "mod", 2);
	a = car(args), b = cadr(args);
	tsp_arg_type(a, "mod", TSP_INT);
	tsp_arg_type(b, "mod", TSP_INT);
	if (num(b) == 0)
		tsp_warn("division by zero");
	return mk_int((int)num(a) % abs((int)num(b)));
}

/* TODO if given function as 2nd arg run it on first arg */
static Val
prim_pow(Tsp st, Rec vars, Val args)
{
	Val b, p;
	double bnum, bden;
	tsp_arg_num(args, "pow", 2);
	b = car(args), p = cadr(args);
	tsp_arg_type(b, "pow", TSP_EXPR);
	tsp_arg_type(p, "pow", TSP_EXPR);
	bnum = pow(num(b), num(p)/den(p));
	bden = pow(den(b), num(p)/den(p));
	if ((bnum == (int)bnum && bden == (int)bden) ||
	     b->t & TSP_DEC || p->t & TSP_DEC)
		return mk_num(b->t, p->t, 0)(bnum, bden);
	return mk_list(st, 3, mk_sym(st, "^"), b, p);
}

#define PRIM_COMPARE(NAME, OP)                          \
static Val                                              \
prim_##NAME(Tsp st, Rec vars, Val args)                 \
{                                                       \
	if (tsp_lstlen(args) != 2)                      \
		return st->t;                           \
	tsp_arg_type(car(args), #OP, TSP_NUM);          \
	tsp_arg_type(car(cdr(args)), #OP, TSP_NUM);     \
	return ((num(car(args))*den(car(cdr(args)))) OP \
		(num(car(cdr(args)))*den(car(args)))) ? \
		st->t : st->nil;                        \
}

PRIM_COMPARE(lt,  <)
PRIM_COMPARE(gt,  >)
PRIM_COMPARE(lte, <=)
PRIM_COMPARE(gte, >=)

#define PRIM_TRIG(NAME)                                      \
static Val                                                   \
prim_##NAME(Tsp st, Rec vars, Val args)                      \
{                                                            \
	tsp_arg_num(args, #NAME, 1);                         \
	tsp_arg_type(car(args), #NAME, TSP_EXPR);            \
	if (car(args)->t & TSP_DEC)                          \
		return mk_dec(NAME(num(car(args))));         \
	return mk_list(st, 2, mk_sym(st, #NAME), car(args)); \
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

static Val
prim_numerator(Tsp st, Rec env, Val args)
{
	tsp_arg_num(args, "numerator", 1);
	tsp_arg_type(car(args), "numerator", TSP_INT | TSP_RATIO);
	return mk_int(car(args)->v.n.num);
}

static Val
prim_denominator(Tsp st, Rec env, Val args)
{
	tsp_arg_num(args, "denominator", 1);
	tsp_arg_type(car(args), "denominator", TSP_INT | TSP_RATIO);
	return mk_int(car(args)->v.n.den);
}

void
tib_env_math(Tsp st)
{
	tsp_env_prim(Int);
	tsp_env_prim(Dec);
	tsp_env_prim(floor);
	tsp_env_prim(ceil);
	tsp_env_prim(round);
	tsp_env_prim(numerator);
	tsp_env_prim(denominator);

	tsp_env_name_prim(+, add);
	tsp_env_name_prim(-, sub);
	tsp_env_name_prim(*, mul);
	tsp_env_name_prim(/, div);
	tsp_env_prim(mod);
	tsp_env_name_prim(^, pow);

	tsp_env_name_prim(<,  lt);
	tsp_env_name_prim(>,  gt);
	tsp_env_name_prim(<=, lte);
	tsp_env_name_prim(>=, gte);

	tsp_env_prim(sin);
	tsp_env_prim(cos);
	tsp_env_prim(tan);
	tsp_env_prim(sinh);
	tsp_env_prim(cosh);
	tsp_env_prim(tanh);
	tsp_env_name_prim(arcsin,  asin);
	tsp_env_name_prim(arccos,  acos);
	tsp_env_name_prim(arctan,  atan);
	tsp_env_name_prim(arcsinh, asinh);
	tsp_env_name_prim(arccosh, acosh);
	tsp_env_name_prim(arctanh, atanh);
	tsp_env_prim(exp);
	tsp_env_prim(log);
}
