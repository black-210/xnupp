#include <stdint.h>
#include <stddef.h>

/*
 * xnu++ kernel/platform layer
 *
 * Provider model:
 *
 *                 xnu++
 *                   |
 *          +--------+--------+
 *          |                 |
 *     XNU provider      Native provider
 *          |                 |
 *       XNU/Mach        xnu++ platform
 *       BSD/IOKit
 *
 * XNU remains the primary provider when linked.
 * Native code provides the xnu++ platform provider.
 */

#define VGA_MEMORY       0xB8000
#define VGA_WIDTH        80
#define VGA_HEIGHT       25

#define HEAP_START       0x00100000
#define HEAP_SIZE        0x00100000

#define PIC1_COMMAND     0x20
#define PIC1_DATA        0x21
#define PIC2_COMMAND     0xA0
#define PIC2_DATA        0xA1
#define PIC_EOI          0x20

#define KBD_DATA         0x60
#define KBD_STATUS       0x64

#define IDT_ENTRIES      256

/* ============================================================
 * Provider interface
 * ============================================================ */

enum xnuxx_provider_type {
    XNUXX_PROVIDER_NONE   = 0,
    XNUXX_PROVIDER_XNU    = 1,
    XNUXX_PROVIDER_NATIVE = 2
};

/*
 * These are weak so the xnu++ platform provider can still link
 * when the XNU provider is not part of the current image.
 *
 * A real XNU provider supplies these symbols.
 */
extern int xnuxx_xnu_provider_available(void)
    __attribute__((weak));

extern int xnuxx_xnu_provider_init(void)
    __attribute__((weak));

extern int xnuxx_xnu_provider_start(void)
    __attribute__((weak));

extern void xnuxx_xnu_provider_idle(void)
    __attribute__((weak));

static enum xnuxx_provider_type active_provider =
    XNUXX_PROVIDER_NONE;

/* ============================================================
 * VGA console
 * ============================================================ */

static volatile uint16_t *vga =
    (volatile uint16_t *)VGA_MEMORY;

static size_t cursor_x;
static size_t cursor_y;

static uint8_t terminal_color = 0x1F;

static uint16_t vga_entry(unsigned char c, uint8_t color)
{
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void terminal_clear(void)
{
    size_t y;
    size_t x;

    for (y = 0; y < VGA_HEIGHT; y++) {
        for (x = 0; x < VGA_WIDTH; x++) {
            vga[y * VGA_WIDTH + x] =
                vga_entry(' ', terminal_color);
        }
    }

    cursor_x = 0;
    cursor_y = 0;
}

static void terminal_scroll(void)
{
    size_t y;
    size_t x;

    if (cursor_y < VGA_HEIGHT)
        return;

    for (y = 1; y < VGA_HEIGHT; y++) {
        for (x = 0; x < VGA_WIDTH; x++) {
            vga[(y - 1) * VGA_WIDTH + x] =
                vga[y * VGA_WIDTH + x];
        }
    }

    for (x = 0; x < VGA_WIDTH; x++) {
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            vga_entry(' ', terminal_color);
    }

    cursor_y = VGA_HEIGHT - 1;
}

static void terminal_putchar(char c)
{
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        terminal_scroll();
        return;
    }

    if (c == '\r') {
        cursor_x = 0;
        return;
    }

    if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga[cursor_y * VGA_WIDTH + cursor_x] =
                vga_entry(' ', terminal_color);
        }
        return;
    }

    vga[cursor_y * VGA_WIDTH + cursor_x] =
        vga_entry((unsigned char)c, terminal_color);

    cursor_x++;

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    terminal_scroll();
}

static void terminal_write(const char *text)
{
    if (text == NULL)
        return;

    while (*text != '\0') {
        terminal_putchar(*text);
        text++;
    }
}

static __attribute__((unused)) void terminal_hex(uint32_t value)
{
    static const char digits[] =
        "0123456789ABCDEF";

    int i;

    terminal_write("0x");

    for (i = 7; i >= 0; i--) {
        terminal_putchar(
            digits[(value >> (i * 4)) & 0xF]
        );
    }
}

static __attribute__((unused)) void terminal_uint(uint32_t value)
{
    char buffer[10];
    size_t count = 0;

    if (value == 0) {
        terminal_putchar('0');
        return;
    }

    while (value != 0 && count < sizeof(buffer)) {
        buffer[count++] =
            (char)('0' + (value % 10));
        value /= 10;
    }

    while (count != 0)
        terminal_putchar(buffer[--count]);
}

/* ============================================================
 * CPU / I/O
 * ============================================================ */

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static inline void io_wait(void)
{
    outb(0x80, 0);
}

