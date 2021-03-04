#include <stdio.h>
#include <curl/curl.h>
#include "php_webops_event.h"
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
#include "webops_event.h"
#include "backtrace.h"

#define PRINT(what) fprintf(stderr, what "\n");

#define WEBOPS_EVENT_ERROR 1
#define WEBOPS_EVENT_EXCEPTION 2

PHP_INI_BEGIN()
  /* Boolean controlling whether the extension is globally active or not */
  STD_PHP_INI_BOOLEAN("webops_event.enabled", "1", PHP_INI_SYSTEM, OnUpdateBool, enabled, zend_webops_event_globals, webops_event_globals)
  /* Application identifier, helps identifying which application is being monitored */
  STD_PHP_INI_ENTRY("webops_event.application_id", "My application", PHP_INI_ALL, OnUpdateString, application_id, zend_webops_event_globals, webops_event_globals)
  /* Boolean controlling whether the event monitoring is active or not */
  STD_PHP_INI_BOOLEAN("webops_event.event_enabled", "1", PHP_INI_ALL, OnUpdateBool, event_enabled, zend_webops_event_globals, webops_event_globals)
  /* Boolean controlling whether the http process should send to REST endpoint */
  STD_PHP_INI_BOOLEAN("webops_event.http_enabled", "1", PHP_INI_ALL, OnUpdateBool, http_enabled, zend_webops_event_globals, webops_event_globals)
  /* Boolean controlling the collection of stats */
  STD_PHP_INI_BOOLEAN("webops_event.http_stats_enabled", "1", PHP_INI_ALL, OnUpdateBool, http_stats_enabled, zend_webops_event_globals, webops_event_globals)
  /* Control which exceptions to collect (0: none exceptions collected, 1: collect uncaught exceptions (default), 2: collect ALL exceptions) */
  STD_PHP_INI_ENTRY("webops_event.http_exception_mode","1", PHP_INI_PERDIR, OnUpdateLongGEZero, http_exception_mode, zend_webops_event_globals, webops_event_globals)
  /* process silenced events? */
  STD_PHP_INI_BOOLEAN("webops_event.http_process_silenced_events", "1", PHP_INI_PERDIR, OnUpdateBool, http_process_silenced_events, zend_webops_event_globals, webops_event_globals)
  STD_PHP_INI_ENTRY("webops_event.http_request_timeout", "1000", PHP_INI_ALL, OnUpdateLong, http_request_timeout, zend_webops_event_globals, webops_event_globals)
  STD_PHP_INI_ENTRY("webops_event.http_server", "http://localhost", PHP_INI_ALL, OnUpdateString, http_server, zend_webops_event_globals, webops_event_globals)
  STD_PHP_INI_ENTRY("webops_event.http_client_certificate", NULL, PHP_INI_ALL, OnUpdateString, http_client_certificate, zend_webops_event_globals, webops_event_globals)
  STD_PHP_INI_ENTRY("webops_event.http_client_key", NULL, PHP_INI_ALL, OnUpdateString, http_client_key, zend_webops_event_globals, webops_event_globals)
  STD_PHP_INI_ENTRY("webops_event.http_certificate_authorities", NULL, PHP_INI_ALL, OnUpdateString, http_certificate_authorities, zend_webops_event_globals, webops_event_globals)
  STD_PHP_INI_ENTRY("webops_event.http_max_backtrace_length", "0", PHP_INI_ALL, OnUpdateLong, http_max_backtrace_length, zend_webops_event_globals, webops_event_globals)
PHP_INI_END()

char *truncate_data(char *input_str, size_t max_len)
{
  char *truncated;
  input_str = input_str ? input_str : NULL;
  if (max_len == 0)
    return strdup(input_str);
  truncated = strndup(input_str, max_len);
  return truncated;
}

