#!/bin/bash
mkdir -p $HOME/.vnc

function vpl_now_ms() {
    local current_time=$(date +%s%3N 2>/dev/null)
    case "$current_time" in
        ''|*[!0-9]*) current_time=$(date +%s); echo $((current_time * 1000)) ;;
        *) echo "$current_time" ;;
    esac
}

start_time=$(vpl_now_ms)
export VPL_START_TIME=$start_time

function vpl_log() {
    local current_time=$(vpl_now_ms)
    local elapsed=$((current_time - start_time))
    echo "$elapsed: $1"
}

function vpl_set_lang() {
    # Set localization and lang
    local NEWLANG
    for NEWLANG in $VPL_LANG en_US.UTF-8 C.utf8 POSIX C
    do
        [ "$(export LC_ALL=$NEWLANG 2>&1)" != "" ] && continue
        break
    done
    vpl_log "LC_ALL => $NEWLANG"
}

function vpl_vncaccel() {
    # Use old VNC accelerate directory
    local VNCACCELDIR=/etc/vncaccel
    [ -d "$VNCACCELDIR" ] && cp -a $VNCACCELDIR/.??* $HOME
}

function vpl_set_vnc_password() {
    # Create and set VNC password file
    VNCPASSWDFILE=$HOME/.vnc/passwd
    local VNCPASSWDSET="$VPL_VNCPASSWD\n$VPL_VNCPASSWD\nn\n"
    if [ -x "$(command -v vncpasswd)" ] ; then
        printf "$VNCPASSWDSET" | vncpasswd -f >$VNCPASSWDFILE
        [ ! -s $VNCPASSWDFILE ] && printf "$VNCPASSWDSET" | vncpasswd
    fi
    # Fallback: generate VNC password file using Python (VNC passwd = bit-reversed password bytes)
    if [ ! -s $VNCPASSWDFILE ] && [ -x "$(command -v python3)" ] ; then
        python3 -c "
import sys
password = sys.argv[1][:8].ljust(8, '\x00')
key = bytes([int(format(ord(c), '08b')[::-1], 2) for c in password])
open(sys.argv[2], 'wb').write(key)
" "$VPL_VNCPASSWD" "$VNCPASSWDFILE"
    fi
    chmod 0600 $VNCPASSWDFILE
    vpl_log "Created VNC password file"
}

function vpl_set_XGEOMETRY_default() {
    # Set default XGEOMETRY
    [ "$VPL_XGEOMETRY" == "" ] && VPL_XGEOMETRY="800x600"
    vpl_log "VPL_XGEOMETRY => $VPL_XGEOMETRY"
}

function vpl_select_VNCPORT() {
    # Select unused VNCPORT
    local VNCPORTRANGE=10000
    local VNCPORTSEARCHLIMIT=100
    function VPL_PORTCHECK {
        lsof -i :\$VNCPORT &>/dev/null
        echo \$?
    }
    if [ -n "$(command -v nc)" ] ; then
        function VPL_PORTCHECK {
           nc -z 127.0.0.1 \$VNCPORT &>/dev/null
           echo \$? 
        }
    fi
    while true; do
        export VNCPORT=$((5900 + $RANDOM % $VNCPORTRANGE))
        [ "$(VPL_PORTCHECK)" != "0" ] && break
        ((limit++)) && ((limit==VNCPORTSEARCHLIMIT)) && break
    done
    export NDIS=$(($VNCPORT - 5900))
    vpl_log "VNCPORT => $VNCPORT"
}

function vpl_generate_cookie {
    for i in {1..8} ; do
        printf '%04X' $RANDOM        
    done
}

function vpl_is_tigervnc {
    if [ -x "$(command -v Xvnc)" ] ; then
        Xvnc -version 2>&1 | grep -qi TigerVNC
        [[ $? -eq 0 ]] && return 0
        Xvnc -help 2>&1 | grep -qi TigerVNC
        [[ $? -eq 0 ]] && return 0
        Xvnc -V 2>&1 | grep -qi TigerVNC
        [[ $? -eq 0 ]] && return 0  
    fi
    return 1
}

function vpl_wait_for_vnc {
    local VNCSTARTUPTIMEOUT=8
    local VNCSTARTUPATTEMPTS=$((VNCSTARTUPTIMEOUT * 10))
    local VNCSTARTUPATTEMPT=0
    local VNC_PORT_CHECK_RESULT
    vpl_log "Waiting for VNC server on port $VNCPORT"
    while [ $VNCSTARTUPATTEMPT -lt $VNCSTARTUPATTEMPTS ] ; do
        if [ -n "$(command -v nc)" ] ; then
            nc -z 127.0.0.1 "$VNCPORT" &>/dev/null
            VNC_PORT_CHECK_RESULT=$?
        elif [ -n "$(command -v lsof)" ] ; then
            lsof -n -P -iTCP:"$VNCPORT" -sTCP:LISTEN &>/dev/null
            VNC_PORT_CHECK_RESULT=$?
        else
            (echo >/dev/tcp/127.0.0.1/"$VNCPORT") &>/dev/null
            VNC_PORT_CHECK_RESULT=$?
        fi
        if [ $VNC_PORT_CHECK_RESULT -eq 0 ] ; then
            vpl_log "VNC server is listening on port $VNCPORT"
            return 0
        fi
        ((VNCSTARTUPATTEMPT++))
        if (( VNCSTARTUPATTEMPT % 10 == 0 )) ; then
            vpl_log "VNC port not ready after ${VNCSTARTUPATTEMPT}00 ms"
        fi
        sleep 0.1
    done
    vpl_log "VNC server did not listen after ${VNCSTARTUPTIMEOUT} seconds"
    return 1
}

