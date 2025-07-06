/* See LICENSE file for copyright and license details. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../eevo.h"
#include "../core.evo.h"

#include "tests.h"

#define LEN(X) (sizeof(X) / sizeof((X)[0]))

int
eevo_test(EevoSt st, const char *input, const char *expect, int output)
{
	Eevo v;

	/* TODO eval expect as well, then compare values? and compare printed strings */
	if (!(st->file = strdup(input)))
		return 0;
	st->filec = 0;
	if (!(v = eevo_read(st)))
		return 0;

	if (!(v = eevo_eval(st, st->env, eevo_list(st, 2, eevo_sym(st, "display"), v)))) {
		if (output)
			putchar('\n');
		return 0;
	}
	if (v->t != EEVO_STR)
		return 0;

	if (output)
		printf("%s\n", v->v.s);
	return strcmp(v->v.s, expect) == 0;
}

int
main(void)
{
	int correct = 0, total = 0, seccorrect = 0, sectotal = 0, last = 1;
	int errors[LEN(tests)] = {0};
	clock_t t;
	EevoSt st = eevo_env_init(1024);
	eevo_env_core(st);
	eevo_env_math(st);
	eevo_env_string(st);
	eevo_env_lib(st, eevo_core);

	t = clock();
	for (int i = 0; ; i++) {
		if (!tests[i][1]) {
			if (i != 0) {
				printf("%d/%d\n", seccorrect, sectotal);
				for (int j = last; j < i; j++)
					if (!tests[j][1]) {
						printf("%12s\n", tests[j][0]);
					} else if (errors[j]) {
						printf(" input: %s\n"
						       "expect: %s\n"
						       "output: ", tests[j][0], tests[j][1]);
						eevo_test(st, tests[j][0], tests[j][1], 1);
					}
				last = i + 1;
			}
			if (!tests[i][0])
				break;
			printf("%12s ", tests[i][0]);
			seccorrect = 0;
			sectotal = 0;
		} else {
			if (eevo_test(st, tests[i][0], tests[i][1], 0)) {
				correct++;
				seccorrect++;
			} else {
				errors[i] = 1;
			}
			total++;
			sectotal++;
		}
	}
	t = clock() - t;
	printf("%12s %d/%d\n", "total", correct, total);
	if (correct == total)
		printf("%12f ms\n", ((double)t)/CLOCKS_PER_SEC*100);

	return correct != total;
}
