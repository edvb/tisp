/* See LICENSE file for copyright and license details. */
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tisp.h"
#ifndef TIB_DYNAMIC
#  include "tibs/math.h"
#  include "tibs/io.h"
#endif

int
main(int argc, char *argv[])
{
	int i;

	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "-v")) {
			fprintf(stderr, "tisp v%s (c) 2017-2019 Ed van Bruggen\n", VERSION);
			exit(0);
		} else if (argv[i][0] == '-' && argv[i][1]) {
			fputs("usage: tisp [-hv] [FILE ...]\n", stderr);
			exit(argv[i][1] == 'h' ? 0 : 1);
		}

	Env env = tisp_env_init(64);
#ifndef TIB_DYNAMIC
	tib_env_math(env);
	tib_env_io(env);
#endif

	if (argc == 1)
		tisp_print(stdout, tisp_eval(env, tisp_parse_file(env, NULL)));

	for (i = 1; i < argc; i++)
		if (argv[i][0] == '-')
			tisp_print(stdout, tisp_eval(env, tisp_parse_file(env, NULL)));
		else
			tisp_print(stdout, tisp_eval(env, tisp_parse_file(env, argv[i])));
	puts("");

	/* tisp_env_free(env); */

	return 0;
}