static inline void cpu_cli(void)
{
    __asm__ volatile ("cli");
    (void)multiboot_info;
}

static inline void cpu_sti(void)
{
    __asm__ volatile ("sti");
}

static inline void cpu_hlt(void)
{
    __asm__ volatile ("hlt");
}

/* ============================================================
 * GDT
 * ============================================================ */

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[3];
static struct gdt_ptr gdt_descriptor;

static void gdt_set_entry(
    int index,
    uint32_t base,
    uint32_t limit,
    uint8_t access,
    uint8_t granularity)
{
    gdt[index].base_low =
        (uint16_t)(base & 0xFFFF);

    gdt[index].base_middle =
        (uint8_t)((base >> 16) & 0xFF);

    gdt[index].base_high =
        (uint8_t)((base >> 24) & 0xFF);

    gdt[index].limit_low =
        (uint16_t)(limit & 0xFFFF);

    gdt[index].granularity =
        (uint8_t)((limit >> 16) & 0x0F);

    gdt[index].granularity |=
        granularity & 0xF0;

    gdt[index].access = access;
}

static void gdt_init(void)
{
    gdt_descriptor.limit =
        sizeof(gdt) - 1;

    gdt_descriptor.base =
        (uint64_t)(uintptr_t)&gdt;

    gdt_set_entry(
        0, 0, 0, 0, 0
    );

    gdt_set_entry(
        1,
        0,
        0xFFFFFFFF,
        0x9A,
        0xCF
    );

    gdt_set_entry(
        2,
        0,
        0xFFFFFFFF,
        0x92,
        0xCF
    );

    __asm__ volatile (
        "lgdt %0\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "pushq $0x08\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:"
        :
        : "m"(gdt_descriptor)
        : "eax"
    );
}

/* ============================================================
 * IDT
 * ============================================================ */

struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idt_descriptor;

static void idt_set_gate(
    uint8_t number,
    uint32_t base,
    uint16_t selector,
    uint8_t flags)
{
    idt[number].base_low =
        (uint16_t)(base & 0xFFFF);

    idt[number].base_high =
        (uint16_t)((base >> 16) & 0xFFFF);

    idt[number].selector = selector;
    idt[number].zero = 0;
    idt[number].flags = flags;
}

static void idt_init(void)
{
    int i;

    idt_descriptor.limit =
        sizeof(idt) - 1;

    idt_descriptor.base =
        (uint64_t)(uintptr_t)&idt;

    for (i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(
            (uint8_t)i,
            0,
            0x08,
            0x8E
        );
    }

    __asm__ volatile (
        "lidt %0"
        :
        : "m"(idt_descriptor)
    );
}

/* ============================================================
 * PIC
 * ============================================================ */

static void pic_init(void)
{
    uint8_t master_mask;
    uint8_t slave_mask;

    master_mask = inb(PIC1_DATA);
    slave_mask = inb(PIC2_DATA);

    outb(PIC1_COMMAND, 0x11);
    io_wait();

    outb(PIC2_COMMAND, 0x11);
    io_wait();

    outb(PIC1_DATA, 0x20);
    io_wait();

    outb(PIC2_DATA, 0x28);
    io_wait();

    outb(PIC1_DATA, 0x04);
    io_wait();

    outb(PIC2_DATA, 0x02);
    io_wait();

    outb(PIC1_DATA, 0x01);
    io_wait();

    outb(PIC2_DATA, 0x01);
    io_wait();

    outb(PIC1_DATA, master_mask);
    outb(PIC2_DATA, slave_mask);
}

static __attribute__((unused)) void pic_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);

    outb(PIC1_COMMAND, PIC_EOI);
}

/* ============================================================
 * Keyboard
 * ============================================================ */

static const char keyboard_map[128] = {
    0,
    27,
    '1','2','3','4','5','6','7','8','9','0',
    '-','=',
    '\b',
    '\t',
    'q','w','e','r','t','y','u','i','o','p',
    '[',']',
    '\n',
    0,
    'a','s','d','f','g','h','j','k','l',
    ';','\'','`',
    0,
    '\\',
    'z','x','c','v','b','n','m',
    ',','.','/',
    0,
    '*',
    0,
    ' '
};

static int keyboard_read(char *character)
{
    uint8_t status;
    uint8_t scancode;

    if (character == NULL)
        return 0;

    status = inb(KBD_STATUS);

    if ((status & 1) == 0)
        return 0;

    scancode = inb(KBD_DATA);

    if (scancode & 0x80)
        return 0;

    if (scancode >= sizeof(keyboard_map))
        return 0;

    if (keyboard_map[scancode] == 0)
        return 0;

    *character = keyboard_map[scancode];

    return 1;
}

