/*
 * xnu++ native kernel
 *
 * Semi-independent kernel layer.
 * XNU is optional as a provider; this kernel does not depend on it.
 *
 * Target: i386 freestanding
 */

#include <stdint.h>
#include <stddef.h>

/* =========================================================
 * Hardware
 * ========================================================= */

#define VGA_ADDR        0xB8000
#define VGA_WIDTH       80
#define VGA_HEIGHT      25

#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1
#define PIC_EOI         0x20

#define KBD_DATA        0x60
#define KBD_STATUS      0x64

#define HEAP_START      0x00100000
#define HEAP_SIZE       0x00100000
#define HEAP_END        (HEAP_START + HEAP_SIZE)

#define IDT_ENTRIES     256
#define GDT_ENTRIES     3

#define KERNEL_VERSION  "0.1.0-native"

/* =========================================================
 * Basic types
 * ========================================================= */

typedef void (*isr_handler_t)(uint32_t);

/* =========================================================
 * VGA console
 * ========================================================= */

static volatile uint16_t *const vga =
    (volatile uint16_t *)VGA_ADDR;

static uint8_t terminal_color = 0x1F;
static size_t cursor_x;
static size_t cursor_y;

static void
terminal_clear(void)
{
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga[y * VGA_WIDTH + x] =
                ((uint16_t)terminal_color << 8) | ' ';
        }
    }

    cursor_x = 0;
    cursor_y = 0;
}

static void
terminal_scroll(void)
{
    if (cursor_y < VGA_HEIGHT)
        return;

    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga[(y - 1) * VGA_WIDTH + x] =
                vga[y * VGA_WIDTH + x];
        }
    }

    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            ((uint16_t)terminal_color << 8) | ' ';
    }

    cursor_y = VGA_HEIGHT - 1;
}

static void
terminal_putchar(char c)
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
                ((uint16_t)terminal_color << 8) | ' ';
        }
        return;
    }

    vga[cursor_y * VGA_WIDTH + cursor_x] =
        ((uint16_t)terminal_color << 8) | (uint8_t)c;

    cursor_x++;

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
        terminal_scroll();
    }
}

static void
terminal_write(const char *text)
{
    if (text == NULL)
        return;

    while (*text)
        terminal_putchar(*text++);
}

static void
terminal_write_hex(uint32_t value)
{
    static const char digits[] =
        "0123456789ABCDEF";

    terminal_write("0x");

    for (int i = 7; i >= 0; i--)
        terminal_putchar(digits[(value >> (i * 4)) & 0xF]);
}

