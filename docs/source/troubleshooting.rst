***************
Troubleshooting
***************

This document describes how can you check and troubleshooting the VPL-Jail-System.
For more details about VPL, visit the `VPL home page`_ or
the `VPL plugin page at Moodle`_.

.. _VPL home page: https://vpl.dis.ulpgc.es/
.. _VPL plugin page at Moodle: https://www.moodle.org/plugins/mod_vpl

Checking your server
--------------------

Service status
^^^^^^^^^^^^^^

You can check the status of the service using the following command in a terminal in your server 

Using systemd

.. code:: console

	systemctl status vpl-jail-system

or using system V

.. code:: console

	service vpl-jail-system status

.. figure:: images/systemctl_status.png
    :alt: Status of the service
  
    Example of systemctl output for an idle service

Accessible from a browser
^^^^^^^^^^^^^^^^^^^^^^^^^

You can check the availability of your execution server using the URL

\http://servername:PORT/OK and \https://servername:SECURE_PORT/OK

Where "servername" is the name of your execution server. The system must return a page with OK.

.. note:: The server must be accessible from the browser that uses VPL.

Accessible from Moodle
^^^^^^^^^^^^^^^^^^^^^^

After adding your new server to the list of execution servers in your Moodle.
You may go to "Advanced settings > Check execution servers" option of a VPL
activity and you must see your server with the *Current status* to *ready*. 

.. figure:: images/check_execution_servers.png
    :alt: Jail servers check

    Example of "Check execution servers" output

Troubleshooting
---------------

Reviewing VPL jail service logs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can see the log of the last installation at "/var/log/vpl_installation.log"
and the service start/stop log at "/var/log/vpl-jail-service.log".

You can obtain a detailed log of the execution process by changing the log level
at the configuration file. Commonly The logs will be written to "/var/log/syslog".

You may filter and explore the vpl logs with the following command

.. code:: console

   grep vpl /var/log/syslog | less


Reviewing brwoser logs
^^^^^^^^^^^^^^^^^^^^^^

All modern browsers have the capability to display JavaScript error logs and network connection logs.
These logs can prove invaluable for examining events occurring on the browser side.
Please be aware that the browser connects with the Moodle server using HTTP GET/POST requests
and transmits data through a JSON payload.
Furthermore, it uses WebSocket connections to communicate with the jail server.

To access the JavaScript console or Network log you can right-click anywhere on a webpage
and select 'Inspect' or 'Inspect Element'. In the panel that opens up, click on the 'Console' or 'Network' tab.
Remember to refresh your webpage if you want to see all the load-time logs.

Bug Reporting Guidelines
^^^^^^^^^^^^^^^^^^^^^^^^

.. _Github repository: https://github.com/jcrodriguez-dis/vpl-xmlrpc-jail/issues

If you believe you've discovered a bug, we kindly request you to report it on our `Github repository`_.

To help us investigate and resolve the issue efficiently, please provide a detailed description of
your system configuration and steps to reproduce the problem. A typical system configuration report includes:

- Operating System Version: The exact version of the operating system you are currently running.
- VPL-Jail-System Version: Specify the version of VPL-Jail-System you're using.
- VPL-Jail-System Configuration File: Please share the content of the VPL-Jail-System configuration file.
- Network Configuration: Describe how your system connects to the internet.
- Browser Logs: If you have significant or relevant information from your browser logs, kindly include it. 
- Screenshots: Including screenshots can be extremely helpful in visualizing the issue.
  Capture any error messages or unusual behavior you're experiencing.
- Other Relevant Information: If there's any additional information you think might be relevant to diagnose the issue,
  please share that as well. 

By providing a detailed bug report, you contribute significantly to expedite the process of identifying and resolving the problem.
Thank you for your cooperation!

Asking for help
^^^^^^^^^^^^^^^

.. _VPL Moodle forum: https://moodle.org/mod/forum/view.php?id=8672


If you don't found the problem, ask for help in the `VPL Moodle forum`_.

Please, describe your system configuration and *how to reproduce* the problem.
