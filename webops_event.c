#include "zend_extensions.h"
#include "php_webops_event.h"
#include "Zend/zend_smart_str.h"

#include "webops_event.h"

#define PRINT(what) fprintf(stderr, what "\n");


static int webops_event_zend_extension_startup(zend_extension *ext) {
  PRINT("WEBOPS_EVENT zend hook startup!");
  return SUCCESS;
}

static void webops_event_zend_extension_shutdown(zend_extension *ext) {
  PRINT("WEBOPS_EVENT zend hook shutdown!");
}


static void webops_event_zend_extension_activate(void) {
  PRINT("WEBOPS_EVENT zend hook activate!");
}

static void webops_event_zend_extension_deactivate(void) {
  PRINT("WEBOPS_EVENT zend hook deactivate!");
}

static void webops_event_zend_extension_message_handler(int code, void *ext) { }

static void webops_event_zend_extension_op_array_handler(zend_op_array *op_array) { }

static void webops_event_zend_extension_fcall_begin_handler(zend_execute_data *ex) { }



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
    webops_event_zend_extension_startup,      /* startup() : module startup */
    webops_event_zend_extension_shutdown,   /* shutdown() : module shutdown */
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
