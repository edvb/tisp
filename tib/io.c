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
#include <fcntl.h>

#include "../tisp.h"

/* write all arguemnts to given file, or stdout/stderr, without newline */
/* first argument is file name, second is option to append file */
static Val
prim_write(Tsp st, Hash env, Val args)
{
	FILE *f;
	const char *mode = "w";
	tsp_arg_min(args, "write", 2);

	/* if second argument is true, append file don't write over */
	if (!nilp(cadr(args)))
		mode = "a";
	/* first argument can either be the symbol stdout or stderr,
	 * or the file as a string */
	if (car(args)->t == TSP_SYM)
		f = !strncmp(car(args)->v.s, "stdout", 7) ? stdout : stderr;
	else if (car(args)->t != TSP_STR)
		tsp_warnf("write: expected file name as string, received %s",
		           type_str(car(args)->t));
	else if (!(f = fopen(car(args)->v.s, mode)))
		tsp_warnf("write: could not load file '%s'", car(args)->v.s);
	if (f == stderr && strncmp(car(args)->v.s, "stderr", 7))
		tsp_warn("write: expected file name as string, "
		                  "or symbol stdout/stderr");

	for (args = cddr(args); !nilp(args); args = cdr(args))
		tisp_print(f, car(args));
	if (f == stdout || f == stderr)
		fflush(f);
	else
		fclose(f);
	return st->none;
}

/* return string of given file or read from stdin */
static Val
prim_read(Tsp st, Hash env, Val args)
{
	char *file, *fname = NULL; /* read from stdin by default */
	tsp_arg_max(args, "read", 1);
	if (list_len(args) == 1) { /* if file name given as string, read it */
		tsp_arg_type(car(args), "read", TSP_STR);
		fname = car(args)->v.s;
	}
	if (!(file = tisp_read_file(fname)))
		return st->nil;
	return mk_str(st, file);
}

/* parse string as tisp expression, return (quit) if given nil */
/* TODO parse more than 1 expression */
static Val
prim_parse(Tsp st, Hash env, Val args)
{
	Val expr;
	char *file = st->file;
	size_t filec = st->filec;
	tsp_arg_num(args, "parse", 1);
	expr = car(args);
	if (nilp(expr))
		return mk_pair(mk_sym(st, "quit"), st->nil);
	tsp_arg_type(expr, "parse", TSP_STR);
	st->file = expr->v.s;
	st->filec = 0;
	expr = tisp_read(st);
	/* for (; tsp_fget(st) && (expr = tisp_read(st));) ; */
	st->file = file;
	st->filec = filec;
	return expr ? expr : st->none;
	/* return tisp_parse_file(st, expr->v.s); */
}

/* save value as binary file to be quickly read again */
static Val
prim_save(Tsp st, Hash env, Val args)
{
	char *fname;
	FILE *f;
	tsp_arg_min(args, "save", 2);
	tsp_arg_type(cadr(args), "save", TSP_STR);
	fname = cadr(args)->v.s;
	if (!(f = fopen(fname, "wb")))
		tsp_warnf("save: could not load file '%s'", fname);
	if (!(fwrite(&*car(args), sizeof(struct Val), 1, f))) {
		fclose(f);
		tsp_warnf("save: could not save file '%s'", fname);
	}
	fclose(f);
	return car(args);
}

/* return read binary value previously saved */
static Val
prim_open(Tsp st, Hash env, Val args)
{
	FILE *f;
	char *fname;
	struct Val v;
	Val ret;
	if (!(ret = malloc(sizeof(struct Val))))
		perror("; malloc"), exit(1);
	tsp_arg_min(args, "open", 1);
	tsp_arg_type(car(args), "open", TSP_STR);
	fname = car(args)->v.s;
	if (!(f = fopen(fname, "rb")))
		tsp_warnf("save: could not load file '%s'", fname);
	while (fread(&v, sizeof(struct Val), 1, f)) ;
	fclose(f);
	memcpy(ret, &v, sizeof(struct Val));
	return ret;
}

void
tib_env_io(Tsp st)
{
	tsp_env_prim(write);
	tsp_env_prim(read);
	tsp_env_prim(parse);
	tsp_env_prim(save);
	tsp_env_prim(open);
}
