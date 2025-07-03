/* See LICENSE file for copyright and license details. */
#include <libgen.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tisp.h"
#ifndef EEVO_NOCORE
#  include "tibs.tsp.h"
#endif

/* read, parse, and eval file given as single element list, or empty list for stdin */
Eevo
read_parse_eval(EevoSt st, Eevo file)
{
	Eevo val = eevo_list(st, 2, eevo_sym(st, "parse"), eevo_pair(eevo_sym(st, "read"), file));
	val = eevo_eval(st, st->env, val); /* read and parse */
	return eevo_eval(st, st->env, val); /* eval resulting expressions */
}

int
main(int argc, char *argv[])
{
	int i = 1;
	Eevo v = NULL;

	EevoSt st = eevo_env_init(1024);
#ifndef EEVO_NOCORE
	eevo_env_core(st);
	eevo_env_math(st);
	eevo_env_io(st);
	eevo_env_os(st);
	eevo_env_string(st);
	eevo_env_lib(st, tibs);
#endif

	if (argc == 1) {
		st->file = "(repl)";
		goto readstr;
	}

	for (; i < argc; i++, v = NULL) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'e') { /* evaluate next argument as eevo expression */
				if (!(st->file = argv[++i])) {
					fputs("eevo: expected expression after -e\n", stderr);
					exit(2);
				}
readstr:
				if ((v = eevo_read(st)))
					v = eevo_eval(st, st->env, v);
			} else if (argv[i][1] == 'r') {
				st->file = "(repl)";
				if ((v = eevo_read(st)))
					v = eevo_eval(st, st->env, v);
			} else if (argv[i][1] == 'v') { /* version and copyright info */
				fprintf(stderr, "eevo v%s (c) 2017-2025 Ed van Bruggen\n", VERSION);
				exit(0);
			} else if (argv[i][1]) { /* unsupported argument or help */
				fputs("usage: eevo [-rhv] [-e EXPRESSION] [FILE ...] [-]\n", stderr);
				exit(argv[i][1] == 'h' ? 0 : 1);
			} else { /* single hypen read from stdin */
				v = read_parse_eval(st, st->nil);
			}
		} else { /* otherwise read as file */
			v = read_parse_eval(st, eevo_pair(eevo_str(st, argv[i]), st->nil));
		}
		if (v && v->t != EEVO_NONE) {
			char *s = eevo_print(v);
			fputs(s, stdout);
			free(s);
		}
	}

	/* if (v && v->t != EEVO_NONE) */
		puts("");

	free(st);

	return 0;
}
