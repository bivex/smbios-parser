/*
 * Unit test for smbios-parser using a synthetic SMBIOS 2.0 binary.
 * Runs on any platform (no /sys/firmware/dmi/tables access required).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "smbios.h"

#define PASS(msg) printf("PASS: %s\n", msg)
#define FAIL(msg) do { fprintf(stderr, "FAIL: %s\n", msg); exit(1); } while(0)
#define CHECK(cond, msg) do { if (!(cond)) FAIL(msg); } while(0)

/* Platform detection at runtime */
static const char* platform_name(void)
{
#if defined(SMBIOS_WINDOWS)
    return "Windows";
#elif defined(SMBIOS_MACOS)
    return "macOS";
#elif defined(SMBIOS_LINUX)
    return "Linux";
#elif defined(SMBIOS_BSD)
    return "BSD";
#else
    return "Unknown";
#endif
}

/*
 * Build a minimal SMBIOS 2.0 binary in-memory:
 *   [0..31]  - 2.x entry point (32 bytes)
 *   [32..]   - SMBIOS structures:
 *                Type 0  (BIOS Info)
 *                Type 1  (System Info)
 *                Type 127 (End of Table)
 */
static size_t build_smbios(uint8_t *buf, size_t cap)
{
    (void)cap;
    memset(buf, 0, cap);

    /* --- Entry point (offsets 0-31) --- */
    /* Anchor "_SM_" */
    buf[0] = '_'; buf[1] = 'S'; buf[2] = 'M'; buf[3] = '_';
    buf[5]  = 0x1F;  /* entry point length (required by parser) */
    buf[6]  = 2;     /* SMBIOS major version */
    buf[7]  = 0;     /* SMBIOS minor version */
    buf[10] = 0;     /* entry point revision (required to be 0 for 2.x) */
    /* Intermediate anchor "_DMI_" at offset 16 */
    buf[16] = '_'; buf[17] = 'D'; buf[18] = 'M'; buf[19] = 'I'; buf[20] = '_';
    buf[28] = 3; buf[29] = 0;   /* number of structures */
    buf[30] = 0x20;              /* BCD revision */

    /* Structure table starts at offset 32 in our buffer */
    uint8_t *p = buf + 32;

    /* --- Type 0: BIOS Info (SMBIOS 2.0 = 18 bytes) --- */
    p[0] = 0;            /* type */
    p[1] = 0x12;         /* formatted area length = 18 */
    p[2] = 0x00; p[3] = 0x00; /* handle */
    p[4] = 1;            /* Vendor_  -> string #1 */
    p[5] = 2;            /* BIOSVersion_ -> string #2 */
    p[6] = 0x00; p[7] = 0xE0; /* BIOSStartingAddressSegment */
    p[8] = 3;            /* BIOSReleaseDate_ -> string #3 */
    p[9] = 0;            /* BIOSROMSize */
    /* BIOSCharacteristics[8] left zero */
    p += 0x12;
    /* String table */
    const char *bios_s1 = "ACME Corp";
    const char *bios_s2 = "2.5.0";
    const char *bios_s3 = "12/31/2023";
    memcpy(p, bios_s1, strlen(bios_s1) + 1); p += strlen(bios_s1) + 1;
    memcpy(p, bios_s2, strlen(bios_s2) + 1); p += strlen(bios_s2) + 1;
    memcpy(p, bios_s3, strlen(bios_s3) + 1); p += strlen(bios_s3) + 1;
    *p++ = 0; /* end-of-strings terminator */

    /* --- Type 1: System Info (SMBIOS 2.0 = 8 bytes formatted area) --- */
    p[0] = 1;            /* type */
    p[1] = 8;            /* formatted area length */
    p[2] = 0x01; p[3] = 0x00; /* handle */
    p[4] = 1;            /* Manufacturer_ -> string #1 */
    p[5] = 2;            /* ProductName_ -> string #2 */
    p[6] = 3;            /* Version_ -> string #3 */
    p[7] = 4;            /* SerialNumber_ -> string #4 */
    p += 8;
    const char *sys_s1 = "ACME Inc";
    const char *sys_s2 = "Test Box 9000";
    const char *sys_s3 = "rev-A";
    const char *sys_s4 = "SN-001122";
    memcpy(p, sys_s1, strlen(sys_s1) + 1); p += strlen(sys_s1) + 1;
    memcpy(p, sys_s2, strlen(sys_s2) + 1); p += strlen(sys_s2) + 1;
    memcpy(p, sys_s3, strlen(sys_s3) + 1); p += strlen(sys_s3) + 1;
    memcpy(p, sys_s4, strlen(sys_s4) + 1); p += strlen(sys_s4) + 1;
    *p++ = 0;

    /* --- Type 127: End of Table --- */
    p[0] = 127;
    p[1] = 4;
    p[2] = 0xFF; p[3] = 0xFF;
    p += 4;
    *p++ = 0; *p++ = 0; /* empty string table */

    return (size_t)(p - buf);
}

