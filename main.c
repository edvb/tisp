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

char *
hints(const char *buf, int *color, int *bold)
{
	struct { char *match, *hint; int color, bold; } hint[] = {
		{ "(lambda", " (args) body)", 35, 0 },
		{ "(define", " var exp)",     35, 0 },
		{ NULL, NULL, 0, 0 }
	};
	for (int i = 0; hint[i].match; i++) {
		if (!strcasecmp(buf, hint[i].match)){
			*color = hint[i].color;
			*bold = hint[i].bold;
			return hint[i].hint;
		}
	}
	return NULL;
}

static Val
read_val(Str cmd)
{
	struct Str str;

	if (cmd->d)
		return tisp_read(cmd);

	if (SHOW_HINTS)
		linenoiseSetHintsCallback(hints);
	if (!(str.d = linenoise("> ")))
		return NULL;
	linenoiseHistoryAdd(str.d);

	return tisp_read(&str);
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
		printf("%s v%s (c) 2017-2018 Ed van Bruggen\n", argv0, VERSION);
		return 0;
	default:
		usage(1);
	} ARGEND;

	size_t nread;
	char buf[BUFSIZ];
	struct Str str = { NULL };
	FILE *fp;
	Val v;
	Hash env = tisp_init_env(64);
	tib_math_env(env);

	if (argc > 0) {
		if (!(fp = fopen(*argv, "r")))
			die(1, "%s: %s:", argv[0], *argv);
		while ((nread = fread(buf, 1, sizeof(buf), fp)) > 0) ;
		str.d = estrdup(buf);
	}

	while ((v = read_val(&str))) {
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
