/*
   test_main.c -- entry point of the test executable (greatest framework).
 */
#include "greatest.h"

SUITE_EXTERN(frame_suite);
SUITE_EXTERN(crc_suite);
SUITE_EXTERN(hexparse_suite);
SUITE_EXTERN(hexdump_suite);
SUITE_EXTERN(pcap_suite);
SUITE_EXTERN(cli_suite);

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(frame_suite);
    RUN_SUITE(crc_suite);
    RUN_SUITE(hexparse_suite);
    RUN_SUITE(hexdump_suite);
    RUN_SUITE(pcap_suite);
    RUN_SUITE(cli_suite);
    GREATEST_MAIN_END();
}
