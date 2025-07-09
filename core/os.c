/* zlib License
 *
 * Copyright (c) 2017-2025 Ed van Bruggen
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
#include <limits.h>
#include <time.h>
#include <unistd.h>

/* TODO sys ls, mv, cp, rm, mkdir */

/* change to new directory */
static Eevo
prim_cd(EevoSt st, EevoRec env, Eevo args)
{
	Eevo dir;
	eevo_arg_num(args, "cd!", 1);
	dir = fst(args);
	if (!(dir->t & (EEVO_STR|EEVO_SYM)))
		eevo_warnf("cd!: expected string or symbol, received %s", eevo_type_str(dir->t));
	if (chdir(dir->v.s))
		return perror("; error: cd"), NULL;
	return st->none;
}

/* return string of current working directory */
static Eevo
prim_pwd(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "pwd", 0);
	char cwd[PATH_MAX];
	if (!getcwd(cwd, sizeof(cwd)) && cwd[0] != '(')
		eevo_warn("pwd: could not get current directory");
	return eevo_str(st, cwd);
}

/* exit program with return value of given int */
static Eevo
prim_exit(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "exit!", 1);
	eevo_arg_type(fst(args), "exit!", EEVO_INT);
	exit((int)fst(args)->v.n.num);
}

/* TODO time formating */
/* return number of seconds since 1970 (unix time stamp) */
static Eevo
prim_now(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "now", 0);
	return eevo_int(time(NULL));
}

/* TODO time-avg: run timeit N times and take average */
/* return time in miliseconds taken to run command given */
static Eevo
form_time(EevoSt st, EevoRec env, Eevo args)
{
	Eevo v;
	clock_t t;
	eevo_arg_num(args, "time", 1);
	t = clock();
	if (!(v = eevo_eval(st, env, fst(args))))
		return NULL;
	t = clock() - t;
	return eevo_dec(((double)t)/CLOCKS_PER_SEC*100);
}

void
eevo_env_os(EevoSt st)
{
	eevo_env_name_prim(cd!, cd);
	eevo_env_prim(pwd);
	eevo_env_name_prim(exit!, exit);
	eevo_env_prim(now);
	eevo_env_form(time);
}
