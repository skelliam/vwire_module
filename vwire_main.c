/*
 * VirtualWire kernel driver
 *
 * Module author:
 * 	William Skellenger (wskellenger@gmail.com)
 *
 * Originally an Arduino library by Mike McCauley (mikem@airspayce.com)
 * All I've done is adapt this to run in the Linux kernel for RaspberryPi.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/err.h>

#include "vwire_config.h"
#include "vwire.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("William Skellenger (wskellenger@gmail.com)");
MODULE_DESCRIPTION("VirtualWire driver, intended for Raspberry Pi");

static struct class     *device_class;
static struct device    *device_object;


static struct hrtimer   vwire_sample_timer;  /* high res timer to sample gpio */

static unsigned short   vwire_baudrate = VWIRE_DEFAULT_BAUD_RATE;  /* speed in bits per sec */
module_param(vwire_baudrate, ushort, 0000);
MODULE_PARM_DESC(vwire_baudrate, 
      "The transmission speed in bits/sec, default 2000.  Can be changed via ioctl().");

static unsigned char    vwire_tx_gpio = VWIRE_DEFAULT_TX_GPIO;
module_param(vwire_tx_gpio, byte, 0000);
MODULE_PARM_DESC(vwire_tx_gpio, 
      "The GPIO pin to use for the transmitter, 0=disabled.  Can be changed via ioctl().");

static unsigned char    vwire_rx_gpio = VWIRE_DEFAULT_RX_GPIO;
module_param(vwire_rx_gpio, byte, 0000);
MODULE_PARM_DESC(vwire_rx_gpio, 
      "The GPIO pin to use for the receiver, 0=disabled.  Can be changed via ioctl().");

static unsigned char    vwire_ptt_gpio = VWIRE_DEFAULT_PTT_GPIO;
module_param(vwire_ptt_gpio, byte, 0000);
MODULE_PARM_DESC(vwire_ptt_gpio, 
      "The GPIO pin to use for PTT (push-to-transmit), 0=disabled.  Can be changed via ioctl().");

static unsigned char    vwire_led_gpio = VWIRE_DEFAULT_LED_GPIO;
module_param(vwire_led_gpio, byte, 0000);
MODULE_PARM_DESC(vwire_led_gpio, 
      "The GPIO pin to use to drive a status LED, 0=disabled.  Can be changed via ioctl().");

static unsigned char    vwire_ptt_invert = VWIRE_DEFAULT_PTT_INVERT;
module_param(vwire_ptt_invert, byte, 0000);
MODULE_PARM_DESC(vwire_ptt_invert, 
      "Invert the PTT signal.  Can be changed via ioctl().");

static unsigned char    vwire_verbose = VWIRE_DEFAULT_VERBOSE_LOG;
module_param(vwire_verbose, byte, 0000);
MODULE_PARM_DESC(vwire_verbose, 
      "Put more verbose debugging info to kernel log.  Can be changed via ioctl().");


/* High speed loop */
/* The high speed loop samples the rx pin and sets thx tx pin.
 * --> 8 samples for each bit 
 * --> 2000 bits/sec (default)
 * --> 16,000 samples/sec
 */
enum hrtimer_restart vwire_sample_timer_callback(struct hrtimer *timer) 
{
   ktime_t ktime;

   /* This is a high speed sampling, at 2000 baud this loop will run 
    * every 62.5 us.  Higher speeds generally mean poorer reception,
    * and I'm not sure how fast we can push this... --wjs */

   /* schedule the next timer hit now */
   ktime = ktime_set(0, DelayFromBaudrate(vwire_baudrate));
   hrtimer_forward_now(timer, ktime);

   /* Mike McCauley's VirtualWire ported from Arduino */
   vw_int_handler();

   /* restart the timer */
   return HRTIMER_RESTART;
}

/* --- callback functions for sysfs */
static ssize_t vwire_set_message(struct device *dev,
                               struct device_attribute *attr,
                               const char* buf,
                               size_t count)
{
   printk(KERN_INFO VWIRE_DRV_NAME ": going to send message %s\n", buf);
   return count;
}

