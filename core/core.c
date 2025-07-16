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

/* return first element of list */
static Eevo
prim_fst(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "fst", 1);
	eevo_arg_type(fst(args), "fst", EEVO_PAIR);
	return ffst(args);
}

/* return elements of a list after the first */
static Eevo
prim_rst(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "rst", 1);
	eevo_arg_type(fst(args), "rst", EEVO_PAIR);
	return rfst(args);
}

/* return new pair */
static Eevo
prim_Pair(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "Pair", 2);
	return eevo_pair(fst(args), snd(args));
}

/* do not evaluate argument */
static Eevo
form_quote(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "quote", 1);
	return fst(args);
}

/* evaluate argument given */
static Eevo
prim_eval(EevoSt st, EevoRec env, Eevo args)
{
	Eevo v;
	eevo_arg_num(args, "eval", 1);
	return (v = eevo_eval(st, st->env, fst(args))) ? v : st->none;
}

/* test equality of all values given */
static Eevo
prim_eq(EevoSt st, EevoRec env, Eevo args)
{
	if (nilp(args))
		return st->t;
	for (; !nilp(rst(args)); args = rst(args))
		if (!vals_eq(fst(args), snd(args)))
			return st->nil;
	/* return nilp(fst(args)) ? st->t : fst(args); */
	return st->t;
}

/* evaluates and returns first expression with a true conditional */
static Eevo
form_cond(EevoSt st, EevoRec env, Eevo args)
{
	Eevo v, cond;
	for (v = args; !nilp(v); v = rst(v))
		if (!(cond = eevo_eval(st, env, ffst(v))))
			return NULL;
		else if (!nilp(cond)) /* TODO incorporate else directly into cond */
			return eevo_eval_body(st, env, rfst(v));
	return st->none;
}

/* return type of eevo value */
static Eevo
prim_typeof(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "typeof", 1);
	return eevo_str(st, eevo_type_str(fst(args)->t));
}

/* return record of properties for given procedure */
static Eevo
prim_procprops(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "procprops", 1);
	Eevo proc = fst(args);
	EevoRec ret = rec_new(6, NULL);
	switch (proc->t) {
	case EEVO_FORM:
	case EEVO_PRIM:
		rec_add(ret, "name", eevo_sym(st, proc->v.pr.name));
		break;
	case EEVO_FUNC:
	case EEVO_MACRO:
		rec_add(ret, "name", eevo_sym(st, proc->v.f.name ? proc->v.f.name : "anon"));
		rec_add(ret, "args", proc->v.f.args);
		rec_add(ret, "body", proc->v.f.body);
		/* rec_add(ret, "env", proc->v.f.env); */
		break;
	default:
		eevo_warnf("procprops: expected Proc, received '%s'", eevo_type_str(proc->t));
	}
	return eevo_rec(st, ret, NULL);
}

/* creates new eevo function */
static Eevo
form_Func(EevoSt st, EevoRec env, Eevo args)
{
	Eevo params, body;
	eevo_arg_min(args, "Func", 1);
	if (nilp(rst(args))) { /* if only 1 argument is given, auto fill func parameters */
		params = eevo_pair(eevo_sym(st, "it"), st->nil);
		body = args;
	} else {
		params = fst(args);
		body = rst(args);
	}
	return eevo_func(EEVO_FUNC, NULL, params, body, env);
}

/* creates new eevo defined macro */
static Eevo
form_Macro(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_min(args, "Macro", 1);
	Eevo ret = form_Func(st, env, args);
	ret->t = EEVO_MACRO;
	return ret;
}

/* display message and return error */
static Eevo
prim_error(EevoSt st, EevoRec env, Eevo args)
{
	char *msg;
	/* TODO have error auto print function name that was pre-defined */
	eevo_arg_min(args, "error", 2);
	eevo_arg_type(fst(args), "error", EEVO_SYM);
	if (!(msg = eevo_print(rst(args))))
		return NULL;
	fprintf(stderr, "; eevo: error: %s: %s\n", fst(args)->v.s, msg);
	free(msg);
	return NULL;
}

/** Records **/

