/*
 +----------------------------------------------------------------------+
 |  APM stands for Alternative PHP Monitor                              |
 +----------------------------------------------------------------------+
 | Copyright (c) 2008-2014  Davide Mendolia, Patrick Allaert            |
 +----------------------------------------------------------------------+
 | This source file is subject to version 3.01 of the PHP license,      |
 | that is bundled with this package in the file LICENSE, and is        |
 | available through the world-wide-web at the following url:           |
 | http://www.php.net/license/3_01.txt                                  |
 | If you did not receive a copy of the PHP license and are unable to   |
 | obtain it through the world-wide-web, please send a note to          |
 | license@php.net so we can mail you a copy immediately.               |
 +----------------------------------------------------------------------+
 | Authors: Patrick Allaert <patrickallaert@php.net>                    |
 +----------------------------------------------------------------------+
*/

#include "php_remote_error_monitor.h"
#include <stddef.h>
#include "php.h"
#include "zend_types.h"
#include "ext/standard/php_string.h"
#include "Zend/zend_generators.h"
#include "backtrace.h"


static void  rem_debug_print_backtrace_args(zval *arg_array , smart_str *trace_str);
static void  rem_append_flat_zval_r(zval *expr , smart_str *trace_str, char depth);
static void  rem_append_flat_hash(HashTable *ht , smart_str *trace_str, char is_object, char depth);
static void  rem_debug_backtrace_get_args(zend_execute_data *call, zval *arg_array);
static int   rem_append_variable(zval *expr, smart_str *trace_str);
static char *rem_addslashes(char *str, uint length, int *new_length);



void rem_append_backtrace(smart_str *trace_str )
{
	/* backtrace variables */
	zend_execute_data *ptr, *skip;
	int lineno;
	char *function_name;
	const char *filename;
	char *call_type;
	const char *include_filename = NULL;
	zval arg_array;
	zend_execute_data *call;
	zend_string *class_name = NULL;
	zend_object *object;
	zend_function *func;
	int indent = 0;
	ZVAL_UNDEF(&arg_array);
	ptr = EG(current_execute_data);
	call = ptr;


	while (ptr) {
		class_name = NULL;
		call_type = NULL;

    ZVAL_UNDEF(&arg_array);

    ptr = zend_generator_check_placeholder_frame(ptr);

		skip = ptr;
		/* skip internal handler */
    if ((!skip->func || !ZEND_USER_CODE(skip->func->common.type)) &&
      skip->prev_execute_data &&
      skip->prev_execute_data->func &&
      ZEND_USER_CODE(skip->prev_execute_data->func->common.type) &&
      skip->prev_execute_data->opline->opcode != ZEND_DO_ICALL &&
      skip->prev_execute_data->opline->opcode != ZEND_DO_UCALL &&
      skip->prev_execute_data->opline->opcode != ZEND_DO_FCALL &&
      skip->prev_execute_data->opline->opcode != ZEND_DO_FCALL_BY_NAME &&
      skip->prev_execute_data->opline->opcode != ZEND_INCLUDE_OR_EVAL) {
      skip = skip->prev_execute_data;
		}

    if (skip->func && ZEND_USER_CODE(skip->func->common.type)) {
      filename = skip->func->op_array.filename->val;
      if (skip->opline->opcode == ZEND_HANDLE_EXCEPTION) {
        if (EG(opline_before_exception)) {
          lineno = EG(opline_before_exception)->lineno;
        } else {
          lineno = skip->func->op_array.line_end;
        }
      } else {
        lineno = skip->opline->lineno;
      }
    }
		else {
			filename = NULL;
			lineno = 0;
		}
/* $this may be passed into regular internal functions */
    object = Z_OBJ(call->This);
    zend_function *func = NULL;
    if (call->func) {
      func = call->func;
      if (object)
      {
        function_name = ZSTR_VAL(object->ce->name);
      }
      else if (func->common.scope->name)
      {
        function_name = ZSTR_VAL(func->common.scope->name);
      }
      else
      {
        function_name = ZSTR_VAL(func->common.function_name);
      }
    }

    if (function_name) {
      if (object) {
        if (func->common.scope) {
          class_name = func->common.scope->name;
        } else if (object->handlers->get_class_name == std_object_handlers.get_class_name) {
          class_name = object->ce->name;
        } else {
          class_name = object->handlers->get_class_name(object);
        }

        call_type = "->";
      } else if (func->common.scope) {
        class_name = func->common.scope->name;
        call_type = "::";
      } else {
        class_name = NULL;
        call_type = NULL;
      }
      if (func->type != ZEND_EVAL_CODE) {
        rem_debug_backtrace_get_args(call, &arg_array);
      }
    }

		else {
			/* i know this is kinda ugly, but i'm trying to avoid extra cycles in the main execution loop */
			zend_bool build_filename_arg = 1;
      if (!ptr->func || !ZEND_USER_CODE(ptr->func->common.type) || ptr->opline->opcode != ZEND_INCLUDE_OR_EVAL) {
				/* can happen when calling eval from a custom sapi */
				function_name = "unknown";
				build_filename_arg = 0;
			} else
			switch (ptr->opline->op2.constant) {
				case ZEND_EVAL:
					function_name = "eval";
					build_filename_arg = 0;
					break;
				case ZEND_INCLUDE:
					function_name = "include";
					break;
				case ZEND_REQUIRE:
					function_name = "require";
					break;
				case ZEND_INCLUDE_ONCE:
					function_name = "include_once";
					break;
				case ZEND_REQUIRE_ONCE:
					function_name = "require_once";
					break;
				default:
					/* this can actually happen if you use debug_backtrace() in your error_handler and
					 * you're in the top-scope */
					function_name = "unknown";
					build_filename_arg = 0;
					break;
			}

			if (build_filename_arg && include_filename) {
        array_init(&arg_array);
        add_next_index_string(&arg_array, include_filename);
			}
			call_type = NULL;
		}
		smart_str_appendc(trace_str, '#');
		smart_str_append_long(trace_str, indent);
		smart_str_appendc(trace_str, ' ');
		if (class_name) {
      smart_str_appends(trace_str, ZSTR_VAL(class_name));
			/* here, call_type is either "::" or "->" */
			smart_str_appendl(trace_str, call_type, 2);
		}
		if (function_name) {
			smart_str_appends(trace_str, function_name);
		} else {
			smart_str_appendl(trace_str, "main", 4);
		}
		smart_str_appendc(trace_str, '(');
    if (Z_TYPE(arg_array) != IS_UNDEF) {
      rem_debug_print_backtrace_args(&arg_array, trace_str);
      zval_ptr_dtor(&arg_array);
    }

		if (filename) {
			smart_str_appendl(trace_str, ") called at [", sizeof(") called at [") - 1);
			smart_str_appends(trace_str, filename);
			smart_str_appendc(trace_str, ':');
			smart_str_append_long(trace_str, lineno);
			smart_str_appendl(trace_str, "]\n", 2);
		} else {
      zend_execute_data *prev_call = skip;
			zend_execute_data *prev = skip->prev_execute_data;

			while (prev) {
        if (prev_call &&
            prev_call->func &&
            !ZEND_USER_CODE(prev_call->func->common.type)) {
          prev = NULL;
          break;
        }
        if (prev->func && ZEND_USER_CODE(prev->func->common.type)) {
          zend_printf(") called at [%s:%d]\n", prev->func->op_array.filename->val, prev->opline->lineno);
          break;
        }
        prev_call = prev;
				prev = prev->prev_execute_data;
			}
			if (!prev) {
				smart_str_appendl(trace_str, ")\n", 2);
			}
		}
		include_filename = filename;
		ptr = skip->prev_execute_data;
		++indent;
    call = skip;
	}
}


