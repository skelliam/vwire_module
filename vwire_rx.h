#ifndef vwire_kmod_h
#define vwire_kmod_h

// The GPIO pin number for receiving data. Defaults to 2.
extern void vw_set_rx_pin(u8 pin);

// speed Desired speed in bits per second.  Default 200.
extern void vw_setup(u16 speed);

// Start the Phase Locked Loop listening to the receiver
// Must do this before you can receive any messages
// When a message is available (good checksum or not), vw_have_message();
// will return true.
extern void vw_rx_start();

// Stop the Phase Locked Loop listening to the receiver
// No messages will be received until vw_rx_start() is called again
// Saves interrupt processing cycles
extern void vw_rx_stop();

// Returns true if an unread message is available
// \return true if a message is available to read
extern uint8_t vw_have_message();

//If a message is available (good checksum or not), copies
// up to *len octets to buf.
// \param[in] buf Pointer to location to save the read data (must be at least *len bytes.
// \param[in,out] len Available space in buf. Will be set to the actual number of octets read
// \return true if there was a message and the checksum was good
extern uint8_t vw_get_message(u8* buf, u8* len);










#endif  /* vwire_kmod_h */
