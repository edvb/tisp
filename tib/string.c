/* zlib License
 *
 * Copyright (c) 2017-2021 Ed van Bruggen
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

/* TODO string tib: lower upper capitalize strpos strsub (python: dir(str))*/

/* TODO simplify by using fmemopen/funopen and tisp_print */
/* TODO NULL check allocs */
static Val
val_string(Tsp st, Val args, MkFn mk_fn)
{
	Val v;
	char s[43], *ret = calloc(1, sizeof(char));
	int len = 1;
	for (; !nilp(args); args = cdr(args)) {
		v = car(args);
		switch (v->t) {
		case TSP_NONE:
			len += 5;
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, "Void");
			break;
		case TSP_NIL:
			len += 4;
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, "Nil");
			break;
		case TSP_INT:
			snprintf(s, 21, "%d", (int)v->v.n.num);
			len += strlen(s);
			s[len] = '\0';
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, s);
			break;
		case TSP_DEC:
			snprintf(s, 17, "%.15g", v->v.n.num);
			len += strlen(s);
			s[len] = '\0';
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, s);
			break;
		case TSP_RATIO:
			snprintf(s, 43, "%d/%d", (int)v->v.n.num, (int)v->v.n.den);
			len += strlen(s);
			s[len] = '\0';
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, s);
			break;
		case TSP_STR:
		case TSP_SYM:
			len += strlen(v->v.s);
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, v->v.s);
			break;
		case TSP_PAIR:
		default:
			tsp_warnf("could not convert type %s into string", type_str(v->t));
		}
	}
	v = mk_fn(st, ret);
	free(ret);
	return v;
}

static Val
prim_Str(Tsp st, Hash env, Val args)
{
	tsp_arg_min(args, "Str", 1);
	return val_string(st, args, mk_str);
}

static Val
prim_Sym(Tsp st, Hash env, Val args)
{
	tsp_arg_min(args, "Sym", 1);
	return val_string(st, args, mk_sym);
}

void
tib_env_string(Tsp st)
{
	tsp_env_prim(Sym);
	tsp_env_prim(Str);
}
