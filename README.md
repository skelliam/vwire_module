# vwire_module
A Linux kernel module wrapper around Mike McCauley's VirtualWire library for Arduino.
The target device for this module is Raspberry Pi, Beagle Bone, etc.

More info about VirtualWire is found here:  http://www.airspayce.com/mikem/arduino/VirtualWire/

##Why?
I wanted to use the RPi as a receiver for devices I've built that send using the VirtualWire protocol.  VirtualWire samples each bit eight times.  If you are sending at 2000 bps, this means you'll be sampling a GPIO pin every 62.5 usec.  This is not practical from userspace.  

There is a VirtualWire solution out there for Pi that use pigpiod and Python, <a href="http://www.raspberrypi.org/forums/viewtopic.php?t=84596&p=598087">for example</a>, but I found that this solution running in userspace is not very reliable and it loads the processor to 70% or more, making the Pi very sluggish and not very useful for other things.

A solution in kernelspace puts the same load on the processor that pigpiod does when it is running.  I will have benchmarks on my blog shortly.

##Compiling the module
This is a little tricky, but not too bad.  The first thing you need to do is have the linux kernel headers installed on your Pi.  We are going to compile ON THE Pi.  You *can* compile on your desktop, but you need to get matching kernel sources and cross compile on the desktop, and later transfer it back to the Pi.  In my opinion this is too much work and there isn't really much benefit.

The first problem you will encounter on the Pi is a lack of kernel headers.  You need the kernel headers to proceed, and there are a few different options out there to get them.  (There is an ongoing discussion at the Pi forums where people are <a href="http://www.raspberrypi.org/forums/viewtopic.php?f=71&t=29326"wondering why</a> the headers aren't available through apt-get.)

The method I used was this cool script called <a href="https://github.com/notro/rpi-source">rpi-source</a>.  Instructions are <a href="https://github.com/notro/rpi-source/wiki">on the wiki</a> explaining how to acquire the scipt on your Pi and download the kernel headers.

There are some roadblocks here, you need to have a newer version of gcc installed.  This is a matter of adding the jessie sources to apt on your Pi, and setting them as a lower priority.  I got my entire Pi screwed up TWICE trying to upgrade the entire disro to jessie...  The much better approach is to only get the latest gcc from jessie and not try to update the entire distro.

Once you have the sources and the latest gcc, you just switch to the vwire_module directory and run:

$ make

## Inserting the module into a running kernel:
$ sudo insmod vwire_module.ko

## Removing the module from a running kernel:
$ sudo rmmod vwire_module.ko
