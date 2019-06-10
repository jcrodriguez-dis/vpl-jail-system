#!/bin/bash
# package:		Part of vpl-jail-system
# copyright:    Copyright (C) 2017 Juan Carlos Rodriguez-del-Pino
# license:      GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl.txt
# Description:  Script to copy configuration of first start of VNC and browser.
#               this configuration files will be used to accelerate start of VNC
#               in jail

#Check current configuration
function usage {
	echo "vncaccel.sh 1.0"
	echo "usage: vncaccel.sh [full path to the model user dir]"
	echo "-------------------------------------------------------------"
	echo "To generate the user model"
	echo "  1) Set configuration DEBUGLEVEL to 8"
	echo "  2) Run in VPL GUI mode (e.g write a index.html file)"
	echo "  3) Save the user dir path"
	echo "  4) Inside VNC run any software that you want to accelerate its load"
	echo "      e.g.: firefox, a java program, jgrasp, ddd, etc."
	echo "This script copy the user dir/.dirs to/etc/vncaccel"
	echo "After running this script you must restart the service"
}
if [ "$USER" != "root" ] ; then
	echo "Error: You must be root"
    usage
    exit 1
fi
if [ ! -d "$1" ] ; then
	echo "Error: You must set a valid dir"
    usage
    exit 1
fi
VNCACCELDIR=/etc/vncaccel
#Create dir
if [ -d $VNCACCELDIR ] ;then
	rm -Rf $VNCACCELDIR
fi
mkdir $VNCACCELDIR
chmod a+xr $VNCACCELDIR
cp -a $1/.??* $VNCACCELDIR
#Clean directory
rm -Rf $VNCACCELDIR/.vnc > /dev/null
rm $VNCACCELDIR/.??* 2> /dev/null
#Change access mode
chown -R nobody.nogroup $VNCACCELDIR/.??* 2>/dev/null
if [ $? -ne 0 ] ; then
    chown -R nobody.nobody $VNCACCELDIR/.??* 2>/dev/null
fi
find $VNCACCELDIR -type d -print0 | xargs -0 chmod a+xr
find $VNCACCELDIR -type f -print0 | xargs -0 chmod a+x



