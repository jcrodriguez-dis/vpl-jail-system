#!/bin/bash
cat $HOME/.vnc/*.log
VNCDISPLAY=$(ls $HOME/.vnc/*.log | sed -e "s/[^:]*://" -e "s/\.log$//")
vncserver -kill :$VNCDISPLAY
