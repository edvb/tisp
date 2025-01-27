/* See LICENSE file for copyright and license details. */
#include <libgen.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tisp.h"
#ifndef TIB_DYNAMIC
#  include "tibs.tsp.h"
#endif

/* read, parse, and eval file given as single element list, or empty list for stdin */
Val
read_parse_eval(Tsp st, Val file)
{
	Val val = mk_list(st, 2, mk_sym(st, "parse"), mk_pair(mk_sym(st, "read"), file));
	val = tisp_eval(st, st->env, val); /* read and parse */
	return tisp_eval(st, st->env, val); /* eval resulting expressions */
}

int
main(int argc, char *argv[])
{
	int i = 1;
	Val v = NULL;

	Tsp st = tisp_env_init(1024);
#ifndef TIB_DYNAMIC
	tib_env_core(st);
	tib_env_math(st);
	tib_env_io(st);
	tib_env_os(st);
	tib_env_string(st);
	tisp_env_lib(st, tibs);
#endif

	/* TODO SIGTERM to handle garbage collection */
	struct sigaction sigint;
	sigint.sa_handler = SIG_IGN;
	sigaction(SIGINT, &sigint, NULL);

	if (argc == 1) {
		st->file = "(repl)";
		goto readstr;
	}

	for (; i < argc; i++, v = NULL) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'c') { /* run next argument as tisp command */
				if (!(st->file = argv[++i])) {
					fputs("tisp: expected command after -c\n", stderr);
					exit(2);
				}
readstr:
				if ((v = tisp_read(st)))
					v = tisp_eval(st, st->env, v);
			} else if (argv[i][1] == 'r') {
				st->file = "(repl)";
				if ((v = tisp_read(st)))
					v = tisp_eval(st, st->env, v);
			} else if (argv[i][1] == 'v') { /* version and copyright info */
				fprintf(stderr, "tisp v%s (c) 2017-2025 Ed van Bruggen\n", VERSION);
				exit(0);
			} else if (argv[i][1]) { /* unsupported argument or help */
				fputs("usage: tisp [-hrv] [-c COMMAND] [-] [FILE ...]\n", stderr);
				exit(argv[i][1] == 'h' ? 0 : 1);
			} else { /* single hypen read from stdin */
				v = read_parse_eval(st, st->nil);
			}
		} else { /* otherwise read as file */
			v = read_parse_eval(st, mk_pair(mk_str(st, argv[i]), st->nil));
		}
		if (v && v->t != TSP_NONE)
			tisp_print(stdout, v);
	}

	/* if (v && v->t != TSP_NONE) */
		puts("");

	return 0;
}
