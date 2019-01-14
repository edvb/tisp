/* See LICENSE file for copyright and license details. */
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "tisp.h"
#include "tib/math.h"

int
tisp_test(Env env, const char *input, const char *expect)
{
	struct Str str;
	Val v;
	FILE *f = fopen("test.out", "w");
	size_t nread;
	char buf[BUFSIZ] = {0};

	str.d = strdup(input);
	v = tisp_read(env, &str);
	if (!(v = tisp_eval(env, v)))
		return 0;

	tisp_print(f, v);
	fclose(f);
	f = fopen("test.out", "r");
	while ((nread = fread(buf, 1, sizeof(buf), f)) > 0) ;

	return strcmp(buf, expect) == 0;
}

/* TODO mark and show which lines error-ed */
char *tests[][2] = {

	{ "self",        NULL          },
	{ "1",           "1"           },
	{ "2",           "2"           },
	{ "0",           "0"           },
	{ "30",          "30"          },
	{ "12",          "12"          },
	{ "-4",          "-4"          },
	{ "-083",        "-83"         },
	{ "-0",          "0"           },
	{ "+4",          "4"           },
	{ "+123",        "123"         },
	{ "12.0",        "12.0"        },
	{ "08",          "8"           },
	{ "+12.34",      "12.34"       },
	{ ".34",         "0.34"        },
	{ "2.",          "2.0"         },
	{ "1e0",         "1"           },
	{ "1E+0",        "1"           },
	{ "1e-0",        "1"           },
	{ "1E4",         "10000"       },
	{ ".1e-4",       "1e-05"       },
	{ "-5.0e006",    "-5000000.0"  },
	{ "-5.E+16",     "-5e+16"      },
	{ "-.05",        "-0.05"       },
	{ "-.0",         "-0.0"        },
	{ "-1.e6",       "-1000000.0"  },
	{ "\"foo\"",     "\"foo\""     },
	{ "\"foo bar\"", "\"foo bar\"" },
	{ "t",           "t" },
	{ "()",          "()" },

	{ "frac",       NULL },
	{ "3/4",        "3/4" },
	{ "4/3",        "4/3" },
	{ "+1/2",       "1/2" },
	{ "2/1",        "2" },
	{ "8/+1",       "8" },
	{ "8/4",        "2" },
	{ "4/8",        "1/2" },
	{ "02384/7238", "1192/3619" },
	{ "-1/2",       "-1/2" },
	{ "1/-2",       "-1/2" },
	{ "-6/3",       "-2" },
	{ "-6/-3",      "2" },

	{ "comments",                  NULL },
	{ "; commment",                ""   },
	{ "; (+ 1 1)",                 ""   },
	{ "(+ 1 ; more comments\n1)",  "2"  },

	{ "whitespace",                      NULL },
	{ "\t \n  \n\n\t\n \t\n",            ""   },
	{ "\t  \t(+   \t\t5 \n \n5  \n\t)",  "10" },

	{ "quote",                   NULL },
	{ "(quote 1)",               "1" },
	{ "(quote 9234)",            "9234" },
	{ "(quote \"foo\")",         "\"foo\"" },
	{ "(quote bar)",             "bar" },
	{ "(quote (1 2 3 4))",       "(1 2 3 4)" },
	{ "(quote (quote 1))",       "(quote 1)" },
	{ "(quote (+ 2 2))",         "(+ 2 2)" },
	{ "'12",                     "12" },
	{ "'foo",                    "foo" },
	{ "'(1 2 3 4)",              "(1 2 3 4)" },

	{ "cons",                         NULL },
	{ "(cons 1 2)",                   "(1 . 2)" },
	{ "(cons 1 (cons 2 3))",          "(1 2 . 3)" },
	{ "(cons 1 (cons 2 (cons 3 4)))", "(1 2 3 . 4)" },
	{ "(cons \"foo\" \"bar\")",       "(\"foo\" . \"bar\")" },
	{ "(cons (+ 1 2) 3)",             "(3 . 3)" },
	{ "(cons (cons 1 2) (cons 3 4))", "((1 . 2) 3 . 4)" },

	{ "cxr",                                       NULL },
	{ "(car (cons 1 2))",                          "1" },
	{ "(cdr (cons 1 2))",                          "2" },
	{ "(car (quote (1 2 3 4)))",                   "1" },
	{ "(car (cdr (quote (1 2 3 4))))",             "2" },
	{ "(car (cdr (cdr (quote (1 2 3 4)))))",       "3" },
	{ "(car (cdr (cdr (cdr (quote (1 2 3 4))))))", "4" },
	{ "(cdr (quote (1 2 3 4)))",                   "(2 3 4)" },
	{ "(cdr (cdr (quote (1 2 3 4))))",             "(3 4)" },
	{ "(car (cons 1 (cons 2 3)))",                 "1" },
	{ "(cdr (cons 1 (cons 2 3)))",                 "(2 . 3)" },
	{ "(cdr (cdr (cons 1 (cons 2 3))))",           "3" },

	{ "cond",                                       NULL },
	{ "(cond)",                                     "()" },
	{ "(cond (t 1))",                               "1" },
	{ "(cond ((= 1 1) 1) ((= 1 2) 2) (t 3))",       "1" },
	{ "(cond ((= 1 2) 1) ((= 1 2) 2) (t (+ 1 2)))", "3" },
	{ "(cond ((= 1 2) 1) ((= 1 1) 2) (t 3))",       "2" },
	{ "(cond ((= 1 2) 1) ((= 1 3) 2))",             "()" },
	{ "(cond ((= 1 2) 1) (\"foo\" 2) (t 3))",       "2" },
	{ "(cond (() (+ 1 2)))",                        "()" },

	{ "eq",                                          NULL },
	{ "(=)",                                         "t" },
	{ "(= 1)",                                       "t" },
	{ "(= \"foo\")",                                 "t" },
	{ "(= 1 1)",                                     "t" },
	{ "(= 1 1 1 1 1 1)",                             "t" },
	{ "(= 1 2)",                                     "()" },
	{ "(= 1 1 2 1 1 1)",                             "()" },
	{ "(= 1 1 1 1 1 2)",                             "()" },
	{ "(= 2 1 1 1 1 1)",                             "()" },
	{ "(= 4/5 4/5)",                                 "t" },
	{ "(= 2/1 2)",                                   "t" },
	{ "(= 2/4 1/2)",                                 "t" },
	{ "(= 2/4 1/2 4/8 3/6)",                         "t" },
	{ "(= 1/2 4/5)",                                 "()" },
	{ "(= 5/4 4/5)",                                 "()" },
	{ "(= 3 3/2)",                                   "()" },
	{ "(= 3 3/2 3 3 3)",                             "()" },
	{ "(= (+ 1 1) (+ 2 0))",                         "t" },
	{ "(= \"foo\" \"foo\")",                         "t" },
	{ "(= \"foo\" \"bar\")",                         "()" },
	{ "(= \"foo\" \"foo\" \"foo\" \"foo\" \"foo\")", "t" },
	{ "(= \"foo\" \"bar\" \"foo\" \"foo\" \"foo\")", "()" },
	{ "(= \"foo\" 3)",                               "()" },
	{ "(= \"foo\" \"foo\" 4 \"foo\" \"foo\")",       "()" },
	{ "(= \"foo\" \"FOO\")",                         "()" },
	{ "(= t t)",                                     "t" },
	{ "(= car car)",                                 "t" },
	{ "(= car cdr)",                                 "()" },
	{ "(= quote quote quote)",                       "t" },

	{ "define",                      NULL },
	{ "(define foo 4)",              ""   },
	{ "foo",                         "4"  },
	{ "(define bar foo)",            ""   },
	{ "bar",                         "4"  },
	{ "(define add +)",              ""   },
	{ "(add foo bar)",               "8"  },
	{ "(define (one x) (add x 1))",  ""   },
	{ "(one foo)",                   "5"  },
	{ "(define (more x)"
	  "        (define term 3)"
	  "        (+ x term))",         ""   },
	{ "(more 8)",                    "11" },
	{ "(define (add2 x)"
	  "        (+ x 1) (+ x 2))",    ""   },
	{ "(add2 2)",                    "4"  },

	{ "lambda",                       NULL },
	{ "((lambda (x) x) 3)",           "3" },
	{ "((lambda (x) x) (+ 1 2))",     "3" },
	{ "((lambda (x) (+ x 1)) 8)",     "9" },
	{ "((lambda (a b) (+ a b)) 2 2)", "4" },
	{ "((lambda () 5))",              "5" },

	{ "add",           NULL },
	{ "(+ 1 1)",       "2" },
	{ "(+ 1 (+ 1 2))", "4" },
	{ "(+ 1029 283)",  "1312" },

	{ "sub",       NULL },
	{ "(- 3)",     "-3" },
	{ "(- +3)",    "-3" },
	{ "(- -289)",  "289" },
	{ "(- 5 4)",   "1" },
	{ "(- 53 88)", "-35" },

	{ "compare",      NULL },
	{ "(< 2 3)",      "t" },
	{ "(< 3 3)",      "()" },
	{ "(< 4 3)",      "()" },
	{ "(<= -2 +4)",   "t" },
	{ "(<= -2 -2)",   "t" },
	{ "(<= 4 -2)",    "()" },
	{ "(> 89 34)",    "t" },
	{ "(> 48 48)",    "()" },
	{ "(> 98 183)",   "()" },
	{ "(>= +4 -282)", "t" },
	{ "(>= 39 39)",   "t" },
	{ "(>= -32 -30)", "()" },

	{ NULL,          NULL },
};

int
main(void)
{
	int correct = 0, total = 0, seccorrect = 0, sectotal = 0;
	Env env = tisp_env_init(64);
	tib_env_math(env);

	for (int i = 0; ; i++) {
		if (!tests[i][1]) {
			if (i != 0)
				printf("%d/%d\n", seccorrect, sectotal);
			if (!tests[i][0])
				break;
			printf("%-10s ", tests[i][0]);
			seccorrect = 0;
			sectotal = 0;
		} else {
			if (tisp_test(env, tests[i][0], tests[i][1])) {
				correct++;
				seccorrect++;
			}
			total++;
			sectotal++;
		}
	}
	printf("%-10s %d/%d\n", "total", correct, total);

	tisp_env_free(env);
	return correct != total;
}