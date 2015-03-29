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

enum hrtimer_restart sample_timer_callback(struct hrtimer *timer) 
{
   ktime_t ktime;

   /* schedule the next timer hit now */
   ktime = ktime_set(0, DelayFromBaudrate(baudrate));
   hrtimer_forward_now(timer, ktime);

   vw_int_handler();

   /* restart the timer */
   return HRTIMER_RESTART;
}

static int __init vwire_kmod_init(void)
{
   int ret = 0;
   ktime_t ktime;

   printk(KERN_INFO "%s\n", __func__);

   /* to start disable tx and rx */
   vw_tx_stop();
   vw_rx_stop();

   ret = vw_setup();  /* perhaps pass sysfs values ? */

   /* init high res timer */
   ktime = ktime_set(0, DelayFromBaudrate(baudrate));
   hrtimer_init(&sample_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
   sample_timer.function = &sample_timer_callback;
   hrtimer_start(&sample_timer, ktime, HRTIMER_MODE_REL);

   vw_rx_start();

   printk(KERN_INFO "Timer will fire callback in %ld nanosecs\n", DelayFromBaudrate(baudrate));

   return ret;
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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("William Skellenger (wskellenger@gmail.com)");
MODULE_DESCRIPTION("VirtualWire driver");


