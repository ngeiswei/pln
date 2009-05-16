/*
 * SchemeEval.cc
 *
 * Simple scheme evaluator
 * Copyright (c) 2008 Linas Vepstas <linas@linas.org>
 */

#ifdef HAVE_GUILE

#include <libguile.h>
#include <libguile/backtrace.h>
#include <libguile/debug.h>
#include <libguile/lang.h>
#include <pthread.h>

#include <opencog/util/Logger.h>

#include "SchemeEval.h"
#include "SchemeSmob.h"

using namespace opencog;

/**
 * This init is called once for every time that this class
 * is instantiated -- i.e. it is a per-instance initializer.
 */
void SchemeEval::init(void)
{
	SchemeSmob::init();

	saved_outport = scm_current_output_port();
	// scm_gc_protect_object(saved_outport);

	outport = scm_open_output_string();
	// outport = scm_gc_protect_object(outport);
	scm_set_current_output_port(outport);

	pending_input = false;
	caught_error = false;
	input_line = "";
	error_string_port = SCM_EOL;
	captured_stack = SCM_BOOL_F;
	pexpr = NULL;
}

void * SchemeEval::c_wrap_init(void *p)
{
	SchemeEval *self = (SchemeEval *) p;
	self->init();
	return self;
}

void SchemeEval::finish(void)
{
	// Restore the previous outport.
	// XXX This works only if the evaluator was run nested;
	// it bombs, if run in parallel, because the saved port
	// might have been closed.
	scm_set_current_output_port(saved_outport);
	// scm_gc_unprotect_object(saved_outport);

	scm_close_port(outport);
	// scm_gc_unprotect_object(outport);
}

void * SchemeEval::c_wrap_finish(void *p)
{
	SchemeEval *self = (SchemeEval *) p;
	self->finish();
	return self;
}

static pthread_once_t eval_init_once = PTHREAD_ONCE_INIT;
static pthread_key_t tid_key = NULL;

#define WORK_AROUND_GUILE_185_BUG
#ifdef WORK_AROUND_GUILE_185_BUG
/* There's a bug in guile-1.8.5, where the second and subsequent
 * threads run in guile mode with a bogus/broken current-module.
 * This cannot be worked around by anything as simple as saying 
 * "(set-current-module the-root-module)" because dynwind undoes
 * any module-setting that we do.
 *
 * So we work around it here, by explicitly setting the module
 * outside of a dynwind context.
 */
static SCM guile_user_module;

static void * do_bogus_scm(void *p)
{
	scm_c_eval_string ("(+ 2 2)\n");
	return p;
}
#endif /* WORK_AROUND_GUILE_185_BUG */

#define WORK_AROUND_GUILE_THREADING_BUG
#ifdef WORK_AROUND_GUILE_THREADING_BUG
/* There are bugs in guile-1.8.6 and earlier that prevent proper
 * multi-threaded operation. Currently, the most serious of these is
 * a prallel-define bug, documented in 
 * https://savannah.gnu.org/bugs/index.php?24867
 * Until that bug is fixed and released, this work-around is needed.
 * The work-around serializes all guile-mode thread execution, by 
 * means of a mutex lock.
 */
static pthread_mutex_t serialize_lock;
static pthread_key_t ser_key = NULL;
#endif /* WORK_AROUND_GUILE_THREADING_BUG */

static void first_time_only(void)
{
   pthread_key_create(&tid_key, NULL);
	pthread_setspecific(tid_key, (const void *) 0x42);
#ifdef WORK_AROUND_GUILE_THREADING_BUG
	pthread_mutex_init(&serialize_lock, NULL);
	pthread_key_create(&ser_key, NULL);
	pthread_setspecific(ser_key, (const void *) 0x0);
#endif /* WORK_AROUND_GUILE_THREADING_BUG */
#ifdef WORK_AROUND_GUILE_185_BUG
	scm_with_guile(do_bogus_scm, NULL);
	guile_user_module = scm_current_module();
#endif /* WORK_AROUND_GUILE_185_BUG */
}

#ifdef WORK_AROUND_GUILE_THREADING_BUG

/** 
 * This lock primitive allow nested locks within one thread,
 * but prevents concurrent threads from running.
 */
void SchemeEval::thread_lock(void)
{
	long cnt = (long) pthread_getspecific(ser_key);
	if (0 >= cnt)
	{
		pthread_mutex_lock(&serialize_lock);
	}
	cnt ++;
	pthread_setspecific(ser_key, (const void *) cnt);
}

void SchemeEval::thread_unlock(void)
{
	long cnt = (long) pthread_getspecific(ser_key);
	cnt --;
	pthread_setspecific(ser_key, (const void *) cnt);
	if (0 >= cnt)
	{
		pthread_mutex_unlock(&serialize_lock);
	}
}
#endif

