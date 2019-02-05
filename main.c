/* See LICENSE file for copyright and license details. */
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "extern/linenoise.h"

#include "config.h"

#include "tisp.h"
#if TIB_STATIC
#  include "tibs/math.h"
#  include "tibs/io.h"
#endif

void
die(int eval, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}

	if (eval > -1)
		exit(eval);
}

static Val
read_val(Env env, Str cmd)
{
	char *strf;
	struct Str str;
	Val ret;

	if (cmd->d)
		return tisp_read(env, cmd);

	if (!(str.d = linenoise("> ")))
		return NULL;
	linenoiseHistoryAdd(str.d);

	strf = str.d;
	ret = tisp_read(env, &str);
	free(strf);
	return ret;
}

int
main(int argc, char *argv[])
{
	int i;

	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "-v")) {
			fprintf(stderr, "tisp v%s (c) 2017-2019 Ed van Bruggen\n", VERSION);
			exit(0);
		} else if (argv[i][0] == '-') {
			fputs("usage: tisp [-hv] [FILE ...]\n", stderr);
			exit(argv[i][1] == 'h' ? 0 : 1);
		}

	size_t nread;
	char buf[BUFSIZ];
	struct Str str = { NULL };
	FILE *fp;
	Val v;
	Env env = tisp_env_init(64);
#if TIB_STATIC
	tib_env_math(env);
	tib_env_io(env);
#endif

	if (argc > 1) {
		if (!(fp = fopen(*argv, "r")))
			die(1, "%s: %s:", argv[0], *argv);
		while ((nread = fread(buf, 1, sizeof(buf), fp)) > 0) ;
		str.d = strdup(buf);
	}

	while ((v = read_val(env, &str))) {
		if (!(v = tisp_eval(env, v)))
			continue;

		tisp_print(stdout, v);
		if (v->t != NONE)
			putchar('\n');

		if (!str.d) continue;
		skip_ws(&str);
		if (!*str.d) break;
	}

	/* tisp_env_free(env); */

	return 0;
}
