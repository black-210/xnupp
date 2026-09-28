#include <xnu++/crypto.h>

int
xnuxx_ed25519_verify(
    const uint8_t public_key[XNUXX_ED25519_PUBLIC_KEY_SIZE],
    const void *message,
    size_t message_size,
    const uint8_t signature[XNUXX_ED25519_SIGNATURE_SIZE])
{
    (void)public_key;
    (void)message;
    (void)message_size;
    (void)signature;

    return -1;
}
