PHP_ARG_ENABLE(
 remote_error_monitor,
 [ Whether to enable the "remote_error_monitor" extension ],
 [ --enable-remote-error-monitor        Enable "remote_error_monitor" extension support ]
)

if test $PHP_REMOTE_ERROR_MONITOR != "no"; then
 PHP_SUBST(REMOTE_ERROR_MONITOR_SHARED)
 PHP_NEW_EXTENSION(remote_error_monitor, remote_error_monitor.c, $ext_shared)
fi
