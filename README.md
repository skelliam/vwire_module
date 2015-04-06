# vwire_module
A Linux kernel module wrapper around Mike McCauley's VirtualWire library for Arduino.
The target device for this module is Raspberry Pi, Beagle Bone, etc.

More info about VirtualWire is found here:  http://www.airspayce.com/mikem/arduino/VirtualWire/

Typically you would use VirtualWire if you wanted to make two devices talk to each other with these cheap and tiny 433 MHz radio transmitter/receiver pairs, like <a href="http://www.icstation.com/433mhz-transmitter-receiver-arduino-project-p-1402.html"this set</a>.

##Why?
I wanted to use the RPi as a receiver for devices I've built that send using the VirtualWire protocol.  VirtualWire samples each bit eight times.  If you are sending at 2000 bps, this means you'll be sampling a GPIO pin every 62.5 usec.  This is not practical from userspace.  

There is a VirtualWire solution out there for Pi that use pigpiod and Python, <a href="http://www.raspberrypi.org/forums/viewtopic.php?t=84596&p=598087">for example</a>, but I found that this solution running in userspace is not very reliable and it loads the processor to 70% or more, making the Pi very sluggish and not very useful for other things.

A solution in kernelspace puts the same load on the processor that pigpiod does when it is running.  I will have benchmarks on my blog shortly.

##Compiling the module
This is a little tricky, but not too bad.  The first thing you need to do is have the linux kernel headers installed on your Pi.  We are going to compile ON THE Pi.  You *can* compile on your desktop, but you need to get matching kernel sources and cross compile on the desktop, and later transfer it back to the Pi.  In my opinion this is too much work and there isn't really much benefit.

The first problem you will encounter on the Pi is a lack of kernel headers.  You need the kernel headers to proceed, and there are a few different options out there to get them.  (There is an ongoing discussion at the Pi forums where people are <a href="http://www.raspberrypi.org/forums/viewtopic.php?f=71&t=29326"wondering why</a> the headers aren't available through apt-get.)

The method I used was this cool script called <a href="https://github.com/notro/rpi-source">rpi-source</a>.  Instructions are <a href="https://github.com/notro/rpi-source/wiki">on the wiki</a> explaining how to acquire the scipt on your Pi and download the kernel headers.

There are some roadblocks here, you need to have a newer version of gcc installed.  This is a matter of adding the jessie sources to apt on your Pi, and setting them as a lower priority.  I got my Pi screwed up TWICE trying to upgrade the entire disro to jessie...  The much better approach is to only get the latest gcc from jessie and not try to update the entire Raspian distro.

Follow <a href="https://github.com/notro/rpi-source/wiki#install-gcc-48">this guide</a> to get the latest gcc on your Pi.  Do some reading.

Once you have the kernel headers and the latest gcc, you just switch to the vwire_module directory and run:
```$ make```

When the make is finished, which takes less than a minute, you should have a kernel module called "vwire_module.ko".

#Using the module

We need to know a few things before you insert the module.

I am using a RPi model B+ so the pin numbers I'm using correspond to that device.

* The GPIO data pin for the transmitter
* The GPIO data pin for the receiver
* The rate at which you want to send data
* The GPIO pin to use for PTT (push-to-talk/push-to-transmit)
* The GPIO pin to use to drive a status LED if you wish
* Whether or not to invert the PTT signal if your transmitter needs it

Once you know the above, you have the following arguments when you load the module:

* vwire_tx_gpio (default 16 if not specified)
* vwire_rx_gpio (default 13 if not specified)
* vwire_ptt_gpio (default 0 -- disabled)
* vwire_led_gpio (default 21 if not specified)
* vwire_ptt_invert (default 0 if not specified)
* vwire_baudrate (default 2000 if not specified)

## Inserting the module into a running kernel
If you like the defaults above, you just do this:
```$ sudo insmod vwire_module.ko```

If you want to change something, you specify it when you load the module, like this:
```$ sudo insmod vwire_module.ko vwire_baudrate=1000 vwire_rx_gpio=7```

To see if the module was loaded, type "dmesg" to see the kernel log.  Look at the last few lines, you should see something like this:
```
[173753.231387] vwire: vwire_init_module
[173753.234030] vwire: Requested GPIO 21 for LED 1
[173753.234079] vwire: Requested GPIO 13 for RX 1
[173753.234106] vwire: Requested GPIO 16 for TX 1
[173753.234145] vwire: VirualWire started: baudrate 2000, vwire_tx_gpio 16, vwire_rx_gpio 13, vwire_ptt_gpio 0, vwire_led_gpio 21, vwire_ptt_invert 0
```

## Removing the module from a running kernel:
```$ sudo rmmod vwire_module.ko```

## Transmitting data
We will use the 'sysfs' filesystem of Linux to interact with the VirtualWire module.  You simply do the following to transmit data:

```
$ su   //you need to become root
$ echo 'HELLO' > /sys/class/vwire/vwire/send
```
Monitoring from an Arduino I see:
Got: 48 45 4C 4C 4F A 

Notice that last 0xA that we didn't send...  That is echo inserting a linefeed character.  To avoid this use the -n argument to echo, like this:

```
$ su   //you need to become root
$ echo -n 'HELLO' > /sys/class/vwire/vwire/send
```
Montoring from an Arduino I see:
Got: 48 45 4C 4C 4F 

##Receiving data
Again with sysfs, this couldn't be easier.  We just need to look at the contents of the "file" called 'receive':

```
root@raspberrypi:/home/pi/Projects/vwire_module# cat /sys/class/vwire/vwire/receive
�root@raspberrypi:/home/pi/Projects/vwire_module# 
```

Now, in my case I'm sending binary data.  So you see all of that garbage on the second line before the word 'root', that a message that was received.  "cat" tries to show you ASCII text and we don't have good values for that here (however if I were sending 'HELLO', I would see that!).

Try this:

```
root@raspberrypi:/home/pi/Projects/vwire_module# xxd -p /sys/class/vwire/vwire/receive
01cc040108031e00
```

There the data that my little transmitter is sending!!  That represents the last message received.  If there is nothing new, you won't see anything, i.e.:

```
root@raspberrypi:/home/pi/Projects/vwire_module# xxd -p /sys/class/vwire/vwire/receive  //asked for data
01cc040108031e00
root@raspberrypi:/home/pi/Projects/vwire_module# xxd -p /sys/class/vwire/vwire/receive  //asked, nothing
root@raspberrypi:/home/pi/Projects/vwire_module# xxd -p /sys/class/vwire/vwire/receive  //ditto
root@raspberrypi:/home/pi/Projects/vwire_module# xxd -p /sys/class/vwire/vwire/receive  //ditto
root@raspberrypi:/home/pi/Projects/vwire_module# xxd -p /sys/class/vwire/vwire/receive
root@raspberrypi:/home/pi/Projects/vwire_module# xxd -p /sys/class/vwire/vwire/receive
root@raspberrypi:/home/pi/Projects/vwire_module# xxd -p /sys/class/vwire/vwire/receive
root@raspberrypi:/home/pi/Projects/vwire_module# xxd -p /sys/class/vwire/vwire/receive
root@raspberrypi:/home/pi/Projects/vwire_module# xxd -p /sys/class/vwire/vwire/receive
root@raspberrypi:/home/pi/Projects/vwire_module# xxd -p /sys/class/vwire/vwire/receive
root@raspberrypi:/home/pi/Projects/vwire_module# xxd -p /sys/class/vwire/vwire/receive  //asked -- bingo
01cc040108031e00
```

This is how it works at the moment, it is very much still under development.
