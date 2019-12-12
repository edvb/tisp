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
#include <fcntl.h>

#include "../tisp.h"

/* write all arguemnts to given file, or stdout/stderr, without newline */
static Val
prim_write(Env env, Val args)
{
	Val v;
	FILE *f;
	tsp_arg_min(args, "write", 2);
	if (!(v = tisp_eval_list(env, args)))
		return NULL;

	/* first argument can either be the symbol stdout or stderr,
	 * or the file as a string */
	if (car(v)->t == SYMBOL)
		f = !strncmp(car(v)->v.s, "stdout", 7) ? stdout : stderr;
	else if (car(v)->t != STRING)
		tsp_warnf("write: expected file name as string, received %s",
		           type_str(car(v)->t));
	else if (!(f = fopen(car(v)->v.s, "w")))
		tsp_warnf("write: could not load file '%s'", car(v)->v.s);
	if (f == stderr && strncmp(car(v)->v.s, "stderr", 7))
		tsp_warn("write: expected file name as string, "
		                  "or symbol stdout/stderr");

	for (v = cdr(v); !nilp(v); v = cdr(v))
		if (car(v)->t & STRING) /* don't print quotes around string */
			fprintf(f, "%s", car(v)->v.s);
		else
			tisp_print(f, car(v));
	fflush(f);
	return env->none;
}

/* return string of given file or read from stdin */
static Val
prim_read(Env env, Val args)
{
	Val v;
	char *file, *fname = NULL; /* read from stdin by default */
	if (list_len(args) > 1)
		tsp_warnf("read: expected 0 or 1 argument, received %d", list_len(args));
	if (list_len(args) == 1) { /* if file name given as string, read it */
		if (!(v = tisp_eval(env, car(args))))
			return NULL;
		tsp_arg_type(v, "read", STRING);
		fname = v->v.s;
	}
	if (!(file = tisp_read_file(fname)))
		return env->nil;
	return mk_str(env, file);
}

/* parse string as tisp expression, return (quit) if given nil */
/* TODO parse more than 1 expression */
static Val
prim_parse(Env env, Val args)
{
	Val v;
	char *file = env->file;
	size_t filec = env->filec;
	tsp_arg_num(args, "parse", 1);
	if (!(v = tisp_eval(env, car(args))))
		return NULL;
	if (nilp(v))
		return mk_pair(mk_sym(env, "quit"), env->nil);
	tsp_arg_type(v, "parse", STRING);
	env->file = v->v.s;
	env->filec = 0;
	v = tisp_read(env);
	env->file = file;
	env->filec = filec;
	return v ? v : env->none;
}

void
tib_env_io(Env env)
{
	tsp_env_fn(write);
	tsp_env_fn(read);
	tsp_env_fn(parse);
}
