#include "php_webops_event.h"

zend_module_entry webops_event_module_entry = {
  #if ZEND_MODULE_API_NO >= 20010901
   STANDARD_MODULE_HEADER,
  #endif
   PHP_WEBOPS_EVENT_EXTNAME,
   NULL, /* Functions */
   NULL, /* MINIT */
   NULL, /* MSHUTDOWN */
   NULL, /* RINIT */
   NULL, /* RSHUTDOWN */
   NULL, /* MINFO */
  #if ZEND_MODULE_API_NO >= 20010901
   PHP_WEBOPS_EVENT_EXTVER,
  #endif
   STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_WEBOPS_EVENT
  ZEND_GET_MODULE(webops_event)
#endif
