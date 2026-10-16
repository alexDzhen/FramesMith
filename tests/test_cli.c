/*
   test_cli.c -- tests for the command-line parser (src/cli.c).
 */
#include "greatest.h"

#include <string.h>

#include "../include/cli.h"

#define COUNT(array) ((int)(sizeof(array) / sizeof((array)[0])))

/* No options at all means interactive mode. */
TEST empty_command_line_is_interactive(void) {
    char *argv[] = {"framesmith"};
    CliOptions options;
    char error[128];
    ASSERT(cli_parse(COUNT(argv), argv, &options, error, sizeof(error)));
    ASSERT(options.interactive);
    PASS();
}

/* Field options are recorded and turn off the interactive fallback. */
TEST collects_frame_fields(void) {
    char *argv[] = {"framesmith", "--dst", "broadcast", "--src", "02:00:00:00:00:01",
                    "--type",     "arp",   "--payload", "de ad"};
    CliOptions options;
    char error[128];
    ASSERT(cli_parse(COUNT(argv), argv, &options, error, sizeof(error)));
    ASSERT_FALSE(options.interactive);
    ASSERT_STR_EQ("broadcast", options.destination);
    ASSERT_STR_EQ("02:00:00:00:00:01", options.source);
    ASSERT_STR_EQ("arp", options.ethertype);
    ASSERT_STR_EQ("de ad", options.payload);
    ASSERT_EQ(PAYLOAD_HEX, options.payload_encoding);
    PASS();
}

/* --ascii and --file select the payload encoding. */
TEST selects_payload_encoding(void) {
    char *ascii_argv[] = {"framesmith", "--ascii", "hello"};
    char *file_argv[] = {"framesmith", "--file", "data.bin"};
    CliOptions options;
    char error[128];

    ASSERT(cli_parse(COUNT(ascii_argv), ascii_argv, &options, error, sizeof(error)));
    ASSERT_EQ(PAYLOAD_ASCII, options.payload_encoding);
    ASSERT_STR_EQ("hello", options.payload);

    ASSERT(cli_parse(COUNT(file_argv), file_argv, &options, error, sizeof(error)));
    ASSERT_EQ(PAYLOAD_FILE, options.payload_encoding);
    ASSERT_STR_EQ("data.bin", options.payload);
    PASS();
}

/* Output options are recorded, with their defaults. */
TEST collects_output_options(void) {
    char *argv[] = {"framesmith", "--type",  "arp", "--pcap",     "out.pcap", "--send",
                    "eth0",       "--count", "5",   "--interval", "100"};
    CliOptions options;
    char error[128];
    ASSERT(cli_parse(COUNT(argv), argv, &options, error, sizeof(error)));
    ASSERT_STR_EQ("out.pcap", options.pcap_path);
    ASSERT_STR_EQ("eth0", options.send_iface);
    ASSERT_EQ(5u, options.send_count);
    ASSERT_EQ(100u, options.send_interval);
    PASS();
}

/* send_count defaults to 1. */
TEST send_count_defaults_to_one(void) {
    char *argv[] = {"framesmith", "--type", "arp"};
    CliOptions options;
    char error[128];
    ASSERT(cli_parse(COUNT(argv), argv, &options, error, sizeof(error)));
    ASSERT_EQ(1u, options.send_count);
    PASS();
}

/* Help and version are recognised. */
TEST help_and_version(void) {
    char *help[] = {"framesmith", "--help"};
    char *version[] = {"framesmith", "-V"};
    CliOptions options;
    char error[128];
    ASSERT(cli_parse(COUNT(help), help, &options, error, sizeof(error)));
    ASSERT(options.show_help);
    ASSERT(cli_parse(COUNT(version), version, &options, error, sizeof(error)));
    ASSERT(options.show_version);
    PASS();
}

/* Unknown options and missing values are errors. */
TEST rejects_bad_command_lines(void) {
    char *unknown[] = {"framesmith", "--bogus"};
    char *missing[] = {"framesmith", "--dst"};
    CliOptions options;
    char error[128];
    ASSERT_FALSE(cli_parse(COUNT(unknown), unknown, &options, error, sizeof(error)));
    ASSERT(strstr(error, "--bogus") != NULL);
    ASSERT_FALSE(cli_parse(COUNT(missing), missing, &options, error, sizeof(error)));
    ASSERT(strstr(error, "needs a value") != NULL);
    PASS();
}

/* The help text mentions every option. */
TEST usage_mentions_options(void) {
    const char *usage = cli_usage_text();
    const char *options[] = {"--dst",   "--src",  "--type", "--vlan", "--payload",
                             "--ascii", "--file", "--pcap", "--send", "--help"};
    for (size_t i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        ASSERTm(options[i], strstr(usage, options[i]) != NULL);
    }
    PASS();
}

SUITE(cli_suite) {
    RUN_TEST(empty_command_line_is_interactive);
    RUN_TEST(collects_frame_fields);
    RUN_TEST(selects_payload_encoding);
    RUN_TEST(collects_output_options);
    RUN_TEST(send_count_defaults_to_one);
    RUN_TEST(help_and_version);
    RUN_TEST(rejects_bad_command_lines);
    RUN_TEST(usage_mentions_options);
}
