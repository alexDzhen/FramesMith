add_executable(framesmith_tests
    test_cli.c
    test_crc.c
    test_frame.c
    test_hexdump.c
    test_hexparse.c
    test_main.c
    test_pcap.c)
target_include_directories(framesmith_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/greatest)
target_link_libraries(framesmith_tests PRIVATE framesmith::core framesmith_warnings)
if(MINGW)
    target_link_options(framesmith_tests PRIVATE -static)
endif()

add_test(NAME unit_tests COMMAND framesmith_tests)

add_test(NAME cli_help COMMAND framesmith --help)
set_tests_properties(cli_help PROPERTIES PASS_REGULAR_EXPRESSION "Usage: framesmith")

add_test(NAME cli_version COMMAND framesmith --version)
set_tests_properties(cli_version PROPERTIES PASS_REGULAR_EXPRESSION "^framesmith ${PROJECT_VERSION}")

add_test(NAME cli_rejects_unknown_option COMMAND framesmith --bogus)
set_tests_properties(cli_rejects_unknown_option PROPERTIES WILL_FAIL TRUE)

# Build a frame from options and check the dump and the decoded summary.
add_test(NAME builds_arp_frame
    COMMAND framesmith --dst broadcast --src 02:00:00:00:00:01 --type arp --payload "00 01")
set_tests_properties(builds_arp_frame PROPERTIES
    PASS_REGULAR_EXPRESSION "type=0x0806 \\(ARP\\)")

add_test(NAME rejects_bad_mac COMMAND framesmith --dst 11:22:33:GG:55:66)
set_tests_properties(rejects_bad_mac PROPERTIES WILL_FAIL TRUE)