static void rem_debug_print_backtrace_args(zval *arg_array, smart_str *trace_str)
{
  zval *tmp;
  int i = 0;

  ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(arg_array), tmp) {
        if (i++) {
          smart_str_appendl(trace_str, ", ", 2);
        }
        rem_append_flat_zval_r(tmp, trace_str, 0);
      } ZEND_HASH_FOREACH_END();
}


// See void zend_print_flat_zval_r in php/Zend/zend.c for template when adapting to newer php versions
static void rem_append_flat_zval_r(zval *expr , smart_str *trace_str, char depth)
{
  if (depth >= REM_GLOBAL(dump_max_depth)) {
    smart_str_appendl(trace_str, "/* [...] */", sizeof("/* [...] */") - 1);
    return;
  }

  switch (Z_TYPE_P(expr)) {
    case IS_REFERENCE:
      ZVAL_DEREF(expr);
      smart_str_appendc(trace_str, '&');
      rem_append_flat_zval_r(expr, trace_str, depth);
      break;
    case IS_ARRAY:
      smart_str_appendc(trace_str, '[');
      if (Z_REFCOUNTED_P(expr)) {
        if (Z_IS_RECURSIVE_P(expr)) {
          smart_str_appendl(trace_str, " *RECURSION*", sizeof(" *RECURSION*") - 1);
          return;
        }
        Z_PROTECT_RECURSION_P(expr);
      }
      rem_append_flat_hash(Z_ARRVAL_P(expr) , trace_str, 0, depth + 1);
      smart_str_appendc(trace_str, ']');
      if (Z_REFCOUNTED_P(expr)) {
        Z_UNPROTECT_RECURSION_P(expr);
      }
      break;
    case IS_OBJECT:
    {
      HashTable *properties = NULL;
      zend_string *class_name = Z_OBJ_HANDLER_P(expr, get_class_name)(Z_OBJ_P(expr));
      smart_str_appends(trace_str, ZSTR_VAL(class_name));
      smart_str_appendl(trace_str, " Object (", sizeof(" Object (") - 1);
      zend_string_release_ex(class_name, 0);

      if (Z_IS_RECURSIVE_P(expr)) {
        smart_str_appendl(trace_str, " *RECURSION*", sizeof(" *RECURSION*") - 1);
        return;
      }

      if (Z_OBJ_HANDLER_P(expr, get_properties)) {
        properties = Z_OBJPROP_P(expr);
      }
      if (properties) {
        Z_PROTECT_RECURSION_P(expr);
        rem_append_flat_hash(properties , trace_str, 1, depth + 1);
        Z_UNPROTECT_RECURSION_P(expr);
      }
      smart_str_appendc(trace_str, ')');
      break;
    }
    default:
      rem_append_variable(expr, trace_str);
      break;
  }
}

