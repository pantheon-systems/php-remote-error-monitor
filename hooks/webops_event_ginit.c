#include "../webops_event.h"
#include "../../standard/info.h"

PHP_INI_BEGIN()
  /* Boolean controlling whether the extension is globally active or not */
  STD_PHP_INI_BOOLEAN("webops_event.enabled", "1", PHP_INI_SYSTEM, OnUpdateBool, enabled, zend_apm_globals, apm_globals)
  /* Application identifier, helps identifying which application is being monitored */
  STD_PHP_INI_ENTRY("webops_event.application_id", "My application", PHP_INI_ALL, OnUpdateString, application_id, zend_apm_globals, apm_globals)
  /* Boolean controlling whether the event monitoring is active or not */
  STD_PHP_INI_BOOLEAN("webops_event.event_enabled", "1", PHP_INI_ALL, OnUpdateBool, event_enabled, zend_apm_globals, apm_globals)

  /* Boolean controlling whether the ip should be stored or not */
  STD_PHP_INI_BOOLEAN("webops_event.store_ip", "1", PHP_INI_ALL, OnUpdateBool, store_ip, zend_apm_globals, apm_globals)
  /* Boolean controlling whether the cookies should be stored or not */
  STD_PHP_INI_BOOLEAN("webops_event.store_cookies", "1", PHP_INI_ALL, OnUpdateBool, store_cookies, zend_apm_globals, apm_globals)
  /* Boolean controlling whether the POST variables should be stored or not */
  STD_PHP_INI_BOOLEAN("webops_event.store_post", "1", PHP_INI_ALL, OnUpdateBool, store_post, zend_apm_globals, apm_globals)
  /* Time (in ms) before a request is considered for stats */
  STD_PHP_INI_ENTRY("webops_event.stats_duration_threshold", "100", PHP_INI_ALL, OnUpdateLong, stats_duration_threshold, zend_apm_globals, apm_globals)

  #ifdef HAVE_GETRUSAGE
  /* User CPU time usage (in ms) before a request is considered for stats */
    STD_PHP_INI_ENTRY("webops_event.stats_user_cpu_threshold", "100", PHP_INI_ALL, OnUpdateLong, stats_user_cpu_threshold, zend_apm_globals, apm_globals)
    /* System CPU time usage (in ms) before a request is considered for stats */
    STD_PHP_INI_ENTRY("webops_event.stats_sys_cpu_threshold", "10", PHP_INI_ALL, OnUpdateLong, stats_sys_cpu_threshold, zend_apm_globals, apm_globals)
  #endif
  /* Maximum recursion depth used when dumping a variable */
  STD_PHP_INI_ENTRY("webops_event.dump_max_depth", "1", PHP_INI_ALL, OnUpdateLong, dump_max_depth, zend_apm_globals, apm_globals)

  /* Boolean controlling whether the driver is active or not */
  STD_PHP_INI_BOOLEAN("webops_event.http_enabled", "1", PHP_INI_ALL, OnUpdateBool, http_enabled, zend_apm_globals, apm_globals)
  /* Boolean controlling the collection of stats */
  STD_PHP_INI_BOOLEAN("webops_event.http_stats_enabled", "1", PHP_INI_ALL, OnUpdateBool, http_stats_enabled, zend_apm_globals, apm_globals)
  /* Control which exceptions to collect (0: none exceptions collected, 1: collect uncaught exceptions (default), 2: collect ALL exceptions) */
  STD_PHP_INI_ENTRY("webops_event.http_exception_mode","1", PHP_INI_PERDIR, OnUpdateLongGEZero, http_exception_mode, zend_apm_globals, apm_globals)
  /* error_reporting of the driver */
  STD_PHP_INI_ENTRY("webops_event.http_error_reporting", NULL, PHP_INI_ALL, OnUpdateAPMhttpErrorReporting, http_error_reporting, zend_apm_globals, apm_globals)
  /* process silenced events? */
  STD_PHP_INI_BOOLEAN("webops_event.http_process_silenced_events", "1", PHP_INI_PERDIR, OnUpdateBool, http_process_silenced_events, zend_apm_globals, apm_globals)
  STD_PHP_INI_ENTRY("webops_event.http_request_timeout", "1000", PHP_INI_ALL, OnUpdateLong, http_request_timeout, zend_apm_globals, apm_globals)
  STD_PHP_INI_ENTRY("webops_event.http_server", "http://localhost", PHP_INI_ALL, OnUpdateString, http_server, zend_apm_globals, apm_globals)
  STD_PHP_INI_ENTRY("webops_event.http_client_certificate", NULL, PHP_INI_ALL, OnUpdateString, http_client_certificate, zend_apm_globals, apm_globals)
  STD_PHP_INI_ENTRY("webops_event.http_client_key", NULL, PHP_INI_ALL, OnUpdateString, http_client_key, zend_apm_globals, apm_globals)
  STD_PHP_INI_ENTRY("webops_event.http_certificate_authorities", NULL, PHP_INI_ALL, OnUpdateString, http_certificate_authorities, zend_apm_globals, apm_globals)
  STD_PHP_INI_ENTRY("webops_event.http_max_backtrace_length", "0", PHP_INI_ALL, OnUpdateLong, http_max_backtrace_length, zend_apm_globals, apm_globals)