/* ============================================================
 * Native memory layer
 * ============================================================ */

static uintptr_t heap_current =
    HEAP_START;

static uintptr_t heap_end =
    HEAP_START + HEAP_SIZE;

static void *native_alloc(size_t size)
{
    uintptr_t address;
    uintptr_t aligned;

    if (size == 0)
        return NULL;

    aligned =
        (size + 15) & ~((uintptr_t)15);

    if (heap_current + aligned > heap_end)
        return NULL;

    address = heap_current;
    heap_current += aligned;

    return (void *)address;
}

static void native_free(void *ptr)
{
    (void)ptr;
}

/* ============================================================
 * Native provider
 * ============================================================ */

static int native_provider_init(void)
{
    gdt_init();
    idt_init();
    pic_init();

    return 0;
}

static int native_provider_start(void)
{
    volatile uint32_t *test;
    test = (volatile uint32_t *)native_alloc(4);

    if (test == NULL)
        return -1;

    *test = 0xDEADBEEF;

    if (*test != 0xDEADBEEF)
        return -1;

    native_free((void *)test);

    return 0;
}

/* ============================================================
 * XNU provider
 * ============================================================ */

static int xnu_provider_available(void)
{
    if (xnuxx_xnu_provider_available == NULL)
        return 0;

    return xnuxx_xnu_provider_available() == 0;
}

static int xnu_provider_init(void)
{
    if (xnuxx_xnu_provider_init == NULL)
        return -1;

    return xnuxx_xnu_provider_init();
}

static int xnu_provider_start(void)
{
    if (xnuxx_xnu_provider_start == NULL)
        return -1;

    return xnuxx_xnu_provider_start();
}

/* ============================================================
 * Provider selection
 * ============================================================ */

static int provider_init(void)
{
    /*
     * XNU is preferred whenever the actual XNU provider
     * is linked into the image.
     */
    if (xnu_provider_available()) {
        if (xnu_provider_init() == 0) {
            active_provider =
                XNUXX_PROVIDER_XNU;

            return 0;
        }
    }

    /*
     * Native provider is an xnu++ platform provider.
     */
    if (native_provider_init() == 0) {
        active_provider =
            XNUXX_PROVIDER_NATIVE;

        return 0;
    }

    active_provider =
        XNUXX_PROVIDER_NONE;

    return -1;
}

static int provider_start(void)
{
    if (active_provider ==
        XNUXX_PROVIDER_XNU) {

        return xnu_provider_start();
    }

    if (active_provider ==
        XNUXX_PROVIDER_NATIVE) {

        return native_provider_start();
    }

    return -1;
}

/* ============================================================
 * Panic
 * ============================================================ */

static void kernel_panic(const char *message)
{
    cpu_cli();

    terminal_write("\n\nxnu++ KERNEL PANIC\n");
    terminal_write(message);
    terminal_write("\n");

    for (;;) {
        cpu_hlt();
    }
}

/* ============================================================
 * Kernel entry
 * ============================================================ */

void kernel_main(uint32_t multiboot_info)
{
    __asm__ volatile ("cli");
    (void)multiboot_info;
    terminal_clear();

    terminal_write(
        "xnu++ platform\n"
        "==============\n"
        "version: 0.2.0\n"
        "architecture: i386\n"
        "provider architecture: XNU + native\n\n"
    );

    terminal_write(
        "initializing provider...\n"
    );

    if (provider_init() != 0)
        kernel_panic("provider initialization failed");

    if (active_provider ==
        XNUXX_PROVIDER_XNU) {

        terminal_write(
            "provider: XNU\n"
            "XNU provider: active\n"
        );
    } else {
        terminal_write(
            "provider: native\n"
            "XNU provider: unavailable\n"
        );
    }

    terminal_write(
        "\nstarting provider...\n"
    );

    if (provider_start() != 0)
        kernel_panic("provider start failed");

    terminal_write(
        "provider started\n"
        "xnu++ platform initialized\n"
    );

    /*
     * Native console/keyboard loop is only used by the
     * xnu++ platform provider.
     *
     * A real XNU provider owns its own scheduler/idle path.
     */
    if (active_provider ==
        XNUXX_PROVIDER_XNU) {

        if (xnuxx_xnu_provider_idle != NULL) {
            for (;;) {
                xnuxx_xnu_provider_idle();
            }
        }

        for (;;) {
            cpu_hlt();
        }
    }

    cpu_sti();

    for (;;) {
        char character;

        if (keyboard_read(&character))
            terminal_putchar(character);

        cpu_hlt();
    }
}
