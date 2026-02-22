/**
 * Copyright (c) 2026 Bivex
 *
 * Author: Bivex
 * Available for contact via email: support@b-b.top
 * For up-to-date contact information:
 * https://github.com/bivex
 *
 * Created: 2026-02-21 03:43
 * Last Updated: 2026-02-21 03:43
 *
 * Licensed under the MIT License.
 * Commercial licensing available upon request.
 */

/*
 * macOS hardware info via IOKit
 * Reads: IOPlatformUUID, serial-number, model (board-id)
 *
 * Compile: -framework IOKit -framework CoreFoundation
 */

#ifndef MACOS_HARDWARE_H
#define MACOS_HARDWARE_H

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum string lengths */
enum { HW_INFO_STR_MAX = 256 };

/* Hardware info structure */
typedef struct {
    char uuid[HW_INFO_STR_MAX];
    char serial_number[HW_INFO_STR_MAX];
    char model[HW_INFO_STR_MAX];
    char board_id[HW_INFO_STR_MAX];
    char platform_uuid[HW_INFO_STR_MAX];
    char manufacturer[HW_INFO_STR_MAX];
    char platform_name[HW_INFO_STR_MAX];
} hw_info_t;

/**
 * Read hardware information from IOKit IOPlatformExpertDevice
 *
 * @param info  Output structure to fill
 * @return 0 on success, -1 on error
 */
int32_t hw_info_get(hw_info_t *info);

/**
 * Get a single string property from IOPlatformExpertDevice
 *
 * @param key   CFStringRef property name (e.g. CFSTR(kIOPlatformUUIDKey))
 * @param out   Output buffer
 * @param size  Buffer size
 * @return 0 on success, -1 on error
 */
int32_t hw_info_get_string(CFStringRef key, char *out, size_t size);

/**
 * Convert CFString to C string (UTF-8)
 *
 * @param cfstr CFString to convert
 * @param out   Output buffer
 * @param size  Buffer size
 * @return 0 on success, -1 on error
 */
int32_t cfstring_to_cstr(CFStringRef cfstr, char *out, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* MACOS_HARDWARE_H */