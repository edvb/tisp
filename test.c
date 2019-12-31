/* See LICENSE file for copyright and license details. */
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "tisp.h"
#include "libs.tsp.h"

tsp_include_tib(math);
tsp_include_tib(string);

#define LEN(X) (sizeof(X) / sizeof((X)[0]))

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
	{ "3/4",         "3/4"         },
	{ "4/3",         "4/3"         },
	{ "+1/2",        "1/2"         },
	{ "2/1",         "2"           },
	{ "8/+1",        "8"           },
	{ "8/4",         "2"           },
	{ "4/8",         "1/2"         },
	{ "02384/7238",  "1192/3619"   },
	{ "-1/2",        "-1/2"        },
	{ "1/-2",        "-1/2"        },
	{ "-6/3",        "-2"          },
	{ "-6/-3",       "2"           },
	{ "\"foo\"",     "\"foo\""     },
	{ "\"foo bar\"", "\"foo bar\"" },
	{ "t",           "t"           },
	{ "()",          "()"          },

	{ "comments",                  NULL },
	{ "; commment",                ""   },
	{ "; (+ 1 1)",                 ""   },
	{ "(+ 1 ; more comments\n1)",  "2"  },

	{ "whitespace",                      NULL },
	{ "\t \n  \n\n\t\n \t\n",            ""   },
	{ "\t  \t(+   \t\t5 \n \n5  \n\t)",  "10" },

	{ "quote",                   NULL        },
	{ "(quote 1)",               "1"         },
	{ "(quote 9234)",            "9234"      },
	{ "(quote \"foo\")",         "\"foo\""   },
	{ "(quote bar)",             "bar"       },
	{ "(quote (1 2 3 4))",       "(1 2 3 4)" },
	{ "(quote (quote 1))",       "(quote 1)" },
	{ "(quote (+ 2 2))",         "(+ 2 2)"   },
	{ "'12",                     "12"        },
	{ "'foo",                    "foo"       },
	{ "'(1 2 3 4)",              "(1 2 3 4)" },

	{ "cons",                         NULL                  },
	{ "(cons 1 2)",                   "(1 . 2)"             },
	{ "(cons 1 (cons 2 3))",          "(1 2 . 3)"           },
	{ "(cons 1 (cons 2 (cons 3 4)))", "(1 2 3 . 4)"         },
	{ "(cons \"foo\" \"bar\")",       "(\"foo\" . \"bar\")" },
	{ "(cons (+ 1 2) 3)",             "(3 . 3)"             },
	{ "(cons (cons 1 2) (cons 3 4))", "((1 . 2) 3 . 4)"     },

	{ "cxr",                                       NULL      },
	{ "(car (cons 1 2))",                          "1"       },
	{ "(cdr (cons 1 2))",                          "2"       },
	{ "(car (quote (1 2 3 4)))",                   "1"       },
	{ "(car (cdr (quote (1 2 3 4))))",             "2"       },
	{ "(car (cdr (cdr (quote (1 2 3 4)))))",       "3"       },
	{ "(car (cdr (cdr (cdr (quote (1 2 3 4))))))", "4"       },
	{ "(cdr (quote (1 2 3 4)))",                   "(2 3 4)" },
	{ "(cdr (cdr (quote (1 2 3 4))))",             "(3 4)"   },
	{ "(car (cons 1 (cons 2 3)))",                 "1"       },
	{ "(cdr (cons 1 (cons 2 3)))",                 "(2 . 3)" },
	{ "(cdr (cdr (cons 1 (cons 2 3))))",           "3"       },

	{ "void",   NULL },
	{ "(void)", ""  },

	{ "do",                                       NULL },
	{ "(do (+ 1 2) (+ 2 2))",                     "4"  },
	{ "(do (+ -4 8) (- 1 2) (* 80 0) (+ 39 -3))", "36" },
	{ "(do (mod 80 2) (/ 4 2) (void))",           ""   },

	{ "eval",                                   NULL        },
	{ "(eval ''hey)",                           "hey"       },
	{ "(eval \"sup\")",                         "\"sup\""   },
	{ "(eval (+ 1 2))",                         "3"         },
	{ "(eval '(- 4 3))",                        "1"         },
	{ "(eval ''(mod 9 3))",                     "(mod 9 3)" },
	{ "(do (define bar '(/ 25 5)) (eval bar))", "5"         },

	{ "cond",                                          NULL },
	{ "(cond)",                                        "" },
	{ "(cond (t 1))",                                  "1"  },
	{ "(cond ((= 1 1) 1) ((= 1 2) 2) (t 3))",          "1"  },
	{ "(cond ((= 1 2) 1) ((= 1 2) 2) (else (+ 1 2)))", "3"  },
	{ "(cond ((= 1 2) 1) ((= 1 1) 2) (else 3))",       "2"  },
	{ "(cond ((= 1 2) 1) ((= 1 3) 2))",                "" },
	{ "(cond ((= 1 2) 1) (\"foo\" 2) (else 3))",       "2"  },
	{ "(cond (() (+ 1 2)))",                           "" },

	{ "eq",                                          NULL },
	{ "(=)",                                         "t"  },
	{ "(= 1)",                                       "t"  },
	{ "(= \"foo\")",                                 "t"  },
	{ "(= 1 1)",                                     "t"  },
	{ "(= 1 1 1 1 1 1)",                             "t"  },
	{ "(= 1 2)",                                     "()" },
	{ "(= 1 1 2 1 1 1)",                             "()" },
	{ "(= 1 1 1 1 1 2)",                             "()" },
	{ "(= 2 1 1 1 1 1)",                             "()" },
	{ "(= 4/5 4/5)",                                 "t"  },
	{ "(= 2/1 2)",                                   "t"  },
	{ "(= 2/4 1/2)",                                 "t"  },
	{ "(= 2/4 1/2 4/8 3/6)",                         "t"  },
	{ "(= 1/2 4/5)",                                 "()" },
	{ "(= 5/4 4/5)",                                 "()" },
	{ "(= 3 3/2)",                                   "()" },
	{ "(= 3 3/2 3 3 3)",                             "()" },
	{ "(= (+ 1 1) (+ 2 0))",                         "t"  },
	{ "(= \"foo\" \"foo\")",                         "t"  },
	{ "(= \"foo\" \"bar\")",                         "()" },
	{ "(= \"foo\" \"foo\" \"foo\" \"foo\" \"foo\")", "t"  },
	{ "(= \"foo\" \"bar\" \"foo\" \"foo\" \"foo\")", "()" },
	{ "(= \"foo\" 3)",                               "()" },
	{ "(= \"foo\" \"foo\" 4 \"foo\" \"foo\")",       "()" },
	{ "(= \"foo\" \"FOO\")",                         "()" },
	{ "(= t t)",                                     "t"  },
	{ "(= car car)",                                 "t"  },
	{ "(= car cdr)",                                 "()" },
	{ "(= quote quote quote)",                       "t"  },
	{ "(= '(1 2 3) '(1 2 3))",                       "t"  },
	{ "(= '(a b c) '(a b c))",                       "t"  },
	{ "(= '(a b c) '(a b d))",                       "()" },
	{ "(= '(1 2 3) '(1 2))",                         "()" },
	{ "(= '(1 2 3) '(1))",                           "()" },
	{ "(= '((1 2) 3 4) '((1 2) 3 4))",               "t"  },
	{ "(= '((1 b) 3 4) '((1 2) 3 4))",               "()" },

	{ "define",                      NULL    },
	{ "(define foo 4)",              ""      },
	{ "foo",                         "4"     },
	{ "(define bar foo)",            ""      },
	{ "bar",                         "4"     },
	{ "(set! foo 5)",                "5"     },
	{ "foo",                         "5"     },
	{ "(set! foo (+ foo bar))",      "9"     },
	{ "foo",                         "9"     },
	{ "(define add +)",              ""      },
	{ "(add foo bar)",               "13"    },
	{ "(define (one x) (add x 1))",  ""      },
	{ "(one foo)",                   "10"    },
	{ "(define (more x)"
	  "        (define term 3)"
	  "        (+ x term))",         ""      },
	{ "(more 8)",                    "11"    },
	{ "(define (add2 x)"
	  "        (+ x 1) (+ x 2))",    ""      },
	{ "(add2 2)",                    "4"     },
	{ "(set! add2 2)",               "2"     },
	{ "add2",                        "2"     },
	{ "(set! add2 \"2\")",           "\"2\"" },
	{ "add2",                        "\"2\"" },

	{ "lambda",                       NULL },
	{ "((lambda (x) x) 3)",           "3"  },
	{ "((lambda (x) x) (+ 1 2))",     "3"  },
	{ "((lambda (x) (+ x 1)) 8)",     "9"  },
	{ "((lambda (a b) (+ a b)) 2 2)", "4"  },
	{ "((lambda () 5))",              "5"  },

	{ "control",                                   NULL  },
	{ "(if t 1 2)",                                "1"   },
	{ "(if () 1 2)",                               "2"   },
	{ "(if (integer? 3) t ())",                    "t"   },
	{ "(if (ratio? car) (cons 1 2) (car '(1 2)))", "1"   },
	{ "(when t 'foo)",                             "foo" },
	{ "(when () 'b  ar)",                          ""    },
	{ "(when (= 1 1) 4)",                          "4"   },
	{ "(unless t 'foo)",                           ""    },
	{ "(unless () 'bar)",                          "bar" },
	{ "(unless 3 4)",                              ""    },
	{ "(unless (< 5 4) 7)",                        "7"   },

	{ "logic",        NULL },
	{ "(not ())",     "t"  },
	{ "(not t)",      "()" },
	{ "(and () ())",  "()" },
	{ "(and t ())",   "()" },
	{ "(and () t)",   "()" },
	{ "(and t t)",    "t"  },
	{ "(nand () ())", "t"  },
	{ "(nand t ())",  "t"  },
	{ "(nand () t)",  "t"  },
	{ "(nand t t)",   "()" },
	{ "(or () ())",   "()" },
	{ "(or t ())",    "t"  },
	{ "(or () t)",    "t"  },
	{ "(or t t)",     "t"  },
	{ "(nor () ())",  "t"  },
	{ "(nor t ())",   "()" },
	{ "(nor () t)",   "()" },
	{ "(nor t t)",    "()" },

	{ "list",                     NULL                 },
	{ "(list 1 2 3)",             "(1 2 3)"            },
	{ "(list (* 2 2) (+ 2 3))",   "(4 5)"              },
	{ "(list 'a 'b 'c 'd 'e 'f)", "(a b c d e f)"      },
	{ "(list \"foo\")",           "(\"foo\")"          },
	{ "(list)",                   "()"                 },
	{ "(list 1/2 2/8 . 1/8)",     "(1/2 1/4 . 1/8)"    },
	{ "(list* .5 .25 .125)",      "(0.5 0.25 . 0.125)" },
	{ "(list* 1 2 3 4 5 6)",      "(1 2 3 4 5 . 6)"    },
	{ "(list* (sqr 3) (cube 4))", "(9 . 64)"           },
	{ "(list* 5/4)",              "5/4"                },

	{ "list get",                               NULL    },
	{ "(last '(1 2 3))",                        "3"     },
	{ "(last (list 4 5))",                      "5"     },
	{ "(last '(a b c d e f))",                  "f"     },
	{ "(last (cons 1 (cons 2 ())))",            "2"     },
	{ "(length '(1 2 3))",                      "3"     },
	{ "(length (list .3 -3/2 12 5))",           "4"     },
	{ "(length '(a b))",                        "2"     },
	{ "(length (list list))",                   "1"     },
	{ "(length ())",                            "0"     },
	{ "(length nil)",                           "0"     },
	{ "(nth '(1 2 3) 1)",                       "2"     },
	{ "(nth (list 3 5/2 .332 -2) 2)",           "0.332" },
	{ "(nth '(a b c) 0)",                       "a"     },
	{ "(nth (list 'foo 'bar 'zar 'baz) 3)",     "baz"   },
	{ "(count 3 '(1 2 3 4))",                   "1"     },
	{ "(count 1/2 (list 1/2 1/3 2/4 8 9.0))",   "2"     },
	{ "(count 'a '(b c a a f h a b c a))",      "4"     },
	{ "(count 3.2 nil)",                        "0"     },
	{ "(count \"Bobandy\" '(1/2 1/4 \"Jim\"))", "0"     },

	{ "list proc",                                              NULL                },
	{ "(apply list '(1 2 3))",                                  "(1 2 3)"           },
	{ "(apply + '(2 90))",                                      "92"                },
	{ "(apply list '(a b c d e))",                              "(a b c d e)"       },
	{ "(map car '((1 a) (2 b) (3 c)))",                         "(1 2 3)"           },
	{ "(map cdr '((1 a) (2 b) (3 c)))",                         "((a) (b) (c))"     },
	{ "(map (lambda (x) (car (cdr x))) '((1 a) (2 b) (3 c)))",  "(a b c)"           },
	{ "(map cadr '((1/2 .5) (\"conky\" .25) ('bubbles .125)))", "(0.5 0.25 0.125)"  },
	{ "(map inc (list 2 4 8 (^ 2 4)))",                         "(3 5 9 17)"        },
	{ "(filter positive? '(1 2 -4 5 -9 10))",                   "(1 2 5 10)"        },
	{ "(filter odd? '(8 6 17 9 82 34 27))",                     "(17 9 27)"         },
	{ "(filter integer? '(1/2 3.e-2 9/3 3.2 0.0 8 17))",        "(3 8 17)"          },

	{ "list mod",                                 NULL                        },
	{ "(reverse '(1 2 3 4 5))",                   "(5 4 3 2 1)"               },
	{ "(reverse (list -20 5/2 .398))",            "(0.398 5/2 -20)"           },
	{ "(reverse '(a b))",                         "(b a)"                     },
	{ "(reverse (list \"foo\" \"bar\" \"baz\"))", "(\"baz\" \"bar\" \"foo\")" },
	{ "(reverse (cons 1/2 nil))",                 "(1/2)"                     },
	{ "(reverse ())",                             "()"                        },
	{ "(append '(1 2 3) '(4 5 6))",               "(1 2 3 4 5 6)"             },
	{ "(append (list (+ 1 2) 4) '(a b c))",       "(3 4 a b c)"               },

	{ "assoc",                                                      NULL          },
	{ "(zip '(1 2 3 4) '(a b c d))",
		"((1 . a) (2 . b) (3 . c) (4 . d))"                                   },
	{ "(zip (list 'ricky 'lahey) (list \"julian\" \"randy\"))",
		"((ricky . \"julian\") (lahey . \"randy\"))"                          },
	{ "(assoc 'baz '((foo . 3) (bar . 8) (baz . 14)))",             "(baz . 14)"  },
	{ "(assoc 'a '((a b) (3 2.1) (3.2 4/3) (3.2 3.2)))",            "(a b)"       },
	{ "(assoc 3 '((1 b)))",                                         "()"          },
	{ "(assoc 4/3 (list (list 1 pi) (list 4/3 1/2 3) (list 2 3)))", "(4/3 1/2 3)" },

	{ "member",                                         NULL                             },
	{ "(memp even? (list 1 3 19 4 7 8 2))",             "(4 7 8 2)"                      },
	{ "(memp negative? (list 1/3 pi 3.2e-9 0 4 -7 2))", "(-7 2)"                         },
	{ "(memp (lambda (x) (> x 8)) '(1/3 1/2 5/3 8 9))", "(9)"                            },
	{ "(memp (lambda (x) (= x \"fry\")) "
		"'(\"fry\" \"nibbler\" \"prof\"))",         "(\"fry\" \"nibbler\" \"prof\")" },
	{ "(member 'foo '(foo bar baz))",                   "(foo bar baz)"                  },
	{ "(member 'bar '(foo bar baz))",                   "(bar baz)"                      },
	{ "(member 4 '(12 38 4 8))",                        "(4 8)"                          },
	{ "(member 3.2 '(4/3 2 8 2 3.14 3.2))",             "(3.2)"                          },
	{ "(member \"quux\" (list 4.2 3 'quux))",           "()"                             },
	{ "(member 'qux '(foo bar baz))",                   "()"                             },

	{ "quasiquote",          NULL              },
	{ "`7.2",                "7.2"             },
	{ "`cory",               "cory"            },
	{ "`,foo",               "9"               },
	{ "`(1 2 3)",            "(1 2 3)"         },
	{ "`(\"sunnyvale\")",    "(\"sunnyvale\")" },
	{ "`(1/2 . 2/1)",        "(1/2 . 2)"       },
	{ "`(cory trevor)",      "(cory trevor)"   },
	{ "`(foo bar quax)",     "(foo bar quax)"  },
	{ "`(,foo ,bar)",        "(9 4)"           },
	{ "`(,foo . ,bar)",      "(9 . 4)"         },
	{ "`(,foo . ,bar)",      "(9 . 4)"         },
	{ "`(foo bar ,foo fry)", "(foo bar 9 fry)" },

	{ "stack",                                   NULL                   },
	{ "(peek '(1 2 3 4 5 6))",                   "1"                    },
	{ "(peek (list 'a 'b 'c))",                  "a"                    },
	{ "(pop (list 1/2 1/4))",                    "(1/4)"                },
	{ "(pop '(\"foo\" \"bar\" \"baz\"))",        "(\"bar\" \"baz\")"    },
	{ "(push '(6 3 5/3 .38) .5)",                "(0.5 6 3 5/3 0.38)"   },
	{ "(push (list \"ni\" 'shrubbery) (* 3 2))", "(6 \"ni\" shrubbery)" },
	{ "(swap '(1 2 3 5 7 11))",                  "(2 1 3 5 7 11)"       },
	{ "(swap (list 1/2 1/4 1/9 1/16))",          "(1/4 1/2 1/9 1/16)"   },

	{ "stack!",                  NULL               },
	{ "(define s '(1 2 3 4 5))", ""                 },
	{ "(peek s)",                "1"                },
	{ "(pop! s)",                "1"                },
	{ "s",                       "(2 3 4 5)"        },
	{ "(pop! s)",                "2"                },
	{ "s",                       "(3 4 5)"          },
	{ "(push! s 3/2)",           "(3/2 3 4 5)"      },
	{ "s",                       "(3/2 3 4 5)"      },
	{ "(push! s (- (/ 2)))",     "(-1/2 3/2 3 4 5)" },
	{ "s",                       "(-1/2 3/2 3 4 5)" },
	{ "(swap! s)",               "(3/2 -1/2 3 4 5)" },
	{ "s",                       "(3/2 -1/2 3 4 5)" },
	{ "(swap! s)",               "(-1/2 3/2 3 4 5)" },
	{ "s",                       "(-1/2 3/2 3 4 5)" },

	{ "numbers",            NULL   },
	{ "(decimal 1/2)",      "0.5"  },
	{ "(decimal 3/-2)",     "-1.5" },
	{ "(decimal 1)",        "1.0"  },
	{ "(decimal 3.14)",     "3.14" },
	{ "(integer 1/2)",      "0"    },
	{ "(integer 3/-2)",     "-1"   },
	{ "(integer 1)",        "1"    },
	{ "(integer 3.14)",     "3"    },
	{ "(numerator 3/2)",    "3"    },
	{ "(numerator 1/2)",    "1"    },
	{ "(numerator -4/2)",   "-2"   },
	{ "(numerator 24)",     "24"   },
	{ "(denominator 1/4)",  "4"    },
	{ "(denominator 4/3)",  "3"    },
	{ "(denominator 4/2)",  "1"    },
	{ "(denominator 14/8)", "4"    },
	{ "(denominator -4)",   "1"    },


	{ "round",               NULL    },
	{ "(round 7/3)",         "2"     },
	{ "(round -3/4)",        "-1"    },
	{ "(round 6.3)",         "6.0"   },
	{ "(round -8.1)",        "-8.0"  },
	{ "(round 3)",           "3"     },
	{ "(round -81)",         "-81"   },
	{ "(round 0)",           "0"     },
	{ "(floor 5/3)",         "1"     },
	{ "(floor -9/4)",        "-3"    },
	{ "(floor 6.3)",         "6.0"   },
	{ "(floor -8.1)",        "-9.0"  },
	{ "(floor 3)",           "3"     },
	{ "(floor -81)",         "-81"   },
	{ "(floor 0)",           "0"     },
	{ "(ceil 1/2)",          "1"     },
	{ "(ceil -8/5)",         "-1"    },
	{ "(ceil pi)",           "4.0"   },
	{ "(ceil (- .2))",       "-0.0"  },
	{ "(ceil 128)",          "128"   },
	{ "(ceil -2)",           "-2"    },
	{ "(ceil 0)",            "0"     },
	{ "(truncate (/ 17 2))", "8"     },
	{ "(truncate -12/5)",    "-2"    },
	{ "(truncate (exp 2.))", "7.0"   },
	{ "(truncate 124.1380)", "124.0" },
	{ "(truncate 8)",        "8"     },
	{ "(truncate -5)",       "-5"    },
	{ "(truncate 0)",        "0"     },

	{ "arithmetic",    NULL               },
	{ "(+ 1 1)",       "2"                },
	{ "(+ 1 (+ 1 2))", "4"                },
	{ "(+ 1029 283)",  "1312"             },
	{ "(+ 204  8.3)",  "212.3"            },
	{ "(+ 33   3/4)",  "135/4"            },
	{ "(+ 1/3 5)",     "16/3"             },
	{ "(+ 7/4 pi)",    "4.89159265358979" },
	{ "(+ 2/5 3/2)",   "19/10"            },
	{ "(+ 2.1 2)",     "4.1"              },
	{ "(+ 8.6 5.3)",   "13.9"             },
	{ "(+ 3.7 1/8)",   "3.825"            },
	{ "(- 3)",         "-3"                },
	{ "(- +3)",        "-3"                },
	{ "(- -289)",      "289"               },
	{ "(- 7/8)",       "-7/8"              },
	{ "(- -6.412E2)",  "641.2"             },
	{ "(- 5 4)",       "1"                 },
	{ "(- 53 88)",     "-35"               },
	{ "(- 204  8.3)",  "195.7"             },
	{ "(- 33   3/4)",  "129/4"             },
	{ "(- 1/3 5)",     "-14/3"             },
	{ "(- 7/4 pi)",    "-1.39159265358979" },
	{ "(- 2/5 3/2)",   "-11/10"            },
	{ "(- 2.1 2)",     "0.1"               },
	{ "(- 8.6 5.3)",   "3.3"               },
	{ "(- 3.7 1/8)",   "3.575"             },
	{ "(* 3 2)",          "6"                 },
	{ "(* -2 8.89)",      "-17.78"            },
	{ "(* 6 3/4)",        "9/2"               },
	{ "(* 1.004 8)",      "8.032"             },
	{ "(* 1.34e3 .0012)", "1.608"             },
	{ "(* e -5/2)",       "-6.79570457114761" },
	{ "(* 1/3 6)",        "2"                 },
	{ "(* 5/2 14.221)",   "35.5525"           },
	{ "(* 6/8 8/7)",      "6/7"               },
	{ "(/ 1 2)",            "1/2"                },
	{ "(/ 8 4)",            "2"                  },
	{ "(/ 6 2.1)",          "2.85714285714286"   },
	{ "(/ 4 4/3)",          "3"                  },
	{ "(/ 5)",              "1/5"                },
	{ "(/ 4473)",           "1/4473"             },
	{ "(/ 10.42 5)",        "2.084"              },
	{ "(/ 1.34e-2 4.3332)", "0.0030924028431644" },
	{ "(/ 1.04 -15/4)",     "-0.277333333333333" },
	{ "(/ 4/3 7)",          "4/21"               },
	{ "(/ 5/4 3.2)",        "0.390625"           },
	{ "(/ 1/3 5/4)",        "4/15"               },
	{ "(mod 10 3)",   "1"  },
	{ "(mod -11 3)",  "-2" },
	{ "(mod 10 -3)",  "1"  },
	{ "(mod -10 -3)", "-1" },
	{ "(mod 10 5)",   "0"  },
	{ "(mod 7 2)",    "1"  },
	{ "(mod 8 5)",    "3"  },

	{ "compare",      NULL },
	{ "(< 2 3)",      "t"  },
	{ "(< 3 3)",      "()" },
	{ "(< 4 3)",      "()" },
	{ "(<= -2 +4)",   "t"  },
	{ "(<= -2 -2)",   "t"  },
	{ "(<= 4 -2)",    "()" },
	{ "(> 89 34)",    "t"  },
	{ "(> 48 48)",    "()" },
	{ "(> 98 183)",   "()" },
	{ "(>= +4 -282)", "t"  },
	{ "(>= 39 39)",   "t"  },
	{ "(>= -32 -30)", "()" },

	{ "abs",        NULL  },
	{ "(abs 4)",    "4"   },
	{ "(abs -3/5)", "3/5" },
	{ "(abs 0.0)",  "0.0" },

	{ "sgn",         NULL   },
	{ "(sgn 239)",   "1"    },
	{ "(sgn -3)",    "-1"   },
	{ "(sgn 5/4)",   "1"    },
	{ "(sgn -1/7)",  "-1"   },
	{ "(sgn 3.17)",  "1.0"  },
	{ "(sgn -.457)", "-1.0" },
	{ "(sgn 0)",     "0"    },
	{ "(sgn 0.0)",   "0.0"  },

	{ "max/min",          NULL    },
	{ "(max 9346 13297)", "13297" },
	{ "(max -3 8)",       "8"     },
	{ "(max 3/2 1/2)",    "3/2"   },
	{ "(max 0 -.5)",      "0"     },
	{ "(min 4 48)",       "4"     },
	{ "(min -80 -148)",   "-148"  },
	{ "(min 7/2 -3)",     "-3"    },
	{ "(min 1/2 1/3)",    "1/3"   },
	{ "(min .05 .06)",    "0.05"  },

	{ NULL,          NULL },
};

