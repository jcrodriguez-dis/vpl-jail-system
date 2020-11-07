# VPL-JAIL-SYSTEM 2.6.0 CONFIGURATION

![VPL Logo](https://vpl.dis.ulpgc.es/images/logo2.png)

[VPL-Jail-System README](REAME.md)

This document describes how to configure the VPL-Jail-System.
For more details about VPL, visit the [VPL home page](http://vpl.dis.ulpgc.es) or
the [VPL plugin page at Moodle](http://www.moodle.org/plugins/mod_vpl).

## Changing configuration
The configuration is done changing the text file */etc/vpl/vpl-jail-system.conf*.
To apply the new configuration value you must restart the service (as user root).

Using systemd

```shell
systemctl restart vpl-jail-system
```
or using system V

```shell
service vpl-jail-system restart
```
## Configuration file format
The configuration file is in text format.
Each line can be empty, be a comment, or a parameter line.
A comment line starts with a '#" (numeric symbol).
A parameter line starts with a parameter name a '=' (equals symbol) and the parameter value.
Format VAR=VALUE.
Beware that no space is allowed before or after the equals symbol "=".

```shell
# This is a comment line
PARM=value
```

## Configuration parameters

### Network parameters

* PORT

Socket port number to listen for HTTP and ws connections (no ciphers). The default value is 80.
The value 0 indicates no use of unciphered connections.
For security reasons, current browsers do not accept ws connexions to no cipher ports.
Example:

```shell
PORT=8080
```

* SECURE_PORT

Socket port number to listen for https and wss connections. Default value 443.
The value 0 indicates no use of ciphered connections.
Notice that the files with the server public certificate and secret key
must exist and have proper content.
Example:

```shell
SECURE_PORT=4430
```
If this option changes, the URL of the execution server used at Moodle must be update. URL format "https://servername:SECURE_PORT/URLPATH".

* INTERFACE

Sets the network interface that the server will use. Use this parameter when your server has multiple IPs and you want to control which to use. The default value is empty, meaning the use of all network interfaces in the system.

```shell
INTERFACE=128.1.1.1
```

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

* SSL_KEY_FILE

Indicates the path to the server's private key in PEM format.
The default value is SSL_KEY_FILE=/etc/vpl/key.pem.

```shell
SSL_KEY_FILE=/ssl/certs/private/mykey.pem
```

### Security parameters

* URLPATH

This parameter acts as a password to access the execution server.
If the PATH of the URL request no matches URLPATH, the request is rejected.
The default value is "/". Example:

```shell
URLPATH=secret
```

If this option changes, the URL of the execution server used at Moodle must be updated.
URL format "https://servername:SECURE_PORT/URLPATH".

* TASK_ONLY_FROM

This parameter limits the servers that can do task requests.
The value must be IPs or networks (type A, B, and C) separate with spaces.
The IP format is the full dot notation. Example: 128.122.11.22.
The network format is the incomplete dot notation ending with a dot. Example: 10.1..
The default value is empty, accepting task requests from all servers.
Example:

```shell
# Accepts tasks from networt 10.10.3.X and IP 192.168.1.56
TASK_ONLY_FROM=10.10.3. 192.168.1.56
```

* ALLOWSUID

This switch allows the execution of programs with a suid bit inside the jail.
The default value is ALLOWSUID=false.
Setting true this option may be a security breach, use at your own risk.

```shell
ALLOWSUID=false
```

* FAIL2BAN

VPL jail service includes a logic to ban IPs with a high number of failed requests.
This feature now can be controlled with a new configuration numeric parameter called FAIL2BAN.
The banning and the account of failed requests take periods of 5 minutes.
If one IP does more than FAIL2BAN*20 failed requests and more failed requests than succeeded,
the offending network IP is banned until the next period.
The FAIL2BAN set to 0 stops these checks.
The default value of FAIL2BAN is 0. Examples:

```shell
FAIL2BAN=10
```

* FIREWALL

Sets the system firewall using iptables.
Accepted values are 0, 1, 2, 3, or 4. The preset value is 0. 

**: No firewall
** 1: VPL service+DNS+internet access
** 2: VPL service+DNS+Limit Internet to port 80 (super user unlimited)
** 3: VPL service+No external access (super user unlimited)
** 4: VPL service+No external access

Note: In level 4 stops the update/upgrade of the system

Note: Does not work in CentOS

```shell
FIREWALL=1
```

* LOGLEVEL

This value goes from 0 to 8.
Use 0 for the minimum log and 8 for the maximum log.
Level 8 doesn't remove the prisoners' home directory.
IMPORTANT: Do not use a high log level in production servers;
you may get a low performance and run out of disk space.
The default value is 3.
Commonly the log is written to the file "/var/log/syslog".
Example:

```shell
LOGLEVEL=1
```

### File system parameters

* JAILPATH

Sets the path to the jail directory.
The system will use this directory as a fake clone of the root directory.
The preset value is JAILPATH=/jail.

```shell
JAILPATH=/myjail
```

* CONTROLPATH

Path to control directory.
The system saves here information on requests in progress.
The preset value is CONTROLPATH="/var/vpl-jail-system

```shell
CONTROLPATH="/vplcontrol"
```

* USETMPFS

This switch allows the use tmpfs file system for "/home" and the "/dev/shm" directories.
The preset value is USETMPFS=true.
If your system memory is low you may use this switch,
but changing this switch to "false" can reduce the performance of the jail system.

```shell
USETMPFS=false
```

From version 2.3 the structure of jail file systems changed to improve
the compatibility and performance of the use of overlayFS in different O.S. configurations.
The upper layer of the overlaid file system is on a tmpfs file system or,
if you set the USETMPFS=false,
is on a loop file system located at a sibling path to the control path
(by default /var/vpl-jail-system.fs).
IMPORTANT! If you set USETMPFS=false, then you can not set HOMESIZE to a system memory percent,
you must set HOMESIZE to a fixed value.

* HOMESIZE

This option sets the size of the "/home" directory.
The value may be a percent of the system memory or a fixed value in megabyte (M) or gigabyte (G).
The default value is 30% of the system memory.
If USETMPFS is set to false, you must use a fixed value. Examples:

```shell
HOMESIZE=25%
```

```shell
HOMESIZE=1400M
```

* SHMSIZE

This option sets the size of the "/dev/shm" directory.
The preset value is 30% of the system memory.
This option is applicable if using tmpfs file system for the "/dev/shm" directory. Example:

```shell
SHMSIZE=10%
```

### Parameters for limiting the resources used by the requested tasks

These parameters set resource limits the requested task can not exceed.

* MAXTIME

This parameter sets the maximum time for a request in seconds. Default value MAXTIME=1800

* MAXFILESIZE

Maximum file size in bytes.
No file created by a task can exceed this size.
The default value is  MAXFILESIZE=64000000.

* MAXMEMORY

Maximum memory size in bytes.
No running task can exceed this size.
The default value is MAXMEMORY=2000000.

* MAXPROCESSES

The maximum number of processes.
No running task can exceed this number of processes/threads.
The default value is MAXPROCESSES=500.

### Other parameters

* MIN_PRISONER_UGID

This parameter sets the start value for the range of user/group ids selected randomly for prisoners.
The preset value is MIN_PRISONER_UGID=10000.

```shell
MIN_PRISONER_UGID=11000
```

* MAX_PRISONER_UGID

This parameter sets the end value for the range of user/group selected randomly for prisoners.
The preset value is MAX_PRISONER_UGID=12000

```shell
MAX_PRISONER_UGID=11200
```

* ENVPATH

This parameter sets the environment PATH variable when running tasks.
IMPORTANT: If you are using RedHat or derived OSes you must set this parameter
to the PATH environment variable of common users (not root).
The default value is empty. Example:

```shell
ENVPATH=/usr/bin:/bin
```
