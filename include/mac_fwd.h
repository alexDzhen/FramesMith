/*
   mac_fwd.h

   One tiny shared helper: MAC bytes to "aa:bb:cc:dd:ee:ff" text. It lives
   on its own so both the hex dump and any future caller can use it without
   pulling in a larger module.
 */

#ifndef FRAMESMITH_MAC_FWD_H
#define FRAMESMITH_MAC_FWD_H

#include <stdint.h>

/* Writes six lowercase colon-separated hex pairs into `out`, which must
   have room for 18 bytes. */
void mac_to_text(const uint8_t mac[6], char out[18]);

#endif /* FRAMESMITH_MAC_FWD_H */
