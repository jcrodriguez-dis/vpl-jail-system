#!/bin/bash
VNCDISPLAY=$(ls $HOME/.vnc/*.log | sed -e "s/[^:]*://" -e "s/\.log$//")
vncserver -kill :$VNCDISPLAY
