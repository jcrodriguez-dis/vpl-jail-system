#!/bin/bash
{
	. vpl_environment.sh
	export LC_ALL=$VPL_LANG 2> .vpl_set_locale_error
	#if current lang not available use en_US.utf8
	if [ -s .vpl_set_locale_error ] ; then
		export LC_ALL=en_US.utf8
		rm .vpl_set_locale_error
	fi
	mkdir .vnc
	VNCACCELDIR=/etc/vncaccel
	if [ -d "$VNCACCELDIR" ] ; then
		cp -a $VNCACCELDIR/.??* $HOME
	fi
	#try vcnpasswd: 1-tightvnc 2:tigervnc
	printf "$VPL_VNCPASSWD\n$VPL_VNCPASSWD\n" | vncpasswd -f >$HOME/.vnc/passwd
	if [ ! -s $HOME/.vnc/passwd ] ; then
		printf "$VPL_VNCPASSWD\n$VPL_VNCPASSWD\n" | vncpasswd
	fi
	chmod 0600 $HOME/.vnc/passwd
	cat >$HOME/.vnc/xstartup <<"END_OF_FILE"
#!/bin/bash
export TERM=xterm
unset SESSION_MANAGER
xrdb
xsetroot -solid MidnightBlue 
#activate clipboard
[ -x vncconfig ] && vncconfig -iconic 2>/dev/null &
#start window manager
if [ -x "$(command -v icewm)" ] ; then
	mkdir .icewm
	echo "Theme=SilverXP/default.theme" > .icewm/theme
	icewm &
elif [ -x "$(command -v fluxbox)" ] ; then
	fluxbox &
elif [ -x "$(command -v openbox)" ] ; then
	openbox &
elif [ -x "$(command -v metacity)" ] ; then
	metacity &
else
	xmessage "Window Manager not found"
fi
#set execution mode
chmod +x $HOME/vpl_wexecution
$HOME/vpl_wexecution &> .std_output
if [ -s .std_output ] ; then
	if [ "$(command -v xterm)" != "" ] ; then
		xterm -T "std output" -bg white -fg red -e less .std_output
	else
		if [ "$(command -v x-terminal-emulator)" != "" ] ; then
			x-terminal-emulator -e less .std_output
		fi
	fi
fi
VNCDISPLAY=$(ls $HOME/.vnc/*.log | sed -e "s/[^:]*://" -e "s/\.log$//")
vncserver -kill :$VNCDISPLAY
END_OF_FILE
	if [ "$VPL_XGEOMETRY" == "" ] ; then
		VPL_XGEOMETRY="800x600"
	fi
	chmod 0755 $HOME/.vnc/xstartup
	while true; do
		export VNCPORT=$((6000+$RANDOM%25000))
		lsof -i :$VNCPORT
		[ "$?" != "0" ] && break
	done
	export DISPLAY=:$VNCPORT.0
	nohup vncserver :$VNCPORT -rfbport $VNCPORT -localhost -geometry $VPL_XGEOMETRY -nevershared -name vpl
} &>/dev/null
echo $VNCPORT

