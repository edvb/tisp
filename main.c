/* See LICENSE file for copyright and license details. */
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "extern/arg.h"
#include "extern/linenoise.h"

#include "config.h"

#include "tisp.h"
#if TIB_STATIC
#  include "tibs/math.h"
#  include "tibs/io.h"
#endif

char *argv0;

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

static void
usage(const int eval)
{
	die(eval, "usage: %s [-hv] [FILENAME]", argv0);
}

int
main(int argc, char *argv[])
{
	ARGBEGIN {
	case 'h':
		usage(0);
	case 'v':
		die(0, "%s v%s (c) 2017-2019 Ed van Bruggen", argv0, VERSION);
	default:
		usage(1);
	} ARGEND;

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

	if (argc > 0) {
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
