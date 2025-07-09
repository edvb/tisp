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

typedef Eevo (*MkFn)(EevoSt, char*);

/* TODO string tib: lower upper capitalize strpos strsub skipto snipto (python: dir(str))*/

/* convert all args to a string */
static Eevo
prim_Str(EevoSt st, EevoRec env, Eevo args)
{
	char *s;
	if (!(s = eevo_print(args)))
		return NULL;
	return eevo_str(st, s);
}

/* convert all args to a symbol */
static Eevo
prim_Sym(EevoSt st, EevoRec env, Eevo args)
{
	char *s;
	if (!(s = eevo_print(args)))
		return NULL;
	return eevo_sym(st, s);
}

static Eevo
prim_strlen(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_min(args, "strlen", 1);
	eevo_arg_type(fst(args), "strlen", EEVO_STR | EEVO_SYM);
	return eevo_int(strlen(fst(args)->v.s));
}

/* perform interpolation on explicit string, evaluating anything inside curly braces */
/* FIXME nested strings shouldn't need to be escaped*/
static Eevo
form_strfmt(EevoSt st, EevoRec env, Eevo args)
{
	char *ret, *str;
	int ret_len, ret_cap, pos = 0;
	Eevo v;
	eevo_arg_num(args, "strfmt", 1);
	eevo_arg_type(fst(args), "strfmt", EEVO_STR);

	str = fst(args)->v.s;
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

			if (!(v = eevo_eval_list(st, env, v))) /* TODO sandboxed eval, no mutable procs */
				return NULL;
			if (!(s = eevo_print(v)))
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
	return eevo_str(st, ret);
}

void
eevo_env_string(EevoSt st)
{
	st->types[5]->v.t.func = eevo_prim(EEVO_PRIM, prim_Str, "Str");
	st->types[6]->v.t.func = eevo_prim(EEVO_PRIM, prim_Sym, "Sym");
	eevo_env_prim(strlen);
	eevo_env_form(strfmt);
}