SchemeEval::SchemeEval(void)
{
	pthread_once(&eval_init_once, first_time_only);

#ifdef WORK_AROUND_GUILE_THREADING_BUG
	thread_lock();
#endif /* WORK_AROUND_GUILE_THREADING_BUG */

	scm_with_guile(c_wrap_init, this);

#ifdef WORK_AROUND_GUILE_THREADING_BUG
	thread_unlock();
#endif /* WORK_AROUND_GUILE_THREADING_BUG */

}

/* This should be called once for every new thread. */
void SchemeEval::per_thread_init(void)
{
	/* Avoid more than one call per thread. */
	if (((void *) 0x2) == pthread_getspecific(tid_key)) return;
	pthread_setspecific(tid_key, (const void *) 0x2);

#ifdef WORK_AROUND_GUILE_185_BUG
	scm_set_current_module(guile_user_module);
#endif /* WORK_AROUND_GUILE_185_BUG */

	// Guile implements the current port as a fluid on each thread.
	// So, for every new thread, we need to set this.  
	scm_set_current_output_port(outport);
}

SchemeEval::~SchemeEval()
{
	scm_with_guile(c_wrap_finish, this);
}

/* ============================================================== */

std::string SchemeEval::prt(SCM node)
{
	if (SCM_SMOB_PREDICATE(SchemeSmob::cog_handle_tag, node))
	{
		return SchemeSmob::handle_to_string(node);
	}

	else if (SCM_SMOB_PREDICATE(SchemeSmob::cog_misc_tag, node))
	{
		return SchemeSmob::misc_to_string(node);
	}

	else if (scm_is_eq(node, SCM_UNSPECIFIED))
	{
		return "";
	}
#if CUSTOM_PRINTING_WHY_DO_WE_HAVE_THIS_DELETE_ME
	else if (scm_is_pair(node))
	{
		std::string str = "(";
      SCM node_list = node;
		const char * sp = "";
      do
      {
			str += sp;
			sp = " ";
         node = SCM_CAR (node_list);
         str += prt (node);
         node_list = SCM_CDR (node_list);
      }
      while (scm_is_pair(node_list));

		// Print the rest -- the CDR part
		if (!scm_is_null(node_list)) 
		{
			str += " . ";
			str += prt (node_list);
		}
		str += ")";
		return str;
   }
	else if (scm_is_true(scm_symbol_p(node))) 
	{
		node = scm_symbol_to_string(node);
		char * str = scm_to_locale_string(node);
		// std::string rv = "'";  // print the symbol escape
		std::string rv = "";      // err .. don't print it
		rv += str;
		free(str);
		return rv;
	}
	else if (scm_is_true(scm_string_p(node))) 
	{
		char * str = scm_to_locale_string(node);
		std::string rv = "\"";
		rv += str;
		rv += "\"";
		free(str);
		return rv;
	}
	else if (scm_is_number(node)) 
	{
		#define NUMBUFSZ 60
		char buff[NUMBUFSZ];
		if (scm_is_signed_integer(node, INT_MIN, INT_MAX))
		{
			snprintf (buff, NUMBUFSZ, "%ld", (long) scm_to_long(node));
		}
		else if (scm_is_unsigned_integer(node, 0, UINT_MAX))
		{
			snprintf (buff, NUMBUFSZ, "%lu", (unsigned long) scm_to_ulong(node));
		}
		else if (scm_is_real(node))
		{
			snprintf (buff, NUMBUFSZ, "%g", scm_to_double(node));
		}
		else if (scm_is_complex(node))
		{
			snprintf (buff, NUMBUFSZ, "%g +i %g", 
				scm_c_real_part(node),
				scm_c_imag_part(node));
		}
		else if (scm_is_rational(node))
		{
			std::string rv;
			rv = prt(scm_numerator(node));
			rv += "/";
			rv += prt(scm_denominator(node));
			return rv;
		}
		return buff;
	}
	else if (scm_is_true(scm_char_p(node))) 
	{
		std::string rv;
		rv = (char) scm_to_char(node);
		return rv;
	}
	else if (scm_is_true(scm_boolean_p(node))) 
	{
		if (scm_to_bool(node)) return "#t";
		return "#f";
	}
	else if (SCM_NULL_OR_NIL_P(node)) 
	{
		// scm_is_null(x) is true when x is SCM_EOL
		// SCM_NILP(x) is true when x is SCM_ELISP_NIL
		return "()";
	}
	else if (scm_is_eq(node, SCM_UNDEFINED))
	{
		return "undefined";
	}
	else if (scm_is_eq(node, SCM_EOF_VAL))
	{
		return "eof";
	}
#endif
	else
	{
		// Let SCM display do the rest of the work.
		SCM port = scm_open_output_string();
		scm_display (node, port);
		SCM rc = scm_get_output_string(port);
		char * str = scm_to_locale_string(rc);
		std::string rv = str;
		free(str);
		scm_close_port(port);
		return rv;
	}

	return "";
}

