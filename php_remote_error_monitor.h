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

#ifdef ZTS
#include "TSRM.h"
#endif

/* Import configure options
 when building outside of
 the PHP source tree */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef E_EXCEPTION
  #define E_EXCEPTION (1<<15L)
#endif

#ifndef PHP_REMOTE_ERROR_MONITOR_H
  /* Prevent double inclusion */
  #define PHP_REMOTE_ERROR_MONITOR_H

  /* Extension Properties
   *
   */
  #define PHP_REMOTE_ERROR_MONITOR_EXTNAME "remote_error_monitor"
  #define PHP_REMOTE_ERROR_MONITOR_VERSION "1.0"

  /* Define the entry point symbol.
   *
   */
  extern zend_module_entry remote_error_monitor_module_entry;
  #define phpext_remote_error_monitor_ptr &remote_error_monitor_module_entry

  #ifdef PHP_WIN32
    #define PHP_REMOTE_ERROR_MONITOR_API __declspec(dllexport)
  #else
    #define PHP_REMOTE_ERROR_MONITOR_API
  #endif

  #define REMOTE_ERROR_MONITOR_E_ALL (E_ALL | E_STRICT)

  #define REMOTE_ERROR_MONITOR_ERROR 1
  #define REMOTE_ERROR_MONITOR_EXCEPTION 2

#endif

#define PROCESS_EVENT_ARGS int type, char * error_filename, uint64_t error_lineno, char * msg, char * trace
#define RD_DEF(var) zval *var; zend_bool var##_found;

struct remote_error_monitor;
struct remote_error_monitor_entry;
struct remote_error_monitor_request_data;

/* Extension globals */
ZEND_BEGIN_MODULE_GLOBALS(remote_error_monitor)
    /* Boolean controlling whether the extension is globally active or not */
    zend_bool enabled;
    /* Application identifier, helps identifying which application is being monitored */
    char *application_id;
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
ZEND_END_MODULE_GLOBALS(remote_error_monitor)

ZEND_EXTERN_MODULE_GLOBALS(remote_error_monitor)

ZEND_DECLARE_MODULE_GLOBALS(remote_error_monitor);


#ifdef COMPILE_DL_REMOTE_ERROR_MONITOR
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(remote_error_monitor)
#endif


