/* See LICENSE file for copyright and license details. */
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "tisp.h"
#include "tibs.tsp.h"

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
	{ "\"foo\"",     "foo"         },
	{ "\"foo bar\"", "foo bar"     },
	{ "True",        "True"        },
	{ "()",          "Nil"         },
	{ "Nil",         "Nil"         },
	{ "Void",        "Void"        },

	{ "comments",                  NULL      },
	{ "; commment",                "Void"    },
	{ "; (+ 1 1)",                 "Void"    },
	{ "(+ 1 ; more comments\n1)",  "2"       },

	{ "whitespace",                      NULL      },
	{ "\t \n  \n\n\t\n \t\n",            "Void"    },
	{ "\t  \t(+   \t\t5 \n \n5  \n\t)",  "10"      },

	{ "quote",                   NULL        },
	{ "(quote 1)",               "1"         },
	{ "(quote 9234)",            "9234"      },
	{ "(quote \"foo\")",         "foo"       },
	{ "(quote bar)",             "bar"       },
	{ "(quote (1 2 3 4))",       "(1 2 3 4)" },
	{ "(quote (quote 1))",       "(quote 1)" },
	{ "(quote (+ 2 2))",         "(+ 2 2)"   },
	{ "'12",                     "12"        },
	{ "'foo",                    "foo"       },
	{ "'(1 2 3 4)",              "(1 2 3 4)" },
	{ "'()",                     "Nil"       },

	{ "cons",                         NULL              },
	{ "(cons 1 2)",                   "(1 . 2)"         },
	{ "(cons 1 (cons 2 3))",          "(1 2 . 3)"       },
	{ "(cons 1 (cons 2 (cons 3 4)))", "(1 2 3 . 4)"     },
	{ "(cons \"foo\" \"bar\")",       "(foo . bar)"     },
	{ "(cons (+ 1 2) 3)",             "(3 . 3)"         },
	{ "(cons (cons 1 2) (cons 3 4))", "((1 . 2) 3 . 4)" },

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

	{ "do",                                       NULL      },
	{ "(do (+ 1 2) (+ 2 2))",                     "4"       },
	{ "(do (+ -4 8) (- 1 2) (* 80 0) (+ 39 -3))", "36"      },
	{ "(do (mod 80 2) (/ 4 2) Void)",             "Void"    },

	{ "eval",                                   NULL        },
	{ "(eval ''hey)",                           "hey"       },
	{ "(eval \"sup\")",                         "sup"       },
	{ "(eval (+ 1 2))",                         "3"         },
	{ "(eval '(- 4 3))",                        "1"         },
	{ "(eval ''(mod 9 3))",                     "(mod 9 3)" },
	{ "(do (def bar '(/ 25 5)) (eval bar))",    "5"         },

	{ "cond",                                          NULL      },
	{ "(cond)",                                        "Void"    },
	{ "(cond (True 1))",                               "1"       },
	{ "(cond ((= 1 1) 1) ((= 1 2) 2) (True 3))",       "1"       },
	{ "(cond ((= 1 2) 1) ((= 1 2) 2) (else (+ 1 2)))", "3"       },
	{ "(cond ((= 1 2) 1) ((= 1 1) 2) (else 3))",       "2"       },
	{ "(cond ((= 1 2) 1) ((= 1 3) 2))",                "Void"    },
	{ "(cond ((= 1 2) 1) (\"foo\" 2) (else 3))",       "2"       },
	{ "(cond (() (+ 1 2)))",                           "Void"    },

	{ "eq",                                          NULL   },
	{ "(=)",                                         "True" },
	{ "(= 1)",                                       "True" },
	{ "(= \"foo\")",                                 "True" },
	{ "(= 1 1)",                                     "True" },
	{ "(= 1 1 1 1 1 1)",                             "True" },
	{ "(= 1 2)",                                     "Nil"  },
	{ "(= 1 1 2 1 1 1)",                             "Nil"  },
	{ "(= 1 1 1 1 1 2)",                             "Nil"  },
	{ "(= 2 1 1 1 1 1)",                             "Nil"  },
	{ "(= 4/5 4/5)",                                 "True" },
	{ "(= 2/1 2)",                                   "True" },
	{ "(= 2/4 1/2)",                                 "True" },
	{ "(= 2/4 1/2 4/8 3/6)",                         "True" },
	{ "(= 1/2 4/5)",                                 "Nil"  },
	{ "(= 5/4 4/5)",                                 "Nil"  },
	{ "(= 3 3/2)",                                   "Nil"  },
	{ "(= 3 3/2 3 3 3)",                             "Nil"  },
	{ "(= (+ 1 1) (+ 2 0))",                         "True" },
	{ "(= \"foo\" \"foo\")",                         "True" },
	{ "(= \"foo\" \"bar\")",                         "Nil"  },
	{ "(= \"foo\" 'foo)",                            "Nil"  },
	{ "(= 'bar 'bar)",                               "True" },
	{ "(= \"foo\" \"foo\" \"foo\" \"foo\" \"foo\")", "True" },
	{ "(= \"foo\" \"bar\" \"foo\" \"foo\" \"foo\")", "Nil"  },
	{ "(= \"foo\" 3)",                               "Nil"  },
	{ "(= \"foo\" \"foo\" 4 \"foo\" \"foo\")",       "Nil"  },
	{ "(= \"foo\" \"FOO\")",                         "Nil"  },
	{ "(= True True)",                               "True" },
	{ "(= car car)",                                 "True" },
	{ "(= car cdr)",                                 "Nil"  },
	{ "(= quote quote quote)",                       "True" },
	{ "(= '(1 2 3) (list 1 2 3))",                   "True" },
	{ "(= '(a b c) '(a b c))",                       "True" },
	{ "(= '(a b c) '(a b d))",                       "Nil"  },
	{ "(= '(1 2 3) '(1 2))",                         "Nil"  },
	{ "(= '(1 2 3) '(1))",                           "Nil"  },
	{ "(= '((1 2) 3 4) '((1 2) 3 4))",               "True" },
	{ "(= '((1 b) 3 4) '((1 2) 3 4))",               "Nil"  },
	{ "(= (Func (it) it) @it)",                      "True" },
	{ "(= @it (Func (x) x))",                        "Nil"  },
	{ "(/=)",                                        "Nil"  },
	{ "(/= 'a)",                                     "Nil"  },
	{ "(/= '(1 . 2) (list* 1 2))",                   "Nil"  },
	{ "(/= 1 2)",                                    "True" },
	{ "(/= 1 1 2 1 1 1)",                            "True" },
	{ "(/= \"foo\" \"bar\")",                        "True" },
	{ "(/= 'greg 'greg 'greg 'greg)",                "Nil"  },

	{ "def",                         NULL      },
	{ "(def foo 4)",                 "Void"    },
	{ "foo",                         "4"       },
	{ "(def bar foo)",               "Void"    },
	{ "bar",                         "4"       },
	{ "(def foo (+ foo bar))",       "Void"    },
	{ "foo",                         "8"       },
	{ "(def add +)",                 "Void"    },
	{ "(add foo bar)",               "12"      },
	{ "(def (one x) (add x 1))",     "Void"    },
	{ "(one foo)",                   "9"       },
	{ "(def (more x)"
	  "        (def term 3)"
	  "        (+ x term))",         "Void"    },
	{ "(more 8)",                    "11"      },
	{ "(def (add2 x)"
	  "        (+ x 1) (+ x 2))",    "Void"    },
	{ "(add2 2)",                    "4"       },

	{ "defined?",                    NULL      },
	{ "(defined? invalid-var)",      "Nil"     },
	{ "(defined? defined?)",         "True"    },
	{ "(defined? car)",              "True"    },
	{ "(defined? when)",             "True"    },
	{ "(defined? apply)",            "True"    },

	{ "Func",                       NULL },
	{ "((Func (x) x) 3)",           "3"  },
	{ "((Func (x) x) (+ 1 2))",     "3"  },
	{ "((Func (x) (+ x 1)) 8)",     "9"  },
	{ "((Func (a b) (+ a b)) 2 2)", "4"  },
	{ "((Func () 5))",              "5"  },

	{ "control",                                              NULL      },
	{ "(if True 1 2)",                                        "1"       },
	{ "(if () 1 2)",                                          "2"       },
	{ "(if (integer? 3) True ())",                            "True"    },
	{ "(if (ratio? car) (cons 1 2) (car '(1 2)))",            "1"       },
	{ "(when True 'foo)",                                     "foo"     },
	{ "(when () 'b  ar)",                                     "Void"    },
	{ "(when (= 1 1) 4)",                                     "4"       },
	{ "(unless True 'foo)",                                   "Void"    },
	{ "(unless () 'bar)",                                     "bar"     },
	{ "(unless 3 4)",                                         "Void"    },
	{ "(unless (< 5 4) 7)",                                   "7"       },
	{ "(switch 5 (3 'yes) (5 'no))",                          "no"      },
	{ "(switch (+ 1 2) ((mod 8 5) 'yes) (err 'no))",          "yes"     },
	{ "(switch 2 (3 'yes) (5 'no))",                          "Void"    },
	{ "(switch \"foo\" (e \"bar\") (\"foo\" 'zar) ('baz 3))", "zar"     },

	/* TODO other syms as well */
	{ "logic",              NULL   },
	{ "(not ())",           "True" },
	{ "(not True)",         "Nil"  },
	{ "(and () ())",        "Nil"  },
	{ "(and True ())",      "Nil"  },
	{ "(and () True)",      "Nil"  },
	{ "(and True True)",    "True" },
	{ "(or () ())",         "Nil"  },
	{ "(or True ())",       "True" },
	{ "(or () True)",       "True" },
	{ "(or True True)",     "True" },
	{ "(xor? Nil ())",      "Nil"  },
	{ "(xor? True Nil)",    "True" },
	{ "(xor? Nil True)",    "True" },
	{ "(xor? True True)",   "Nil"  },

	{ "list",                     NULL                 },
	{ "(list 1 2 3)",             "(1 2 3)"            },
	{ "(list (* 2 2) (+ 2 3))",   "(4 5)"              },
	{ "(list 'a 'b 'c 'd 'e 'f)", "(a b c d e f)"      },
	{ "(list \"foo\")",           "(foo)"              },
	{ "(list)",                   "Nil"                },
	{ "(list 1/2 2/8 . 1/8)",     "(1/2 1/4 . 1/8)"    },
	{ "(list* .5 .25 .125)",      "(0.5 0.25 . 0.125)" },
	{ "(list* 1 2 3 4 5 6)",      "(1 2 3 4 5 . 6)"    },
	{ "(list* (sqr 3) (cube 4))", "(9 . 64)"           },
	{ "(list* 5/4)",              "5/4"                },

	{ "list get",                               NULL            },
	{ "(last '(1 2 3))",                        "3"             },
	{ "(last (list 4 5))",                      "5"             },
	{ "(last '(a b c d e f))",                  "f"             },
	{ "(last (cons 1 (cons 2 ())))",            "2"             },
	{ "(length '(1 2 3))",                      "3"             },
	{ "(length (list .3 -3/2 12 5))",           "4"             },
	{ "(length '(a b))",                        "2"             },
	{ "(length (list list))",                   "1"             },
	{ "(length ())",                            "0"             },
	{ "(length Nil)",                           "0"             },
	{ "(nth '(1 2 3) 1)",                       "2"             },
	{ "(nth (list 3 5/2 .332 -2) 2)",           "0.332"         },
	{ "(nth '(a b c) 0)",                       "a"             },
	{ "(nth (list 'foo 'bar 'zar 'baz) 3)",     "baz"           },
	{ "(head '(1.2 1.3 1.4 1.5) 2)",            "(1.2 1.3)"     },
	{ "(head '(1 1e1 1e2 1e3) 3)",              "(1 10 100)"    },
	{ "(head '(1 2 3) 1)",                      "(1)"           },
	{ "(head '(1 2) 0)",                        "Nil"           },
	{ "(tail '(randy bobandy lahey bubs) 3)",   "(bubs)"        },
	{ "(tail (list 1/2 1/3 1/4) 0)",            "(1/2 1/3 1/4)" },
	{ "(tail '(2 4 9 16 25 36) 2)",             "(9 16 25 36)"  },
	{ "(count 3 '(1 2 3 4))",                   "1"             },
	{ "(count 1/2 (list 1/2 1/3 2/4 8 9.0))",   "2"             },
	{ "(count 'a '(b c a a f h a b c a))",      "4"             },
	{ "(count 3.2 Nil)",                        "0"             },
	{ "(count \"Bobandy\" '(1/2 1/4 \"Jim\"))", "0"             },

	{ "list proc",                                              NULL                    },
	{ "(apply list '(1 2 3))",                                  "(1 2 3)"               },
	{ "(apply + '(2 90))",                                      "92"                    },
	{ "(apply list '(a b c d e))",                              "(a b c d e)"           },
	{ "(map car '((1 a) (2 b) (3 c)))",                         "(1 2 3)"               },
	{ "(map cdr '((1 a) (2 b) (3 c)))",                         "((a) (b) (c))"         },
	{ "(map (Func (x) (car (cdr x))) '((1 a) (2 b) (3 c)))",    "(a b c)"               },
	{ "(map cadr '((1/2 .5) (\"conky\" .25) ('bubbles .125)))", "(0.5 0.25 0.125)"      },
	{ "(map inc (list 2 4 8 (^ 2 4)))",                         "(3 5 9 17)"            },
	{ "(convert 1 2 '(1 2 3 1 1 4 5 6 7 1))",                   "(2 2 3 2 2 4 5 6 7 2)" },
	{ "(convert 'hey 'hello '(hi sup hey hey hello hola))",
		"(hi sup hello hello hello hola)"                                           },
	{ "((compose - sqrt) 9)",                                   "-3"                    },
	{ "((compose / sqrt sqr) 18)",                              "1/18"                  },
	{ "((compose - sqrt cube) 4)",                              "-8"                    },
	{ "((compose -) 5/3)",                                      "-5/3"                  },
	{ "((compose - +) 5 6)",                                    "-11"                   },
	{ "((compose sqrt Int *) 4.5 2)",                           "3"                     },
	/* { "(foldr + 0 '(1 2 4 5))",                              "12"                    }, */
	/* { "(foldr list 0 '(1 2 3 4))",                           "((((0 1) 2) 3) 4)"     }, */

	{ "list filter",                                         NULL                  },
	{ "(filter positive? '(1 2 -4 5 -9 10))",                "(1 2 5 10)"          },
	{ "(filter odd? '(8 6 17 9 82 34 27))",                  "(17 9 27)"           },
	{ "(filter integer? (list 1/2 3.e-2 9/3 3.2 0.0 8 17))", "(3 8 17)"            },
	{ "(keep '+ '(+ * - - + + - / sqrt))",                   "(+ + +)"             },
	{ "(keep 5 (list (+ 1 2) (- 10 5) 3/2 5 (/ 15 5))))",    "(5 5)"               },
	{ "(keep 3.2 '(3. 3 3.02 3.12 3.20 3.7))",               "(3.2)"               },
	{ "(keep 'a '('a b c d e))",                             "Nil"                 },
	{ "(remove 1/2 (list 3/4 4/8 6/7 19/17 6/8 1/2))",       "(3/4 6/7 19/17 3/4)" },
	{ "(remove 2 '(1 3 4 5))",                               "(1 3 4 5)"           },
	{ "(remove \"greg\" '(greg 'greg \"wirt\" beatrice))",
		"(greg (quote greg) wirt beatrice)"                                    },

	{ "list mod",                                 NULL              },
	{ "(reverse '(1 2 3 4 5))",                   "(5 4 3 2 1)"     },
	{ "(reverse (list -20 5/2 .398))",            "(0.398 5/2 -20)" },
	{ "(reverse '(a b))",                         "(b a)"           },
	{ "(reverse (list \"foo\" \"bar\" \"baz\"))", "(baz bar foo)"   },
	{ "(reverse (cons 1/2 Nil))",                 "(1/2)"           },
	{ "(reverse ())",                             "Nil"             },
	{ "(append '(1 2 3) '(4 5 6))",               "(1 2 3 4 5 6)"   },
	{ "(append (list (+ 1 2) 4) '(a b c))",       "(3 4 a b c)"     },

	{ "assoc",                                                      NULL          },
	{ "(zip '(1 2 3 4) '(a b c d))",
		"((1 . a) (2 . b) (3 . c) (4 . d))"                                   },
	{ "(zip (list 'ricky 'lahey) (list \"julian\" \"randy\"))",
		"((ricky . julian) (lahey . randy))"                                  },
	{ "(assoc 'baz '((foo . 3) (bar . 8) (baz . 14)))",             "(baz . 14)"  },
	{ "(assoc 'a '((a b) (3 2.1) (3.2 4/3) (3.2 3.2)))",            "(a b)"       },
	{ "(assoc 3 '((1 b)))",                                         "Nil"         },
	{ "(assoc 4/3 (list (list 1 pi) (list 4/3 1/2 3) (list 2 3)))", "(4/3 1/2 3)" },

	{ "list member",                                    NULL                 },
	{ "(memp even? (list 1 3 19 4 7 8 2))",             "(4 7 8 2)"          },
	{ "(memp negative? (list 1/3 pi 3.2e-9 0 4 -7 2))", "(-7 2)"             },
	{ "(memp (Func (x) (> x 8)) '(1/3 1/2 5/3 8 9))",   "(9)"                },
	{ "(memp (Func (x) (= x \"fry\")) "
		"'(\"fry\" \"nibbler\" \"prof\"))",         "(fry nibbler prof)" },
	{ "(member 'foo '(foo bar baz))",                   "(foo bar baz)"      },
	{ "(member 'bar '(foo bar baz))",                   "(bar baz)"          },
	{ "(member 4 '(12 38 4 8))",                        "(4 8)"              },
	{ "(member 3.2 '(4/3 2 8 2 3.14 3.2))",             "(3.2)"              },
	{ "(member \"quux\" (list 4.2 3 'quux))",           "Nil"                },
	{ "(member 'qux '(foo bar baz))",                   "Nil"                },
	{ "(everyp? even? '(2 4 10 18))",                   "True"               },
	{ "(everyp? odd? '(1 2 3 9 10))",                   "Nil"                },
	{ "(everyp? integer? '(1. 2/3 3.14 4/5))",          "Nil"                },
	{ "(every? 'foo '(foo bar baz))",                   "Nil"                },
	{ "(every? \"a\" '(a 'a \"a\"))",                   "Nil"                },
	{ "(every? 3 (list 3 (+ 1 2) (- 5 2)))",            "True"               },

	{ "quasiquote",               NULL              },
	{ "`7.2",                     "7.2"             },
	{ "`cory",                    "cory"            },
	{ "`,foo",                    "8"               },
	{ "`(1 2 3)",                 "(1 2 3)"         },
	{ "`(\"sunnyvale\")",         "(sunnyvale)"     },
	{ "`(1/2 . 2/1)",             "(1/2 . 2)"       },
	{ "`(cory trevor)",           "(cory trevor)"   },
	{ "`(foo bar quax)",          "(foo bar quax)"  },
	{ "`(,foo ,bar)",             "(8 4)"           },
	{ "`(,foo . ,bar)",           "(8 . 4)"         },
	{ "`(,foo ,@bar)",            "(8 . 4)"         },
	{ "`(foo bar ,foo fry)",      "(foo bar 8 fry)" },
	{ "`(1 ,(+ 1 2) 5 ,(- 9 2))", "(1 3 5 7)"       },
	{ "`(1 ,@(list 4 9))",        "(1 4 9)"         },
	{ "`(3 ,@foo)",               "(3 . 8)"         },
	{ "`(a b c ,@foo)",           "(a b c . 8)"     },
	{ "`(0 ,@(list 1 2) 3 4)",    "(0 1 2 3 4)"     },

	{ "stack",                                   NULL                 },
	{ "(peek '(1 2 3 4 5 6))",                   "1"                  },
	{ "(peek (list 'a 'b 'c))",                  "a"                  },
	{ "(pop (list 1/2 1/4))",                    "(1/4)"              },
	{ "(pop '(\"foo\" \"bar\" \"baz\"))",        "(bar baz)"          },
	{ "(push '(6 3 5/3 .38) .5)",                "(0.5 6 3 5/3 0.38)" },
	{ "(push (list \"ni\" 'shrubbery) (* 3 2))", "(6 ni shrubbery)"   },
	{ "(swap '(1 2 3 5 7 11))",                  "(2 1 3 5 7 11)"     },
	{ "(swap (list 1/2 1/4 1/9 1/16))",          "(1/4 1/2 1/9 1/16)" },

	{ "numbers",            NULL   },
	{ "(Dec 1/2)",          "0.5"  },
	{ "(Dec 3/-2)",         "-1.5" },
	{ "(Dec 1)",            "1.0"  },
	{ "(Dec 3.14)",         "3.14" },
	{ "(Int 1/2)",          "0"    },
	{ "(Int 3/-2)",         "-1"   },
	{ "(Int 1)",            "1"    },
	{ "(Int 3.14)",         "3"    },
	{ "(numerator 3)",       "3"   },
	{ "(numerator 9/2)",     "9"   },
	{ "(numerator 9/15)",    "3"   },
	{ "(denominator 83)",    "1"   },
	{ "(denominator 3/2)",   "2"   },
	{ "(denominator 10/15)", "3"   },

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

	{ "compare",      NULL   },
	{ "(< 2 3)",      "True" },
	{ "(< 3 3)",      "Nil"  },
	{ "(< 4 3)",      "Nil"  },
	{ "(<= -2 +4)",   "True" },
	{ "(<= -2 -2)",   "True" },
	{ "(<= 4 -2)",    "Nil"  },
	{ "(> 89 34)",    "True" },
	{ "(> 48 48)",    "Nil"  },
	{ "(> 98 183)",   "Nil"  },
	{ "(>= +4 -282)", "True" },
	{ "(>= 39 39)",   "True" },
	{ "(>= -32 -30)", "Nil"  },

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

	/* TODO eval expect as well, then compare values? and compare printed strings */
	if (!(st->file = strdup(input)))
		return 0;
	st->filec = 0;
	if (!(v = tisp_read(st)))
		return 0;
	if (!(v = tisp_eval(st, st->env, v))) {
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
	clock_t t;
	Tsp st = tisp_env_init(1024);
	tib_env_core(st);
	tib_env_math(st);
	tib_env_string(st);
	tisp_env_lib(st, tibs);

	t = clock();
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
	t = clock() - t;
	printf("%-10s %d/%d\n", "total", correct, total);
	printf("%f ms\n", ((double)t)/CLOCKS_PER_SEC*100);

	return correct != total;
}
