PHP_ARG_ENABLE(
 [remote-error-monitor],
 [ Whether to enable the "remote_error_monitor" extension ],
 [ --enable-remote-error-monitor        Enable "remote_error_monitor" extension support ]
)

if test $PHP_REMOTE_ERROR_MONITOR != "no"; then
    if test -r $PHP_CURL/include/curl/easy.h; then
      CURL_DIR=$PHP_CURL
    else
      AC_MSG_CHECKING(for cURL in default path)
      for i in /usr/local /usr; do
        if test -r $i/include/curl/easy.h; then
          CURL_DIR=$i
          AC_MSG_RESULT(found in $i)
          break
        fi
      done
    fi

    PHP_ADD_INCLUDE($CURL_DIR/include)
    PHP_EVAL_LIBLINE($CURL_LIBS, APM_SHARED_LIBADD)
    PHP_ADD_LIBRARY_WITH_PATH(curl, $CURL_DIR/$PHP_LIBDIR, APM_SHARED_LIBADD)

    PHP_CHECK_LIBRARY(curl,curl_easy_perform,
      [
        AC_DEFINE(HAVE_CURL,1,[ ])
      ],[
        AC_MSG_ERROR(There is something wrong. Please check config.log for more information.)
      ],[
        $CURL_LIBS -L$CURL_DIR/$PHP_LIBDIR
      ])

    PHP_ADD_INCLUDE($CURL_DIR/include)
    PHP_NEW_EXTENSION(remote_error_monitor, remote_error_monitor.c backtrace.c, $ext_shared)
    PHP_SUBST(REMOTE_ERROR_MONITOR_SHARED)

fi
