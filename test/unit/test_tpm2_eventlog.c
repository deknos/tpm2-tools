/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <setjmp.h>
#include <cmocka.h>

#include <tss2/tss2_tpm2_types.h>

#include "tpm2_eventlog.h"

#define TCG_DIGEST2_SHA1_SIZE (sizeof(TCG_DIGEST2) + TPM2_SHA_DIGEST_SIZE)

static bool foreach_digest2_test_callback(TCG_DIGEST2 const *digest, size_t size, void *data){

    (void)digest;
    (void)size;
    (void)data;

    return mock_type(bool);
}
static void test_foreach_digest2_null(void **state){

    (void)state;

    assert_false(foreach_digest2(NULL, 0, sizeof(TCG_DIGEST2), NULL, NULL));
}
static void test_foreach_digest2_size(void **state) {

    (void)state;
    uint8_t buf [sizeof(TCG_DIGEST2) - 1] = { 0, };
    TCG_DIGEST2 *digest = (TCG_DIGEST2*)buf;

    assert_false(foreach_digest2(digest, 1, sizeof(TCG_DIGEST2) - 1, foreach_digest2_test_callback, NULL));
}
static void test_foreach_digest2(void **state) {

    (void)state;
    uint8_t buf [TCG_DIGEST2_SHA1_SIZE] = { 0, };
    TCG_DIGEST2* digest = (TCG_DIGEST2*)buf;

    will_return(foreach_digest2_test_callback, true);
    assert_true(foreach_digest2(digest, 1, TCG_DIGEST2_SHA1_SIZE, foreach_digest2_test_callback, NULL));
}
static void test_foreach_digest2_cbnull(void **state){

    (void)state;
    uint8_t buf [TCG_DIGEST2_SHA1_SIZE] = { 0, };
    TCG_DIGEST2* digest = (TCG_DIGEST2*)buf;

    assert_true(foreach_digest2(digest, 1, TCG_DIGEST2_SHA1_SIZE, NULL, NULL));
}
static void test_foreach_digest2_cbfail(void **state){

    (void)state;
    uint8_t buf [TCG_DIGEST2_SHA1_SIZE] = { 0, };
    TCG_DIGEST2* digest = (TCG_DIGEST2*)buf;

    will_return(foreach_digest2_test_callback, false);
    assert_false(foreach_digest2(digest, 1, TCG_DIGEST2_SHA1_SIZE, foreach_digest2_test_callback, NULL));
}
static void test_digest2_accumulator_callback(void **state) {

    (void)state;
    char buf[TCG_DIGEST2_SHA1_SIZE];
    TCG_DIGEST2 *digest = (TCG_DIGEST2*)buf;
    size_t size = TPM2_SHA1_DIGEST_SIZE, accumulated = 0;

    digest->AlgorithmId = TPM2_ALG_SHA1;
    assert_true(digest2_accumulator_callback (digest, size, &accumulated));
    assert_int_equal(accumulated, TCG_DIGEST2_SHA1_SIZE);
}
static void test_digest2_accumulator_callback_null(void **state) {

    (void)state;

    assert_false(digest2_accumulator_callback (NULL, 0, NULL));
}
static bool test_event2hdr_callback(TCG_EVENT_HEADER2 const *eventhdr, size_t size, void *data) {

    (void)eventhdr;
    (void)size;
    (void)data;

    return mock_type(bool);
}
static bool test_event2_callback(TCG_EVENT2 const *event, UINT32 type,
                                 void *data) {

    (void)event;
    (void)type;
    (void)data;

    return mock_type(bool);
}

