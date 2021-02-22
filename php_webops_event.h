#include "php.h"
#include "zend_errors.h"
#include "zend_smart_str.h"

#ifndef E_EXCEPTION
  #define E_EXCEPTION (1<<15L)
#endif

#ifndef PHP_WEBOPS_EVENT_H
  /* Prevent double inclusion */
  #define PHP_WEBOPS_EVENT_H

  /* Extension Properties
   *
   */
  #define PHP_WEBOPS_EVENT_EXTNAME "webops_event"
  #define PHP_WEBOPS_EVENT_EXTVER "1.0"

    /* Import Configure Options
     * when building outside of
     * PHP Source Tree
     */
    #ifdef HAVE_CONFIG_H
      #include "config.h"
    #endif

  /* Import standard PHP Header.
   *
   */
  #include "php.h"

  /* Define the entry point symbol.
   *
   */
  extern zend_module_entry webops_event_entry;
  #define PHPext_webops_event_ptr &webops_event_module_entry

  #define APM_E_ALL (E_ALL | E_STRICT)

  #define APM_EVENT_ERROR 1
  #define APM_EVENT_EXCEPTION 2

  #define PROCESS_EVENT_ARGS int type, char * error_filename, uint error_lineno, char * msg, char * trace


#endif

/**
 * DATA STRUCTURES
 *
 *
 * Data structures used by all hooks.
 */

typedef struct webops_event {
    int event_type;
    int type;
    char * error_filename;
    uint error_lineno;
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
    int (* minit)(int );
    int (* rinit)();
    int (* mshutdown)(SHUTDOWN_FUNC_ARGS);
    int (* rshutdown)();
    zend_bool (* is_enabled)();
    zend_bool (* want_event)(int, int, char * );
    zend_bool (* want_stats)();
    int (* error_reporting)();
    zend_bool is_request_created;
} webops_event_driver;

typedef struct webops_event_driver_entry {
    webops_event_driver driver;
    struct webops_event_driver_entry *next;
} webops_event_driver_entry;

# define RD_DEF(var) zval *var; zend_bool var##_found;

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