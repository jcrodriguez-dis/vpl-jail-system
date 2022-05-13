************
Installation
************

This document describes how to install the VPL-Jail-System.
For more details about VPL, visit the `VPL home page`_ or
the `VPL plugin page at Moodle`_.

.. _VPL home page: https://vpl.dis.ulpgc.es/
.. _VPL plugin page at Moodle: https://www.moodle.org/plugins/mod_vpl

Selecting the hardware
----------------------

the recommended option is using a dedicated machine.
If you can not use a dedicated computer try using a Virtual Machine in a shared one, e.g. using VirtualBox.
This approach will provide isolation and limit the resources used by the service.
If you decide to install the service in a machine with other uses, beware that
the use of resources by the VPL-Jail-System may decrease the performance of the other services.
Although no security breach has been reported,
the nature of the service (execute external code) carry an intrinsic threat.

Preparing the system
--------------------

Install a Linux O.S. as clean as possible. At least install the "en_US.UTF-8" locale
and all other locales you want. Notice that the VPL plugin will try to use the locale
in use on the moodle, but will use "en_US.UTF-8" as fall back locale if setting
the Moodle one fails.

If you have enough resources, you can install a GUI interface.
Stop any service that you don't need as the web server, ssh server, etc.
If the O.S. has a firewall, you must configure it (or stop it) to give access
to the only two ports needed by the VPL-Jail-System.
If you use automatic updates, you must restart the VPL-Jail-System to take into account the update.
You can use cron to automate this process.

Getting VPL-Jail-System
-----------------------

VPL-Jail-System is distributed only as source files.
You must get the source package from https://vpl.dis.ulpgc.es/, e.g., using

.. code:: bash

    wget https://vpl.dis.ulpgc.es/releases/vpl-jail-system-[version].tar.gz


or from the GitHub repository, generating the package with

.. code:: console

 sudo apt-get install g++ make autotools-dev autoconf
 aclocal
 autoheader
 autoconf
 automake
 ./configure
 make distcheck

This will generate a package file named vpl-jail-system-[version].tar.gz

Running the installer
---------------------

After getting the package, you must decompress it and run the installer.

.. code:: bash

	tar xvf vpl-jail-system-[version].tar.gz
	cd vpl-jail-system-[version]
	./install-vpl-sh

.. note:: The "./install-vpl-sh" script must be run as root.

Follow the instructions and wait for the necessary downloads.
The installation script will try to install the development software commonly used.

The installer will ask you about:

- If you want to use Let's Encrypt, creates a self-signed SSL certificate or use your own certificates.
- When updating, if you want to replace the configuration file with a fresh one.
- If you want to install different compilers and interpreters.

.. note:: If you have your own SSL certificates for the HTTPS and WSS communications,
  you must copy your public certificate to the "/etc/vpl/cert.pem" file and the
  private key to the "/etc/vpl/key.pem" file. You also can change the configuration
  file to tell the system the location of your certificates. If your Certification
  Authority (CA) is not a root CA you may need to add the intermediate CA certificates
  to the "cert.pem" file. You may restart the service to apply the modifications.
   
Adding the jail/execution server to the VPL plugin at Moodle
------------------------------------------------------------

The URL of the service is
\https://yourservername:SECURE_PORT/URLPATH or \http://yourservername:PORT/URLPATH 

:SECURE_PORT and :PORT can be omitted if using the standard ports numbers.

You can use the jail server URL in the VPL plugin configuration (requiere administration rigths)
and, in the *local execution server* settings of a VPL activity.
   
Updating VPL-Jail-System
------------------------

If you want to update the VPL-Jail-System to a new version, follow the same steps that the first installation.
The installer will update the current version to the new one.

Updating the software in the jail
---------------------------------

After installing or updating packages or files in the host system, you must restart the service with

.. code:: bash

   systemctl restart vpl-jail-system

to make available the changes in the jail.
If you don't want to restart the service,
you can drop the kernel caches to do the overlayFS file system aware of the changes.
To drop the kernel caches run as root 

.. code:: bash

   sync; echo 7 > /proc/sys/vm/dropcaches

Removing VPL-Jail-System
------------------------

Run uninstall-sh of the current version.

Developments tools the installer may install
--------------------------------------------

If you wish the installer can try to install some development tools for you,
also you can install by hand the development tools you prefer.
The automatically installed tools depend on your Linux distribution, package manager,
and the names of the packages. The installer checks for using yum or apt-get.

