drivers/sercomm README
======================

If CONFIG_SERCOMM_CONSOLE is defined in the TinyAra configuration file, TinyAra
will attempt to use sercomm (HDLC protocol) to communicate with the
host system. Sercomm is the transport used by osmocom-bb that runs on top
of serial.  See http://bb.osmocom.org/trac/wiki/nuttx-bb/run for detailed
the usage of TinyAra with sercomm.

The drivers/sercomm build that you have the osmocom-bb project directory
at same level as the TinyAra project:

  |- os
  |- apps
  `- osmocom-bb

If you attempt to build this driver without osmocom-bb, you will get
compilation errors because of header files that are needed from the
osmocom-bb directory.
