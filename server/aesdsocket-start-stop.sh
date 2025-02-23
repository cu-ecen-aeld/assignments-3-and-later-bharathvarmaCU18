#!/bin/sh

# Startup script for aesdsocket daemon

NAME=aesdsocket
EXEC="aesdsocket" 
echo $EXEC
ARGS="-d"

case "$1" in
  start)
    echo "Starting $NAME"
    if [ ! -x "$EXEC" ]; then
      echo "Error: $EXEC not found or not executable"
      exit 1
    fi
    start-stop-daemon -S -n $NAME -a $EXEC -- $ARGS
    ;;
  stop)
    echo "Stopping $NAME"
    start-stop-daemon -K -n $NAME
    ;;
  *)
    echo "Usage: $0 {start|stop}"
    exit 1
    ;;
esac

exit 0