/* ============================================================== */

SCM SchemeEval::preunwind_handler_wrapper (void *data, SCM tag, SCM throw_args)
{
	SchemeEval *ss = (SchemeEval *)data;
	return ss->preunwind_handler(tag, throw_args);
	return SCM_EOL;
}

SCM SchemeEval::catch_handler_wrapper (void *data, SCM tag, SCM throw_args)
{
	SchemeEval *ss = (SchemeEval *)data;
	return ss->catch_handler(tag, throw_args);
}

SCM SchemeEval::preunwind_handler (SCM tag, SCM throw_args)
{
	// We can only record the stack before it is unwound. 
	// The normal catch handler body runs only *after* the stack
	// has been unwound.
	captured_stack = scm_make_stack (SCM_BOOL_T, SCM_EOL);
	return SCM_EOL;
}

SCM SchemeEval::catch_handler (SCM tag, SCM throw_args)
{
	// Check for read error. If a read error, then wait for user to correct it.
	SCM re = scm_symbol_to_string(tag);
	char * restr = scm_to_locale_string(re);
	pending_input = false;
	if (0 == strcmp(restr, "read-error"))
	{
		pending_input = true;
		free(restr);
		return SCM_EOL;
	}

	// If its not a read error, then its a regular error; report it.
	caught_error = true;

	/* get string port into which we write the error message and stack. */
	error_string_port = scm_open_output_string();
	SCM port = error_string_port;

	if (scm_is_true(scm_list_p(throw_args)) && (scm_ilength(throw_args) >= 1))
	{
		long nargs = scm_ilength(throw_args);
		SCM subr    = SCM_CAR (throw_args);
		SCM message = SCM_EOL;
		if (nargs >= 2)
			message = SCM_CADR (throw_args);
		SCM parts   = SCM_EOL;
		if (nargs >= 3)
			parts   = SCM_CADDR (throw_args);
		SCM rest    = SCM_EOL;
		if (nargs >= 4)
			rest    = SCM_CADDDR (throw_args);

		if (scm_is_true (captured_stack))
		{
			SCM highlights;

			if (scm_is_eq (tag, scm_arg_type_key) ||
			    scm_is_eq (tag, scm_out_of_range_key))
				highlights = rest;
			else
				highlights = SCM_EOL;

			scm_puts ("Backtrace:\n", port);
			scm_display_backtrace_with_highlights (captured_stack, port,
			      SCM_BOOL_F, SCM_BOOL_F, highlights);
			scm_newline (port);
		}
		scm_display_error (captured_stack, port, subr, message, parts, rest);
	}
	else
	{
		scm_puts ("ERROR: thow args are unexpectedly short!\n", port);
	}
	scm_puts("ABORT: ", port);
	scm_puts(restr, port);
	free(restr);

	return SCM_BOOL_F;
}

/* ============================================================== */

/**
 * Evaluate the expression
 */
std::string SchemeEval::eval(const std::string &expr)
{
	pexpr = &expr;

#ifdef WORK_AROUND_GUILE_THREADING_BUG
	thread_lock();
#endif /* WORK_AROUND_GUILE_THREADING_BUG */

	scm_with_guile(c_wrap_eval, this);

#ifdef WORK_AROUND_GUILE_THREADING_BUG
	thread_unlock();
#endif /* WORK_AROUND_GUILE_THREADING_BUG */

	return answer;
}

void * SchemeEval::c_wrap_eval(void * p)
{
	SchemeEval *self = (SchemeEval *) p;
	self->answer = self->do_eval(*(self->pexpr));
	return self;
}

