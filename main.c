/* See LICENSE file for copyright and license details. */
#include <libgen.h>
#include <stdio.h>
#include <string.h>

#include "extern/arg.h"
#include "extern/linenoise.h"

#include "util.h"
#include "config.h"

#include "tisp.h"
#include "tib/math.h"

char *argv0;

static Val
read_val(Env env, Str cmd)
{
	struct Str str;

	if (cmd->d)
		return tisp_read(env, cmd);

	if (!(str.d = linenoise("> ")))
		return NULL;
	linenoiseHistoryAdd(str.d);

	return tisp_read(env, &str);
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
	tib_env_math(env);

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

	return 0;
}
