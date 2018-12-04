#!/bin/sh

. /etc/rc.d/init.d/functions

PROG=/root/bin/iowait_checkd
PROGNAME=`basename ${PROG}`
CONF=/root/conf/iowait_checkd.conf

error() {
    echo ERROR: `basename $0`: $@ 1>&2
}

abort() {
    error $@
    exit 1
}

start() {
    echo -n "Starting ${PROGNAME}: "
    daemon ${PROG} ${CONF}
    echo
}

stop() {
    echo -n "Stopping ${PROGNAME}: "
    killproc ${PROGNAME}
    echo
}

reload() {
    echo -n "Reloading ${PROGNAME}: "
    killproc ${PROGNAME} -HUP
    echo
}

[ `whoami` == 'root' ] || abort 'This program must be run as root.'

[ -f ${PROG} ] || abort "Program '${PROG}' does not exist."

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        stop
        start
        ;;
    reload)
        reload
        ;;
    status)
        status ${PROGNAME}
        ;;
    *)
        abort "Usage: `basename $0` {start|stop|restart|reload|status}"
        ;;
esac

exit 0