static void
terminal_write_uint(uint32_t value)
{
    char buffer[10];
    size_t i = 0;

    if (value == 0) {
        terminal_putchar('0');
        return;
    }

    while (value > 0) {
        buffer[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (i > 0)
        terminal_putchar(buffer[--i]);
}

/* =========================================================
 * Port I/O
 * ========================================================= */

static inline uint8_t
inb(uint16_t port)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static inline void
outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline void
io_wait(void)
{
    outb(0x80, 0);
}

/* =========================================================
 * CPU
 * ========================================================= */

static inline void
cpu_cli(void)
{
    __asm__ volatile ("cli");
}

static inline void
cpu_sti(void)
{
    __asm__ volatile ("sti");
}

static inline void
cpu_hlt(void)
{
    __asm__ volatile ("hlt");
}

/* =========================================================
 * GDT
 * ========================================================= */

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gdt_descriptor;

static void
gdt_set_entry(
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

static void
gdt_init(void)
{
    gdt_descriptor.limit =
        sizeof(gdt) - 1;

    gdt_descriptor.base =
        (uint32_t)&gdt;

    gdt_set_entry(
        0, 0, 0,
        0, 0
    );

    gdt_set_entry(
        1, 0, 0xFFFFFFFF,
        0x9A, 0xCF
    );

    gdt_set_entry(
        2, 0, 0xFFFFFFFF,
        0x92, 0xCF
    );

    __asm__ volatile (
        "lgdt %0\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "ljmp $0x08, $1f\n"
        "1:\n"
        :
        : "m"(gdt_descriptor)
        : "ax"
    );
}

/* =========================================================
 * IDT
 * ========================================================= */

struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idt_descriptor;

static isr_handler_t handlers[IDT_ENTRIES];

static void
idt_set_gate(
    uint8_t vector,
    uint32_t base,
    uint16_t selector,
    uint8_t flags)
{
    idt[vector].base_low =
        (uint16_t)(base & 0xFFFF);

    idt[vector].base_high =
        (uint16_t)((base >> 16) & 0xFFFF);

    idt[vector].selector = selector;
    idt[vector].zero = 0;
    idt[vector].flags = flags;
}

static void
idt_init(void)
{
    idt_descriptor.limit =
        sizeof(idt) - 1;

    idt_descriptor.base =
        (uint32_t)&idt;

    for (size_t i = 0; i < IDT_ENTRIES; i++) {
        idt[i].base_low = 0;
        idt[i].base_high = 0;
        idt[i].selector = 0x08;
        idt[i].zero = 0;
        idt[i].flags = 0x8E;
        handlers[i] = NULL;
    }

    /*
     * Actual ISR entry points can be installed
     * by the architecture layer.
     */

    __asm__ volatile (
        "lidt %0"
        :
        : "m"(idt_descriptor)
    );
}

/* =========================================================
 * PIC
 * ========================================================= */

static void
pic_remap(void)
{
    uint8_t master_mask = inb(PIC1_DATA);
    uint8_t slave_mask  = inb(PIC2_DATA);

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

static void
pic_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);

    outb(PIC1_COMMAND, PIC_EOI);
}

/* =========================================================
 * Keyboard
 * ========================================================= */

static const char keyboard_map[128] = {
    0,   27,
    '1','2','3','4','5','6','7','8','9','0',
    '-','=', '\b',
    '\t',
    'q','w','e','r','t','y','u','i','o','p',
    '[',']','\n',
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

static char
keyboard_read(void)
{
    uint8_t status;
    uint8_t scancode;

    status = inb(KBD_STATUS);

    if ((status & 1) == 0)
        return 0;

    scancode = inb(KBD_DATA);

    if (scancode & 0x80)
        return 0;

    if (scancode >= sizeof(keyboard_map))
        return 0;

    return keyboard_map[scancode];
}

static void
keyboard_poll(void)
{
    char c = keyboard_read();

    if (c != 0)
        terminal_putchar(c);
}

/* =========================================================
 * Heap
 * ========================================================= */

static uintptr_t heap_current =
    HEAP_START;

static uintptr_t
align_up(uintptr_t value, uintptr_t alignment)
{
    return (value + alignment - 1) &
           ~(alignment - 1);
}

static void *
kmalloc(size_t size)
{
    uintptr_t address;

    if (size == 0)
        return NULL;

    address = align_up(heap_current, 16);

    if (address > HEAP_END ||
        size > HEAP_END - address)
        return NULL;

    heap_current = address + size;

    return (void *)address;
}

static void
kfree(void *ptr)
{
    /*
     * Native allocator is currently bump-only.
     * A real reclaiming allocator replaces this.
     */
    (void)ptr;
}

/* =========================================================
 * Memory test
 * ========================================================= */

static int
memory_test(void)
{
    uint32_t *value =
        (uint32_t *)kmalloc(sizeof(uint32_t));

    if (value == NULL)
        return -1;

    *value = 0xDEADBEEF;

    if (*value != 0xDEADBEEF)
        return -1;

    return 0;
}

/* =========================================================
 * Kernel panic
 * ========================================================= */

static void
kernel_panic(const char *reason)
{
    cpu_cli();

    terminal_color = 0x4F;
    terminal_clear();

    terminal_write("xnu++ KERNEL PANIC\n");
    terminal_write("==================\n\n");
    terminal_write(reason);
    terminal_write("\n");

    for (;;)
        cpu_hlt();
}

/* =========================================================
 * Architecture initialization
 * ========================================================= */

static void
arch_init(void)
{
    gdt_init();
    idt_init();
    pic_remap();
}

/* =========================================================
 * Kernel information
 * ========================================================= */

static void
kernel_banner(void)
{
    terminal_write(
        "xnu++ native kernel\n"
        "===================\n"
    );

    terminal_write("version: ");
    terminal_write(KERNEL_VERSION);
    terminal_write("\n");

    terminal_write("architecture: i386\n");
    terminal_write("provider: native\n");
    terminal_write("xnu dependency: none\n");
    terminal_write("\n");
}

/* =========================================================
 * Kernel initialization
 * ========================================================= */

static int
kernel_init(void)
{
    arch_init();

    if (memory_test() != 0)
        return -1;

    return 0;
}

/* =========================================================
 * Kernel idle
 * ========================================================= */

static void
kernel_idle(void)
{
    for (;;) {
        keyboard_poll();
        cpu_hlt();
    }
}

/* =========================================================
 * Native kernel entry
 * ========================================================= */

void
kernel_main(void)
{
    cpu_cli();

    terminal_color = 0x1F;
    terminal_clear();

    kernel_banner();

    terminal_write("initializing architecture...\n");

    if (kernel_init() != 0)
        kernel_panic("kernel initialization failed");

    terminal_write("gdt: OK\n");
    terminal_write("idt: OK\n");
    terminal_write("pic: OK\n");
    terminal_write("memory: OK\n");

    terminal_write("\n");
    terminal_write("xnu++ native layer initialized\n");
    terminal_write("kernel is independent from XNU\n");
    terminal_write("\n");
    terminal_write("keyboard input enabled\n");

    cpu_sti();

    kernel_idle();
}
void 
kernel_loop(void){
    
    for (;;){
        keyboard_poll();
        cpu_hlt();
    }
    for (;;){
        keyboard_poll();
        cpu_hlt();
    }   




}
void kernel_start(void){
    cpu_cli();

    terminal_color = 0x1F;
    terminal_clear();

    kernel_banner();

    terminal_write("initializing architecture...\n");

    if (kernel_init() != 0)
        kernel_panic("kernel initialization failed");

    terminal_write("gdt: OK\n");
    terminal_write("idt: OK\n");
    terminal_write("pic: OK\n");
    terminal_write("memory: OK\n");

    terminal_write("\n");
    terminal_write("xnu++ native layer initialized\n");
    terminal_write("kernel is independent from XNU\n");
    terminal_write("\n");
    terminal_write("keyboard input enabled\n");

    cpu_sti();

}
void kernal_stop(void){
    kernel_panic("kernel stopped");
    kernal_start(void);
    termnal_







}
