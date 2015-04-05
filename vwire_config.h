#ifndef vwire_config_h
#define vwire_config_h


#define VERBOSE_DMESG  1

/* The name of the driver within the kernel (e.g. shows up in logs, etc...) */
#define VWIRE_DRV_NAME     "vwire"

/* The textual part of the device name in /dev */
#define VWIRE_DEV_NAME     "vwire"

#define VWIRE_DEFAULT_BAUD_RATE   (2000)
#define VWIRE_DEFAULT_RX_GPIO     (13)
#define VWIRE_DEFAULT_TX_GPIO     (16)
#define VWIRE_DEFAULT_LED_GPIO    (21)
#define VWIRE_DEFAULT_PTT_GPIO    (0)
#define VWIRE_DEFAULT_PTT_INVERT  (0)
#define VWIRE_DEFAULT_VERBOSE_LOG (0)

#define NSINSEC       (unsigned long)(1000000000)

#define BAUD_MIN      (1000)  /* minimum allowed baudrate */
#define BAUD_MAX      (5000)  /* maximum allowed baudrate */

#define Limit(x, min, max)            ( (x<min)?(min):( (x>max)?(max):(x) ) )
#define LimitErr(x, min, max, err)    ( (x<min)?(err):( (x>max)?(err):(x) ) )
#define DelayFromBaudrate(baud)       (unsigned long)((NSINSEC/Limit(baud, BAUD_MIN, BAUD_MAX))/8)  /* bits/sec and 8 samples/bit */

#endif