/* merge second record into first record, without mutation */
static Eevo
prim_recmerge(EevoSt st, EevoRec env, Eevo args)
{
	Eevo ret = eevo_val(EEVO_REC);
	eevo_arg_num(args, "recmerge", 2);
	eevo_arg_type(fst(args), "recmerge", EEVO_REC);
	eevo_arg_type(snd(args), "recmerge", EEVO_REC);
	ret->v.r = rec_new(snd(args)->v.r->size*EEVO_REC_FACTOR, fst(args)->v.r);
	for (EevoRec r = snd(args)->v.r; r; r = r->next)
		for (int i = 0, c = 0; c < r->size; i++)
			if (r->items[i].key)
				c++, rec_add(ret->v.r, r->items[i].key, r->items[i].val);
	return ret;
}

/* retrieve list of every entry in given record */
static Eevo
prim_records(EevoSt st, EevoRec env, Eevo args)
{
	Eevo ret = st->nil;
	eevo_arg_num(args, "records", 1);
	eevo_arg_type(fst(args), "records", EEVO_REC);
	for (EevoRec r = fst(args)->v.r; r; r = r->next)
		for (int i = 0, c = 0; c < r->size; i++)
			if (r->items[i].key) {
				Eevo entry = eevo_pair(eevo_sym(st, r->items[i].key),
				                               r->items[i].val);
				ret = eevo_pair(entry, ret);
				c++;
			}
	return ret;
}

/* creates new variable of given name and value
 * if pair is given as name of variable, creates function with the first value as the
 * function name and the rest as the function arguments */
/* TODO if var not func error if more than 2 args */
static Eevo
form_def(EevoSt st, EevoRec env, Eevo args)
{
	Eevo sym, val;
	eevo_arg_min(args, "def", 1);
	if (fst(args)->t == EEVO_PAIR) { /* create function if given argument list */
		sym = ffst(args); /* first element of argument list is function name */
		if (sym->t != EEVO_SYM)
			eevo_warnf("def: expected symbol for function name, received '%s'",
			          eevo_type_str(sym->t));
		val = eevo_func(EEVO_FUNC, sym->v.s, rfst(args), rst(args), env);
	} else if (fst(args)->t == EEVO_SYM) { /* create variable */
		sym = fst(args); /* if only symbol given, make it self evaluating */
		val = nilp(rst(args)) ? sym : eevo_eval(st, env, snd(args));
	} else eevo_warn("def: incorrect format, no variable name found");
	if (!val)
		return NULL;
	/* set procedure name if it was previously anonymous */
	if (val->t & (EEVO_FUNC|EEVO_MACRO) && !val->v.f.name)
		val->v.f.name = sym->v.s; /* TODO some bug here */
	rec_add(env, sym->v.s, val);
	return st->none;
}

/* TODO fix crashing if try to undefine builtin */
static Eevo
form_undefine(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_min(args, "undefine!", 1);
	eevo_arg_type(fst(args), "undefine!", EEVO_SYM);
	for (EevoRec r = env; r; r = r->next) {
		EevoEntry e = entry_get(r, fst(args)->v.s);
		if (e->key) {
			e->key = NULL;
			/* TODO eevo_free(e->val); */
			return st->none;
		}
	}
	eevo_warnf("undefine!: could not find symbol %s to undefine", fst(args)->v.s);
}

static Eevo
form_definedp(EevoSt st, EevoRec env, Eevo args)
{
	EevoEntry e = NULL;
	eevo_arg_min(args, "defined?", 1);
	eevo_arg_type(fst(args), "defined?", EEVO_SYM);
	for (EevoRec r = env; r; r = r->next) {
		e = entry_get(r, fst(args)->v.s);
		if (e->key)
			break;
	}
	return (e && e->key) ? st->t : st->nil;
}


void
eevo_env_core(EevoSt st)
{
	eevo_env_prim(fst);
	eevo_env_prim(rst);
	st->types[11]->v.t.func = eevo_prim(EEVO_PRIM, prim_Pair, "Pair");
	eevo_env_form(quote);
	eevo_env_prim(eval);
	eevo_env_name_prim(=, eq);
	eevo_env_form(cond);
	eevo_env_add(st, "do", eevo_prim(EEVO_FORM, eevo_eval_body, "do"));

	eevo_env_prim(typeof);
	eevo_env_prim(procprops);
	st->types[9]->v.t.func  = eevo_prim(EEVO_FORM, form_Func,  "Func");
	st->types[10]->v.t.func = eevo_prim(EEVO_FORM, form_Macro, "Macro");
	eevo_env_prim(error);

	eevo_env_prim(recmerge);
	eevo_env_prim(records);
	eevo_env_form(def);
	eevo_env_name_form(undefine!, undefine);
	eevo_env_name_form(defined?, definedp);
}
