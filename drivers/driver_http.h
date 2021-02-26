/*
 +----------------------------------------------------------------------+
 |  APM stands for Alternative PHP Monitor                              |
 +----------------------------------------------------------------------+
 | Copyright (c) 2011-2016  David Strauss                                |
 +----------------------------------------------------------------------+
 | This source file is subject to version 3.01 of the PHP license,      |
 | that is bundled with this package in the file LICENSE, and is        |
 | available through the world-wide-web at the following url:           |
 | http://www.php.net/license/3_01.txt                                  |
 | If you did not receive a copy of the PHP license and are unable to   |
 | obtain it through the world-wide-web, please send a note to          |
 | license@php.net so we can mail you a copy immediately.               |
 +----------------------------------------------------------------------+
 | Authors: David Strauss <david@davidstrauss.net>                      |
 +----------------------------------------------------------------------+
*/



#ifndef DRIVER_HTTP_H
#define DRIVER_HTTP_H
#include "../webops_event.h"
#include "php_ini.h"
#include "zend_API.h"

#define WEBOPS_EVENT_E_http WEBOPS_EVENT_E_ALL

webops_event_driver_entry * webops_event_driver_http_create();

PHP_INI_MH(OnUpdateAPMhttpErrorReporting);

#endif
