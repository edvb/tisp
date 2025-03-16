/* zlib License
 *
 * Copyright (c) 2017-2025 Ed van Bruggen
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

typedef Val (*MkFn)(Tsp, char*);

/* TODO string tib: lower upper capitalize strpos strsub skipto snipto (python: dir(str))*/

/* convert all args to a string */
static Val
prim_Str(Tsp st, Rec env, Val args)
{
	char *s;
	if (!(s = tisp_print(args)))
		return NULL;
	return mk_str(st, s);
}

/* convert all args to a symbol */
static Val
prim_Sym(Tsp st, Rec env, Val args)
{
	char *s;
	if (!(s = tisp_print(args)))
		return NULL;
	return mk_sym(st, s);
}

static Val
prim_strlen(Tsp st, Rec env, Val args)
{
	tsp_arg_min(args, "strlen", 1);
	tsp_arg_type(car(args), "strlen", TSP_STR | TSP_SYM);
	return mk_int(strlen(car(args)->v.s));
}

/* perform interpolation on explicit string, evaluating anything inside curly braces */
/* FIXME nested strings shouldn't need to be escaped*/
static Val
form_strformat(Tsp st, Rec env, Val args)
{
	char *ret, *str;
	int ret_len, ret_cap, pos = 0;
	Val v;
	tsp_arg_num(args, "strformat", 1);
	tsp_arg_type(car(args), "strformat", TSP_STR);

	str = car(args)->v.s;
	ret_len = strlen(str), ret_cap = 2*ret_len;
	if (!(ret = malloc(sizeof(char) * ret_cap)))
		perror("; malloc"), exit(1);

	while (*str)
		if (*str == '{' && str[1] != '{') {
			int l;
			char *file = st->file;
			size_t filec = st->filec;
			char *s;
			st->file = ++str, st->filec = 0;
			/* TODO skip until } to allow for comments */
			if (!(v = read_pair(st, '}')))
				return NULL;
			str += st->filec;
			st->file = file, st->filec = filec;

			if (!(v = tisp_eval_list(st, env, v))) /* TODO sandboxed eval, no mutable procs */
				return NULL;
			if (!(s = tisp_print(v)))
				return NULL;
			/* TODO if last = !d run display converter on it */
			l = strlen(s);
			ret_len += l;

			if (ret_len >= ret_cap) /* resize output if necessary */
				if (!(ret = realloc(ret, ret_cap *= 2)))
					perror("; realloc"), exit(1);
			memcpy(ret + pos, s, l);
			free(s);
			pos += l;
		} else {
			if (*str == '{' || *str == '}') str++; /* only add 1 curly brace when escaped */
			ret[pos++] = *str++;
		}
	ret[pos] = '\0';
	return mk_str(st, ret);
}

void
tib_env_string(Tsp st)
{
	st->types[5]->v.t.func = mk_prim(TSP_PRIM, prim_Str, "Str");
	st->types[6]->v.t.func = mk_prim(TSP_PRIM, prim_Sym, "Sym");
	tsp_env_prim(strlen);
	tsp_env_form(strformat);
}