static void rem_append_flat_hash(HashTable *ht , smart_str *trace_str, char is_object, char depth)
{
	int i = 0;
  zval *tmp;
  zend_string *string_key;
  zend_ulong num_key;

  ZEND_HASH_FOREACH_KEY_VAL_IND(ht, num_key, string_key, tmp) {
    if (depth >= REM_GLOBAL(dump_max_depth)) {
      smart_str_appendl(trace_str, "/* [...] */", sizeof("/* [...] */") - 1);
      return;
    }

    if (i++ > 0) {
      smart_str_appendl(trace_str, ", ", 2);
    }
    smart_str_appendc(trace_str, '[');
    if (string_key) {
      smart_str_appendl(trace_str, ZSTR_VAL(string_key), ZSTR_LEN(string_key));
    } else {
      smart_str_append_long(trace_str, num_key);
    }

    smart_str_appendl(trace_str, "] => ", 5);
    rem_append_flat_zval_r(tmp, trace_str, depth);
  }
  ZEND_HASH_FOREACH_END();
}

static int rem_append_variable(zval *expr, smart_str *trace_str)
{
	zval expr_copy;
	int use_copy;
	char is_string = 0;
	char * temp;
	int new_len;

	if (Z_TYPE_P(expr) == IS_STRING) {
		smart_str_appendc(trace_str, '"');
		is_string = 1;
	}
  use_copy = zend_make_printable_zval(expr, &expr_copy);
	if (use_copy) {
		expr = &expr_copy;
	}
	if (Z_STRLEN_P(expr) == 0) { /* optimize away empty strings */
		if (is_string) {
			smart_str_appendc(trace_str, '"');
		}
		if (use_copy) {
			zval_dtor(expr);
		}
		return 0;
	}

	if (is_string) {
		temp = rem_addslashes(Z_STRVAL_P(expr), Z_STRLEN_P(expr), &new_len);
		smart_str_appendl(trace_str, temp, new_len);
		smart_str_appendc(trace_str, '"');
		if (temp) {
			efree(temp);
		}
	} else {
		smart_str_appendl(trace_str, Z_STRVAL_P(expr), Z_STRLEN_P(expr));
	}

	if (use_copy) {
		zval_dtor(expr);
	}
	return Z_STRLEN_P(expr);
}

static void rem_debug_backtrace_get_args(zend_execute_data *call, zval *arg_array)
{
  uint32_t num_args = ZEND_CALL_NUM_ARGS(call);

  array_init_size(arg_array, num_args);
  if (num_args) {
    uint32_t i = 0;
    zval *p = ZEND_CALL_ARG(call, 1);

    zend_hash_real_init(Z_ARRVAL_P(arg_array), 1);
    ZEND_HASH_FILL_PACKED(Z_ARRVAL_P(arg_array)) {
        if (call->func->type == ZEND_USER_FUNCTION) {
          uint32_t first_extra_arg = call->func->op_array.num_args;

          if (ZEND_CALL_NUM_ARGS(call) > first_extra_arg) {
            while (i < first_extra_arg) {
              if (Z_OPT_REFCOUNTED_P(p)) Z_ADDREF_P(p);
              ZEND_HASH_FILL_ADD(p);
              zend_hash_next_index_insert_new(Z_ARRVAL_P(arg_array), p);
              p++;
              i++;
            }
            p = ZEND_CALL_VAR_NUM(call, call->func->op_array.last_var + call->func->op_array.T);
          }
        }

        while (i < num_args) {
          if (Z_OPT_REFCOUNTED_P(p)) Z_ADDREF_P(p);
          ZEND_HASH_FILL_ADD(p);
          p++;
          i++;
        }
      } ZEND_HASH_FILL_END();
  }
}

static char *rem_addslashes(char *str, uint length, int *new_length)
{
	/* maximum string length, worst case situation */
	char *new_str;
	char *source, *target;
	char *end;
	int local_new_length;

	if (!new_length) {
		new_length = &local_new_length;
	}

	if (!str) {
		*new_length = 0;
		return str;
	}
	new_str = (char *) safe_emalloc(2, (length ? length : (length = strlen(str))), 1);
	source = str;
	end = source + length;
	target = new_str;

	while (source < end) {
		switch (*source) {
			case '\0':
				*target++ = '\\';
				*target++ = '0';
				break;
			case '\"':
			case '\\':
				*target++ = '\\';
				/* break is missing *intentionally* */
			default:
				*target++ = *source;
				break;
		}

		source++;
	}

	*target = 0;
	*new_length = target - new_str;
	return (char *) erealloc(new_str, *new_length + 1);
}
