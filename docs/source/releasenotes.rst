*************
Release notes 
*************

V2.7.0
======

This release notes decribe the changes included in this release from version 2.6.0.

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

The installer ask if you want to install the kotlin command line compiler.
You must introduce a kotlin version number to download and install it.
See `Kotlin home page`_ for getting the version number.

.. note:: At this moment VPL-Jail-System does not support Kotlin distributes using Snap 

.. _Kotlin home page: https://kotlinlang.org/

Configuration
-------------

Adds new parameter to control limits of data in request and evaluation result.
See :ref:`REQUEST_MAX_SIZE` and :ref:`RESULT_MAX_SIZE` for more details.

