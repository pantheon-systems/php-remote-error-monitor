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
  #define PHP_REMOTE_ERROR_MONITOR_VERSION "1.0.5"

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

  /* Saving this specification, as we may want to return to it when our
   * error reporting mechanism isn't causing platform stability issues.

    #define REMOTE_ERROR_MONITOR_E_ALL (E_ALL & ~E_DEPRECATED & ~E_NOTICE & ~E_WARNING | E_STRICT)
   */

  #define REMOTE_ERROR_MONITOR_E_ALL (E_ERROR & E_PARSE & E_CORE_ERROR & E_COMPILE_ERROR & E_USER_ERROR)

  #define REMOTE_ERROR_MONITOR_ERROR 1
  #define REMOTE_ERROR_MONITOR_EXCEPTION 2

  #define PROCESS_EVENT_ARGS int type, char * error_filename, uint64_t error_lineno, char * msg, char * trace
  #define RD_DEF(var) zval *var; zend_bool var##_found

  typedef struct remote_error_monitor {
      int event_type;
      int type;
      char * error_filename;
      uint64_t error_lineno;
      char * msg;
      char * trace;
  } remote_error_monitor;

  typedef struct remote_error_monitor_entry {
      remote_error_monitor event;
      struct remote_error_monitor_entry *next;
  } remote_error_monitor_entry;

  typedef struct remote_error_monitor_request_data {
      RD_DEF(uri);
      RD_DEF(host);
      RD_DEF(ip);
      RD_DEF(referer);
      RD_DEF(ts);
      RD_DEF(script);
      RD_DEF(method);

      zend_bool initialized, cookies_found, post_vars_found;
      smart_str cookies, post_vars;
  } remote_error_monitor_request_data;

  PHP_MINIT_FUNCTION(remote_error_monitor);
  PHP_MSHUTDOWN_FUNCTION(remote_error_monitor);
  PHP_RINIT_FUNCTION(remote_error_monitor);
  PHP_RSHUTDOWN_FUNCTION(remote_error_monitor);
  PHP_MINFO_FUNCTION(remote_error_monitor);

  /* Extension globals */
  ZEND_BEGIN_MODULE_GLOBALS(remote_error_monitor)
    /* Boolean controlling whether the extension is globally active or not */
    zend_bool enabled;
    /* Application identifier, helps identifying which application is being monitored */
    char *application_id;
    long http_request_timeout;
    char *http_server;
    char *http_client_certificate;
    char *http_client_key;
    char *http_certificate_authorities;
    long dump_max_depth;
  ZEND_END_MODULE_GLOBALS(remote_error_monitor)

  ZEND_EXTERN_MODULE_GLOBALS(remote_error_monitor)

  #ifdef ZTS
    #define REM_GLOBAL(v) TSRMG( remote_error_monitor_globals_id, zend_remote_error_monitor_globals *, v )
  #else
    #define REM_GLOBAL(v) ( remote_error_monitor_globals.v )
  #endif

#endif







