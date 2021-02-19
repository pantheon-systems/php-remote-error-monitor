PHP_ARG_ENABLE(
 webops_event,
 [ Whether to enable the "webops_event" extension ],
 [ --enable-webops-event        Enable "webops_event" extension support ],
 yes,
 no
)

if test $PHP_WEBOPS_EVENT != "no"; then
 PHP_SUBST(WEBOPS_EVENT_SHARED)
 PHP_NEW_EXTENSION(webops_event, webops_event.c, $ext_shared)
fi