static ssize_t vwire_get_message(struct device *dev, 
                                  struct device_attribute *attr,
                                  char *buf)
{
   unsigned char len = VWIRE_MAX_MESSAGE_LEN;  /* todo max here */

   if (vw_get_message(buf, &len)) {
      /* the message should be in buf and len will be updated */
   }
   else {
      len = 0;
   }

   return len;
}

/* --- end callback functions */



/* --- define device attributes */
static DEVICE_ATTR(outbox, S_IWUSR|S_IRUGO, NULL, vwire_set_message);
static DEVICE_ATTR(inbox, S_IWUSR|S_IRUGO, vwire_get_message, NULL);


/* --- end device attributes */



static int vwire_fs_init(void)
{
   int err = 0;

   device_class = class_create(THIS_MODULE, VWIRE_DEV_NAME);
   err |= IS_ERR(device_class);

   device_object = device_create(device_class, NULL, 0, NULL, VWIRE_DEV_NAME);
   err |= IS_ERR(device_object);

   err |= device_create_file(device_object, &dev_attr_inbox);
   err |= device_create_file(device_object, &dev_attr_outbox);

   return err;
}

static void vwire_fs_cleanup(void)
{
   device_remove_file(device_object, &dev_attr_inbox);
   device_remove_file(device_object, &dev_attr_outbox);

   device_destroy(device_class, 0);
   class_destroy(device_class);

#if 0
	device_remove_file(&interface->dev, &dev_attr_blue);
	device_remove_file(&interface->dev, &dev_attr_red);
	device_remove_file(&interface->dev, &dev_attr_green);
#endif
}

static int __init vwire_init_module(void)
{
   int err = 0;
   ktime_t ktime;

   printk(KERN_INFO VWIRE_DRV_NAME ": %s\n", __func__);

   /* to start disable tx and rx */
   vw_tx_stop();
   vw_rx_stop();

   /* init pins */
   vw_set_tx_pin(vwire_tx_gpio);
   vw_set_rx_pin(vwire_rx_gpio);
   vw_set_ptt_pin(vwire_ptt_gpio);
   vw_set_ptt_inverted(vwire_ptt_invert);
   vw_set_led_pin(vwire_led_gpio);

   /* set up sysfs */
   err = vwire_fs_init();
   if (err) goto fail_fs_init;

   /* call setup */
   err = vw_setup();
   if (err) goto fail_setup;

   /* start the sample loop */
   ktime = ktime_set(0, DelayFromBaudrate(vwire_baudrate));
   hrtimer_init(&vwire_sample_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
   vwire_sample_timer.function = &vwire_sample_timer_callback;
   err = hrtimer_start(&vwire_sample_timer, ktime, HRTIMER_MODE_REL);
   if (err) goto fail_timer;

   /* start receiving */
   vw_rx_start();

   printk(KERN_INFO VWIRE_DRV_NAME 
         ": VirualWire started: baudrate %d, vwire_tx_gpio %d, vwire_rx_gpio %d, vwire_ptt_gpio %d, vwire_led_gpio %d, vwire_ptt_invert %d, vwire_verbose %d \n",
         vwire_baudrate, vwire_tx_gpio, vwire_rx_gpio, vwire_ptt_gpio, vwire_led_gpio, vwire_ptt_invert, vwire_verbose);
   return 0;  /* success */

fail_timer:
   printk(KERN_INFO VWIRE_DRV_NAME ": unrolling highres timer setup\n");
fail_setup:
   printk(KERN_INFO VWIRE_DRV_NAME ": unrolling vw_setup()\n");
fail_fs_init:
   printk(KERN_INFO VWIRE_DRV_NAME ": unrolling sysfs init\n");
   vwire_fs_cleanup();

   return err;
}

static void __exit vwire_cleanup_module(void)
{
   int ret;

   printk(KERN_INFO VWIRE_DRV_NAME ": %s\n", __func__);

   vw_shutdown();
   vwire_fs_cleanup();  

   /* cancel timer */
   ret = hrtimer_cancel(&vwire_sample_timer);
   if (ret) printk(KERN_INFO VWIRE_DRV_NAME ": The timer was still in use...\n");

   return;
}

module_init(vwire_init_module);
module_exit(vwire_cleanup_module);




