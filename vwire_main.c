/*
 * VirtualWire driver
 *
 * Author:
 * 	William Skellenger (wskellenger@gmail.com)
 *
 * Originally an Arduino library by Mike McCauley (mikem@airspayce.com)
 * All I've done is adapt this to run in the Linux kernel for RaspberryPi 
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.U
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

static struct hrtimer sample_timer;
static unsigned short baudrate = 2000;  /* speed in bits per sec */
static unsigned char tx_gpio = 0;
static unsigned char rx_gpio = 0;
static unsigned char ptt_gpio = 0;
static unsigned char ptt_invert = 0;
static unsigned char verbose = 0;
#if (LED_STATUS)
static unsigned char led_gpio = 0;
#endif

/* High speed loop */
/* The high speed loop samples the rx pin and sets thx tx pin.
 * --> 8 samples for each bit 
 * --> 2000 bits/sec (default)
 * --> 16,000 samples/sec
 */
enum hrtimer_restart sample_timer_callback(struct hrtimer *timer) 
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

static int __init vwire_kmod_init(void)
{
   int err = 0;
   ktime_t ktime;

   printk(KERN_INFO "%s\n", __func__);

   /* to start disable tx and rx */
   vw_tx_stop();
   vw_rx_stop();

   /* init pins */
   vw_set_tx_pin(tx_gpio);
   vw_set_rx_pin(rx_gpio);
   vw_set_ptt_pin(ptt_gpio);
   vw_set_ptt_inverted(ptt_invert);
#if LED_STATUS
   vw_set_led_pin(led_gpio);
#endif

   /* call setup */
   err = vw_setup();
   if (err) goto fail_setup;

   /* start the sample loop */
   ktime = ktime_set(0, DelayFromBaudrate(baudrate));
   hrtimer_init(&sample_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
   sample_timer.function = &sample_timer_callback;
   err = hrtimer_start(&sample_timer, ktime, HRTIMER_MODE_REL);
   if (err) goto fail_timer;

   /* start receiving */
   vw_rx_start();

   printk(KERN_INFO "Timer will fire callback in %ld nanosecs\n", DelayFromBaudrate(baudrate));
   return 0;  /* success */

fail_timer:
   printk(KERN_INFO "could not start highres timer, was already running\n");
fail_setup:
   printk(KERN_INFO "vw_setup() failed\n");

   return err;
}

static void __exit vwire_kmod_exit(void)
{
   int ret;

   printk(KERN_INFO "%s\n", __func__);

   vw_shutdown();

   /* cancel timer */
   ret = hrtimer_cancel(&sample_timer);
   if (ret) printk("The timer was still in use...\n");

   return;
}

module_init(vwire_kmod_init);
module_exit(vwire_kmod_exit);

/* user can adjust baudrate */
module_param(baudrate, ushort, 0644);
MODULE_PARM_DESC(baudrate, "The transmission speed in bits/sec, default 2000");

module_param(rx_gpio, byte, 0644);
MODULE_PARM_DESC(rx_gpio, "The GPIO pin to use for the receiver, 0=disabled");

module_param(tx_gpio, byte, 0644);
MODULE_PARM_DESC(tx_gpio, "The GPIO pin to use for the transmitter, 0=disabled");

module_param(ptt_gpio, byte, 0644);
MODULE_PARM_DESC(ptt_gpio, "The GPIO pin to use for PTT (push-to-transmit), 0=disabled");

module_param(ptt_invert, byte, 0644);
MODULE_PARM_DESC(ptt_invert, "Invert the PTT signal");

module_param(led_gpio, byte, 0644);
MODULE_PARM_DESC(led_gpio, "The GPIO pin to use to drive a status LED, 0=disabled");

module_param(verbose, byte, 0644);
MODULE_PARM_DESC(verbose, "Put more verbose debugging info to kernel log");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("William Skellenger (wskellenger@gmail.com)");
MODULE_DESCRIPTION("VirtualWire driver, intended for Raspberry Pi");
