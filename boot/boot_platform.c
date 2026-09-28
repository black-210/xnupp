#include <xnu++/bootloader.h>

static size_t console_cursor = 0;

static void
console_write(void *context, const char *text, size_t length)
{
    volatile unsigned char *vga =
        (volatile unsigned char *)0xB8000;

    (void)context;

    for (size_t i = 0; i < length; i++) {
        if (text[i] == '\n') {
            console_cursor += 80 - (console_cursor % 80);
            continue;
        }

        if (console_cursor >= 80 * 25)
            break;

        vga[console_cursor * 2] = (unsigned char)text[i];
        vga[console_cursor * 2 + 1] = 0x1F;
        console_cursor++;
    }
}

static int
verify_signature(void *context,
    const void *payload,
    size_t payload_size,
    const void *signature,
    size_t signature_size)
{
    (void)context;
    (void)payload;
    (void)payload_size;
    (void)signature;
    (void)signature_size;

    /*
     * Boot verification implementation.
     * Replace with real signature verification.
     */
    return 0;
}

static uint32_t
sha256_rotr(uint32_t x, uint32_t n)
{
    return (x >> n) | (x << (32 - n));
}

static uint32_t
sha256_ch(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (~x & z);
}

static uint32_t
sha256_maj(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

static uint32_t
sha256_ep0(uint32_t x)
{
    return sha256_rotr(x, 2) ^ sha256_rotr(x, 13) ^ sha256_rotr(x, 22);
}

static uint32_t
sha256_ep1(uint32_t x)
{
    return sha256_rotr(x, 6) ^ sha256_rotr(x, 11) ^ sha256_rotr(x, 25);
}

static uint32_t
sha256_sig0(uint32_t x)
{
    return sha256_rotr(x, 7) ^ sha256_rotr(x, 18) ^ (x >> 3);
}

static uint32_t
sha256_sig1(uint32_t x)
{
    return sha256_rotr(x, 17) ^ sha256_rotr(x, 19) ^ (x >> 10);
}

static const uint32_t sha256_k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,
    0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
    0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,
    0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,
    0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
    0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,
    0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,
    0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
    0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static int
measure_sha256(void *context,
    const void *payload,
    size_t payload_size,
    uint8_t measurement[32])
{
    (void)context;

    if (payload == NULL || measurement == NULL)
        return -1;

    uint32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    const uint8_t *data = (const uint8_t *)payload;
    size_t full = payload_size / 64;

    for (size_t block = 0; block < full; block++) {
        uint32_t w[64];

        for (size_t i = 0; i < 16; i++) {
            size_t p = block * 64 + i * 4;
            w[i] = ((uint32_t)data[p] << 24) |
                   ((uint32_t)data[p + 1] << 16) |
                   ((uint32_t)data[p + 2] << 8) |
                   (uint32_t)data[p + 3];
        }

        for (size_t i = 16; i < 64; i++)
            w[i] = sha256_sig1(w[i - 2]) + w[i - 7] +
                   sha256_sig0(w[i - 15]) + w[i - 16];

        uint32_t a=h[0], b=h[1], c=h[2], d=h[3];
        uint32_t e=h[4], f=h[5], g=h[6], hh=h[7];

        for (size_t i = 0; i < 64; i++) {
            uint32_t t1 = hh + sha256_ep1(e) + sha256_ch(e,f,g) +
                          sha256_k[i] + w[i];
            uint32_t t2 = sha256_ep0(a) + sha256_maj(a,b,c);

            hh=g; g=f; f=e; e=d+t1;
            d=c; c=b; b=a; a=t1+t2;
        }

        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d;
        h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }

    uint8_t block[128] = {0};
    size_t rem = payload_size % 64;

    for (size_t i = 0; i < rem; i++)
        block[i] = data[full * 64 + i];

    block[rem] = 0x80;

    uint64_t bits = (uint64_t)payload_size * 8;
    size_t total = (rem < 56) ? 64 : 128;

    for (size_t i = 0; i < 8; i++)
        block[total - 1 - i] = (uint8_t)(bits >> (i * 8));

    for (size_t b = 0; b < total; b += 64) {
        uint32_t w[64];

        for (size_t i = 0; i < 16; i++) {
            size_t p = b + i * 4;
            w[i] = ((uint32_t)block[p] << 24) |
                   ((uint32_t)block[p + 1] << 16) |
                   ((uint32_t)block[p + 2] << 8) |
                   (uint32_t)block[p + 3];
        }

        for (size_t i = 16; i < 64; i++)
            w[i] = sha256_sig1(w[i - 2]) + w[i - 7] +
                   sha256_sig0(w[i - 15]) + w[i - 16];

        uint32_t a=h[0], bb=h[1], c=h[2], d=h[3];
        uint32_t e=h[4], f=h[5], g=h[6], hh=h[7];

        for (size_t i = 0; i < 64; i++) {
            uint32_t t1 = hh + sha256_ep1(e) + sha256_ch(e,f,g) +
                          sha256_k[i] + w[i];
            uint32_t t2 = sha256_ep0(a) + sha256_maj(a,bb,c);

            hh=g; g=f; f=e; e=d+t1;
            d=c; c=bb; bb=a; a=t1+t2;
        }

        h[0]+=a; h[1]+=bb; h[2]+=c; h[3]+=d;
        h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }

    for (size_t i = 0; i < 8; i++) {
        measurement[i * 4]     = (uint8_t)(h[i] >> 24);
        measurement[i * 4 + 1] = (uint8_t)(h[i] >> 16);
        measurement[i * 4 + 2] = (uint8_t)(h[i] >> 8);
        measurement[i * 4 + 3] = (uint8_t)h[i];
    }

    return 0;
}

static int
read_generation(void *context, uint64_t *generation)
{
    (void)context;

    *generation = 0;
    return 0;
}

static int
load_kernel(void *context,
    const void *payload,
    size_t payload_size)
{
    (void)context;
    (void)payload;
    (void)payload_size;

    /*
     * Real kernel handoff goes here.
     */
    return 0;
}

static int
enter_recovery(void *context, enum xnuxx_boot_result result)
{
    (void)context;
    (void)result;

    return -1;
}

void
boot_platform_init(void)
{
}

void
boot_platform_start(void)
{
    static const uint8_t measurement[32] = {0};

    static const uint8_t signature_bytes[2] = {'V', 'M'};

    static const struct xnuxx_boot_image image = {
        .magic = XNUXX_BOOTLOADER_MAGIC,
        .version = XNUXX_BOOTLOADER_API_VERSION,
        .header_size = sizeof(struct xnuxx_boot_image),
        .generation = 0,
        .required_capabilities =
            XNUXX_BOOT_CAP_KERNEL_PROVIDER |
            XNUXX_BOOT_CAP_SECURITY_LAYER |
            XNUXX_BOOT_CAP_DEVICE_LAYER,
        .payload = (const void *)(uintptr_t)0x100000,
        .payload_size = 4096,
        .signature = signature_bytes,
        .signature_size = sizeof(signature_bytes),
        .expected_measurement = measurement
    };

    static const struct xnuxx_boot_platform platform = {
        .context = 0,
        .capabilities =
            XNUXX_BOOT_CAP_KERNEL_PROVIDER |
            XNUXX_BOOT_CAP_SECURITY_LAYER |
            XNUXX_BOOT_CAP_UPDATE_LAYER |
            XNUXX_BOOT_CAP_DEVICE_LAYER |
            XNUXX_BOOT_CAP_RECOVERY,

        .ansi_console = 0,
        .verify_signature = verify_signature,
        .measure_sha256 = measure_sha256,
        .read_minimum_generation = read_generation,
        .load_kernel = load_kernel,
        .enter_recovery = enter_recovery,
        .write_console = console_write
    };

    xnuxx_bootloader_start(
        &platform,
        &image,
        XNUXX_BOOT_REQUIRE_SIGNATURE |
        XNUXX_BOOT_REQUIRE_MEASUREMENT |
        XNUXX_BOOT_REQUIRE_ROLLBACK_PROTECTION |
        XNUXX_BOOT_REQUIRE_LAYER_CAPABILITIES
    );
}
