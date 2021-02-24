#include "zend_types.h"
#include "zend_API.h"
#include "zend_modules.h"

/**
 * DATA STRUCTURES
 *
 *
 * Data structures used by all hooks.
 */

#define PROCESS_EVENT_ARGS int type, char * error_filename, uint64_t error_lineno, char * msg, char * trace
#define RD_DEF(var) zval *var; zend_bool var##_found;



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

