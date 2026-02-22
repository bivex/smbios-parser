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
 */

#include "macos_hardware.h"
#include <string.h>
#include <IOKit/IOKitLib.h>

int32_t cfstring_to_cstr(CFStringRef cfstr, char *out, size_t size) {
    if (cfstr == NULL || out == NULL || size == 0)
        return -1;

    /* Get UTF8 string length */
    CFIndex len = CFStringGetLength(cfstr);
    CFIndex max_bytes = CFStringGetMaximumSizeForEncoding(len, kCFStringEncodingUTF8);

    if (max_bytes >= (CFIndex) size)
        return -1; /* Buffer too small */

    /* Convert to C string */
    if (!CFStringGetCString(cfstr, out, size, kCFStringEncodingUTF8))
        return -1;

    return 0;
}

int32_t hw_info_get_string(CFStringRef key, char *out, size_t size) {
    CFMutableDictionaryRef matching;
    io_service_t service;
    CFTypeRef property;
    int32_t ret = -1;

    if (out == NULL || size == 0)
        return -1;

    out[0] = '\0';

    /* Get IOPlatformExpertDevice */
    matching = IOServiceMatching("IOPlatformExpertDevice");
    if (matching == NULL)
        return -1;

    service = IOServiceGetMatchingService(kIOMainPortDefault, matching);
    if (service == 0)
        return -1;

    /* Read property */
    property = IORegistryEntryCreateCFProperty(service, key, kCFAllocatorDefault, 0);
    if (property != NULL) {
        if (CFGetTypeID(property) == CFStringGetTypeID()) {
            ret = cfstring_to_cstr((CFStringRef) property, out, size);
        } else if (CFGetTypeID(property) == CFDataGetTypeID()) {
            /* Some properties return CFData (e.g. serial-number) */
            CFDataRef data = (CFDataRef) property;
            CFIndex len = CFDataGetLength(data);
            if (len > 0 && len < (CFIndex)(size - 1)) {
                memcpy(out, CFDataGetBytePtr(data), len);
                out[len] = '\0';
                ret = 0;
            }
        }
        CFRelease(property);
    }

    IOObjectRelease(service);
    return ret;
}

int32_t hw_info_get(hw_info_t *info) {
    if (info == NULL)
        return -1;

    memset(info, 0, sizeof(*info));

    /* IOPlatformUUID - required */
    if (hw_info_get_string(CFSTR(kIOPlatformUUIDKey),
                           info->platform_uuid, sizeof(info->platform_uuid)) != 0)
        goto error;

    /* Map to uuid field (same data) */
    strncpy(info->uuid, info->platform_uuid, sizeof(info->uuid) - 1);

    /* serial-number (CFData, not CFString) - required */
    if (hw_info_get_string(CFSTR("serial-number"),
                           info->serial_number, sizeof(info->serial_number)) != 0)
        goto error;

    /* model (CFData) - required */
    if (hw_info_get_string(CFSTR("model"),
                           info->model, sizeof(info->model)) != 0)
        goto error;

    /* Optional fields - don't fail if missing */
    hw_info_get_string(CFSTR("board-id"),
                       info->board_id, sizeof(info->board_id));
    hw_info_get_string(CFSTR("manufacturer"),
                       info->manufacturer, sizeof(info->manufacturer));
    hw_info_get_string(CFSTR("platform-name"),
                       info->platform_name, sizeof(info->platform_name));

    return 0;

error:
    memset(info, 0, sizeof(*info));
    return -1;
}