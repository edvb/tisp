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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../tisp.h"

typedef Val (*MkFn)(Tsp, char*);

/* TODO string tib: strlen lower upper strpos strsub */

/* TODO return string val in val_string to be concated */
static Val
val_string(Tsp st, Val v, MkFn mk_fn)
{
	char s[43];
	switch (v->t) {
	case NONE:
		return mk_fn(st, "void");
	case NIL:
		return mk_fn(st, "nil");
	case INTEGER:
		snprintf(s, 21, "%d", (int)v->v.n.num);
		return mk_fn(st, s);
	case DECIMAL:
		snprintf(s, 17, "%.15g", v->v.n.num);
		return mk_fn(st, s);
	case RATIO:
		snprintf(s, 43, "%d/%d", (int)v->v.n.num, (int)v->v.n.den);
		return mk_fn(st, s);
	case STRING:
	case SYMBOL:
		return mk_fn(st, v->v.s);
	case PAIR:
	default:
		tsp_warnf("could not convert type %s into string", type_str(v->t));
	}
}

/* TODO string and symbol: multi arguments to concat */
static Val
prim_string(Tsp st, Hash env, Val args)
{
	Val v;
	tsp_arg_num(args, "string", 1);
	if (!(v = tisp_eval(st, env, car(args))))
		return NULL;
	return val_string(st, v, mk_str);
}

static Val
prim_symbol(Tsp st, Hash env, Val args)
{
	Val v;
	tsp_arg_num(args, "symbol", 1);
	if (!(v = tisp_eval(st, env, car(args))))
		return NULL;
	return val_string(st, v, mk_sym);
}

void
tib_env_string(Tsp st)
{
	tsp_env_fn(symbol);
	tsp_env_fn(string);
}
