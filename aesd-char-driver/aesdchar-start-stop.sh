#!/bin/sh

case "$1" in
  start)
    echo "Starting aesdchar"
    aesdchar_load
    ;;
  stop)
    echo "Stopping aesdchar"
    aesdchar_unload
    ;;
  *)
    echo "Usage: $0 [start|stop]"
    exit 1
    ;;
esac

exit 0
