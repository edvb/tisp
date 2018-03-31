/* See LICENSE file for copyright and license details. */
#include <stdio.h>

#include "../tisp.h"
#include "math.h"

static Val
prim_add(Hash env, Val args)
{
	Val v;
	int i = 0;
	if (!(v = eval_list(env, args)))
		return NULL;
	for (; !nilp(v); v = cdr(v))
		if (car(v)->t == INTEGER)
			i += car(v)->v.i;
		else
			warnf("+: expected integer, recieved type [%d]", car(v)->t);
	return mk_int(i);
}

void
tib_math_env(Hash ht)
{
	hash_add(ht, "+", mk_prim(prim_add));
}
