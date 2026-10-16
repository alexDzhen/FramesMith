/*
   sender_linux.c

   Putting a frame on the wire on Linux, see sender.h. Only compiled on
   Linux.

   A "packet socket" (AF_PACKET with SOCK_RAW) hands the bytes we give it
   straight to the network driver, Ethernet header and all, with nothing
   added except the hardware CRC. That is exactly what a frame generator
   needs. Opening such a socket requires the CAP_NET_RAW capability, i.e.
   root or `setcap cap_net_raw+ep`, because it can forge any frame; the
   function turns the "permission denied" case into a clear message.
 */

#define _POSIX_C_SOURCE 200809L

#include "../../include/sender.h"

#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

bool frame_send_supported(void) {
    return true;
}

/* Fills a SendResult that failed, with a formatted message. */
static SendResult failure(const char *format, ...) {
    SendResult result;
    result.ok = false;
    va_list args;
    va_start(args, format);
    vsnprintf(result.message, sizeof(result.message), format, args);
    va_end(args);
    return result;
}

SendResult frame_send(const char *interface, const uint8_t *bytes, size_t length, unsigned count,
                      unsigned interval_ms) {
    // ETH_P_ALL in the protocol field lets us send any EtherType.
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) {
        if (errno == EPERM || errno == EACCES) {
            return failure("permission denied opening a packet socket.\n"
                           "  Run with sudo, or grant the capability once with:\n"
                           "    sudo setcap cap_net_raw+ep <path to framesmith>");
        }
        return failure("cannot open a packet socket: %s", strerror(errno));
    }

    // Translate the interface name ("eth0") into the index the kernel wants.
    unsigned index = if_nametoindex(interface);
    if (index == 0) {
        close(fd);
        return failure("no interface named '%s'", interface);
    }

    // The destination for sendto: the interface to send out of. The
    // destination MAC inside the frame is what actually addresses it; this
    // structure only selects the outgoing port.
    struct sockaddr_ll address;
    memset(&address, 0, sizeof(address));
    address.sll_family = AF_PACKET;
    address.sll_ifindex = (int)index;
    address.sll_halen = 6;
    memcpy(address.sll_addr, bytes, 6); // the frame's destination MAC

    if (count == 0) {
        count = 1;
    }
    for (unsigned i = 0; i < count; ++i) {
        ssize_t sent = sendto(fd, bytes, length, 0, (struct sockaddr *)&address, sizeof(address));
        if (sent < 0) {
            SendResult result = failure("send failed: %s", strerror(errno));
            close(fd);
            return result;
        }
        // Pause between copies of a burst, except after the last one.
        if (interval_ms > 0 && i + 1 < count) {
            struct timespec pause = {.tv_sec = interval_ms / 1000,
                                     .tv_nsec = (long)(interval_ms % 1000) * 1000000L};
            nanosleep(&pause, NULL);
        }
    }

    close(fd);
    SendResult result;
    result.ok = true;
    snprintf(result.message, sizeof(result.message), "sent %u frame%s of %zu bytes on %s", count,
             count == 1 ? "" : "s", length, interface);
    return result;
}
