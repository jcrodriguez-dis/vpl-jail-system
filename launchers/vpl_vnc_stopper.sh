#!/bin/bash
# Kill vncserver
PIDFILE=$(ls $HOME/.vnc/*.pid)
if [ -x "$(command -v tightvncserver)" ] ; then
    FILENAME=${PIDFILE##*/}
    TIGHTDIS=${FILENAME%.*}
    [ -n "$TIGHTDIS" ] && tightvncserver -kill $TIGHTDIS
fi

if [ -s $PIDFILE ] ; then 
    kill -SIGTERM $(cat $PIDFILE)
    [ $? = 0 ] && sleep 2
    [ -s $PIDFILE ] && kill -SIGKILL $(cat $PIDFILE)
fi
