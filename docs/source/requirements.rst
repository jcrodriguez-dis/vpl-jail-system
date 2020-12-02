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

