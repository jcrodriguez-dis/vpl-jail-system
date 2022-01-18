************
Requirements
************

This document describes the requirements of the VPL-Jail-System.
For more details about VPL, visit the `VPL home page`_ or
the `VPL plugin page at Moodle`_.

.. _VPL home page: https://vpl.dis.ulpgc.es/
.. _VPL plugin page at Moodle: https://www.moodle.org/plugins/mod_vpl

The VPL-Jail-System is an open software execution system and requires a specific environment.

Software requirements
===================== 

From VPL-Jail-System 2.4 the system requires a Linux distribution with YUM or APT as a package manager
and systemd or system V as a service manager.
The system has been tested on Debian, Ubuntu, and CentOS.

+--------+---------+----------+---------------------------------------------+
|O.S.    | Version | Arch.    | Results                                     |
+========+=========+==========+=============================================+
| Ubuntu | 20.04   | 64b      | Compatible                                  |
+--------+---------+----------+---------------------------------------------+
| Ubuntu | 18.04   | 64b      | Compatible                                  |
+--------+---------+----------+---------------------------------------------+
| Ubuntu | 16.04   | 32b/64b  | Compatible                                  |
+--------+---------+----------+---------------------------------------------+
| Ubuntu | 14.04   | 32b/64b  | Not functional due to the lack of OverlayFS |
+--------+---------+----------+---------------------------------------------+
| Debian | 10      | 32b/64b  | Compatible                                  |
+--------+---------+----------+---------------------------------------------+
| Debian | 9       | 32b/64b  | Compatible                                  |
+--------+---------+----------+---------------------------------------------+
| CentOS | 7       | 64b      | GUI programs not available.                 |
|        |         |          | Requires to disable or configure SELinux.   |  
+--------+---------+----------+---------------------------------------------+
| CentOS | 6       |          | Not functional                              |
+--------+---------+----------+---------------------------------------------+

Hardware requirements
=====================

The system has been developed to offers immediate and interactive execution of students' programs.
It means that the system can attend multiple-executions simultaneously.

The hardware required to accomplish this task depends on the number of simultaneous executions at a time,
the requisites of the program, and the programming language used.
For example, a PHP Web program may require a considerable amount of RAM,
especially for the Web Browser execution, but a Python program may need one hundred times less of RAM.

Our experience is that a machine with only 2Gb of RAM and two cores can support
a class with 50 students online using Java (Non-GUI).
If you are conducting an exam, the hardware required maybe tripled.
Possibly the critical resource may be the RAM.
If the system exhausts the RAM, the O.S. will start swapping,
and the throughput will decrease drastically.
Our tests indicate that the 32-bit O.S. uses less memory and CPU than the 64-bit version.
Remember that you can add (or remove) VPL-Jail-systems to a VPL installation online.


Network requirements
====================

This section describes the network requirement to use the execution server with the VPL plugin.

Connections that must be available
----------------------------------

* From the user machine to the Moodle Server.
  The client machine that runs the browser must have access to the Moodle server.
  This is the obvious connection when the user interacts with Moodle.
  Notice that is highly recommended to use cyphered connections (HTTPS),
  that is the Moodle server must have a certificate signed by a known authority
  and the user must use an HTTPS connection (the URL to the Moodle server must start with "https://").

* From the Moodle Server to the Execution Server (Jail server).
  The VPL plugin in Moodle requires sending requests to the execution server.
  Notice that is highly recommended to use cyphered connections (HTTPS),
  that is the Execution server must have a certificate signed by a known authority
  and the VPL plugin must use an HTTPS connection (the URL to the execution server must start with "https://").

* From the user machine to the Execution Server (Jail server).
  The client machine that runs the browser must have access to the Execution server.
  When an execution/evaluation is requested, the execution server requires a connection from the browser
  to know that the user is there and if needed a connection to allow interactions with the running program.
  These connections are WebSocket and it is highly recommended to use cyphered connections (WSS),
  that is the Execution server must have a certificate signed by a known authority.

Proxies
-------

If your network is not using a transparent proxy you must:

* Configure the user machines to use the proxy properly.
  Notice that your proxy must be compatible with WebSocket.

* Configure the VPL plugin (Moodle administration VPL plugin settings) to use the proxy
  to access properly to the executions servers.

If your network is using a transparent proxy it must be compatible with WebSocket.

Load balancer
-------------

The architecture of the connections of the execution server seems to not allow to use of a load balancer
to access a cluster of execution servers.
Notice that the execution server that runs a task receive several requests from the Moodle server
and the client machine to manage the task.

Private network
---------------

The system does not require external connections,
then there are is no restriction to install all elements (clients, Moodle server and execution servers)
in a private network.