int main(void)
{
    uint8_t buf[512];
    size_t size = build_smbios(buf, sizeof(buf));

    printf("=== smbios-parser unit tests ===\n");
    printf("Platform: %s\n\n", platform_name());

    /* --- Test 1: initialize with valid data --- */
    struct ParserContext ctx;
    int32_t ret = smbios_initialize(&ctx, buf, size, SMBIOS_2_0);
    CHECK(ret == SMBERR_OK, "smbios_initialize with valid 2.0 data");
    PASS("smbios_initialize (valid data)");

    /* --- Test 2: version reporting --- */
    int32_t selected = 0, original = 0;
    ret = smbios_get_version(&ctx, &selected, &original);
    CHECK(ret == SMBERR_OK, "smbios_get_version returned error");
    CHECK(original == SMBIOS_2_0, "original version should be 2.0");
    printf("PASS: smbios_get_version  selected=%d.%d  original=%d.%d\n",
           selected >> 8, selected & 0xFF, original >> 8, original & 0xFF);

    /* --- Test 3: first entry = TYPE_BIOS_INFO --- */
    const struct Entry *entry = NULL;
    ret = smbios_next(&ctx, &entry);
    CHECK(ret == SMBERR_OK, "smbios_next first entry");
    CHECK(entry->type == TYPE_BIOS_INFO, "first entry should be TYPE_BIOS_INFO");
    PASS("first entry is TYPE_BIOS_INFO");

    /* --- Test 4: BIOS Info string fields --- */
    CHECK(strcmp(entry->data.bios_info.Vendor, "ACME Corp") == 0,
          "Vendor field mismatch");
    CHECK(strcmp(entry->data.bios_info.BIOSVersion, "2.5.0") == 0,
          "BIOSVersion field mismatch");
    CHECK(strcmp(entry->data.bios_info.BIOSReleaseDate, "12/31/2023") == 0,
          "BIOSReleaseDate field mismatch");
    printf("PASS: BiosInfo fields  Vendor=\"%s\"  Version=\"%s\"  Date=\"%s\"\n",
           entry->data.bios_info.Vendor,
           entry->data.bios_info.BIOSVersion,
           entry->data.bios_info.BIOSReleaseDate);

    /* --- Test 5: smbios_get_string via index --- */
    const char *s = smbios_get_string(entry, 1);
    CHECK(s != NULL && strcmp(s, "ACME Corp") == 0, "smbios_get_string(1)");
    s = smbios_get_string(entry, 2);
    CHECK(s != NULL && strcmp(s, "2.5.0") == 0, "smbios_get_string(2)");
    s = smbios_get_string(entry, 999);
    CHECK(s == NULL, "smbios_get_string out-of-range should return NULL");
    PASS("smbios_get_string indexing");

    /* --- Test 6: second entry = TYPE_SYSTEM_INFO --- */
    ret = smbios_next(&ctx, &entry);
    CHECK(ret == SMBERR_OK, "smbios_next second entry");
    CHECK(entry->type == TYPE_SYSTEM_INFO, "second entry should be TYPE_SYSTEM_INFO");
    CHECK(strcmp(entry->data.system_info.Manufacturer, "ACME Inc") == 0,
          "Manufacturer mismatch");
    CHECK(strcmp(entry->data.system_info.ProductName, "Test Box 9000") == 0,
          "ProductName mismatch");
    printf("PASS: SystemInfo  Manufacturer=\"%s\"  Product=\"%s\"\n",
           entry->data.system_info.Manufacturer,
           entry->data.system_info.ProductName);

    /* --- Test 7: end-of-stream after Type 127 --- */
    ret = smbios_next(&ctx, &entry);
    CHECK(ret == SMBERR_END_OF_STREAM, "expected SMBERR_END_OF_STREAM after Type 127");
    PASS("SMBERR_END_OF_STREAM detected at Type 127");

    /* --- Test 8: smbios_reset re-iterates from start --- */
    ret = smbios_reset(&ctx);
    CHECK(ret == SMBERR_OK, "smbios_reset failed");
    ret = smbios_next(&ctx, &entry);
    CHECK(ret == SMBERR_OK && entry->type == TYPE_BIOS_INFO,
          "after reset, first entry should be TYPE_BIOS_INFO again");
    PASS("smbios_reset re-iterates from beginning");

    /* --- Test 9: reject too-small buffer --- */
    struct ParserContext ctx2;
    ret = smbios_initialize(&ctx2, buf, 10, SMBIOS_2_0);
    CHECK(ret == SMBERR_INVALID_DATA, "too-small buffer should return SMBERR_INVALID_DATA");
    PASS("reject buffer smaller than header (SMBERR_INVALID_DATA)");

    /* --- Test 10: reject garbage data --- */
    uint8_t garbage[64];
    memset(garbage, 0xAB, sizeof(garbage));
    ret = smbios_initialize(&ctx2, garbage, sizeof(garbage), SMBIOS_2_0);
    CHECK(ret == SMBERR_INVALID_DATA, "garbage buffer should return SMBERR_INVALID_DATA");
    PASS("reject garbage anchor (SMBERR_INVALID_DATA)");

    printf("\n=== All 10 tests passed ===\n");
    return 0;
}
