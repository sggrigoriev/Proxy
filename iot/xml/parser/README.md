How Firmware Upgrade and Packagae Management Works
==================================================
Files in this folder are XML parser and and upgrade commands which for different system.
For OpenWrt, the predefined variable OPENWRT_BUILD in Makefile is controling which system command source file is
going to link with libiotxml.


Related Files
-------------
1. iotparser.c (XML command parser)
2. openwrt.c (System commands for OpenWrt)
3. ubuntu.c (System commands for Ubuntu, just for example for different platform, not work)
4. upgrade.h (Header file)
5. ../../../lib/libiotxml/Makefile (OPENWRT_BUILD decide which System command .C file to link with libiotxml)


How it works? (OpenWrt)
-----------------------
The XML parser will fetch out ppm.action, ppm.url, ppm.size, ppm.md5, ppm.name in the XML commands.
FIRMWARE_UPGRADE action need ppm.url, ppm.size, and ppm.md5 these three parameters.
INSTALL/UNINSTALL/UPDATE packages only need ppm.name parameter.


To Do List
----------
1. Fetch datas from JSON command.
2. Convert JSON command to XML command and broadcast to all legacy XML Agent.
3. For update firmware upgrade progress on OpenWrt modified the sysupgrade script to report status and progress in script.
4. We need to add a listen port for JSON Agents.
