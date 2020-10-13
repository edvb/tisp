/* zlib License
 *
 * Copyright (c) 2017-2020 Ed van Bruggen
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
#include <time.h>
#include <unistd.h>
#include <limits.h>

#include "../tisp.h"

/* change to new directory */
static Val
prim_cd(Tsp st, Hash env, Val args)
{
	Val dir;
	tsp_arg_num(args, "cd", 1);
	dir = car(args);
	if (!(dir->t & (TSP_STR|TSP_SYM)))
		tsp_warnf("strlen: expected string or symbol, received %s", type_str(dir->t));
	if (chdir(dir->v.s))
		return perror("; error: cd"), NULL;
	return st->none;
}

/* TODO rename to cwd ? */
/* return string of current working directory */
static Val
form_pwd(Tsp st, Hash env, Val args)
{
	tsp_arg_num(args, "pwd", 0);
	char cwd[PATH_MAX];
	if (!getcwd(cwd, sizeof(cwd)) && cwd[0] != '(')
		tsp_warn("pwd: could not get current directory");
	return mk_str(st, cwd);
}

/* return number of seconds since 1970 (unix time stamp) */
static Val
form_time(Tsp st, Hash env, Val args)
{
	tsp_arg_num(args, "time", 0);
	return mk_int(time(NULL));
}

/* return time taken to run command given */
static Val
form_timeit(Tsp st, Hash env, Val args)
{
	Val v;
	clock_t t;
	tsp_arg_num(args, "timeit", 1);
	t = clock();
	if (!(v = tisp_eval(st, env, car(args))))
		return NULL;
	t = clock() - t;
	return mk_dec(((double)t)/CLOCKS_PER_SEC);
}

void
tib_env_os(Tsp st)
{
	tsp_env_name_prim(cd!, cd);
	tsp_env_form(pwd);
	tsp_env_form(time);
	tsp_env_form(timeit);
}