Development tools installed using yum
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+------------------------------+--------------------------------+
| Package name                 | Description                    |
+==============================+================================+
| nasm                         | Assembler                      |
+------------------------------+--------------------------------+
| gcc-gnat                     | Ada compiler (GNU)             |
+------------------------------+--------------------------------+
| gcc                          | C compiler (GNU)               |
+------------------------------+--------------------------------+
| gcc-gfortran                 | Fortran compiler (GNU)         |
+------------------------------+--------------------------------+
| gdb                          | General purpose debugger (GNU) |
+------------------------------+--------------------------------+
|| java-1.8.0-openjdk-devel or || Java (OpenJDK)                |
|| java-1.7.0-openjdk-devel    ||                               |
+------------------------------+--------------------------------+
| junit                        | Junit framework                |
+------------------------------+--------------------------------+
| perl                         | Perl interpreter               |
+------------------------------+--------------------------------+
| php-cli                      | PHP interpreter                |
+------------------------------+--------------------------------+
| python                       | Python interpreter             |
+------------------------------+--------------------------------+
| pl                           | Prolog                         |
+------------------------------+--------------------------------+
| sqlite                       | SQL DBM                        |
+------------------------------+--------------------------------+
| tcl                          | TCL interpreter                |
+------------------------------+--------------------------------+
| valgrind                     | Valgrind tool                  |
+------------------------------+--------------------------------+

