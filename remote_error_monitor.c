#include <stdio.h>
#include <curl/curl.h>
#include "php_remote_error_monitor.h"
#include "php.h"
#include "php_ini.h"
#include "zend_API.h"
#include "zend_types.h"
#include "zend_string.h"
#include "zend_exceptions.h"
#include "zend_extensions.h"
#include "zend_interfaces.h"
#include "zend_smart_str.h"
#include "ext/pcre/php_pcre.h"
#include "spprintf.h"
#include "remote_error_monitor.h"
#include "backtrace.h"

#define PRINT(what) fprintf(stderr, what "\n");

#define REMOTE_ERROR_MONITOR_ERROR 1
#define REMOTE_ERROR_MONITOR_EXCEPTION 2


PHP_INI_BEGIN()
  /* Boolean controlling whether the extension is globally active or not */
  STD_PHP_INI_BOOLEAN("remote_error_monitor.enabled", "1", PHP_INI_SYSTEM, OnUpdateBool, enabled, zend_remote_error_monitor_globals, remote_error_monitor_globals)
  /* Application identifier, helps identifying which application is being monitored */
  STD_PHP_INI_ENTRY("remote_error_monitor.application_id", "My application", PHP_INI_ALL, OnUpdateString, application_id, zend_remote_error_monitor_globals, remote_error_monitor_globals)
  /* Boolean controlling whether the event monitoring is active or not */
  STD_PHP_INI_BOOLEAN("remote_error_monitor.event_enabled", "1", PHP_INI_ALL, OnUpdateBool, event_enabled, zend_remote_error_monitor_globals, remote_error_monitor_globals)
  /* Boolean controlling whether the http process should send to REST endpoint */
  STD_PHP_INI_BOOLEAN("remote_error_monitor.http_enabled", "1", PHP_INI_ALL, OnUpdateBool, http_enabled, zend_remote_error_monitor_globals, remote_error_monitor_globals)
  /* Boolean controlling the collection of stats */
  STD_PHP_INI_BOOLEAN("remote_error_monitor.http_stats_enabled", "1", PHP_INI_ALL, OnUpdateBool, http_stats_enabled, zend_remote_error_monitor_globals, remote_error_monitor_globals)
  /* Control which exceptions to collect (0: none exceptions collected, 1: collect uncaught exceptions (default), 2: collect ALL exceptions) */
  STD_PHP_INI_ENTRY("remote_error_monitor.http_exception_mode","1", PHP_INI_PERDIR, OnUpdateLongGEZero, http_exception_mode, zend_remote_error_monitor_globals, remote_error_monitor_globals)
  /* process silenced events? */
  STD_PHP_INI_BOOLEAN("remote_error_monitor.http_process_silenced_events", "1", PHP_INI_PERDIR, OnUpdateBool, http_process_silenced_events, zend_remote_error_monitor_globals, remote_error_monitor_globals)
  STD_PHP_INI_ENTRY("remote_error_monitor.http_request_timeout", "1000", PHP_INI_ALL, OnUpdateLong, http_request_timeout, zend_remote_error_monitor_globals, remote_error_monitor_globals)
  STD_PHP_INI_ENTRY("remote_error_monitor.http_server", "http://localhost", PHP_INI_ALL, OnUpdateString, http_server, zend_remote_error_monitor_globals, remote_error_monitor_globals)
  STD_PHP_INI_ENTRY("remote_error_monitor.http_client_certificate", NULL, PHP_INI_ALL, OnUpdateString, http_client_certificate, zend_remote_error_monitor_globals, remote_error_monitor_globals)
  STD_PHP_INI_ENTRY("remote_error_monitor.http_client_key", NULL, PHP_INI_ALL, OnUpdateString, http_client_key, zend_remote_error_monitor_globals, remote_error_monitor_globals)
  STD_PHP_INI_ENTRY("remote_error_monitor.http_certificate_authorities", NULL, PHP_INI_ALL, OnUpdateString, http_certificate_authorities, zend_remote_error_monitor_globals, remote_error_monitor_globals)
  STD_PHP_INI_ENTRY("remote_error_monitor.http_max_backtrace_length", "0", PHP_INI_ALL, OnUpdateLong, http_max_backtrace_length, zend_remote_error_monitor_globals, remote_error_monitor_globals)
PHP_INI_END()

ZEND_DECLARE_MODULE_GLOBALS(remote_error_monitor);

void (*old_error_cb)(int type, const char *error_filename, const uint32_t error_lineno, zend_string *args);




char *truncate_data(char *input_str, size_t max_len)
{
  char *truncated;
  input_str = input_str ? input_str : NULL;
  if (max_len == 0)
    return strdup(input_str);
  truncated = strndup(input_str, max_len);
  return truncated;
}

