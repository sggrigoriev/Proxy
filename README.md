before remove

Presto SDK in C
=====================
http://developer.peoplepowerco.com

Presto SDK in C - Connecting devices, homes, communities, and buildings.

This software SDK works in conjunection with People Power Presto to cloud-enable IoT devices and protocols.

The Presto Device API, implemented by this C SDK, is designed for connecting low memory embedded hubs, gateways, WiFi routers, set-top boxes, and embedded end-devices to Presto. Once connected to Presto, these devices are instantly exposed through mobile apps, web consoles, and data analytics + rules engines that you can customize.

It includes several directories:
* /apps
> Standalone applications that you can use for reference source code to build and use your own agents.

* /include
> Global headers. Library headers are automatically copied in when you build the lib directory.

* /iot
> IOT stands for "Internet of Things". This directory contains tools your applications need to connect to the Internet and communicate.

* /lib
> Libraries. Make this directory and copy or symlink the .so's into /usr/lib.  You may need to make the 3rd party libraries individually.

* /support
> Support files for the compile toolchain

The $IOTSDK environment variable can define where your iotsdk directory is
located. This environment variable may be used in your own applications'
Makefiles outside of this open source repository, and is sometimes
used in application Makefiles within this open source repository as well.


Requirements
--------------
* Pthreads support
* libxml2 (found in ./lib/3rdparty) for XML parsing, compiled with thread support
* libcurl (found in ./lib/3rdparty) for HTTP support
* openssl (found in ./lib/3rdparty) for SSL support



Here's how it works...

PROXY
-----
The iot/proxy directory is one of the main components of this SDK. 

It implements a persistent HTTP connection with a cloud server which allows 
clients to send data (which are buffered and pushed out periodically),
and receive commands almost instantaneously from a cloud server.

The proxy itself doesn't care about the data that is being passed back and
forth, but it does wrap the data in proper XML headers for communication
with the cloud.

If you're building this source code into an end-product, you can build
your application to sit on top of the proxy directly.  Otherwise, if you're
building a router, set-top box, or gateway, check out the "proxyserver" application
below.

The proxy.h file provides these functions:

* error_t proxy_start(const char *url);
    > Start the proxy, opens a persistent connection with the server

* void proxy_stop();
    > Stop the proxy

* error_t proxy_addListener(proxylistener l);
    > Add a listener for commands

* error_t proxy_removeListener(proxylistener l);
    > Remove a listener for commands

* error_t proxy_send(const char *data, int len);
    > Send a measurement. Use the iotapi.h to form XML to pass in.


PROXYSERVER APPLICATION
-----------------------
You'll use a Proxy server application on hubs, gateways, routers, and set-top boxes.
It manages a single pipe to the server and pumps all the other device's
data through that pipe, meaning your products are more scalable.

The apps/proxyserver application creates a proxy instance and opens a socket so 
multiple agents can all share the same proxy instance.  Users of the proxyserver
can use the clientsocket component found in iot/client to connect to the 
proxyserver's open socket.

You must activate your proxyserver using the command: proxyserver -a <key>,
where the <key> is given to you by People Power Company to bind your
physical device with your username.


SENDING AND RECEIVING XML
--------------------------------------
The iot/xml directory contains an important file:  iotapi.h.

This header defines function prototypes you would use to create XML
to send to the server, or receive commands from the server.

The header is implemented in the iot/xml/parser and iot/xml/generator 
directories. You would normally compile in the parser and the generate
with your agent in order to automatically deal with the XML data from the
cloud servers.

When you send a message from here, the library triggers an event at your
application so you can decide how you want the message delivered --
either directly to the proxy, or through a connection to a proxyserver socket.
* error_t application_send(const char *msg, int len);
    > You are responsible for implementing this function in your application to direct traffic
    to the correct proxy.

* void application_receive(const char *msg, int len);
    > You are responsible for implementing this function in your application to receive traffic
    from the correct proxy.

iotapi.h functions
-------------------
* int iotxml_newMsg(char *destMsg, int maxSize);
    > Create a new message to hold some measurements

* error_t iotxml_addDevice(const char *deviceId, int deviceType);
    > Start adding measurements for a new device

* error_t iotxml_alertDeviceIsGone(const char *deviceId);
    > Alert that this device is no longer seen in the local network

* int iotxml_addString(char *dest, int maxSize, const char *deviceId, int deviceType, param_type_e paramType, const char *paramName, const char *multiplier, char asciiParamIndex, const char *paramValue);
    > Add a string measurement

* int iotxml_addInt(char *dest, int maxSize, const char *deviceId, int deviceType, param_type_e paramType, const char *paramName, const char *multiplier, char asciiParamIndex, int paramValue);
    > Add an integer measurement

* error_t iotxml_send(char *destMsg, int maxSize);
    > Send the message

* void iotxml_abortMsg();
    > Abort any messages that are sending

* error_t iotxml_sendResult(int commandId, result_code_e result);
    > Send a result back to the server after executing a command. Each command you receive needs to send back a result.

* error_t iotxml_parse(const char *xml, int len);
    > Parse some XML that was received - this will automatically trigger command listeners

* error_t iotxml_addCommandListener(commandlistener_f l, char *type);
    > Add a listener for commands of a certain type

* error_t iotxml_removeCommandListener(commandlistener_f l);
    > Remove a command listener

* error_t iotxml_pushMeasurementNow(const char *deviceId);
    > Force the proxy to push its measurements to the server without buffering



EXAMPLE APPLICATIONS
---------------------
The *apps/exampleagent* is a template for an agent that would connect to the
proxyserver's socket and manage a set of similar devices.  It actually
does not function, but it compiles. 

The *apps/rtoaagent* is a fully working example application that connects
Radio Thermostat of America thermostats to Presto.  Full source
code is available. We show how to discover, measure, control, and 
synchronize the time on many RTOA thermostats on the local network from
a computer, hub, gateway, router, or set-top box that acts as a bridge
or a proxy to Presto.



COMPILE GUIDE
-------------
To compile proxyserver for Presto, follow the instructions below:

> $ cd presto-sdk-c

> $ export IOTSDK=\`pwd\`

> $ cd apps/proxyserve

> $ make



COPYRIGHT
----------
Copyright 2016 People Power Company

This code was developed with funding from People Power Company

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

