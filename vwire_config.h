#ifndef vwire_config_h
#define vwire_config_h


#define VERBOSE_DMESG  1
#define LED_STATUS     1

#define VW_RX_GPIO    (13)
#define VW_LED_GPIO   (21)
#define NSINSEC       (unsigned long)(1000000000)

#define BAUD_MIN      (1000)  /* minimum allowed baudrate */
#define BAUD_MAX      (5000)  /* maximum allowed baudrate */

#define Limit(x, min, max)            ( (x<min)?(min):( (x>max)?(max):(x) ) )
#define LimitErr(x, min, max, err)    ( (x<min)?(err):( (x>max)?(err):(x) ) )
#define DelayFromBaudrate(baud)       (unsigned long)((NSINSEC/Limit(baud, BAUD_MIN, BAUD_MAX))/8)  /* bits/sec and 8 samples/bit */

#endif
