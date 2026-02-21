/*
 * Debug test - check what properties are available
 */

#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

static void print_property(io_service_t service, const char *key_name)
{
    CFStringRef key = CFStringCreateWithCString(NULL, key_name, kCFStringEncodingUTF8);
    if (!key) return;

    CFTypeRef prop = IORegistryEntryCreateCFProperty(service, key, kCFAllocatorDefault, 0);
    CFRelease(key);

    if (!prop) {
        printf("  %-20s: <not found>\n", key_name);
        return;
    }

    printf("  %-20s: ", key_name);

    if (CFGetTypeID(prop) == CFStringGetTypeID()) {
        char buf[256];
        if (CFStringGetCString((CFStringRef)prop, buf, sizeof(buf), kCFStringEncodingUTF8)) {
            printf("%s\n", buf);
        } else {
            printf("<cfstring conversion error>\n");
        }
    } else if (CFGetTypeID(prop) == CFDataGetTypeID()) {
        CFDataRef data = (CFDataRef)prop;
        CFIndex len = CFDataGetLength(data);
        const UInt8 *bytes = CFDataGetBytePtr(data);
        printf("[CFData len=%ld] ", len);
        for (CFIndex i = 0; i < len && i < 64; i++) {
            if (bytes[i] >= 32 && bytes[i] < 127) {
                putchar(bytes[i]);
            } else {
                printf("\\x%02X", bytes[i]);
            }
        }
        if (len >= 64) printf("...");
        putchar('\n');
    } else {
        printf("<unknown type>\n");
    }

    CFRelease(prop);
}

int main(void)
{
    CFMutableDictionaryRef matching;
    io_service_t service;

    printf("=== IOKit Property Discovery ===\n\n");

    matching = IOServiceMatching("IOPlatformExpertDevice");
    if (!matching) {
        fprintf(stderr, "ERROR: IOServiceMatching failed\n");
        return 1;
    }

    service = IOServiceGetMatchingService(kIOMainPortDefault, matching);
    if (service == 0) {
        fprintf(stderr, "ERROR: IOServiceGetMatchingService failed\n");
        return 1;
    }

    printf("Found IOPlatformExpertDevice\n");

    /* Try common properties */
    print_property(service, kIOPlatformUUIDKey);
    print_property(service, "serial-number");
    print_property(service, "model");
    print_property(service, "board-id");
    print_property(service, "platform-name");
    print_property(service, "product-name");
    print_property(service, "compatible");
    print_property(service, "manufacturer");
    print_property(service, "location");

    IOObjectRelease(service);
    return 0;
}