std::string SchemeEval::do_eval(const std::string &expr)
{
	bool newin = false;
	per_thread_init();

	/* Avoid a string buffer copy if there is no pending input */
	const char *expr_str;
	if (pending_input)
	{
		input_line += expr;
		expr_str = input_line.c_str();
	}
	else
	{
		expr_str = expr.c_str();
		newin = true;
	}

	caught_error = false;
	pending_input = false;
	captured_stack = SCM_BOOL_F;
	SCM rc = scm_c_catch (SCM_BOOL_T,
	            (scm_t_catch_body) scm_c_eval_string, (void *) expr_str,
	            SchemeEval::catch_handler_wrapper, this,
	            SchemeEval::preunwind_handler_wrapper, this);

	/* An error is thrown if the input expression is incomplete,
	 * in which case the error handler sets the pending_input flag
	 * to true. */
	if (pending_input)
	{
		/* Save input for later */
		if (newin) input_line += expr;
		return "";
	}
	pending_input = false;
	input_line = "";

	if (caught_error)
	{
		std::string rv;
		rc = scm_get_output_string(error_string_port);
		char * str = scm_to_locale_string(rc);
		rv = str;
		free(str);
		scm_close_port(error_string_port);
		error_string_port = SCM_EOL;
		captured_stack = SCM_BOOL_F;

		scm_truncate_file(outport, scm_from_uint16(0));

		rv += "\n";
		return rv;
	}
	else
	{
		std::string rv;
		// First, we get the contents of the output port,
		// and pass that on.
		SCM out = scm_get_output_string(outport);
		char * str = scm_to_locale_string(out);
		rv = str;
		free(str);
		scm_truncate_file(outport, scm_from_uint16(0));

		// Next, we append the "interpreter" output
		rv += prt(rc);
		rv += "\n";

		return rv;
	}
	return "#<Error: Unreachable statement reached>";
}

/* ============================================================== */

/**
 * Return true if the expression was incomplete, and more is expected
 * (for example, more closing parens are expected)
 */
bool SchemeEval::input_pending(void)
{
	return pending_input;
}

/**
 * Return true if an error occured during the evaluation of the expression
 */
bool SchemeEval::eval_error(void)
{
	return caught_error;
}

/**
 * Clear the error state, the input buffers, etc.
 */
void SchemeEval::clear_pending(void)
{
	input_line = "";
	pending_input = false;
	caught_error = false;
}

/* ============================================================== */

SCM SchemeEval::wrap_scm_eval(void *expr)
{
	SCM sexpr = (SCM)expr;
	// return scm_local_eval (sexpr, SCM_EOL);
	// return scm_local_eval (sexpr, scm_procedure_environment(scm_car(sexpr)));
	return scm_eval (sexpr, scm_interaction_environment());
}

/**
 * do_scm_eval -- evaluate expression
 * More or less the same as eval, except the expression is assumed
 * to be an SCM already.  This method *must* be called in guile
 * mode, in order for garbage collection, etc. to work correctly!
 *
 * Logs info to the logger, as otherwise, the output would be 
 * invisible to the developer.
 */
SCM SchemeEval::do_scm_eval(SCM sexpr)
{
	caught_error = false;
	captured_stack = SCM_BOOL_F;
	SCM rc = scm_c_catch (SCM_BOOL_T,
	            (scm_t_catch_body) wrap_scm_eval, (void *) sexpr,
	            SchemeEval::catch_handler_wrapper, this,
	            SchemeEval::preunwind_handler_wrapper, this);

	if (caught_error)
	{
		rc = scm_get_output_string(error_string_port);
		char * str = scm_to_locale_string(rc);
		scm_close_port(error_string_port);
		error_string_port = SCM_EOL;
		captured_stack = SCM_BOOL_F;

		scm_truncate_file(outport, scm_from_uint16(0));

		// Unlike errors seen on the interpreter, log these to the logger.
		// That's because these errors will be predominantly script
		// errors that are otherwise invisible to the user/developer.
		Logger::Level save = logger().getBackTraceLevel();
		logger().setBackTraceLevel(Logger::NONE);
		logger().error("%s: guile error was: %s\nFailing expression was %s",
			 __FUNCTION__, str, prt(sexpr).c_str());
		logger().setBackTraceLevel(save);

		free(str);
		return SCM_EOL;
	}

	// Get the contents of the output port, and log it
	SCM out = scm_get_output_string(outport);
	char * str = scm_to_locale_string(out);
	if (0 < strlen(str))
	{
		logger().info("do_scm_eval(): Output: %s\n"
			"Was generated by expr: %s\n",
			 str, prt(sexpr).c_str());
	}
	scm_truncate_file(outport, scm_from_uint16(0));
	free(str);

	return rc;
}

/* ============================================================== */

/**
 * Execute the schema specified in an ExecutionLink
 */
Handle SchemeEval::apply(const std::string &func, Handle varargs)
{
	pexpr = &func;
	hargs = varargs;

#ifdef WORK_AROUND_GUILE_THREADING_BUG
	thread_lock();
#endif /* WORK_AROUND_GUILE_THREADING_BUG */

	scm_with_guile(c_wrap_apply, this);

#ifdef WORK_AROUND_GUILE_THREADING_BUG
	thread_unlock();
#endif /* WORK_AROUND_GUILE_THREADING_BUG */

	return hargs;
}

void * SchemeEval::c_wrap_apply(void * p)
{
	SchemeEval *self = (SchemeEval *) p;
	self->hargs = self->do_apply(*self->pexpr, self->hargs);
	return self;
}

#endif
/* ===================== END OF FILE ============================ */