function vpl_set_xauth {
    local COOKIE
    COOKIE=$(mcookie 2>/dev/null)
    [ "$?" != "0" ] && COOKIE=$(vpl_generate_cookie)
    export XAUTHORITY=$HOME/.Xauthority
    touch $XAUTHORITY
    if [ -x "$(command -v xauth)" ] ; then
        xauth add :$NDIS . $COOKIE
        [ $? != 0 ] && printf "add :$NDIS . $COOKIE\n" | xauth
        [ $? != 0 ] && printf "add :$NDIS . $COOKIE\nexit\n" | xauth
        vpl_log "Set xauth"
    else
        vpl_log "xauth not found, skipping X authority setup"
    fi
}

function vpl_create_xresources_file {
    export XRESOURCES=$HOME/.Xresources
    cat >$XRESOURCES <<"END_OF_FILE"
Xft.dpi: 100
Xft.antialias: false
Xft.hinting: true
Xft.hintstyle: hintslight
Xft.rgba: rgb
session.screen0.workspaces: 1
END_OF_FILE
    vpl_log "Created resources file"
}

function vpl_create_xstartup_file {
    export XSTARTUPFILE=$HOME/.vnc/xstartup
    cat >$XSTARTUPFILE <<"END_OF_SCRIPT"
#!/bin/bash
export DISPLAY=:$NDIS
export TERM=xterm
unset SESSION_MANAGER
unset SESSION_MANAGER
unset DBUS_SESSION_BUS_ADDRESS

function vpl_log() {
    function vpl_now_ms() {
        local current_time=$(date +%s%3N 2>/dev/null)
        case "$current_time" in
            ''|*[!0-9]*) current_time=$(date +%s); echo $((current_time * 1000)) ;;
            *) echo "$current_time" ;;
        esac
    }
    local current_time=$(vpl_now_ms)
    local elapsed=$((current_time - VPL_START_TIME))
    echo "$elapsed: $1"
}

vpl_log "X startup script initiated"

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

# Waits until X server is running
vpl_log "Waiting X start up"
if [ -x "$(command -v xmodmap)" ] ; then
    while true ; do
        vpl_log "Checking X with xmodmap"
        timeout 1 xmodmap &> /dev/null
        [ $? = 0 ] && break
        sleep 0.1
        ((nwait++))
        [ $nwait -gt 20 ] && break
    done
else
    vpl_log "Waiting 5 seconds"
    sleep 5
fi
vpl_log "X running"

# Configure X setting
[ -x "$(command -v xset)" ] && xset fp= $FONTS &> /dev/null
[ -x "$(command -v xrdb)" ] && xrdb -merge $HOME/.Xresources &> /dev/null
[ -x "$(command -v xsetroot)" ] && xsetroot -solid MidnightBlue &> /dev/null

vpl_log "X options set"

# Activate clipboard
[ -x "$(command -v vncconfig)" ] && vncconfig -iconic 2>/dev/null &
vpl_log "vncconfig running if available"
# Start window manager, falling back if a candidate exits during startup.
function vpl_start_window_manager {
    local WINDOW_MANAGER
    local WINDOW_MANAGER_PID
    for WINDOW_MANAGER in icewm openbox fluxbox metacity ; do
        if [ ! -x "$(command -v "$WINDOW_MANAGER")" ] ; then
            continue
        fi
        if [ "$WINDOW_MANAGER" = "icewm" ] ; then
            mkdir -p .icewm
            echo "Theme=SilverXP/default.theme" > .icewm/theme
        fi
        "$WINDOW_MANAGER" &
        WINDOW_MANAGER_PID=$!
        sleep 0.2
        if kill -0 "$WINDOW_MANAGER_PID" 2>/dev/null ; then
            vpl_log "$WINDOW_MANAGER started with PID $WINDOW_MANAGER_PID"
            return 0
        fi
        wait "$WINDOW_MANAGER_PID" 2>/dev/null
        vpl_log "$WINDOW_MANAGER exited during startup; trying next window manager"
    done
    [ -x "$(command -v xmessage)" ] && xmessage "Window Manager not found"
    vpl_log "No window manager started"
    return 1
}

vpl_start_window_manager
vpl_log "Window manager startup check completed"

# Runs task
OUTPUTFILE=$HOME/.std_output
{
    # Run task
    chmod +x $HOME/vpl_wexecution
    $HOME/vpl_wexecution
} &> $OUTPUTFILE

