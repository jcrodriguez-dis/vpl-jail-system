*************
Release notes
*************

V3.0.0
======

This release includes new features and improvements.

* Adds a new run mode for web apps.
  This mode allows accessing web apps from the client browser directly instead of interacting with a browser running on the server.
  This will drastically reduce the server resources used by this type of app.

* Adds support for JSON-RPC. The server detects if the request is XML-RPC or JSON-RPC and responds appropriately.
  This feature allows using the server with older clients (Moodle VPL plugin version < 4.0.0)
  and new clients that run on PHP 8 or higher without XML-RPC support.
  Using JSON-RPC also removes the limits of XML-RPC ints.

* Adds a new RPC call named "update".
  This RPC call allows updating files in the execution environment from the client without stopping the executing task.
  This call is useful for interpreted languages such as PHP in a web app.

* Adds a new RPC call named "directrun".
  This RPC call will allow new future features.

* The WebSocket protocol is improved to accept larges packets and fragmented packets.

* Adds :ref:`SSL_CIPHER_SUITES` configuration parameter.
  This parameter is used to set ciphers for TLSv1.3 if available.

* Adds :ref:`HSTS_MAX_AGE` configuration parameter.
  This parameter allows HTTP Strict-Transport-Security by setting the max-age parameter of the Strict-Transport-Security header.
  This parameter requires the use of :ref:`PORT` = 0.

* The installer adds **Julia programming language** to the list of development software installable.

V2.7.2
======

This is a bug-fix release of version 2.7.1 with small improvements.

* This release includes a workaround to a problem with the limits of int
  in the XMLRPC protocol (the protocol uses int32).
  This problem avoids setting a memory size or file size larger than the maximum int32 value.
  A full solution to this problem requires modifications in the Moodle plugin side.
  This temporal workaround switches the size limit to the jail server local size limit
  when the problem is found.

* Uses long long int to represent memory a file size.

* The installer includes the tool bc and new modules when installing python3:
  mypy, pycodestyle, and pydocstyle.

* The system checks for a change in the SSL certificate, reloading it if changed.
  This allows updating certificates without stopping the service.

V2.7.1
======

This is a bug-fix release of version 2.7.0.
This release fixes a problem that affects systems
with old versions of g++ that are not compatible with std::regex class.
This problem is known to affect CentOS 7.

V2.7.0
======

This release note describes the changes included in this release
from version 2.6.0.

Installation
------------

The new version moves the location of programs and script from the directory
"/etc/vpl" to directory "/usr/sbin/vpl" and the location of log files from
the directory "/etc/vpl" to directory "/var/log/vpl". Resolves `issue 45`_.

.. _issue 45: https://github.com/jcrodriguez-dis/vpl-xmlrpc-jail/issues/45

The installer adds **MiniZinc** and **Groovy** to the development software
and renames Python to Python2.

Kotlin
^^^^^^

The installer asks if you want to install the kotlin command-line compiler.
You must introduce a kotlin version number to download and install it.
See `Kotlin home page`_ for getting the version number.

.. note:: At this moment VPL-Jail-System does not support Kotlin distributes using Snap 

.. _Kotlin home page: https://kotlinlang.org/

Configuration
-------------

Adds a new parameter to control limits of data in request and evaluation result.
See :ref:`REQUEST_MAX_SIZE` and :ref:`RESULT_MAX_SIZE` for more details.

