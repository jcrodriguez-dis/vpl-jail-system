# VPL-JAIL-SYSTEM 2.6.0

[![Build Status](https://travis-ci.org/jcrodriguez-dis/vpl-xmlrpc-jail.svg?branch=V2.6.0)](https://travis-ci.org/jcrodriguez-dis/vpl-xmlrpc-jail)

![VPL Logo](https://vpl.dis.ulpgc.es/images/logo2.png)

The VPL-Jail-System serves an execution sandbox for the VPL Moodle plugin. This sandbox provides interactive execution, textual by xterm and graphical by VNC, and non-iterative execution for code evaluation purposes.

For more details about VPL, visit the [VPL home page](http://vpl.dis.ulpgc.es) or
the [VPL plugin page at Moodle](http://www.moodle.org/plugins/mod_vpl).
# Requirements
The VPL-Jail-System is an open software execution system and requires a specific environment. 

## Software requirements 

The VPL-Jail-System 2.4 requires a Linux O.S with YUM or APT as a package manager and systemd or system V as a service manager. The system has been tested on Debian, Ubuntu, and CentOS.

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

The system has been developed to offers immediate and interactive execution of students' programs. It means that the system can attend multiple-executions simultaneously.

The hardware required to accomplish this task depends on the number of simultaneous executions at a time, the requisites of the program, and the programming language used. For example, a PHP Web program may require a considerable amount of RAM, especially for the Web Browser execution, but a Python program may need one hundred times less of RAM.

Our experience is that a machine with only 2Gb of RAM and two cores can support a class with 50 students online using Java (Non-GUI). If you are conducting an exam, the hardware required maybe tripled. Possibly the critical resource may be the RAM. If the system exhausts the RAM, the O.S. will start swapping, and the throughput will decrease drastically. Our tests indicate that the 32-bit O.S. uses less memory and CPU than the 64-bit version. Remember that you can add (or remove) VPL-Jail-systems to a VPL installation online.

## Installation

### Selecting the hardware
The recommended option is using a dedicated machine. If you can not use a dedicated computer try using a Virtual Machine, e.g. using VirtualBox. This approach will provide aisle and limit the resources used by the service.
If you decide to use other services in the same machine that the use of resources by the VPL-Jail-System may decrease the performance of the other services. Although no security breach has been reported, notice that the nature of the service (execute external code) leads to an intrinsic threat.

### Preparing the system
Install a Linux O.S. as clean as possible. If you have enough resources, you can install a GUI interface. Stop any service that you don't need as the web server, ssh server, etc. If the O.S. has a firewall, you must configure it (or stop it) to give access to the only two ports needed by the VPL-Jail-System. If you use automatic updates, you must restart the VPL-Jail-System to take into account the update. You can use cron to automate this process.

### Getting VPL-Jail-System
VPL-Jail-System is distributed only as source files. You must get the source package from https://vp.dis.ulpgc.es, e.g., using
```shell
wget https://vpl.dis.ulpgc.es/releases/vpl-jail-system-[version].tar.gz
```

or from the GitHub repository, generating the package with
```shell
make distcheck
```

## Running the installer

After getting the package, you must decompress it and run the installer.
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
If you want to update the VPL-Jail-System, follow the same steps that the first installation. The installer will update the current version.

# Removing VPL-Jail-System
Run uninstall-sh of the current version.

# Configuration

After installing the VPL-Jail-Service, the service will be started with a default configuration. If you want to change the setting you must edit the file */etc/vpl/vpl-jail-system.conf*, see [VPL-Jail-System configuration](CONFIGURATION.md) for more details.

After configuration changes, you must restart (as user root) the service to use the new configuration values.
Using systemd

```shell
systemctl restart vpl-jail-system
```
or using system V

```shell
service vpl-jail-system restart
```

# Checking

You can check the availability of your execution server using the URL

http://servername:PORT/OK and https://servername:SECURE_PORT/OK

Where "server" is the name of your execution server. The system must return a page with OK

# Updating the software in the jail

After installing or updating packages or files in the host system, you must restart the service with "systemctl restart vpl-jail-system" to make available the changes in the jail. If you don't want to restart the service, you can drop the kernel caches to do the overlayFS file system aware of the changes. To drop the kernel caches run as root 

```
sync; echo 7 > /proc/sys/vm/dropcaches".
```

# Troubleshooting

You can obtain a detailed log of the execution process by changing the log level at the configuration file. Commonly The logs will be written to "/var/log/syslog".

# Adding the jail/execution server to the VPL plugin at Moodle

The URL of the service is
http://server:PORT/URLPATH or https://server:SECURE_PORT/URLPATH

:PORT and :SECURE_PORT can be omitted if using the standard ports numbers.

You can use the jail server URL in the VPL plugin configuration and, in the "local execution server" settings of a VPL activity.


# Changes from the 2.5 to 2.6 version

The 2.6 version include the following new features.

The installer includes the installation and basic configuration of the Cerbot software.
This package allows the system to get and renew Let's Encrypt certificates.
The server configuration includes new parameters that improve the management
of the cipher communications with https and wss.

* SSL_CIPHER_LIST

This parameter specifies ciphering options for SSL.
In case of wanting to have Forward Secrecy, the value must be ECDHE.
The default value is SSL_CIPHER_LIST=

```shell
SSL_CIPHER_LIST=ECDHE
```

* SSL_CERT_FILE

Indicates the path to the server's certificate in PEM format.
If your Certification Authority is not a root authority
you may need to add the chain of certificates of the intermediate CAs to this file.
The default value is SSL_CERT_FILE=/etc/vpl/cert.pem

```shell
SSL_CERT_FILE=/ssl/certs/mycert.pem
```

* SSLKEYFILE

Indicates the path to the server's private key in PEM format.
The default value is SSL_KEY_FILE=/etc/vpl/key.pem.

```shell
SSL_KEY_FILE=/ssl/certs/private/mykey.pem
```

