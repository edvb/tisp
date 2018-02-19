/* See LICENSE file for copyright and license details. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if (!(p = calloc(nmemb, size)))
		die(1, "calloc:");

	return p;
}

void *
emalloc(size_t size)
{
	void *p;

	if (!(p = malloc(size)))
		die(1, "malloc:");

	return p;
}

void *
erealloc(void *p, size_t size)
{
	if (!(p = realloc(p, size)))
		die(1, "realloc:");

	return p;
}

char *
estrdup(char *s)
{
	if (!(s = strdup(s)))
		die(1, "strdup:");

	return s;
}

void
efree(void *p)
{
	if (p)
		free(p);
}

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
