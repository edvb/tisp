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

/* TODO NULL check allocs */
static Val
val_string(Tsp st, Val args, MkFn mk_fn)
{
	Val v;
	char s[43], *ret = calloc(1, sizeof(char));
	int len = 1;
	for (; args->t == TSP_PAIR; args = cdr(args)) {
		v = car(args);
		switch (v->t) {
		case TSP_NONE: break;
		case TSP_NIL:
			len += 4;
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, "Nil");
			break;
		case TSP_INT:
			snprintf(s, 21, "%d", (int)v->v.n.num);
			len += strlen(s);
			s[len] = '\0';
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, s);
			break;
		case TSP_DEC:
			snprintf(s, 22, "%.15g", v->v.n.num);
			len += strlen(s);
			s[len] = '\0';
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, s);
			break;
		case TSP_RATIO:
			snprintf(s, 43, "%d/%d", (int)v->v.n.num, (int)v->v.n.den);
			len += strlen(s);
			s[len] = '\0';
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, s);
			break;
		case TSP_STR:
		case TSP_SYM:
			len += strlen(v->v.s);
			ret = realloc(ret, len*sizeof(char));
			strcat(ret, v->v.s);
			break;
		case TSP_PAIR:
		default:
			tsp_warnf("could not convert type %s into string", tsp_type_str(v->t));
		}
	}
	v = mk_fn(st, ret);
	free(ret);
	return v;
}

/* convert all args to a string */
static Val
prim_Str(Tsp st, Rec env, Val args)
{
	tsp_arg_min(args, "Str", 1);
	return val_string(st, args, mk_str);
}

/* convert all args to a symbol */
static Val
prim_Sym(Tsp st, Rec env, Val args)
{
	tsp_arg_min(args, "Sym", 1);
	return val_string(st, args, mk_sym);
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
			st->file = ++str, st->filec = 0;
			/* TODO skip until } to allow for comments */
			if (!(v = read_pair(st, '}')))
				return NULL;
			str += st->filec;
			st->file = file, st->filec = filec;

			if (!(v = tisp_eval_list(st, env, v))) /* TODO sandboxed eval, no mutable procs */
				return NULL;
			if (!(v = val_string(st, v, mk_str)))
				return NULL;
			/* TODO if last = !d run display converter on it */
			l = strlen(v->v.s);
			ret_len += l;

			if (ret_len >= ret_cap) /* resize output if necessary */
				if (!(ret = realloc(ret, ret_cap *= 2)))
					perror("; realloc"), exit(1);
			memcpy(ret + pos, v->v.s, l);
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
	tsp_env_prim(Sym);
	tsp_env_prim(Str);
	tsp_env_prim(strlen);
	tsp_env_form(strformat);
}