int
tisp_test(Tsp st, const char *input, const char *expect, int output)
{
	Val v;
	FILE *f;
	size_t nread;
	char buf[BUFSIZ] = {0};

	if (!(st->file = strdup(input)))
		return 0;
	st->filec = 0;
	if (!(v = tisp_read(st)))
		return 0;
	if (!(v = tisp_eval(st, st->global, v))) {
		if (output)
			putchar('\n');
		return 0;
	}

	f = fopen("test.out", "w");
	tisp_print(f, v);
	fclose(f);

	f = fopen("test.out", "r");
	while ((nread = fread(buf, 1, sizeof(buf), f)) > 0) ;
	fclose(f);
	remove("test.out");

	if (output)
		printf("%s\n", buf);
	return strcmp(buf, expect) == 0;
}

int
main(void)
{
	int correct = 0, total = 0, seccorrect = 0, sectotal = 0, last = 1;
	int errors[LEN(tests)] = {0};
	Tsp st = tisp_env_init(1024);
	tib_env_math(st);
	tib_env_string(st);
	tisp_env_lib(st, libs_tsp);

	for (int i = 0; ; i++) {
		if (!tests[i][1]) {
			if (i != 0) {
				printf("%d/%d\n", seccorrect, sectotal);
				for (int j = last; j < i; j++)
					if (!tests[j][1]) {
						printf("%-10s\n", tests[j][0]);
					} else if (errors[j]) {
						printf("  input: %s\n"
						       "    expect: %s\n"
						       "    output: ", tests[j][0], tests[j][1]);
						tisp_test(st, tests[j][0], tests[j][1], 1);
					}
				last = i + 1;
			}
			if (!tests[i][0])
				break;
			printf("%-10s ", tests[i][0]);
			seccorrect = 0;
			sectotal = 0;
		} else {
			if (tisp_test(st, tests[i][0], tests[i][1], 0)) {
				correct++;
				seccorrect++;
			} else {
				errors[i] = 1;
			}
			total++;
			sectotal++;
		}
	}
	printf("%-10s %d/%d\n", "total", correct, total);

	return correct != total;
}
