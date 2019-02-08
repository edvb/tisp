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

static Val
prim_print(Env env, Val args)
{
	Val v;
	if (!(v = tisp_eval_list(env, args)))
		return NULL;
	for (; !nilp(v); v = cdr(v)) {
		if (car(v)->t & STRING) /* don't print quotes arond string */
			fprintf(stdout, "%s", car(v)->v.s);
		else
			tisp_print(stdout, car(v));
		if (!nilp(cdr(v)))
			putchar(' ');
	}
	fflush(stdout);
	return env->none;
}

static Val
prim_read(Env env, Val args)
{
	char *file;
	Val v;
	struct Str str = { NULL };
	if (!(file = tisp_read_file(NULL)))
		return mk_sym(env, "quit");
	str.d = file;
	v = tisp_read(env, &str);
	free(file);
	return v;
}

static Val
prim_newline(Env env, Val args)
{
	return mk_str(env, "\n");
}

void
tib_env_io(Env env)
{
	tsp_env_fn(print);
	tsp_env_fn(read);
	tsp_env_fn(newline);
}
