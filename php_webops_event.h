#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_errors.h"
#include "zend_modules.h"
#include "zend_smart_str.h"
#include "zend_types.h"
#include "zend_API.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"

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


#include "webops_event.h"
#include "hooks/webops_event_ginit.h"
#include "hooks/webops_event_minit.h"
#include "hooks/webops_event_rinit.h"
#include "hooks/webops_event_minfo.h"
#include "hooks/webops_event_gshutdown.h"
#include "hooks/webops_event_mshutdown.h"
#include "hooks/webops_event_rshutdown.h"

zend_module_entry webops_event_module_entry = {
    STANDARD_MODULE_HEADER,
    WEBOPS_EVENT_EXTNAME,
    NULL,
    WEBOPS_EVENT_MINIT_FUNCTION(webops_event),
    WEBOPS_EVENT_MSHUTDOWN_FUNCTION(webops_event),
    WEBOPS_EVENT_RINIT_FUNCTION(webops_event),
    WEBOPS_EVENT_RSHUTDOWN_FUNCTION(webops_event),
    WEBOPS_EVENT_MINFO_FUNCTION(webops_event),
    WEBOPS_EVENT_EXTVER,
    PHP_MODULE_GLOBALS(webops_event),
    WEBOPS_EVENT_GINIT_FUNCTION(webops_event),
    WEBOPS_EVENT_GSHUTDOWN_FUNCTION(webops_event),
    NULL,
    STANDARD_MODULE_PROPERTIES_EX
};