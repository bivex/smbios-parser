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
 * Test macOS hardware info reading
 */

#include <stdio.h>
#include "macos_hardware.h"

int main(void) {
    hw_info_t info;

    printf("=== macOS Hardware Info Test ===\n\n");

    if (hw_info_get(&info) != 0) {
        fprintf(stderr, "ERROR: Failed to read hardware info\n");
        return 1;
    }

    printf("IOPlatformUUID:  %s\n", info.platform_uuid);
    printf("UUID (copy):     %s\n", info.uuid);
    printf("Serial Number:   %s\n", info.serial_number);
    printf("Model:           %s\n", info.model);
    printf("Board ID:        %s\n", info.board_id);
    printf("Manufacturer:    %s\n", info.manufacturer);
    printf("Platform Name:   %s\n", info.platform_name);

    printf("\n=== Success ===\n");
    return 0;
}