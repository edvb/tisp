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
prim_car(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "car", 1);
	eevo_arg_type(car(args), "car", EEVO_PAIR);
	return caar(args);
}

/* return elements of a list after the first */
static Eevo
prim_cdr(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "cdr", 1);
	eevo_arg_type(car(args), "cdr", EEVO_PAIR);
	return cdar(args);
}

/* return new pair */
static Eevo
prim_cons(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "cons", 2);
	return eevo_pair(car(args), cadr(args));
}

/* do not evaluate argument */
static Eevo
form_quote(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "quote", 1);
	return car(args);
}

/* evaluate argument given */
static Eevo
prim_eval(EevoSt st, EevoRec env, Eevo args)
{
	Eevo v;
	eevo_arg_num(args, "eval", 1);
	return (v = eevo_eval(st, st->env, car(args))) ? v : st->none;
}

/* test equality of all values given */
static Eevo
prim_eq(EevoSt st, EevoRec env, Eevo args)
{
	if (nilp(args))
		return st->t;
	for (; !nilp(cdr(args)); args = cdr(args))
		if (!vals_eq(car(args), cadr(args)))
			return st->nil;
	/* return nilp(car(args)) ? st->t : car(args); */
	return st->t;
}

/* evaluates and returns first expression with a true conditional */
static Eevo
form_cond(EevoSt st, EevoRec env, Eevo args)
{
	Eevo v, cond;
	for (v = args; !nilp(v); v = cdr(v))
		if (!(cond = eevo_eval(st, env, caar(v))))
			return NULL;
		else if (!nilp(cond)) /* TODO incorporate else directly into cond */
			return eevo_eval_body(st, env, cdar(v));
	return st->none;
}

/* return type of eevo value */
static Eevo
prim_typeof(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "typeof", 1);
	return eevo_str(st, eevo_type_str(car(args)->t));
}

/* return record of properties for given procedure */
static Eevo
prim_procprops(EevoSt st, EevoRec env, Eevo args)
{
	eevo_arg_num(args, "procprops", 1);
	Eevo proc = car(args);
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
	if (nilp(cdr(args))) { /* if only 1 argument is given, auto fill func parameters */
		params = eevo_pair(eevo_sym(st, "it"), st->nil);
		body = args;
	} else {
		params = car(args);
		body = cdr(args);
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
	eevo_arg_type(car(args), "error", EEVO_SYM);
	if (!(msg = eevo_print(cdr(args))))
		return NULL;
	fprintf(stderr, "; eevo: error: %s: %s\n", car(args)->v.s, msg);
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
	eevo_arg_type(car(args), "recmerge", EEVO_REC);
	eevo_arg_type(cadr(args), "recmerge", EEVO_REC);
	ret->v.r = rec_new(cadr(args)->v.r->size*EEVO_REC_FACTOR, car(args)->v.r);
	for (EevoRec r = cadr(args)->v.r; r; r = r->next)
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
	eevo_arg_type(car(args), "records", EEVO_REC);
	for (EevoRec r = car(args)->v.r; r; r = r->next)
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
 * if pair is given as name of variable, creates function with the car as the
 * function name and the cdr the function arguments */
/* TODO if var not func error if more than 2 args */
static Eevo
form_def(EevoSt st, EevoRec env, Eevo args)
{
	Eevo sym, val;
	eevo_arg_min(args, "def", 1);
	if (car(args)->t == EEVO_PAIR) { /* create function if given argument list */
		sym = caar(args); /* first element of argument list is function name */
		if (sym->t != EEVO_SYM)
			eevo_warnf("def: expected symbol for function name, received '%s'",
			          eevo_type_str(sym->t));
		val = eevo_func(EEVO_FUNC, sym->v.s, cdar(args), cdr(args), env);
	} else if (car(args)->t == EEVO_SYM) { /* create variable */
		sym = car(args); /* if only symbol given, make it self evaluating */
		val = nilp(cdr(args)) ? sym : eevo_eval(st, env, cadr(args));
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
	eevo_arg_type(car(args), "undefine!", EEVO_SYM);
	for (EevoRec r = env; r; r = r->next) {
		EevoEntry e = entry_get(r, car(args)->v.s);
		if (e->key) {
			e->key = NULL;
			/* TODO eevo_free(e->val); */
			return st->none;
		}
	}
	eevo_warnf("undefine!: could not find symbol %s to undefine", car(args)->v.s);
}

static Eevo
form_definedp(EevoSt st, EevoRec env, Eevo args)
{
	EevoEntry e = NULL;
	eevo_arg_min(args, "defined?", 1);
	eevo_arg_type(car(args), "defined?", EEVO_SYM);
	for (EevoRec r = env; r; r = r->next) {
		e = entry_get(r, car(args)->v.s);
		if (e->key)
			break;
	}
	return (e && e->key) ? st->t : st->nil;
}


void
eevo_env_core(EevoSt st)
{
	eevo_env_prim(car);
	eevo_env_prim(cdr);
	eevo_env_prim(cons);
	st->types[11]->v.t.func = eevo_prim(EEVO_PRIM, prim_cons, "Pair");
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
