#ifndef XNUXX_CRYPTO_H
#define XNUXX_CRYPTO_H

#include <stddef.h>
#include <stdint.h>

#define XNUXX_ED25519_PUBLIC_KEY_SIZE 32
#define XNUXX_ED25519_SIGNATURE_SIZE 64

int xnuxx_ed25519_verify(
    const uint8_t public_key[XNUXX_ED25519_PUBLIC_KEY_SIZE],
    const void *message,
    size_t message_size,
    const uint8_t signature[XNUXX_ED25519_SIGNATURE_SIZE]);

#endif
