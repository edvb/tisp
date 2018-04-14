/* See LICENSE file for copyright and license details. */
#include <stdio.h>

#include "../tisp.h"
#include "math.h"

#define INC(SIGN, FUNC) do {                                                               \
	if (car(v)->t == INTEGER)                                                          \
		i = i SIGN car(v)->v.i;                                                    \
	else                                                                               \
		warnf(FUNC ": expected integer, recieved type [%s]", type_str(car(v)->t)); \
} while (0)

static Val
prim_add(Hash env, Val args)
{
	Val v;
	int i = 0;
	if (!(v = eval_list(env, args)))
		return NULL;
	for (; !nilp(v); v = cdr(v))
		INC(+, "+");
	return mk_int(i);
}

static Val
prim_sub(Hash env, Val args)
{
	Val v;
	int i = 0;
	if (!(v = eval_list(env, args)))
		return NULL;
	INC(+, "-");
	v = cdr(v);
	if (nilp(v))
		return mk_int(-i);
	for (; !nilp(v); v = cdr(v))
		INC(-, "-");
	return mk_int(i);
}

void
tib_math_env(Hash ht)
{
	hash_add(ht, "+",  mk_prim(prim_add));
	hash_add(ht, "-",  mk_prim(prim_sub));
}
