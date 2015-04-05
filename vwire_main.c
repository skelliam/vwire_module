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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/gpio.h>

#include "vwire_config.h"
#include "vwire.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("William Skellenger (wskellenger@gmail.com)");
MODULE_DESCRIPTION("VirtualWire driver, intended for Raspberry Pi");

static struct hrtimer   vwire_sample_timer;

static unsigned short   baudrate = VWIRE_DEFAULT_BAUD_RATE;  /* speed in bits per sec */
module_param(baudrate, ushort, 0000);
MODULE_PARM_DESC(baudrate, 
      "The transmission speed in bits/sec, default 2000.  Can be changed via ioctl().");

static unsigned char    tx_gpio = VWIRE_DEFAULT_TX_GPIO;
module_param(tx_gpio, byte, 0000);
MODULE_PARM_DESC(tx_gpio, 
      "The GPIO pin to use for the transmitter, 0=disabled.  Can be changed via ioctl().");

static unsigned char    rx_gpio = VWIRE_DEFAULT_RX_GPIO;
module_param(rx_gpio, byte, 0000);
MODULE_PARM_DESC(rx_gpio, 
      "The GPIO pin to use for the receiver, 0=disabled.  Can be changed via ioctl().");

static unsigned char    ptt_gpio = VWIRE_DEFAULT_PTT_GPIO;
module_param(ptt_gpio, byte, 0000);
MODULE_PARM_DESC(ptt_gpio, 
      "The GPIO pin to use for PTT (push-to-transmit), 0=disabled.  Can be changed via ioctl().");

static unsigned char    ptt_invert = VWIRE_DEFAULT_PTT_INVERT;
module_param(ptt_invert, byte, 0000);
MODULE_PARM_DESC(ptt_invert, 
      "Invert the PTT signal.  Can be changed via ioctl().");

static unsigned char    verbose = VWIRE_DEFAULT_VERBOSE_LOG;
module_param(verbose, byte, 0000);
MODULE_PARM_DESC(verbose, 
      "Put more verbose debugging info to kernel log.  Can be changed via ioctl().");

static unsigned char    led_gpio = VWIRE_DEFAULT_LED_GPIO;
module_param(led_gpio, byte, 0000);
MODULE_PARM_DESC(led_gpio, 
      "The GPIO pin to use to drive a status LED, 0=disabled.  Can be changed via ioctl().");

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
   ktime = ktime_set(0, DelayFromBaudrate(baudrate));
   hrtimer_forward_now(timer, ktime);

   /* Mike McCauley's VirtualWire ported from Arduino */
   vw_int_handler();

   /* restart the timer */
   return HRTIMER_RESTART;
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
   vw_set_tx_pin(tx_gpio);
   vw_set_rx_pin(rx_gpio);
   vw_set_ptt_pin(ptt_gpio);
   vw_set_ptt_inverted(ptt_invert);
   vw_set_led_pin(led_gpio);

   /* call setup */
   err = vw_setup();
   if (err) goto fail_setup;

   /* start the sample loop */
   ktime = ktime_set(0, DelayFromBaudrate(baudrate));
   hrtimer_init(&vwire_sample_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
   vwire_sample_timer.function = &vwire_sample_timer_callback;
   err = hrtimer_start(&vwire_sample_timer, ktime, HRTIMER_MODE_REL);
   if (err) goto fail_timer;

   /* start receiving */
   vw_rx_start();

   printk(KERN_INFO VWIRE_DRV_NAME 
         ": VirualWire started: baudrate %d, tx_gpio %d, rx_gpio %d, ptt_gpio %d, ptt_invert %d, verbose %d, "
         "led_gpio: %d "
         "\n",
         baudrate, tx_gpio, rx_gpio, ptt_gpio, ptt_invert, verbose
         , led_gpio
         );
   return 0;  /* success */

fail_timer:
   printk(KERN_INFO VWIRE_DRV_NAME ": could not start highres timer, was already running\n");
fail_setup:
   printk(KERN_INFO VWIRE_DRV_NAME ": vw_setup() failed\n");

   return err;
}

static void __exit vwire_cleanup_module(void)
{
   int ret;

   printk(KERN_INFO VWIRE_DRV_NAME ": %s\n", __func__);

   (void)vw_shutdown();

   /* cancel timer */
   ret = hrtimer_cancel(&vwire_sample_timer);
   if (ret) printk(KERN_INFO VWIRE_DRV_NAME ": The timer was still in use...\n");

   return;
}

module_init(vwire_init_module);
module_exit(vwire_cleanup_module);




