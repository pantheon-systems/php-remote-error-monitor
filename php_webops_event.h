#include "php.h"
#include "php_globals.h"
#include "php_ini.h"
#include "zend_errors.h"
#include "zend_modules.h"
#include "zend_smart_str.h"
#include "zend_types.h"
#include "zend_API.h"
#include "zend_exceptions.h"
#include "zend_extensions.h"
#include "zend_interfaces.h"
#include "../ext/standard/info.h"

/* Import configure options
 when building outside of
 the PHP source tree */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef E_EXCEPTION
  #define E_EXCEPTION (1<<15L)
#endif

#ifndef PHP_WEBOPS_EVENT_H
  /* Prevent double inclusion */
  #define PHP_WEBOPS_EVENT_H

  /* Extension Properties
   *
   */
  #define WEBOPS_EVENT_EXTNAME "webops_event"
  #define WEBOPS_EVENT_EXTVER "1.0"

  /* Define the entry point symbol.
   *
   */
  #define phpext_webops_event_ptr &webops_event_module_entry
  extern zend_module_entry webops_event_module_entry;

  #ifdef PHP_WIN32
    #define PHP_WEBOPS_EVENT_API __declspec(dllexport)
  #else
    #define PHP_WEBOPS_EVENT_API
  #endif

  #define WEBOPS_E_ALL (E_ALL | E_STRICT)

  #define WEBOPS_EVENT_ERROR 1
  #define WEBOPS_EVENT_EXCEPTION 2

#endif

#define PROCESS_EVENT_ARGS int type, char * error_filename, uint64_t error_lineno, char * msg, char * trace
#define RD_DEF(var) zval *var; zend_bool var##_found;

#ifdef ZTS
  #define WE_G(v) TSRMG(webops_event_globals_id, webops_event_globals *, v)
#else
  #define WE_G(v) (webops_event_globals.v)
#endif

typedef struct webops_event {
    int event_type;
    int type;
    char * error_filename;
    uint64_t error_lineno;
    char * msg;
    char * trace;
} webops_event;

typedef struct webops_event_entry {
    webops_event event;
    struct webops_event_entry *next;
} webops_event_entry;

typedef struct webops_event_driver {
    void (* process_event)(PROCESS_EVENT_ARGS);
    void (* process_stats)();
    int (* ZEND_MINIT)(int );
    int (* ZEND_RINIT)();
    int (* ZEND_MSHUTDOWN)(SHUTDOWN_FUNC_ARGS);
    int (* ZEND_RSHUTDOWN)();
    int (*is_enabled)();
    int (*want_event)(int, int, char * );
    int (*want_stats)();
    int (* error_reporting)();
    int is_request_created;
} webops_event_driver;

typedef struct webops_event_driver_entry {
    webops_event_driver driver;
    struct webops_event_driver_entry *next;
} webops_event_driver_entry;


typedef struct webops_event_request_data {
    RD_DEF(uri);
    RD_DEF(host);
    RD_DEF(ip);
    RD_DEF(referer);
    RD_DEF(ts);
    RD_DEF(script);
    RD_DEF(method);

    zend_bool initialized, cookies_found, post_vars_found;
    smart_str cookies, post_vars;
} webops_event_request_data;





/* Extension globals */
ZEND_BEGIN_MODULE_GLOBALS(webops_event)
    /* Boolean controlling whether the extension is globally active or not */
    zend_bool enabled;
    /* Application identifier, helps identifying which application is being monitored */
    char      *application_id;
    /* Boolean controlling whether the driver is active and whether to send data to a REST endpoint */
    zend_bool event_enabled;
    zend_bool http_enabled;
    /* Boolean controlling the collection of stats */
    zend_bool http_stats_enabled;
    /* (unused for HTTP) */
    long http_exception_mode;
    /* (unused for HTTP) */
    int http_error_reporting;
    /* Option to process silenced events */
    zend_bool http_process_silenced_events;

    long http_request_timeout;
    char *http_server;
    char *http_client_certificate;
    char *http_client_key;
    char *http_certificate_authorities;
    long http_max_backtrace_length;
ZEND_END_MODULE_GLOBALS(webops_event)

ZEND_EXTERN_MODULE_GLOBALS(webops_event)

ZEND_DECLARE_MODULE_GLOBALS(webops_event);







