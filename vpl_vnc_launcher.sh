#!/bin/bash
mkdir -p $HOME/.vnc
{
    . vpl_environment.sh
    for NEWLANG in $VPL_LANG en_US.UTF-8 C.utf8 POSIX C
    do
        [ "$(export LC_ALL=$NEWLANG 2>&1)" != "" ] && continue
        break
    done
    echo "LC_ALL => $NEWLANG"
    VNCACCELDIR=/etc/vncaccel
    [ -d "$VNCACCELDIR" ] && cp -a $VNCACCELDIR/.??* $HOME
    VNCPASSWDFILE=$HOME/.vnc/passwd
    VNCPASSWDSET="$VPL_VNCPASSWD\n$VPL_VNCPASSWD\nn\n"
    printf "$VNCPASSWDSET" | vncpasswd -f >$VNCPASSWDFILE
    [ ! -s $VNCPASSWDFILE ] && 	printf "$VNCPASSWDSET" | vncpasswd
    chmod 0600 $VNCPASSWDFILE

    [ "$VPL_XGEOMETRY" == "" ] && VPL_XGEOMETRY="800x600"
    echo "VPL_XGEOMETRY => $VPL_XGEOMETRY"
    if [ "$(command -v nc)" != "" ] ; then
        while true; do
            export VNCPORT=$((5900+$RANDOM%95))
            [ "$(nc -z 127.0.0.1 $VNCPORT ; echo $?)" == "1" ] && break
            ((limit++)) && ((limit==100)) && break
        done
    else
        while true; do
            export VNCPORT=$((5900+$RANDOM%95))
            lsof -i :$VNCPORT &>/dev/null
            [ "$?" != "0" ] && break
            ((limit++)) && ((limit==100)) && break
        done
    fi
    echo "VNCPORT => $VNCPORT"
    ((NDIS=$VNCPORT-5900))
    export NDIS
    COOKIE=$(mcookie)
    [ "$?" != "0" ] && COOKIE="$RANDOM$RANDOM$RANDOM$RANDOM$RANDOM"
    touch $HOME/.Xauthority
    printf "add :$NDIS . ${COOKIE:1:10}\nexit\n" | xauth
    export XSTARTUPFILE=$HOME/.vnc/xstartup
    cat >$XSTARTUPFILE <<"END_OF_SCRIPT"
#!/bin/bash
export DISPLAY=:$NDIS
export TERM=xterm
unset SESSION_MANAGER
unset SESSION_MANAGER
unset DBUS_SESSION_BUS_ADDRESS
FONTPATHS=( '/usr/share/X11/fonts' '/usr/share/fonts/X11/' '/usr/lib/X11/fonts'
            '/usr/X11/lib/X11/fonts' '/usr/X11R6/lib/X11/fonts' '/usr/X11/lib/X11/fonts' )
FTYPES=( 'misc' '75dpi' '100dpi' 'Speedo' 'Type1' )
for FONTPATH in "${FONTPATHS[@]}" ; do
    if [ -d $FONTPATH ] ; then
        for FTYPE in "${FTYPES[@]}" ; do
            FONT=${FONTPATH}/${FTYPE}
            if [ -f "${FONT}/fonts.dir" ] ; then
                [ -z "${FONTS}" ] && FONTS="${FONT}"
                [ -n "${FONTS}" ] && FONTS="${FONTS},${FONT}"
            fi
        done
    fi
done

[ "$1" == "xinitrc" ] && [ -x /etc/X11/xinit/xinitrc ] && /etc/X11/xinit/xinitrc
[ -x "$(command -v xset)" ] && xset fp= $FONTS
[ -x "$(command -v xsetroot)" ] && xsetroot -solid MidnightBlue 
#activate clipboard
[ -x "$(command -v vncconfig)" ] && vncconfig -iconic 2>/dev/null &
#start window manager
if [ -x "$(command -v icewm)" ] ; then
    mkdir -p .icewm
    echo "Theme=SilverXP/default.theme" > .icewm/theme
    icewm &
elif [ -x "$(command -v fluxbox)" ] ; then
    fluxbox &
elif [ -x "$(command -v openbox)" ] ; then
    openbox &
elif [ -x "$(command -v metacity)" ] ; then
    metacity &
else
    [ -x "$(command -v xmessage)" ] && xmessage "Window Manager not found"
fi
# Set execution mode
chmod +x $HOME/vpl_wexecution &> .std_output
$HOME/vpl_wexecution &> .std_output
if [ -s .std_output ] ; then
    if [ -x "$(command -v xterm)" ] ; then
        xterm -T "std output" -bg white -fg red -e more .std_output
    else
        if [ -x "$(command -v x-terminal-emulator)" ] ; then
            x-terminal-emulator -e more .std_output
        fi
    fi
fi

END_OF_SCRIPT

    chmod 0755 $XSTARTUPFILE
} &>$HOME/.vnc/starting.log
echo $VNCPORT
{

    if [ -x "$(command -v tightvncserver)" ] ; then
        echo "Using tightvncserver"
        nohup tightvncserver \
            -rfbport $VNCPORT \
            -geometry $VPL_XGEOMETRY \
            -localhost \
            -nevershared \
            -name vpl \
            :$NDIS > $HOME/.vnc/vncserver.log
    elif [ -x "$(command -v Xvnc)" ] ; then
        if [ "$(Xvnc -version | grep -q TigerVNC ; echo $?)" = "0" ] ; then
            echo "Using TigerVNC with Xvnc"
            $XSTARTUPFILE xinitrc &
            {
                echo "rfbport=$VNCPORT"
                echo "geometry $VPL_XGEOMETRY"
                echo "localhost"
                echo "nevershared"
            } > $HOME/.vnc/config
            nohup Xvnc \
                -rfbport=$VNCPORT \
                -nevershared \
                -localhost \
                -SecurityTypes=VncAuth \
                -PasswordFile=$VNCPASSWDFILE \
                -geometry $VPL_XGEOMETRY \
                -desktop vpl$NDIS \
                :$NDIS > $HOME/.vnc/vncserver.log
        else
            echo "Using Tightvnc with Xvnc"
            $XSTARTUPFILE xinitrc &
            echo "Xvnc"
            {
                echo "rfbport=$VNCPORT"
                echo "geometry $VPL_XGEOMETRY"
                echo "localhost"
                echo "nevershared"
            } > $HOME/.vnc/config
            nohup Xvnc \
                -rfbport=$VNCPORT \
                -nevershared \
                -localhost \
                -SecurityTypes=VncAuth \
                -geometry $VPL_XGEOMETRY \
                -name vpl$NDIS \
                :$NDIS > $HOME/.vnc/vncserver.log
        fi
    fi
    sleep 10d
} &>>$HOME/.vnc/starting.log
