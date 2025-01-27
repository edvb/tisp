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
static Val
prim_car(Tsp st, Rec env, Val args)
{
	tsp_arg_num(args, "car", 1);
	tsp_arg_type(car(args), "car", TSP_PAIR);
	return caar(args);
}

/* return elements of a list after the first */
static Val
prim_cdr(Tsp st, Rec env, Val args)
{
	tsp_arg_num(args, "cdr", 1);
	tsp_arg_type(car(args), "cdr", TSP_PAIR);
	return cdar(args);
}

/* return new pair */
static Val
prim_cons(Tsp st, Rec env, Val args)
{
	tsp_arg_num(args, "cons", 2);
	return mk_pair(car(args), cadr(args));
}

/* do not evaluate argument */
static Val
form_quote(Tsp st, Rec env, Val args)
{
	tsp_arg_num(args, "quote", 1);
	return car(args);
}

/* evaluate argument given */
static Val
prim_eval(Tsp st, Rec env, Val args)
{
	Val v;
	tsp_arg_num(args, "eval", 1);
	return (v = tisp_eval(st, st->env, car(args))) ? v : st->none;
}

/* test equality of all values given */
static Val
prim_eq(Tsp st, Rec env, Val args)
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
static Val
form_cond(Tsp st, Rec env, Val args)
{
	Val v, cond;
	for (v = args; !nilp(v); v = cdr(v))
		if (!(cond = tisp_eval(st, env, caar(v))))
			return NULL;
		else if (!nilp(cond)) /* TODO incorporate else directly into cond */
			return tisp_eval_body(st, env, cdar(v));
	return st->none;
}

/* return type of tisp value */
static Val
prim_typeof(Tsp st, Rec env, Val args)
{
	tsp_arg_num(args, "typeof", 1);
	return mk_str(st, tsp_type_str(car(args)->t));
}

/* return record of properties for given procedure */
static Val
prim_procprops(Tsp st, Rec env, Val args)
{
	tsp_arg_num(args, "procprops", 1);
	Val proc = car(args);
	Rec ret = rec_new(6, NULL);
	switch (proc->t) {
	case TSP_FORM:
	case TSP_PRIM:
		rec_add(ret, "name", mk_sym(st, proc->v.pr.name));
		break;
	case TSP_FUNC:
	case TSP_MACRO:
		rec_add(ret, "name", mk_sym(st, proc->v.f.name ? proc->v.f.name : "anon"));
		rec_add(ret, "args", proc->v.f.args);
		rec_add(ret, "body", proc->v.f.body);
		/* rec_add(ret, "env", proc->v.f.env); */
		break;
	default:
		tsp_warnf("procprops: expected Proc, received '%s'", tsp_type_str(proc->t));
	}
	return mk_rec(st, ret, NULL);
}

/* creates new tisp function */
static Val
form_Func(Tsp st, Rec env, Val args)
{
	Val params, body;
	tsp_arg_min(args, "Func", 1);
	if (nilp(cdr(args))) { /* if only 1 argument is given, auto fill func parameters */
		params = mk_pair(mk_sym(st, "it"), st->nil);
		body = args;
	} else {
		params = car(args);
		body = cdr(args);
	}
	return mk_func(TSP_FUNC, NULL, params, body, env);
}

/* creates new tisp defined macro */
static Val
form_Macro(Tsp st, Rec env, Val args)
{
	tsp_arg_min(args, "Macro", 1);
	Val ret = form_Func(st, env, args);
	ret->t = TSP_MACRO;
	return ret;
}

/* display message and return error */
static Val
prim_error(Tsp st, Rec env, Val args)
{
	/* TODO have error auto print function name that was pre-defined */
	tsp_arg_min(args, "error", 2);
	tsp_arg_type(car(args), "error", TSP_SYM);
	/* TODO specify error raised by error func */
	fprintf(stderr, "; tisp: error: %s: ", car(args)->v.s);
	for (args = cdr(args); !nilp(args); args = cdr(args))
		tisp_print(stderr, car(args));
	fputc('\n', stderr);
	return NULL;
}

/** Records **/

