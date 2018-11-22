/* See LICENSE file for copyright and license details. */
#include <stdio.h>

#include "../tisp.h"

#define warnf(M, ...) do {                          \
	fprintf(stderr, "tisp:%d: error: " M "\n",  \
	                  __LINE__, ##__VA_ARGS__); \
	return NULL;                                \
} while(0)
#define warn(M) do {                                \
	fprintf(stderr, "tisp:%d: error: " M "\n",  \
	                 __LINE__);                 \
	return NULL;                                \
} while(0)


#define PRIM_OP(NAME, OP, FUNC)                                                        \
static Val                                                                             \
prim_##NAME(Env env, Val args)                                                         \
{                                                                                      \
	Val a, b;                                                                      \
	int len = list_len(args);                                                      \
	if (len != 2)                                                                  \
		warnf(FUNC ": expected 2 arguments, recieved %d", len);                \
	if (!(a = tisp_eval(env, car(args))) || !(b = tisp_eval(env, car(cdr(args))))) \
		return NULL;                                                           \
	if (a->t != INTEGER)                                                           \
		warnf(FUNC ": expected integer, recieved type [%s]", type_str(a->t));  \
	if (b->t != INTEGER)                                                           \
		warnf(FUNC ": expected integer, recieved type [%s]", type_str(b->t));  \
	return mk_int(a->v.i OP b->v.i);                                               \
}

PRIM_OP(add, +, "+")
PRIM_OP(mul, *, "*")
PRIM_OP(mod, %, "mod")

static Val
prim_sub(Env env, Val args)
{
	Val a, b;
	int len = list_len(args);
	if (len != 2 && len != 1)
		warnf("-: expected 1 or 2 arguments, recieved %d", len);
	if (!(a = tisp_eval(env, car(args))))
		return NULL;
	if (a->t != INTEGER)
		warnf("-: expected integer, recieved type [%s]", type_str(a->t));
	if (len == 1)
		return mk_int(-a->v.i);
	if (!(b = tisp_eval(env, car(cdr(args)))))
		return NULL;
	if (b->t != INTEGER)
		warnf("-: expected integer, recieved type [%s]", type_str(b->t));
	return mk_int(a->v.i - b->v.i);
}

static Val
prim_div(Env env, Val args)
{
	Val a, b;
	int len = list_len(args);
	if (len != 2)
		warnf("/: expected 2 arguments, recieved %d", len);
	if (!(a = tisp_eval(env, car(args))) || !(b = tisp_eval(env, car(cdr(args)))))
		return NULL;
	if (a->t != INTEGER)
		warnf("/: expected integer, recieved type [%s]", type_str(a->t));
	if (b->t != INTEGER)
		warnf("/: expected integer, recieved type [%s]", type_str(b->t));
	return mk_rat(a->v.i, b->v.i);
}

#define INT_TEST(V, FUNC) do {                                                        \
	if (V->t != INTEGER)                                                          \
		warnf(FUNC ": expected integer, recieved type [%s]", type_str(V->t)); \
} while (0)

#define PRIM_COMPARE(NAME, OP, FUNC)                                  \
static Val                                                            \
prim_##NAME(Env env, Val args)                                        \
{                                                                     \
	Val v;                                                        \
	if (!(v = tisp_eval_list(env, args)))                         \
		return NULL;                                          \
	if (list_len(v) != 2)                                         \
		return env->t;                                        \
	INT_TEST(car(v), FUNC);                                       \
	INT_TEST(car(cdr(v)), FUNC);                                  \
	return (car(v)->v.i OP car(cdr(v))->v.i) ? env->t : env->nil; \
}

PRIM_COMPARE(lt,  <,  "<")
PRIM_COMPARE(gt,  >,  ">")
PRIM_COMPARE(lte, <=, "<=")
PRIM_COMPARE(gte, >=, ">=")

void
tib_env_math(Env env)
{
	tisp_env_add(env, "+",   mk_prim(prim_add));
	tisp_env_add(env, "-",   mk_prim(prim_sub));
	tisp_env_add(env, "*",   mk_prim(prim_mul));
	tisp_env_add(env, "/",   mk_prim(prim_div));
	tisp_env_add(env, "mod", mk_prim(prim_mod));
	tisp_env_add(env, "<",   mk_prim(prim_lt));
	tisp_env_add(env, ">",   mk_prim(prim_gt));
	tisp_env_add(env, "<=",  mk_prim(prim_lte));
	tisp_env_add(env, ">=",  mk_prim(prim_gte));
}
