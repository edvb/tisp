/* See LICENSE file for copyright and license details. */
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tisp.h"
#ifndef TIB_DYNAMIC
	tsp_include_tib(math);
	tsp_include_tib(io);
	tsp_include_tib(time);
	tsp_include_tib(string);
#endif

int
main(int argc, char *argv[])
{
	int i;
	struct Str str = { NULL };
	Val v = NULL;

	Env env = tisp_env_init(64);
#ifndef TIB_DYNAMIC
	tib_env_math(env);
	tib_env_io(env);
	tib_env_time(env);
	tib_env_string(env);
#  include "libs.tsp.h"
	tisp_env_lib(env, libs_tsp);
#endif

	/* TODO reduce redunecy by setting argv[i][0] = '-' ? */
	if (argc == 1)
		if ((v = tisp_eval_list(env, tisp_parse_file(env, NULL))))
			tisp_print(stdout, v);

	for (i = 1; i < argc; i++, v = NULL) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'c') { /* run next argument as tisp command */
				if (!(str.d = argv[++i])) {
					fputs("tisp: expected command after -c\n", stderr);
					exit(2);
				}
				if ((v = tisp_read(env, &str)))
					v = tisp_eval(env, v);
			} else if (argv[i][1] == 'v') { /* version and copyright info */
				fprintf(stderr, "tisp v%s (c) 2017-2019 Ed van Bruggen\n", VERSION);
				exit(0);
			} else if (argv[i][1]) { /* unsupported argument or help */
				fputs("usage: tisp [-hv] [FILE ...]\n", stderr);
				exit(argv[i][1] == 'h' ? 0 : 1);
			} else { /* single hypen read from stdin */
				v = tisp_eval_list(env, tisp_parse_file(env, NULL));
			}
		} else { /* otherwise read as file */
			v = tisp_eval_list(env, tisp_parse_file(env, argv[i]));
		}
		if (v) tisp_print(stdout, v);
	}

	puts("");

	/* tisp_env_free(env); */

	return 0;
}
