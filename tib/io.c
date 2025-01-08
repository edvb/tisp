/* zlib License
 *
 * Copyright (c) 2017-2024 Ed van Bruggen
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
#include <unistd.h>

#include <dlfcn.h>
#include <limits.h>

#include "../tisp.h"

/* write all arguemnts to given file, or stdout/stderr, without newline */
/* first argument is file name, second is option to append file */
static Val
prim_write(Tsp st, Rec env, Val args)
{
	FILE *f;
	const char *mode = "w";
	tsp_arg_min(args, "write", 2);

	/* if second argument is true, append file don't write over */
	if (!nilp(cadr(args)))
		mode = "a";
	/* first argument can either be the symbol stdout or stderr, or the file as a string */
	if (car(args)->t == TSP_SYM)
		f = !strncmp(car(args)->v.s, "stdout", 7) ? stdout : stderr;
	else if (car(args)->t != TSP_STR)
		tsp_warnf("write: expected file name as string, received %s",
		           tsp_type_str(car(args)->t));
	else if (!(f = fopen(car(args)->v.s, mode)))
		tsp_warnf("write: could not load file '%s'", car(args)->v.s);
	if (f == stderr && strncmp(car(args)->v.s, "stderr", 7)) /* validate stderr symbol */
		tsp_warn("write: expected file name as string, or symbol stdout/stderr");

	for (args = cddr(args); !nilp(args); args = cdr(args))
		tisp_print(f, car(args));

	if (f == stdout || f == stderr) /* clean up */
		fflush(f);
	else
		fclose(f);
	return st->none;
}

/* return string of given file or read from stdin */
static Val
prim_read(Tsp st, Rec env, Val args)
{
	char *file, *fname = NULL; /* read from stdin by default */
	tsp_arg_max(args, "read", 1);
	if (tsp_lstlen(args) == 1) { /* if file name given as string, read it */
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
prim_parse(Tsp st, Rec env, Val args)
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
	expr = tisp_read_line(st, 0);
	st->file = file;
	st->filec = filec;
	return expr ? expr : st->none;
	/* return expr; */
}

/* loads tisp file or C dynamic library */
static Val
prim_load(Tsp st, Rec env, Val args)
{
	Val tib;
	void (*tibenv)(Tsp);
	char name[PATH_MAX];
	const char *paths[] = {
		"/usr/local/lib/tisp/pkgs/", "/usr/lib/tisp/pkgs/", "./", NULL
	};

	tsp_arg_num(args, "load", 1);
	tib = car(args);
	tsp_arg_type(tib, "load", TSP_STR);

	for (int i = 0; paths[i]; i++) {
		strcpy(name, paths[i]);
		strcat(name, tib->v.s);
		strcat(name, ".tsp");
		if (access(name, R_OK) != -1) {
			tisp_eval_body(st, env, tisp_parse_file(st, name));
			return st->none;
		}
	}

	/* If not tisp file, try loading shared object library */
	if (!(st->libh = realloc(st->libh, (st->libhc+1)*sizeof(void*))))
		perror("; realloc"), exit(1);

	memset(name, 0, sizeof(name));
	strcpy(name, "libtib");
	strcat(name, tib->v.s);
	strcat(name, ".so");
	if (!(st->libh[st->libhc] = dlopen(name, RTLD_LAZY)))
		tsp_warnf("load: could not load '%s':\n; %s", tib->v.s, dlerror());
	dlerror();

	memset(name, 0, sizeof(name));
	strcpy(name, "tib_env_");
	strcat(name, tib->v.s);
	tibenv = dlsym(st->libh[st->libhc], name);
	if (dlerror())
		tsp_warnf("load: could not run '%s':\n; %s", tib->v.s, dlerror());
	(*tibenv)(st);

	st->libhc++;
	return st->none;
}

void
tib_env_io(Tsp st)
{
	tsp_env_prim(write);
	tsp_env_prim(read);
	tsp_env_prim(parse);
	tsp_env_prim(load);
}
