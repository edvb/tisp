/* See LICENSE file for copyright and license details. */
#include <libgen.h>
#include <stdio.h>
#include <string.h>

#include "extern/arg.h"
#include "extern/linenoise.h"

#include "util.h"
#include "config.h"

#include "tisp.h"
#if TIB_STATIC
#  include "tib/math.h"
#endif

char *argv0;

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
	efree(strf);
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
		die(0, "%s v%s (c) 2017-2018 Ed van Bruggen", argv0, VERSION);
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
#endif

	if (argc > 0) {
		if (!(fp = fopen(*argv, "r")))
			die(1, "%s: %s:", argv[0], *argv);
		while ((nread = fread(buf, 1, sizeof(buf), fp)) > 0) ;
		str.d = estrdup(buf);
	}

	while ((v = read_val(env, &str))) {
		if (!(v = tisp_eval(env, v)))
			continue;

		tisp_print(v);
		putchar('\n');

		if (!str.d) continue;
		skip_spaces(&str);
		if (!*str.d) break;
	}

	tisp_env_free(env);

	return 0;
}
