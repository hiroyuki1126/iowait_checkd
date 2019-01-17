#!/bin/bash

. /etc/rc.d/init.d/functions

readonly PROG=/root/bin/iowait_checkd
readonly PROGNAME=$(basename ${PROG})
readonly CONF=/root/conf/iowait_checkd.conf

function error() {
    echo ERROR: $(basename $0): $@ 1>&2
}

function abort() {
    error $@
    exit 1
}

function start() {
    echo -n "Starting ${PROGNAME}: "
    daemon ${PROG} ${CONF}
    echo
}

function stop() {
    echo -n "Stopping ${PROGNAME}: "
    killproc ${PROGNAME}
    echo
}

function reload() {
    echo -n "Reloading ${PROGNAME}: "
    killproc ${PROGNAME} -HUP
    echo
}

[ $(whoami) == 'root' ] || abort 'This program must be run as root.'

[ -x ${PROG} ] || abort "cannot run ${PROG}"

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
        abort "Usage: $(basename $0) {start|stop|restart|reload|status}"
        ;;
esac

exit 0