static void webops_event_process(int type, const char *error_filename, const uint32_t error_lineno, zend_string *message, char * trace) {
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

    if (WE_G(http_max_backtrace_length) >= 0)
      max_len = WE_G(http_max_backtrace_length);

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

    curl_easy_setopt(curl, CURLOPT_URL, WE_G(http_server));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, WE_G(http_request_timeout));
    if (WE_G(http_client_certificate) != NULL) {
      curl_easy_setopt(curl, CURLOPT_SSLCERT, WE_G(http_client_certificate));
    }
    if (WE_G(http_client_key) != NULL) {
      curl_easy_setopt(curl, CURLOPT_SSLKEY, WE_G(http_client_key));
    }
    if (WE_G(http_certificate_authorities) != NULL) {
      curl_easy_setopt(curl, CURLOPT_CAINFO, WE_G(http_certificate_authorities));
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    }

    res = curl_easy_perform(curl);

    // APM_DEBUG("[HTTP driver] Result: %s\n", curl_easy_strerror(res));

    /* Always clean up. */
    curl_easy_cleanup(curl);
    free(trace_to_send);
  }
}


static int webops_event_zend_extension_startup(zend_extension *ext) {
  // PRINT("WEBOPS_EVENT zend hook startup!");
  return SUCCESS;
}

static void webops_event_zend_extension_shutdown(zend_extension *ext) {
  // PRINT("WEBOPS_EVENT zend hook shutdown!");
}


static void webops_event_error_callback(int type, const char *error_filename, const uint32_t error_lineno, zend_string *args) {
  smart_str trace_str = {0};
  append_backtrace(&trace_str);
  smart_str_0(&trace_str);
  webops_event_process(WEBOPS_EVENT_ERROR, error_filename, error_lineno, args, trace_str.s ? ZSTR_VAL(trace_str.s) : "");
}


static void webops_event_exception_handler(zend_object *exception) {
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

  webops_event_process(WEBOPS_EVENT_EXCEPTION, Z_STRVAL_P(file), Z_LVAL_P(line), Z_STR_P(message), trace_str.s ? ZSTR_VAL(trace_str.s) : "");
}


static void webops_event_zend_extension_activate(void) {
  zend_error_cb = webops_event_error_callback;
  zend_throw_exception_hook = webops_event_exception_handler;
}

static void webops_event_zend_extension_deactivate(void) {
  // PRINT("WEBOPS_EVENT zend hook deactivate!");
}

static void webops_event_zend_extension_message_handler(int code, void *ext) {
  // PRINT("WEBOPS_EVENT zend hook message handler!");
}

static void webops_event_zend_extension_op_array_handler(zend_op_array *op_array) {
  // PRINT("WEBOPS_EVENT zend hook ope_array_handler!");
}

static void webops_event_zend_extension_fcall_begin_handler(zend_execute_data *ex) {
  // PRINT("WEBOPS_EVENT zend hook fcall_begin_handler!");
}



zend_extension_version_info extension_version_info = {
    ZEND_EXTENSION_API_NO,
    ZEND_EXTENSION_BUILD_ID
};

zend_extension zend_extension_entry = {
    WEBOPS_EVENT_EXTNAME,
    WEBOPS_EVENT_EXTVER,
    "Pantheon",
    "https://pantheon.io",
    "&copy; 2021 All Rights Reserved",
    webops_event_zend_extension_startup,             /* startup() : module startup */
    webops_event_zend_extension_shutdown,            /* shutdown() : module shutdown */
    webops_event_zend_extension_activate,            /* activate() : request startup */
    webops_event_zend_extension_deactivate,          /* deactivate() : request shutdown */
    webops_event_zend_extension_message_handler,     /* message_handler() */
    webops_event_zend_extension_op_array_handler,    /* compiler op_array_handler() */
    NULL,                                            /* VM statement_handler() */
    NULL,                                            /* VM fcall_begin_handler() */
    webops_event_zend_extension_fcall_begin_handler, /* VM fcall_end_handler() */
    NULL,                                            /* compiler op_array_ctor() */
    NULL,                                            /* compiler op_array_dtor() */
    STANDARD_ZEND_EXTENSION_PROPERTIES               /* Structure-ending macro */
};


#ifdef COMPILE_DL_SAMPLE
ZEND_GET_MODULE(webops_event)
#endif

