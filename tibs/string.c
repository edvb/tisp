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

/* TODO simplify by using fmemopen and tisp_print */
static Val
val_string(Tsp st, Val args, MkFn mk_fn)
{
	Val v;
	char s[43], *ret = calloc(1, sizeof(char));
	int len = 1;
	for (; !nilp(args); args = cdr(args)) {
		v = car(args);
		switch (v->t) {
		case NONE:
			len += 5;
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, "void");
			break;
		case NIL:
			len += 4;
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, "nil");
			break;
		case INTEGER:
			snprintf(s, 21, "%d", (int)v->v.n.num);
			len += strlen(s);
			s[len] = '\0';
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, s);
			break;
		case DECIMAL:
			snprintf(s, 17, "%.15g", v->v.n.num);
			len += strlen(s);
			s[len] = '\0';
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, s);
			break;
		case RATIO:
			snprintf(s, 43, "%d/%d", (int)v->v.n.num, (int)v->v.n.den);
			len += strlen(s);
			s[len] = '\0';
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, s);
			break;
		case STRING:
		case SYMBOL:
			len += strlen(v->v.s);
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, v->v.s);
			break;
		case PAIR:
		default:
			tsp_warnf("could not convert type %s into string", type_str(v->t));
		}
	}
	v = mk_fn(st, ret);
	free(ret);
	return v;
}

/* TODO string and symbol: multi arguments to concat */
static Val
prim_string(Tsp st, Hash env, Val args)
{
	Val v;
	tsp_arg_min(args, "string", 1);
	if (!(v = tisp_eval_list(st, env, args)))
		return NULL;
	return val_string(st, v, mk_str);
}

static Val
prim_symbol(Tsp st, Hash env, Val args)
{
	Val v;
	tsp_arg_min(args, "symbol", 1);
	if (!(v = tisp_eval_list(st, env, args)))
		return NULL;
	return val_string(st, v, mk_sym);
}

void
tib_env_string(Tsp st)
{
	tsp_env_fn(symbol);
	tsp_env_fn(string);
}
