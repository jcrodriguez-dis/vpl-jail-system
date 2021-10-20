*************
Release notes
*************

V2.7.2
======

This is mainly a bug-fix release of version 2.7.1.

* This release includes a workaround to a problem with the limits of int
  in the XMLRPC protocol (the protocol uses int32).
  This problem avoids setting a memory size or file size larger than the maximum int32 value.
  A full solution to this problem requires modifications in the Moodle plugin side.
  This workaround switches the size limit to the jail server local size limit when the problem is found.

* Adds new tools to the installer: bc, mypy, pycodestyle, and pydocstyle.

* Uses long long int to represent memory a file size.

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

