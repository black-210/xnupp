#include <stdint.h>

void boot_platform_init(void);
void boot_platform_start(void);
void kernel_main(uint32_t multiboot_info);

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289u

void boot_entry(uint32_t magic, uint32_t multiboot_info)
{
    __asm__ volatile ("cli");

    boot_platform_init();

    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        for (;;) {
            __asm__ volatile ("cli\n\thlt");
        }
    }

    boot_platform_start();

    kernel_main(multiboot_info);

    for (;;) {
        __asm__ volatile ("cli\n\thlt");
    }
}
void boot_entry_loop(void){
    __asm__ volatile ("cli");

    boot_platform_init();

    for (;;) {
        __asm__ volatile ("cli\n\thlt");

    for if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        for (;;) {
            __asm__ volatile ("cli\n\thlt");
        }
    }
}