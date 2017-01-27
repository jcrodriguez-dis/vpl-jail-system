#!/bin/bash
# package:		Part of vpl-jail-system
# copyright:    Copyright (C) 2017 Juan Carlos Rodriguez-del-Pino
# license:      GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl.txt
# Description:  Script to copy configuration of first start of VNC and browser.
#               this configuration files will be used to accelerate start of VNC
#               in jail

#Check current configuration
ls -d $HOME/.??*/
if [ "$?" == "2" ] ; then
	echo "I can not generate the VNC accelerate directory due to existence of"
	echo "local configuration directories. Save it and try again"
	echo
	echo "Directories to save:"
	ls -d $HOME/.??*/
	exit 0
fi
#Prepare VNC launch
VNCACCELDIR=/etc/vncaccel
VPL_LANG=en_US.utf8
VPL_VNCPASSWD="$RANDOM$RANDOM"
cp vpl_vnc_launcher.sh $HOME
cp vpl_vnc_stopper.sh $HOME
cd $HOME
touch .vpl_environment.sh
chmod +x .vpl_environment.sh
cat > vpl_wexecution << END_OF_SCRIPT
x-www-browser & >/dev/null
firefox & >/dev/null
ddd & > /dev/null
END_OF_SCRIPT
./vpl_vnc_launcher &
echo "VNC launched at port number shown and using password $VPL_VNCPASSWD"
echo "You can use a vcnclient to configure better the software"
echo "Waiting 1 minute to start software first time and then will be closed"
sleep 60
./vpl_vnc_stopper.sh
#Copy results
VNCACCELDIR=/etc/vncaccel
if [ -d "$VNCACCELDIR" ] ; then
	rm -Rf $VNCACCELDIR
fi
mkdir "$VNCACCELDIR"
cp -a .??* "$VNCACCELDIR"
rm $VNCACCELDIR/* 2> /dev/null
chmod -R +r "$VNCACCELDIR"


