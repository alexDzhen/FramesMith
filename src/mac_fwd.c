/* mac_fwd.c -- see mac_fwd.h. */
#include "../include/mac_fwd.h"

#include <stdio.h>

void mac_to_text(const uint8_t mac[6], char out[18]) {
    snprintf(out, 18, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4],
             mac[5]);
}