static void remote_error_monitor_process(int type, const char *error_filename, const uint32_t error_lineno, zend_string *message, char * trace)
{
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if(curl) {
    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastptr = NULL;
    struct curl_slist *headerlist = NULL;
    static const char buf[] = "Expect:";
    char int2string[64];
    char *trace_to_send;
    size_t max_len = 0;

    if (REM_G(http_max_backtrace_length) >= 0)
      max_len = REM_G(http_max_backtrace_length);

    trace_to_send = truncate_data(trace, max_len);

    sprintf(int2string, "%d", type);
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "type",
                 CURLFORM_COPYCONTENTS, int2string,
                 CURLFORM_END);

    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "file",
                 CURLFORM_COPYCONTENTS, error_filename ? error_filename : "",
                 CURLFORM_END);

    sprintf(int2string, "%d", error_lineno);
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "line",
                 CURLFORM_COPYCONTENTS, int2string,
                 CURLFORM_END);

    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "message",
                 CURLFORM_COPYCONTENTS, message ? ZSTR_VAL(message) : "",
                 CURLFORM_END);

    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "backtrace",
                 CURLFORM_COPYCONTENTS, trace_to_send,
                 CURLFORM_END);

    headerlist = curl_slist_append(headerlist, buf);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    curl_easy_setopt(curl, CURLOPT_URL, REM_G(http_server));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, REM_G(http_request_timeout));
    if (REM_G(http_client_certificate) != NULL) {
      curl_easy_setopt(curl, CURLOPT_SSLCERT, REM_G(http_client_certificate));
    }
    if (REM_G(http_client_key) != NULL) {
      curl_easy_setopt(curl, CURLOPT_SSLKEY, REM_G(http_client_key));
    }
    if (REM_G(http_certificate_authorities) != NULL) {
      curl_easy_setopt(curl, CURLOPT_CAINFO, REM_G(http_certificate_authorities));
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    }

    res = curl_easy_perform(curl);

    // APM_DEBUG("[HTTP driver] Result: %s\n", curl_easy_strerror(res));

    /* Always clean up. */
    curl_easy_cleanup(curl);
    free(trace_to_send);
  }
}


static void remote_error_monitor_error_callback(int type, const char *error_filename, const uint32_t error_lineno, zend_string *args)
{
  smart_str trace_str = {0};
  append_backtrace(&trace_str);
  smart_str_0(&trace_str);
  remote_error_monitor_process(REMOTE_ERROR_MONITOR_ERROR, error_filename, error_lineno, args, trace_str.s ? ZSTR_VAL(trace_str.s) : "");
}


static void remote_error_monitor_exception_handler(zend_object *exception)
{
  zval *message, *file, *line, rv;
  int type;
  zend_class_entry *default_ce;
  smart_str trace_str = {0};
  append_backtrace(&trace_str);
  smart_str_0(&trace_str);

  default_ce = zend_get_exception_base(exception);
  // zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, bool silent, zval *rv
  message = zend_read_property( default_ce, exception, "message", sizeof("message")-1, 0,  &rv);
  file = zend_read_property(default_ce, exception, "file", sizeof("file")-1, 0, &rv);
  line = zend_read_property(default_ce, exception, "line", sizeof("line")-1, 0, &rv);

  remote_error_monitor_process(REMOTE_ERROR_MONITOR_EXCEPTION, Z_STRVAL_P(file), Z_LVAL_P(line), Z_STR_P(message), trace_str.s ? ZSTR_VAL(trace_str.s) : "");
}


PHP_MINIT_FUNCTION(remote_error_monitor)
{
  REGISTER_INI_ENTRIES();
  old_error_cb = zend_error_cb;
  zend_error_cb = remote_error_monitor_error_callback;
  zend_throw_exception_hook = remote_error_monitor_exception_handler;
  PRINT("MODULE INIT FUNCTION!");
  return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(remote_error_monitor)
{
  UNREGISTER_INI_ENTRIES();
  zend_error_cb = old_error_cb;
  PRINT("MODULE SHUTDOWN FUNCTION!");
  return SUCCESS;
}

PHP_MINFO_FUNCTION(remote_error_monitor)
{
  php_info_print_table_start();
  //https://pantheon.io/sites/default/files/logos/logo-382x120-01.svg

  php_info_print_table_header(2, "Remote Error Monitor Support", "enabled");
  php_info_print_table_row(2, "Version", PHP_REMOTE_ERROR_MONITOR_VERSION);

  php_info_print_table_end();

  DISPLAY_INI_ENTRIES();

}

PHP_GINIT_FUNCTION(remote_error_monitor)
{
  #if defined(COMPILE_DL_REMOTE_ERROR_MONITOR) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
  #endif

  ZEND_SECURE_ZERO(remote_error_monitor_globals, sizeof(*remote_error_monitor_globals));
}

zend_module_entry remote_error_monitor_module_entry = {
    STANDARD_MODULE_HEADER,
    "remote_error_monitor",					/* Extension name */
    NULL,					                                /* zend_function_entry */
    PHP_MINIT(remote_error_monitor),							/* PHP_MINIT - Module initialization */
    PHP_MSHUTDOWN(remote_error_monitor),							/* PHP_MSHUTDOWN - Module shutdown */
    NULL,		            	                      /* PHP_RINIT - Request initialization */
    NULL,							                          /* PHP_RSHUTDOWN - Request shutdown */
    PHP_MINFO(remote_error_monitor),			/* PHP_MINFO - Module info */
    PHP_REMOTE_ERROR_MONITOR_VERSION,		/* Version */
    STANDARD_MODULE_PROPERTIES,
    PHP_MODULE_GLOBALS(remote_error_monitor),
    PHP_GINIT(remote_error_monitor)

};



#ifdef COMPILE_DL_SAMPLE
ZEND_GET_MODULE(remote_error_monitor)
#endif

