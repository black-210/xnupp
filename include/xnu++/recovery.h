#ifndef XNUXX_RECOVERY_H
#define XNUXX_RECOVERY_H

#include <stdint.h>

#define XNUXX_RECOVERY_API_VERSION 1u

enum xnuxx_recovery_reason {
    XNUXX_RECOVERY_BOOT_FAILURE = 1,
    XNUXX_RECOVERY_PROVIDER_FAILURE = 2,
    XNUXX_RECOVERY_UPDATE_FAILURE = 3,
    XNUXX_RECOVERY_SECURITY_FAILURE = 4
};

enum xnuxx_recovery_mode {
    XNUXX_RECOVERY_NORMAL = 0,
    XNUXX_RECOVERY_SAFE = 1,
    XNUXX_RECOVERY_OFFLINE = 2
};

struct xnuxx_recovery_record {
    uint32_t api_version;
    enum xnuxx_recovery_reason reason;
    enum xnuxx_recovery_mode mode;
    uint64_t generation;
    uint64_t timestamp;
    uint32_t attempts;
};

int xnuxx_recovery_enter(enum xnuxx_recovery_reason, uint64_t generation);
int xnuxx_recovery_record(const struct xnuxx_recovery_record *);
int xnuxx_recovery_clear(uint64_t generation);
int xnuxx_recovery_get(struct xnuxx_recovery_record *record, uint64_t *generation);
int xnuxx_recovery_get_latest(struct xnuxx_recovery_record *record);
int xnuxx_recovery_set_mode(enum xnuxx_recovery_mode mode);
int xnuxx_recovery_get_mode(enum xnuxx_recovery_mode *mode);
#endif
