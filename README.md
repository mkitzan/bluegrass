# Bluegrass

The goal of Bluegrass is to provide a high-level Bluetooth interface for Linux systems, so developers can use the technology without 
reading the full Bluetooth SIG standard. A significant portion of the Bluetooth knowledge which helped kick-start this project was
derived from ["The Use of Bluetooth in Linux and Location Aware Computing"](http://people.csail.mit.edu/albert/pubs/2005-ashuang-sm-thesis.pdf) 
by Albert Huang.

Bluetooth enables entirely wireless communication without the need for additional complex infrastructure. Many devices and machines 
support Bluetooth communication, but for a developer to harness the technology it often requires wadding through extensive APIs or 
poorly documented libraries. Simply put it's not beginner friendly to begin employing Bluetooth despite its utility. 

# Description

Bluegrass is built on top of the Blue-Z stack. It abstracts the core utilities a developer needs to begin using Bluetooth. It doesn't 
provide the low-level granular control the full Blue-Z stack provides: this isn't the library you're looking for if you need that. 
Every `*.hpp` file is well documented in-source and the examples in the `test` folder should get a developer familiar with usage.

Bluegrass provides tools to enable performant Bluetooth network programming. For example, `router.hpp` defines an entirely asynchronous 
socket network node through Linux asynchronous IO signals. Essentially this means a `router` object doesn't loop infinitely waiting for the next
server request, saving processor time while waiting for requests. `router` objects connect with other `router` objects on remote devices to build a Bluetooth network. When working on resource constrained systems, tools like `router.hpp` will help you get the most out of your hardware. 

The `CMakeLists.txt` file will show you, by example, how to build/include the library in your own project. Run CMake to build the test 
examples for your self to make sure everything is installed correctly. The file transfer example test requires some extra effort to setup. 
You just need to move `zimmermann.txt` or change the expected file location of it in `file_transfer_server.cpp`.

# Bluetooth Primer

You still need some basic knowledge of Bluetooth to use this library. Here's what you need to know. Most of the following is paraphrased
from ["The Use of Bluetooth in Linux and Location Aware Computing"](http://people.csail.mit.edu/albert/pubs/2005-ashuang-sm-thesis.pdf) 
by Albert Huang.

## Basics

Each Bluetooth device has a unique fixed Bluetooth address which constitutes six bytes (48 bits) and a not-necessarily unique device 
name. The device name is a human readable string which can be changed. A Bluetooth device can be "discovered" by other Bluetooth devices
if the device is in discoverable mode. However, a Bluetooth device may connect to another Bluetooth device (even those not in discoverable 
mode). 

A Bluetooth connection, like an internet connection, requires creating a socket connection between two devices. In Bluegrass, a 
`server` object opens a socket in "listen" mode waiting for connections, and a `socket` object attempts to connect to an open socket in 
"listen" mode. 

## Protocol

There's a lot of different Bluetooth protocols used for many different applications, but Bluegrass supports one protocol: L2CAP. L2CAP is a connection-oriented protocol that transmits fixed-length datagrams with best-effort (analogous to UDP). L2CAP supports odd port numbers between `4097 to 32767`.

## Host Controller Interface

All Bluetooth transmissions and requests go through the HCI. It's the computer's interface to the on-board Bluetooth hardware. From 
a practical standpoint the HCI allows your to find out the local device's Bluetooth address, and the address and names of remote device's
You will use an `hci` object to query the addresses/names of remote nearby devices.

# Bluetooth Setup

To setup your Linux system for Bluetooth development, run the `bluetooth.sh` script as super user from the directory you wish to store 
the Bluetooth library tools needed.

Inorder to use the bluegrass::sdp_controller you must follow [this guide](https://raspberrypi.stackexchange.com/questions/41776/failed-to-connect-to-sdp-server-on-ffffff000000-no-such-file-or-directory) on your system.

To put your Linux system into discoverable mode run `hciconfig hci0 piscan` from the terminal, or add that to your start up script 
(`/etc/rc.local`) to enable discoverable mode on boot.

To change your Linux system's Bluetooth device name (on Raspberry Pi at least), edit `/etc/machine-info` and change the "name" field of 
`PRETTY_NAME="name"` to the device name you want to display.
