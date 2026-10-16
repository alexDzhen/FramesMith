/*
   cli.c

   Command-line parsing, see cli.h. The fields of the frame and the choice
   of outputs are all options; with no field options given the program
   falls back to interactive mode.
 */

#include "../include/cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Returns the value for an option that needs one, or NULL (with a message)
   when it is missing. Advances *i past the value. */
static const char *take_value(int argc, char **argv, int *i, char *error, size_t error_size) {
    if (*i + 1 >= argc) {
        snprintf(error, error_size, "option '%s' needs a value", argv[*i]);
        return NULL;
    }
    return argv[++(*i)];
}

bool cli_parse(int argc, char **argv, CliOptions *options, char *error, size_t error_size) {
    memset(options, 0, sizeof(*options));
    // Defaults: hex payload, pad short frames, send one copy.
    options->payload_encoding = PAYLOAD_HEX;
    options->send_count = 1;

    bool any_field = false; // did the operator describe the frame on the command line?

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        const char *value;

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            options->show_help = true;
        } else if (strcmp(arg, "-V") == 0 || strcmp(arg, "--version") == 0) {
            options->show_version = true;
        } else if (strcmp(arg, "-i") == 0 || strcmp(arg, "--interactive") == 0) {
            options->interactive = true;
        } else if (strcmp(arg, "--dst") == 0 || strcmp(arg, "--destination") == 0) {
            if ((value = take_value(argc, argv, &i, error, error_size)) == NULL)
                return false;
            options->destination = value;
            any_field = true;
        } else if (strcmp(arg, "--src") == 0 || strcmp(arg, "--source") == 0) {
            if ((value = take_value(argc, argv, &i, error, error_size)) == NULL)
                return false;
            options->source = value;
            any_field = true;
        } else if (strcmp(arg, "--type") == 0 || strcmp(arg, "--ethertype") == 0) {
            if ((value = take_value(argc, argv, &i, error, error_size)) == NULL)
                return false;
            options->ethertype = value;
            any_field = true;
        } else if (strcmp(arg, "--vlan") == 0) {
            if ((value = take_value(argc, argv, &i, error, error_size)) == NULL)
                return false;
            options->vlan = value;
            any_field = true;
        } else if (strcmp(arg, "--payload") == 0 || strcmp(arg, "--hex") == 0) {
            if ((value = take_value(argc, argv, &i, error, error_size)) == NULL)
                return false;
            options->payload = value;
            options->payload_encoding = PAYLOAD_HEX;
            any_field = true;
        } else if (strcmp(arg, "--ascii") == 0 || strcmp(arg, "--text") == 0) {
            if ((value = take_value(argc, argv, &i, error, error_size)) == NULL)
                return false;
            options->payload = value;
            options->payload_encoding = PAYLOAD_ASCII;
            any_field = true;
        } else if (strcmp(arg, "--file") == 0) {
            if ((value = take_value(argc, argv, &i, error, error_size)) == NULL)
                return false;
            options->payload = value;
            options->payload_encoding = PAYLOAD_FILE;
            any_field = true;
        } else if (strcmp(arg, "--no-pad") == 0) {
            options->no_pad = true;
        } else if (strcmp(arg, "-q") == 0 || strcmp(arg, "--quiet") == 0) {
            options->quiet = true;
        } else if (strcmp(arg, "--pcap") == 0) {
            if ((value = take_value(argc, argv, &i, error, error_size)) == NULL)
                return false;
            options->pcap_path = value;
        } else if (strcmp(arg, "--send") == 0) {
            if ((value = take_value(argc, argv, &i, error, error_size)) == NULL)
                return false;
            options->send_iface = value;
        } else if (strcmp(arg, "--count") == 0) {
            if ((value = take_value(argc, argv, &i, error, error_size)) == NULL)
                return false;
            options->send_count = (unsigned)strtoul(value, NULL, 10);
        } else if (strcmp(arg, "--interval") == 0) {
            if ((value = take_value(argc, argv, &i, error, error_size)) == NULL)
                return false;
            options->send_interval = (unsigned)strtoul(value, NULL, 10);
        } else {
            snprintf(error, error_size, "unknown option '%s'", arg);
            return false;
        }
    }

    // With no field described on the command line (and not just asking for
    // help or version), fall into interactive mode so the program is never
    // silent about what to do.
    if (!any_field && !options->show_help && !options->show_version) {
        options->interactive = true;
    }
    return true;
}

const char *cli_usage_text(void) {
    return "Usage: framesmith [frame options] [output options]\n"
           "       framesmith --interactive\n"
           "\n"
           "Builds an Ethernet frame from fields you choose, shows it as a hex dump,\n"
           "and can save it to a .pcap file or send it on a network interface.\n"
           "With no frame options it asks for each field interactively.\n"
           "\n"
           "Frame options:\n"
           "  --dst MAC         Destination MAC, or 'broadcast' (default: broadcast)\n"
           "  --src MAC         Source MAC (default: 02:00:00:00:00:01)\n"
           "  --type TYPE       EtherType: 0x0800, or a name like ipv4, arp, ipv6\n"
           "  --vlan ID[:PCP]   Add an 802.1Q VLAN tag (id 0-4095, priority 0-7)\n"
           "  --payload HEX     Payload as hex bytes, e.g. \"de ad be ef\"\n"
           "  --ascii TEXT      Payload as literal text\n"
           "  --file PATH       Payload read from a file\n"
           "  --no-pad          Do not pad a short frame to the 60-byte minimum\n"
           "\n"
           "Output options:\n"
           "  --pcap FILE       Write the frame to FILE.pcap (open it in Wireshark)\n"
           "  --send IFACE      Send the frame out of interface IFACE (Linux, needs root)\n"
           "  --count N         Send N copies (default 1)\n"
           "  --interval MS     Wait MS milliseconds between copies\n"
           "  -q, --quiet       Do not print the hex dump\n"
           "  -i, --interactive Ask for each field instead of using options\n"
           "  -h, --help        Show this help and exit\n"
           "  -V, --version     Show the version and exit\n"
           "\n"
           "Examples:\n"
           "  framesmith --dst broadcast --type arp --payload \"00 01 08 00\"\n"
           "  framesmith --dst 11:22:33:44:55:66 --ascii \"hello\" --pcap hello.pcap\n"
           "  sudo framesmith --src 02:00:00:00:00:01 --type 0x88b5 --send eth0 --count 5\n"
           "\n"
           "Exit status: 0 on success, 1 if sending failed, 2 on a usage or input error.\n";
}
