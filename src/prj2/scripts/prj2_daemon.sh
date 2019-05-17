#!/bin/sh
#

case "$1" in
  start)
    /usr/bin/ee242_prj2 -d
    ;;
  stop)
    killall -KILL ee242_prj2
    ;;
  restart|reload)
    "$0" stop
    "$0" start
    ;;
  *)
    echo $"Usage: $0 {start|stop|restart}"
    exit 1
esac

exit $?
