#include "php_webops_event.h"

zend_module_entry webops_event_module_entry = {
   STANDARD_MODULE_HEADER,
   PHP_WEBOPS_EVENT_EXTNAME,
   NULL, /* Functions */
   NULL, /* MINIT */
   NULL, /* MSHUTDOWN */
   NULL, /* RINIT */
   NULL, /* RSHUTDOWN */
   NULL, /* MINFO */
   PHP_WEBOPS_EVENT_EXTVER,
   STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_WEBOPS_EVENT
  ZEND_GET_MODULE(webops_event)
#endif