Development tools installed using apt-get
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+---------------------+-----------------------------------------------------+
| Package name        | Description                                         |
+=====================+=====================================================+
| gnat                | Ada compiler (GNU)                                  |
+---------------------+-----------------------------------------------------+
|                     |                                                     |
+---------------------+-----------------------------------------------------+
| nasm                | Assembler                                           |
+---------------------+-----------------------------------------------------+
| gcc                 | C compiler (GNU)                                    |
+---------------------+-----------------------------------------------------+
| mono-complete       | C# development framework (mono)                     |
+---------------------+-----------------------------------------------------+
| ddd                 | DDD graphical front end debugger (GNU)              |
+---------------------+-----------------------------------------------------+
| gfortran            | Fortran compiler (GNU)                              |
+---------------------+-----------------------------------------------------+
| gdb                 | General purpose debugger (GNU)                      |
+---------------------+-----------------------------------------------------+
| hgc hugs            | Haskell                                             |
+---------------------+-----------------------------------------------------+
|| default-jre or     || Java runtime                                       |
|| openjdk-11-jre or  ||                                                    |
|| openjdk-8-jre or   ||                                                    |
|| openjdk-7-jre      ||                                                    |
+---------------------+-----------------------------------------------------+
|| default-jdk        || Java jdk                                           |
|| openjdk-11-jdk or  ||                                                    |
|| openjdk-8-jdk or   ||                                                    |
|| openjdk-7-jdk      ||                                                    |
+---------------------+-----------------------------------------------------+
| openjfx             | JavaFX                                              |
+---------------------+-----------------------------------------------------+
| checkstyle          | Java Checkstyle                                     |
+---------------------+-----------------------------------------------------+
| junit4 junit        | Junit framework                                     |
+---------------------+-----------------------------------------------------+
| nodejs              | JavaScript (Nodejs)                                 |
+---------------------+-----------------------------------------------------+
| nodejs-legacy       | JavaScript (Nodejs-legacy)                          |
+---------------------+-----------------------------------------------------+
| octave              | Octave (GNU)                                        |
+---------------------+-----------------------------------------------------+
| fp-compiler         | Pascal compiler                                     |
+---------------------+-----------------------------------------------------+
| perl                | Perl interpreter                                    |
+---------------------+-----------------------------------------------------+
|| php-cli or         || PHP interpreter                                    |
|| php5-cli           ||                                                    |
+---------------------+-----------------------------------------------------+
| php-readline        | PHP readline                                        |
+---------------------+-----------------------------------------------------+
|| php-sqlite3 or     || Sqlite for PHP                                     |
|| php5-sqlite        ||                                                    |
+---------------------+-----------------------------------------------------+
| swi-prolog          | Prolog                                              |
+---------------------+-----------------------------------------------------+
|| python or          || Python2 interpreter                                |
|| python2            ||                                                    |
+---------------------+-----------------------------------------------------+
| pydb                | Python2 pydb                                        |
+---------------------+-----------------------------------------------------+
| python-pudb         | Python2 pudb                                        |
+---------------------+-----------------------------------------------------+
| python-tk           | Python2 Tk                                          |
+---------------------+-----------------------------------------------------+
| python-numpy        | Python2 NumPy                                       |
+---------------------+-----------------------------------------------------+
| python-pandas       | Python2 pandas                                      |
+---------------------+-----------------------------------------------------+
| python-matplotlib   | Python2 Matplotlib                                  |
+---------------------+-----------------------------------------------------+
| python3             | Python3                                             |
+---------------------+-----------------------------------------------------+
| python3-tk          | Python3 Tk                                          |
+---------------------+-----------------------------------------------------+
| python3-numpy       | Python3 NumPy                                       |
+---------------------+-----------------------------------------------------+
| python3-pandas      | Python3 pandas                                      |
+---------------------+-----------------------------------------------------+
| python3-matplotlib  | Python3 Matplotlib                                  |
+---------------------+-----------------------------------------------------+
| python3-pudb        | Python3 pudb                                        |
+---------------------+-----------------------------------------------------+
| python3-pycodestyle | Python3 pycodestyle                                 |
+---------------------+-----------------------------------------------------+
| python3-networkx    | Python3 NetworkX                                    |
+---------------------+-----------------------------------------------------+
| mypy                | Python mypy                                         |
+---------------------+-----------------------------------------------------+
| pycodestyle         | Python pycodestyle                                  |
+---------------------+-----------------------------------------------------+
| pydocstyle          | Python pydocstyle                                   |
+---------------------+-----------------------------------------------------+
| thonny              | Pythom Thonny                                       |
+---------------------+-----------------------------------------------------+
| ruby                | Ruby interpreter                                    |
+---------------------+-----------------------------------------------------+
| scala               | Scala programming language                          |
+---------------------+-----------------------------------------------------+
|| plt-scheme or      || Scheme interpreter                                 |
|| racket             ||                                                    |
+---------------------+-----------------------------------------------------+
| sqlite3             | SQL interpreter                                     |
+---------------------+-----------------------------------------------------+
| tcl                 | TCL interpreter                                     |
+---------------------+-----------------------------------------------------+
| valgrind            | Valgrind tool                                       |
+---------------------+-----------------------------------------------------+
| clisp               | Clisp                                               |
+---------------------+-----------------------------------------------------+
|| clojure or         || Clojure                                            |
|| clojure1.6 or      ||                                                    |
|| clojure1.6         ||                                                    |
+---------------------+-----------------------------------------------------+
| open-cobol          | Cobol                                               |
+---------------------+-----------------------------------------------------+
| coffeescript        | CoffeeScript                                        |
+---------------------+-----------------------------------------------------+
| gdc                 | D compiler (GNU)                                    |
+---------------------+-----------------------------------------------------+
| erlang              | Erlang                                              |
+---------------------+-----------------------------------------------------+
| golang              | Go programming language compiler                    |
+---------------------+-----------------------------------------------------+
| haxe                | Haxe programming language                           |
+---------------------+-----------------------------------------------------+
| libjs-jquery        | JQuery JavaScript Lib                               |
+---------------------+-----------------------------------------------------+
| libjs-jquery-ui     | JQuery-UI JavaScript Lib                            |
+---------------------+-----------------------------------------------------+
| julia               | Julia                                               |
+---------------------+-----------------------------------------------------+
| lua5.1              | Lua compiler 5.1                                    |
+---------------------+-----------------------------------------------------+
| r-base              | R statistical computation and graphics system (GNU) |
+---------------------+-----------------------------------------------------+
| spim                | MIPS R2000/R3000 emulator                           |
+---------------------+-----------------------------------------------------+
| minizinc            | MiniZinc constraint modeling language               |
+---------------------+-----------------------------------------------------+
| galax               | XQuery interpreter                                  |
+---------------------+-----------------------------------------------------+
| iverilog            | Verilog compiler                                    |
+---------------------+-----------------------------------------------------+
| freehdl             | VHDL compiler                                       |
+---------------------+-----------------------------------------------------+
| libtool-bin         | libtool required for VHDL                           |
+---------------------+-----------------------------------------------------+
| groovy              | Groovy programming language                         |
+---------------------+-----------------------------------------------------+

Using npm packge manager

+--------------+-------------+
| Package name | Description |
+==============+=============+
| typescript   | TypeScript  |
+--------------+-------------+
| sass         | Sass        |
+--------------+-------------+

The system also asks if you wish to install **Kotlin** and **JGrasp** without using a package manager.
JGrasp allows debugging Java using a GUI interface.
