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

#endif