PHP_INI_END()


/* Extension globals */
ZEND_BEGIN_MODULE_GLOBALS(apm)
  /* Boolean controlling whether the extension is globally active or not */
  zend_bool enabled;
  /* Application identifier, helps identifying which application is being monitored */
  char      *application_id;
  /* Boolean controlling whether the event monitoring is active or not */
  zend_bool event_enabled;
  /* Boolean controlling whether the stacktrace should be generated or not */
  zend_bool store_stacktrace;
  /* Boolean controlling whether the ip should be generated or not */
  zend_bool store_ip;
  /* Boolean controlling whether the cookies should be generated or not */
  zend_bool store_cookies;
  /* Boolean controlling whether the POST variables should be generated or not */
  zend_bool store_post;
  /* Time (in ms) before a request is considered for stats */
  long      stats_duration_threshold;
  /* User CPU time usage (in ms) before a request is considered for stats */
  long      stats_user_cpu_threshold;
  /* System CPU time usage (in ms) before a request is considered for stats */
  long      stats_sys_cpu_threshold;
  /* Maximum recursion depth used when dumping a variable */
  long      dump_max_depth;
  /* Determines whether we're currently silenced */
  zend_bool currently_silenced;

  apm_driver_entry *drivers;
  smart_str *buffer;

  /* Structure used to store request data */
  apm_request_data request_data;

  float duration;

  long mem_peak_usage;
  #ifdef HAVE_GETRUSAGE
  float user_cpu;

    float sys_cpu;
  #endif

  #ifdef APM_DEBUGFILE
  FILE * debugfile;
  #endif

  #ifdef APM_DRIVER_HTTP
  /* Boolean controlling whether the driver is active or not */
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
  #endif

ZEND_END_MODULE_GLOBALS(apm)

ZEND_EXTERN_MODULE_GLOBALS(apm)



static WEBOPS_EVENT_GINIT_FUNCTION(webops_event) {
    apm_driver_entry **next;
    webops_event_globals->buffer = NULL;
    webops_event_globals->drivers = (apm_driver_entry *) malloc(sizeof(apm_driver_entry));
    webops_event_globals->drivers->driver.process_event = (void (*)(PROCESS_EVENT_ARGS)) NULL;
    webops_event_globals->drivers->driver.process_stats = (void (*)()) NULL; c
    webops_event_globals->drivers->driver.minit = (int (*)(int )) NULL;
    webops_event_globals->drivers->driver.rinit = (int (*)()) NULL;
    webops_event_globals->drivers->driver.mshutdown = (int (*)()) NULL;
    webops_event_globals->drivers->driver.rshutdown = (int (*)()) NULL;
    next = &webops_event_globals->drivers->next;
    *next = (apm_driver_entry *) NULL;
    *next = apm_driver_http_create();
}