# Shows task output stdout & stderr if any content
if [ -s $OUTPUTFILE ] ; then
    if [ -x "$(command -v xterm)" ] ; then
        xterm -T "std output" -bg white -fg red -e /bin/bash -c "more $OUTPUTFILE; sleep 3"
    elif [ -x "$(command -v x-terminal-emulator)" ] ; then
        x-terminal-emulator -e /bin/bash -c "more $OUTPUTFILE; sleep 3"
    else
        sleep 5s
    fi
else
    sleep 5s
fi

# Kill X server
ls $HOME/.vnc/*.pid &> /dev/null
[ $? != 0 ] && exit
PIDFILE=$(ls $HOME/.vnc/*.pid)
if [ -x "$(command -v tightvncserver)" ] ; then
    FILENAME=${PIDFILE##*/}
    TIGHTDIS=${FILENAME%.*}
    [ -n "$TIGHTDIS" ] && tightvncserver -kill $TIGHTDIS
fi

if [ -f $PIDFILE ] ; then 
    kill -SIGTERM $(cat $PIDFILE)
    [ $? = 0 ] && sleep 2
    [ -s $PIDFILE ] && kill -SIGKILL $(cat $PIDFILE)
fi
exit
END_OF_SCRIPT
    chmod 0755 $XSTARTUPFILE
    vpl_log "Created xstartup file"
}
{
    vpl_log "VNC setup started"
    . vpl_environment.sh
    vpl_log "VPL environment loaded"
    vpl_set_lang
    vpl_log "VNC acceleration setup started"
    vpl_vncaccel
    vpl_log "VNC acceleration setup finished"
    vpl_set_vnc_password
    vpl_set_XGEOMETRY_default
    vpl_select_VNCPORT
    vpl_set_xauth
    vpl_create_xresources_file
    vpl_create_xstartup_file
    vpl_log "VNC setup finished"
} &>$HOME/.vnc/starting.log

exec 3>&1
{
    PIDFILE=$HOME/.vnc/vncserver.pid
    VNC_SERVER_STARTED=0
    vpl_log "VNC server startup started on port $VNCPORT"
    if [ -x "$(command -v tightvncserver)" ] ; then
        vpl_log "Using tightvncserver"
        tightvncserver \
            -rfbport $VNCPORT \
            -geometry $VPL_XGEOMETRY \
            -localhost \
            -nevershared \
            -name vpl \
            :$NDIS &> $HOME/.vnc/vncserver.log &
        VNC_PID=$!
        VNC_SERVER_STARTED=1
        vpl_log "tightvncserver started with PID $VNC_PID"
    elif [ -x "$(command -v Xvnc)" ] ; then
        if vpl_is_tigervnc ; then
            vpl_log "Using TigerVNC with Xvnc"
            $XSTARTUPFILE &
            XSTARTUP_PID=$!
            vpl_log "X startup script started with PID $XSTARTUP_PID"
            {
                echo "rfbport=$VNCPORT"
                echo "geometry $VPL_XGEOMETRY"
                echo "localhost"
                echo "nevershared"
            } > $HOME/.vnc/config
            Xvnc \
                -rfbport=$VNCPORT \
                -nevershared \
                -localhost \
                -SecurityTypes=VncAuth \
                -PasswordFile=$VNCPASSWDFILE \
                -geometry $VPL_XGEOMETRY \
                -desktop vpl$NDIS \
                :$NDIS &> $HOME/.vnc/vncserver.log &
            VNC_PID=$!
            VNC_SERVER_STARTED=1
            echo -n "$VNC_PID $$" > $PIDFILE
            vpl_log "TigerVNC started with PID $VNC_PID"
        else
            vpl_log "Using TightVNC-compatible Xvnc"
            $XSTARTUPFILE &
            XSTARTUP_PID=$!
            vpl_log "X startup script started with PID $XSTARTUP_PID"
            echo "Xvnc"
            {
                echo "rfbport=$VNCPORT"
                echo "geometry $VPL_XGEOMETRY"
                echo "localhost"
                echo "nevershared"
            } > $HOME/.vnc/config
            Xvnc \
                -rfbport=$VNCPORT \
                -nevershared \
                -localhost \
                -SecurityTypes=VncAuth \
                -geometry $VPL_XGEOMETRY \
                -name vpl$NDIS \
                :$NDIS &> $HOME/.vnc/vncserver.log &
            VNC_PID=$!
            VNC_SERVER_STARTED=1
            echo -n "$VNC_PID $$" > $PIDFILE
            vpl_log "TightVNC-compatible Xvnc started with PID $VNC_PID"
        fi
    else
        vpl_log "No VNC server found (Xvnc or tightvncserver)"
    fi
    if [ $VNC_SERVER_STARTED -eq 1 ] ; then
        vpl_wait_for_vnc
    else
        vpl_log "Skipping VNC readiness check because no server was started"
    fi
    vpl_log "VNC server startup commands completed"
    # IMPORTANT: Do not remove next line
    echo $VNCPORT >&3
    sleep 1000d
}  &>> $HOME/.vnc/starting.log
