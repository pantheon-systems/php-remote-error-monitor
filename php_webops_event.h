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
  #define WE_G(v) TSRMG(webops_event_globals_id, zend_webops_event_globals *, v)
#else
  #define WE_G(v) (webops_event_globals.v)
#endif

#ifdef WE_DEBUGFILE
  #define APM_INIT_DEBUG APM_G(debugfile) = fopen(APM_DEBUGFILE, "a+");
  #define APM_DEBUG(...) if (APM_G(debugfile)) { fprintf(APM_G(debugfile), __VA_ARGS__); fflush(APM_G(debugfile)); }
  #define APM_SHUTDOWN_DEBUG if (APM_G(debugfile)) { fclose(APM_G(debugfile)); APM_G(debugfile) = NULL; }
#else
#define WE_INIT_DEBUG
#define WE_DEBUG(...)
#define WE_SHUTDOWN_DEBUG
#endif

#define WEBOPS_EVENT_DRIVER_CREATE(name) \
  void apm_driver_##name##_process_event(PROCESS_EVENT_ARGS); \
  void apm_driver_##name##_process_stats(); \
  int apm_driver_##name##_minit(int ); \
  int apm_driver_##name##_rinit(); \
  int apm_driver_##name##_mshutdown(); \
  int apm_driver_##name##_rshutdown(); \
  PHP_INI_MH(OnUpdateAPM##name##ErrorReporting) \
  { \
    APM_GLOBAL(name, error_reporting) = (apm_error_reporting_new_value : APM_E_##name); \
    return SUCCESS; \
  } \
  zend_bool apm_driver_##name##_is_enabled() \
  { \
    return APM_GLOBAL(name, enabled); \
  } \
  int apm_driver_##name##_error_reporting() \
  { \
    return APM_GLOBAL(name, error_reporting); \
  } \
  zend_bool apm_driver_##name##_want_event(int event_type, int error_level, char *msg ) \
  { \
    return \
      APM_GLOBAL(name, enabled) \
      && ( \
        (event_type == APM_EVENT_EXCEPTION && APM_GLOBAL(name, exception_mode) == 2) \
        || \
        (event_type == APM_EVENT_ERROR && ((APM_GLOBAL(name, exception_mode) == 1) || (strncmp(msg, "Uncaught exception", 18) != 0)) && (error_level & APM_GLOBAL(name, error_reporting))) \
      ) \
      && ( \
        !APM_G(currently_silenced) || APM_GLOBAL(name, process_silenced_events) \
      ) \
    ; \
  } \
  zend_bool apm_driver_##name##_want_stats() \
  { \
    return \
      APM_GLOBAL(name, enabled) \
      && ( \
        APM_GLOBAL(name, stats_enabled)\
      ) \
    ; \
  } \
  apm_driver_entry * apm_driver_##name##_create() \
  { \
    apm_driver_entry * driver_entry; \
    driver_entry = (apm_driver_entry *) malloc(sizeof(apm_driver_entry)); \
    driver_entry->driver.process_event = apm_driver_##name##_process_event; \
    driver_entry->driver.minit = apm_driver_##name##_minit; \
    driver_entry->driver.rinit = apm_driver_##name##_rinit; \
    driver_entry->driver.mshutdown = apm_driver_##name##_mshutdown; \
    driver_entry->driver.rshutdown = apm_driver_##name##_rshutdown; \
    driver_entry->driver.process_stats = apm_driver_##name##_process_stats; \
    driver_entry->driver.is_enabled = apm_driver_##name##_is_enabled; \
    driver_entry->driver.error_reporting = apm_driver_##name##_error_reporting; \
    driver_entry->driver.want_event = apm_driver_##name##_want_event; \
    driver_entry->driver.want_stats = apm_driver_##name##_want_stats; \
    driver_entry->driver.is_request_created = 0; \
    driver_entry->next = NULL; \
    return driver_entry; \
  }

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

ZEND_END_MODULE_GLOBALS(webops_event)

ZEND_EXTERN_MODULE_GLOBALS(webops_event)

ZEND_DECLARE_MODULE_GLOBALS(webops_event);







