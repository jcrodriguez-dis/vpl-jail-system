# VPL-JAIL-SYSTEM 2.5.2

[![Build Status](https://travis-ci.org/aitorSDL/vpl-xmlrpc-jail.svg?branch=2.5.4)](https://travis-ci.org/github/aitorSDL/vpl-xmlrpc-jail)
![VPL Logo](https://vpl.dis.ulpgc.es/images/logo2.png)

The VPL-Jail-System serves an execution sandbox for the VPL Moodle plugin. This sandbox provides interactive execution, textual by xterm and graphical by VNC, and non-iterative execution for code evaluation purpose.

For more details about VPL, visit the [VPL home page](http://vpl.dis.ulpgc.es) or
the [VPL plugin page at Moodle](http://www.moodle.org/plugins/mod_vpl).
# Requirements
The VPL-Jail-System is an open software execution system and requires a specific environment. 

## Software requirements 

The VPL-Jail-System 2.4 requires a Linux O.S with YUM or APT as package manager and systemd or system V as service manager. The system has been tested on Debian, Ubuntu and CentOS.

O.S.   | Version | Arch.   | Results
-------|---------|---------|----------------
Ubuntu | 18.04   | 32b/64b | Compatible
Ubuntu | 16.04   | 32b/64b | Compatible
Ubuntu | 14.04   | 32b/64b | Not functional due to the lack of OverlayFS
Debian | 9       | 32b/64b | Compatible
Debian | 10      | 32b/64b | Compatible
CentOS | 7       | 64b     | GUI programs not available. Requires to disable or configure SELinux  
CentOS | 6       |         | Not functional

## Hardware requirements

The system has been developed to offers immediate and interactive execution of student's programs. This means that the system can attend multiple-executions simultaneously.

The hardware required to accomplish this task depends on the number of simultaneous executions at a time, the requisites of the program, and the programming language used. For example, a PHP Web program may require a huge amount of RAM, especially for the Web Browser execution, but a Python program may need one hundred times less of RAM.

Our experience is that a machine with only 2Gb of RAM and 2 cores can support a class with 50 students online using Java (Non-GUI). If you are conducting an exam the hardware required may be tripled. Possibly the critical resource may be the RAM. If the system exhausts the RAM the O.S. will start swapping and the throughput will decrease drastically. Our tests indicate that the 32-bit O.S. uses less memory and CPU than the 64-bit version. Remember that you can add (or remove) VPL-Jail-systems to a VPL installation online.

## Installation

### Selecting the hardware
The recommended option is using a dedicated machine. If you can not use a dedicated machine try using a Virtual Machine e.g. using VirtualBox. This will aisle and limit the resources used by the service.
If you decide to use other services in the same machine that the use of resources by VPL-Jail-System may decrease the performance of the others service. Although no security breach has been reported, notice that the nature of the service (execute external code) leads to an intrinsic threat.

### Preparing the system
Install a Linux O.S. as clean as possible. If you have enough resources you can install a GUI interface. Stop any service that you don't need as web server, ssh server, etc. If the O.S. has a firewall, you must configure it (or stop it) to give access to the only two ports needed by the VPL-Jail-System. If you use automatic updates, you must restart the VPL-Jail-System to take into account the update. You can use cron to automate this process.

### Getting VPL-Jail-System
VPL-Jail-System is distributed only as source files. You must get the source package from https://vp.dis.ulpgc.es eg. using
```shell
wget https://vpl.dis.ulpgc.es/releases/vpl-jail-system-[version].tar.gz
```

or from the github repository, generating the package with
```shell
make distcheck
```

## Running the installer

After getting the package you must decompress it and run the installer.
```shell
tar xvf vpl-jail-system-[version].tar.gz
cd vpl-jail-system-[version]
./install-vpl-sh
```

The "./install-vpl-sh" must be run as root.

Follow the instructions and wait for the necessary downloads. The installation script will try to install the development software commonly used.

The installer will ask you about:
- If you want that the installer creates a self-signed SSL certificate.
- (updating) If you want to replace the configuration file with a fresh one.
- If you want to install different compilers and interpreters.


## Updating VPL-Jail-System
If you want to update VPL-Jail-System follow the same steps that the first installation. The installer will update the current version.

# Removing VPL-Jail-System
Run uninstall-sh of the current version.

# Configuration

After installing the VPL-Jail-Service, the service will be started with a default configuration. If you want to change the configuration you must edit the file */etc/vpl/vpl-jail-system.conf*.

After configuration changes, you must restart (as user root) the service to use the new configuration values.
Using systemd
```shell
systemctl restart vpl-jail-system
```
or using system V
```shell
service vpl-jail-system restart
```

## Main configuration parameters
- PORT. Socket port number to listen for http and ws connections. The default value is 80
- SECURE_PORT. Socket port number to listen for https and wss connections. Default value 443
- URLPATH. Act as a password, if no matches with the path of the URL request then it's rejected. The default value is "/".
- LOGLEVEL. This value goes from 0 to 8. Use 0 for minimum log and 8 for the maximum log. Level 8 doesn't remove the prisoners' home directory. IMPORTANT: Do not use high loglevel in production servers, you may get low performance. The default value is 3.

# Checking

You can check the availability of your execution server using the URL

http://server:PORT/OK and https://server:SECURE_PORT/OK

where "server" is the name of your execution server. The system must return a page with OK

# Troubleshooting

You can obtain a detailed log of the execution process by changing the log level at the configuration file. Commonly The logs will be written to "/var/log/syslog".

# Adding the VPL-Jail-System to VPL 

The URL of the service in the general module configuration or in the local execution server settings of your Moodle server is

http://server:PORT/URLPATH or https://server:SECURE_PORT/URLPATH

:PORT and :SECURE_PORT can be omitted if using the standard ports.

# Changes from the 2.2 to 2.3 version

The main new of the 2.3 version is the change of file system used to replicate root directory on jail. This version includes some minor fixes and is compatible and interchangeable with the previous one.

The replication of the root file system is done with overlayfs, allowing to adapt the replica to the needs of the VPL-Jail-System easily and safe. To accelerate the execution and limit the file system changes, the users' home directory has been mounted as a tmpfs. Also the possibility of mounting the replica allowing SETUID has been added.

The use of the tmpfs removes the need of the "vncaccel.sh" script.

The new parameters to control these new features are:
- USETMPFS. This switch allows the use of tmpfs for "/home" and the "/dev/shm" directories. Changing this switch to "false" can degrade the performance of the jail system. To deactivate this option use USETMPFS=false. The default value is USETMPFS=true.
- HOMESIZE. This option set the size of the "/home" directory. The default value is 30% of the system memory. This option is applicable if using tmpfs file system for the "/home" directory.
- SHMSIZE. This option set the size of the "/dev/shm" directory. The default value is 30% of the system memory. This option is applicable if using tmpfs file system for the "/dev/shm" directory.
- ALLOWSUID. This switch allows the execution of programs with a suid bit inside the jail. This may be a security threat, use at your own risk. To activate this option, set ALLOWSUID=true.

# Changes from the 2.3 to 2.4 version
The installer and service control script has been update to support systemd service manager. Versions before 2.4 use only system V service manager. The change allows to install vpl-jail-system on Linux distributions that use YUM or APT and systemd or system V. Other fixes and changes are:

- The default log level has been increased to 3.
- The size of the SSL key created when installing has been increase to 2048. New versions of OpenSSL lib require this size.
- Improves the cleaning of finished tasks

# Changes from the 2.4 to 2.5 version

From the first versions of the VPL jail service the system includes a logic to ban IPs with high number of failed requests. This feature now can be controlled with a new configuration numeric parameter called FAIL2BAN. The banning and the account of failed requests take periods of 5 minutes. If one IP does more than FAIL2BAN*20 failed requests and more failed request than succeeded then the IP is banned until the next period. The FAIL2BAN set to 0 stop the banning process. The default value of FAIL2BAN is 0 then this feature has been disable by default. 

The structure of jail file systems has change to improve the compatibility and performance of the use of overlayFS in different O.S. configurations. Now the upper layer of the overlaid file system is on a tmpfs file system or, if you set the USETMPFS=false, is on a loop file system located at a sibling path to the control path (by default /var/vpl-jail-system.fs). IMPORTANT! if you set USETMPFS=false the you can not set HOMESIZE to a system memory percent, you must set HOMESIZE to an fixed value. The HOMESIZE value can be in megabyte or gigabyte. E.g.

* HOMESIZE=8G
* HOMESIZE=4500M

