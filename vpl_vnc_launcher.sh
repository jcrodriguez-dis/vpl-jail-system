#!/bin/bash
export LC_ALL=$VPL_LANG 2> .vpl_set_locale_error
#if current lang not available use en_US.utf8
if [ -s .vpl_set_locale_error ] ; then
	export LC_ALL=en_US.utf8
	rm .vpl_set_locale_error
fi
mkdir .vnc
printf "$VPL_VNCPASSWD\n$VPL_VNCPASSWD\n" | vncpasswd -f >$HOME/.vnc/passwd
chmod 0600 $HOME/.vnc/passwd
cat >$HOME/.vnc/xstartup <<"END_OF_FILE"
#!/bin/bash
unset SESSION_MANAGER
xrdb
xsetroot -solid MidnightBlue 
#activate clipboard
[ -x vncconfig ] && vncconfig -iconic 2>/dev/null &
#start window manager
metacity &
#set execution mode
chmod +x $HOME/vpl_wexecution
$HOME/vpl_wexecution &> .std_output
if [ -s .std_output ] ; then
	xterm -T "std output" -bg white -fg red -e less .std_output
fi
VNCDISPLAY=$(ls $HOME/.vnc/*.log | sed -e "s/[^:]*://" -e "s/\.log$//")
vncserver -kill :$VNCDISPLAY
END_OF_FILE
chmod 0755 $HOME/.vnc/xstartup
export DISPLAY=:$UID.0
nohup vncserver :$UID -rfbport $UID -localhost -geometry 800x600 -nevershared -name vpl