static void test_parse_event2_badhdr(void **state){

    (void)state;
    char buf[sizeof(TCG_EVENT_HEADER2) - 1] = { 0, };
    TCG_EVENT_HEADER2 *eventhdr = (TCG_EVENT_HEADER2*)buf;
    size_t size_event = 0, size_digest = 0;

    assert_false(parse_event2(eventhdr, sizeof(buf), &size_event, &size_digest));
}
static void test_parse_event2_baddigest(void **state){

    (void)state;
    char buf[sizeof(TCG_EVENT_HEADER2) + TCG_DIGEST2_SHA1_SIZE - 1] = { 0, };
    TCG_EVENT_HEADER2 *eventhdr = (TCG_EVENT_HEADER2*)buf;
    TCG_DIGEST2 *digest = eventhdr->Digests;
    size_t size_event = 0, size_digest = 0;

    eventhdr->DigestCount = 1;
    digest->AlgorithmId = TPM2_ALG_SHA1;

    assert_false(parse_event2(eventhdr, sizeof(buf), &size_event, &size_digest));
}
static void test_parse_event2_badeventsize(void **state){

    (void)state;
    char buf[sizeof(TCG_EVENT_HEADER2) + TCG_DIGEST2_SHA1_SIZE + sizeof(TCG_EVENT2) - 1] = { 0, };
    TCG_EVENT_HEADER2 *eventhdr = (TCG_EVENT_HEADER2*)buf;
    TCG_DIGEST2 *digest = eventhdr->Digests;
    size_t size_event = 0, size_digest = 0;

    eventhdr->DigestCount = 1;
    digest->AlgorithmId = TPM2_ALG_SHA1;

    assert_false(parse_event2(eventhdr, sizeof(buf), &size_event, &size_digest));
}
static void test_parse_event2_badeventbuf(void **state){

    (void)state;
    char buf[sizeof(TCG_EVENT_HEADER2) + TCG_DIGEST2_SHA1_SIZE + sizeof(TCG_EVENT2)] = { 0, };
    TCG_EVENT_HEADER2 *eventhdr = (TCG_EVENT_HEADER2*)buf;
    TCG_DIGEST2 *digest = eventhdr->Digests;
    TCG_EVENT2 *event = (TCG_EVENT2*)(buf + sizeof(*eventhdr) + TCG_DIGEST2_SHA1_SIZE);
    size_t size_event = 0, size_digest = 0;

    eventhdr->DigestCount = 1;
    digest->AlgorithmId = TPM2_ALG_SHA1;
    event->EventSize = 1;

    assert_false(parse_event2(eventhdr, sizeof(buf), &size_event, &size_digest));
}
static void test_foreach_event2_null(void **state){

    (void)state;

    assert_false(foreach_event2(NULL, 0, NULL, NULL, NULL, NULL));
}
static void test_foreach_event2(void **state){

    (void)state;
    char buf[sizeof(TCG_EVENT_HEADER2) + TCG_DIGEST2_SHA1_SIZE + sizeof(TCG_EVENT2) + 6] = { 0, };
    TCG_EVENT_HEADER2 *eventhdr = (TCG_EVENT_HEADER2*)buf;
    TCG_DIGEST2 *digest = eventhdr->Digests;
    TCG_EVENT2 *event = (TCG_EVENT2*)((uintptr_t)digest + TCG_DIGEST2_SHA1_SIZE);

    eventhdr->DigestCount = 1;
    digest->AlgorithmId = TPM2_ALG_SHA1;
    event->EventSize = 6;

    will_return(test_event2hdr_callback, true);
    will_return(foreach_digest2_test_callback, true);
    will_return(test_event2_callback, true);
    assert_true(foreach_event2(eventhdr, sizeof(buf), test_event2hdr_callback,
                               foreach_digest2_test_callback,
                               test_event2_callback, NULL));
}
static void test_foreach_event2_event2hdr_fail(void **state){

    (void)state;
    char buf[sizeof(TCG_EVENT_HEADER2) + TCG_DIGEST2_SHA1_SIZE + sizeof(TCG_EVENT2) + 1] = { 0, };
    TCG_EVENT_HEADER2 *eventhdr = (TCG_EVENT_HEADER2*)buf;
    TCG_DIGEST2 *digest = eventhdr->Digests;
    TCG_EVENT2 *event = (TCG_EVENT2*)(buf + sizeof(*eventhdr) + TCG_DIGEST2_SHA1_SIZE);

    eventhdr->DigestCount = 1;
    digest->AlgorithmId = TPM2_ALG_SHA1;
    event->EventSize = 1;

    will_return(test_event2hdr_callback, false);
    assert_false(foreach_event2(eventhdr, sizeof(buf), test_event2hdr_callback,
                                NULL, NULL, NULL));
}
static void test_foreach_event2_digest2_fail(void **state){

    (void)state;
    char buf[sizeof(TCG_EVENT_HEADER2) + TCG_DIGEST2_SHA1_SIZE + sizeof(TCG_EVENT2) + 1] = { 0, };
    TCG_EVENT_HEADER2 *eventhdr = (TCG_EVENT_HEADER2*)buf;
    TCG_DIGEST2 *digest = eventhdr->Digests;
    TCG_EVENT2 *event = (TCG_EVENT2*)(buf + sizeof(*eventhdr) + TCG_DIGEST2_SHA1_SIZE);

    eventhdr->DigestCount = 1;
    digest->AlgorithmId = TPM2_ALG_SHA1;
    event->EventSize = 1;

    will_return(test_event2hdr_callback, true);
    will_return(foreach_digest2_test_callback, false);
    assert_false(foreach_event2(eventhdr, sizeof(buf),
                                test_event2hdr_callback,
                                foreach_digest2_test_callback, NULL, NULL));
}
static void test_foreach_event2_parse_event2body_fail(void **state){

    (void)state;

    char buf[sizeof(TCG_EVENT_HEADER2) + TCG_DIGEST2_SHA1_SIZE + sizeof(TCG_EVENT2) + 1] = { 0, };
    TCG_EVENT_HEADER2 *eventhdr = (TCG_EVENT_HEADER2*)buf;
    TCG_DIGEST2 *digest = eventhdr->Digests;
    TCG_EVENT2 *event = (TCG_EVENT2*)(buf + sizeof(*eventhdr) + TCG_DIGEST2_SHA1_SIZE);

    eventhdr->DigestCount = 1;
    eventhdr->EventType = EV_EFI_VARIABLE_BOOT;
    digest->AlgorithmId = TPM2_ALG_SHA1;
    event->EventSize = 1;

    will_return(test_event2hdr_callback, true);
    will_return(foreach_digest2_test_callback, true);
    assert_false(foreach_event2(eventhdr, sizeof(buf),
                                test_event2hdr_callback,
                                foreach_digest2_test_callback, NULL, NULL));
}
static void test_foreach_event2_event2body_fail(void **state){

    (void)state;
    char buf[sizeof(TCG_EVENT_HEADER2) + TCG_DIGEST2_SHA1_SIZE + sizeof(TCG_EVENT2) + 1] = { 0, };
    TCG_EVENT_HEADER2 *eventhdr = (TCG_EVENT_HEADER2*)buf;
    TCG_DIGEST2 *digest = eventhdr->Digests;
    TCG_EVENT2 *event = (TCG_EVENT2*)(buf + sizeof(*eventhdr) + TCG_DIGEST2_SHA1_SIZE);

    eventhdr->DigestCount = 1;
    digest->AlgorithmId = TPM2_ALG_SHA1;
    event->EventSize = 1;

    will_return(test_event2hdr_callback, true);
    will_return(foreach_digest2_test_callback, true);
    will_return(test_event2_callback, false);
    assert_false(foreach_event2(eventhdr, sizeof(buf),
                                test_event2hdr_callback,
                                foreach_digest2_test_callback,
                                test_event2_callback, NULL));
}
static void test_parse_event2body_uefivar_badsize(void **state){

    (void)state;

    TCG_EVENT2 event = { 0, };

    assert_false(parse_event2body(&event, EV_EFI_VARIABLE_DRIVER_CONFIG));
}
#include <inttypes.h>
static void test_parse_event2body_uefivar_badlength(void **state){

    (void)state;

    char buf[sizeof(TCG_EVENT2) + sizeof(UEFI_VARIABLE_DATA) + sizeof(char16_t)] = { 0, };
    TCG_EVENT2 *event = (TCG_EVENT2*)buf;
    event->EventSize = sizeof(UEFI_VARIABLE_DATA) + sizeof(char16_t) - 1;
    UEFI_VARIABLE_DATA *data = (UEFI_VARIABLE_DATA*)event->Event;
    data->UnicodeNameLength = 1;

    assert_false(parse_event2body(event, EV_EFI_VARIABLE_DRIVER_CONFIG));
}
static void test_parse_event2body_postcode_badlength(void **state){

    (void)state;

    char buf[sizeof(TCG_EVENT2)] = { 0, };
    TCG_EVENT2 *event = (TCG_EVENT2*)buf;
    event->EventSize = sizeof(UEFI_PLATFORM_FIRMWARE_BLOB) - 1;

    assert_false(parse_event2body(event, EV_EFI_PLATFORM_FIRMWARE_BLOB));
}
static void test_specid_event_nohdr(void **state){

    (void)state;

    TCG_EVENT event = { 0, };
    TCG_EVENT_HEADER2 *next = NULL;

    assert_false(specid_event(&event, sizeof(event) - 1, &next));
}
static void test_specid_event_badeventtype(void **state){

    (void)state;

    TCG_EVENT event = { .eventType = EV_ACTION, };
    TCG_EVENT_HEADER2 *next = NULL;

    assert_false(specid_event(&event, sizeof(event), &next));
}
static void test_specid_event_badpcrindex(void **state){

    (void)state;

    TCG_EVENT event = {
        .eventType = EV_NO_ACTION,
        .pcrIndex = 1,
    };
    TCG_EVENT_HEADER2 *next = NULL;

    assert_false(specid_event(&event, sizeof(event), &next));
}
static void test_specid_event_baddigest(void **state){

    (void)state;

    TCG_EVENT event = {
        .eventType = EV_NO_ACTION,
        .digest = { 0x01, },
    };
    TCG_EVENT_HEADER2 *next = NULL;

    assert_false(specid_event(&event, sizeof(event), &next));

}
static void test_specid_event_badeventsize(void **state) {

    (void)state;

    TCG_EVENT *event;
    TCG_SPECID_EVENT *event_specid;
    TCG_EVENT_HEADER2 *next = NULL;
    char buf[sizeof(*event) + sizeof(*event_specid)] = { 0, };

    event = (TCG_EVENT*)buf;
    event->eventType = EV_NO_ACTION;
    event->eventDataSize = 1;

    assert_false(specid_event(event, sizeof(buf), &next));
}
static void test_specid_event_badsize(void **state){

    (void)state;

    TCG_EVENT *event;
    TCG_SPECID_EVENT *event_specid;
    TCG_EVENT_HEADER2 *next = NULL;
    char buf[sizeof(*event) + 1] = { 0, };

    event = (TCG_EVENT*)buf;
    event->eventType = EV_NO_ACTION;
    event->eventDataSize = sizeof(*event_specid);
    assert_false(specid_event(event, sizeof(buf), &next));
}
static void test_specid_event_noalgs(void **state) {

    (void)state;

    TCG_EVENT *event;
    TCG_SPECID_EVENT *event_specid;
    TCG_EVENT_HEADER2 *next = NULL;
    char buf[sizeof(*event) + sizeof(*event_specid)] = { 0, };

    event = (TCG_EVENT*)buf;
    event->eventType = EV_NO_ACTION;
    event->eventDataSize = sizeof(*event_specid);

    assert_false(specid_event(event, sizeof(buf), &next));
}
static void test_specid_event_nosizeforalgs(void **state) {

    (void)state;

    TCG_EVENT *event;
    TCG_SPECID_EVENT *event_specid;
    TCG_EVENT_HEADER2 *next = NULL;
    char buf[sizeof(*event) + sizeof(*event_specid)] = { 0, };

    event = (TCG_EVENT*)buf;
    event->eventType = EV_NO_ACTION;
    event->eventDataSize = sizeof(*event_specid);
    event_specid = (TCG_SPECID_EVENT*)event->event;
    event_specid->numberOfAlgorithms = 5;

    assert_false(specid_event(event, sizeof(buf), &next));
}
static void test_specid_event_nosizeforvendorstruct(void **state) {

    (void)state;

    TCG_EVENT *event;
    TCG_SPECID_EVENT *event_specid;
    TCG_SPECID_ALG *alg;
    TCG_EVENT_HEADER2 *next = NULL;
    char buf[sizeof(*event) + sizeof(*event_specid) + sizeof(*alg)] = { 0, };

    event = (TCG_EVENT*)buf;
    event->eventType = EV_NO_ACTION;
    event->eventDataSize = sizeof(*event_specid);
    event_specid = (TCG_SPECID_EVENT*)event->event;
    event_specid->numberOfAlgorithms = 1;

    assert_false(specid_event(event, sizeof(buf), &next));
}
static void test_specid_event_nosizeforvendordata(void **state) {

    (void)state;

    TCG_EVENT *event;
    TCG_SPECID_EVENT *event_specid;
    TCG_SPECID_ALG *alg;
    TCG_VENDOR_INFO *vendor;
    TCG_EVENT_HEADER2 *next = NULL;
    char buf[sizeof(*event) + sizeof(*event_specid) + sizeof(*alg) + sizeof(*vendor)] = { 0, };

    event = (TCG_EVENT*)buf;
    event->eventType = EV_NO_ACTION;
    event->eventDataSize = sizeof(*event_specid);
    event_specid = (TCG_SPECID_EVENT*)event->event;
    event_specid->numberOfAlgorithms = 1;
    vendor = (TCG_VENDOR_INFO*)((uintptr_t)event_specid->digestSizes + sizeof(*alg) * event_specid->numberOfAlgorithms);
    vendor->vendorInfoSize = 1;
    printf("data size: %zu\n", sizeof(buf));

    assert_false(specid_event(event, sizeof(buf), &next));
}
static void test_specid_event(void **state) {

    (void)state;

    TCG_EVENT *event;
    TCG_SPECID_EVENT *event_specid;
    TCG_SPECID_ALG *alg;
    TCG_VENDOR_INFO *vendor;
    TCG_EVENT_HEADER2 *next = NULL;
    char buf[sizeof(*event) + sizeof(*event_specid) + sizeof(*alg) + sizeof(*vendor) + 5] = { 0, };

    event = (TCG_EVENT*)buf;
    event->eventType = EV_NO_ACTION;
    event->eventDataSize = sizeof(*event_specid);
    event_specid = (TCG_SPECID_EVENT*)event->event;
    event_specid->numberOfAlgorithms = 1;
    vendor = (TCG_VENDOR_INFO*)((uintptr_t)event_specid->digestSizes + sizeof(*alg) * event_specid->numberOfAlgorithms);
    vendor->vendorInfoSize = 5;

    printf("sizeof(buf): %zu\n", sizeof(buf));

    assert_true(specid_event(event, sizeof(buf), &next));
}
int main(void) {

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_foreach_digest2_null),
        cmocka_unit_test(test_foreach_digest2_size),
        cmocka_unit_test(test_foreach_digest2),
        cmocka_unit_test(test_foreach_digest2_cbfail),
        cmocka_unit_test(test_foreach_digest2_cbnull),
        cmocka_unit_test(test_digest2_accumulator_callback),
        cmocka_unit_test(test_digest2_accumulator_callback_null),
        cmocka_unit_test(test_parse_event2_badhdr),
        cmocka_unit_test(test_parse_event2_baddigest),
        cmocka_unit_test(test_parse_event2_badeventsize),
        cmocka_unit_test(test_parse_event2_badeventbuf),
        cmocka_unit_test(test_foreach_event2_null),
        cmocka_unit_test(test_foreach_event2),
        cmocka_unit_test(test_foreach_event2_event2hdr_fail),
        cmocka_unit_test(test_foreach_event2_event2body_fail),
        cmocka_unit_test(test_foreach_event2_digest2_fail),
        cmocka_unit_test(test_foreach_event2_parse_event2body_fail),
        cmocka_unit_test(test_parse_event2body_uefivar_badsize),
        cmocka_unit_test(test_parse_event2body_uefivar_badlength),
        cmocka_unit_test(test_parse_event2body_postcode_badlength),
        cmocka_unit_test(test_specid_event_nohdr),
        cmocka_unit_test(test_specid_event_badeventtype),
        cmocka_unit_test(test_specid_event_badpcrindex),
        cmocka_unit_test(test_specid_event_baddigest),
        cmocka_unit_test(test_specid_event_badeventsize),
        cmocka_unit_test(test_specid_event_badsize),
        cmocka_unit_test(test_specid_event_noalgs),
        cmocka_unit_test(test_specid_event_nosizeforalgs),
        cmocka_unit_test(test_specid_event_nosizeforvendorstruct),
        cmocka_unit_test(test_specid_event_nosizeforvendordata),
        cmocka_unit_test(test_specid_event),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
