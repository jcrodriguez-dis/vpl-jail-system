# VPL-JAIL-SYSTEM 

![VPL Logo](https://vpl.dis.ulpgc.es/images/logo2.png)

The VPL-Jail-System serves an execution sandbox for the VPL Moodle plugin. This sandbox provides interactive execution, textual by xterm and graphical by VNC, and non-iterative execution for code evaluation purposes.

For more details about VPL, visit the [VPL home page](http://vpl.dis.ulpgc.es) or
the [VPL plugin page at Moodle](http://www.moodle.org/plugins/mod_vpl).

# Using Docker

1. Build vpl-jail image with with any desired variable:
```
docker build -t vpl-jail .
```
2. Run if need or use compose with [Moodle VPL](https://github.com/jcrodriguez-dis/moodle-mod_vpl)
