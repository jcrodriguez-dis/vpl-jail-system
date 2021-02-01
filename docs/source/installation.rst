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

Install a Linux O.S. as clean as possible.
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

.. code:: bash

    make distcheck

This will generate a package in tar.gz format

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
- (updating) If you want to replace the configuration file with a fresh one.
- If you want to install different compilers and interpreters.

.. note:: If you have your own SSL certificates for the https and wss communications,
   you must write your certificates using the default location, you must use "/etc/vpl/cert.pem"
   filename for the public certificate and "/etc/vpl/key.pem" for the private key,
   or use the configuration file to change the default location.
   If your Certification Authority (CA) is not a root CA you may need to add the intermediate CA
   certificates to the "cert.pem" file.
   You must restart the service to apply the modifications.
   
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