/* merge second record into first record, without mutation */
static Val
prim_recmerge(Tsp st, Rec env, Val args)
{
	Val ret = mk_val(TSP_REC);
	tsp_arg_num(args, "recmerge", 2);
	tsp_arg_type(car(args), "recmerge", TSP_REC);
	tsp_arg_type(cadr(args), "recmerge", TSP_REC);
	ret->v.r = rec_new(cadr(args)->v.r->size*TSP_REC_FACTOR, car(args)->v.r);
	for (Rec r = cadr(args)->v.r; r; r = r->next)
		for (int i = 0, c = 0; c < r->size; i++)
			if (r->items[i].key)
				c++, rec_add(ret->v.r, r->items[i].key, r->items[i].val);
	return ret;
}

/* retrieve list of every entry in given record */
static Val
prim_records(Tsp st, Rec env, Val args)
{
	Val ret = st->nil;
	tsp_arg_num(args, "records", 1);
	tsp_arg_type(car(args), "records", TSP_REC);
	for (Rec r = car(args)->v.r; r; r = r->next)
		for (int i = 0, c = 0; c < r->size; i++)
			if (r->items[i].key) {
				Val entry = mk_pair(mk_sym(st, r->items[i].key),
				                               r->items[i].val);
				ret = mk_pair(entry, ret);
				c++;
			}
	return ret;
}

/* creates new variable of given name and value
 * if pair is given as name of variable, creates function with the car as the
 * function name and the cdr the function arguments */
/* TODO if var not func error if more than 2 args */
static Val
form_def(Tsp st, Rec env, Val args)
{
	Val sym, val;
	tsp_arg_min(args, "def", 1);
	if (car(args)->t == TSP_PAIR) { /* create function if given argument list */
		sym = caar(args); /* first element of argument list is function name */
		if (sym->t != TSP_SYM)
			tsp_warnf("def: expected symbol for function name, received '%s'",
			          tsp_type_str(sym->t));
		val = mk_func(TSP_FUNC, sym->v.s, cdar(args), cdr(args), env);
	} else if (car(args)->t == TSP_SYM) { /* create variable */
		sym = car(args); /* if only symbol given, make it self evaluating */
		val = nilp(cdr(args)) ? sym : tisp_eval(st, env, cadr(args));
	} else tsp_warn("def: incorrect format, no variable name found");
	if (!val)
		return NULL;
	/* set procedure name if it was previously anonymous */
	if (val->t & (TSP_FUNC|TSP_MACRO) && !val->v.f.name)
		val->v.f.name = sym->v.s; /* TODO some bug here */
	rec_add(env, sym->v.s, val);
	return st->none;
}

/* TODO fix crashing if try to undefine builtin */
static Val
form_undefine(Tsp st, Rec env, Val args)
{
	tsp_arg_min(args, "undefine!", 1);
	tsp_arg_type(car(args), "undefine!", TSP_SYM);
	for (Rec r = env; r; r = r->next) {
		Entry e = entry_get(r, car(args)->v.s);
		if (e->key) {
			e->key = NULL;
			/* TODO tsp_free(e->val); */
			return st->none;
		}
	}
	tsp_warnf("undefine!: could not find symbol %s to undefine", car(args)->v.s);
}

static Val
form_definedp(Tsp st, Rec env, Val args)
{
	Entry e = NULL;
	tsp_arg_min(args, "defined?", 1);
	tsp_arg_type(car(args), "defined?", TSP_SYM);
	for (Rec r = env; r; r = r->next) {
		e = entry_get(r, car(args)->v.s);
		if (e->key)
			break;
	}
	return (e && e->key) ? st->t : st->nil;
}


void
tib_env_core(Tsp st)
{
	tsp_env_prim(car);
	tsp_env_prim(cdr);
	tsp_env_prim(cons);
	tsp_env_form(quote);
	tsp_env_prim(eval);
	tsp_env_name_prim(=, eq);
	tsp_env_form(cond);
	tisp_env_add(st, "do", mk_prim(TSP_FORM, tisp_eval_body, "do"));

	tsp_env_prim(typeof);
	tsp_env_prim(procprops);
	tsp_env_form(Func);
	tsp_env_form(Macro);
	tsp_env_prim(error);

	tisp_env_add(st, "Rec", mk_prim(TSP_FORM, mk_rec, "Rec"));
	tsp_env_prim(recmerge);
	tsp_env_prim(records);
	tsp_env_form(def);
	tsp_env_name_form(undefine!, undefine);
	tsp_env_name_form(defined?, definedp);
}
