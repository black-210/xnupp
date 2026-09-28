#!/data/data/com.termux/files/usr/bin/bash
set -u

PASS=0
FAIL=0
WARN=0

pass(){ printf '[PASS] %s\n' "$1"; PASS=$((PASS+1)); }
fail(){ printf '[FAIL] %s\n' "$1"; FAIL=$((FAIL+1)); }
warn(){ printf '[WARN] %s\n' "$1"; WARN=$((WARN+1)); }

echo '============================================================'
echo '                    XNU++ FULL AUDIT'
echo '============================================================'

echo
echo '=== 1. REQUIRED DIRECTORIES ==='
for d in include/xnu++ boot kernel security userspace tools/xnu++ osfmk bsd iokit libkern libsyscall; do
    [ -d "$d" ] && pass "$d" || fail "$d missing"
done

echo
echo '=== 2. REQUIRED XNU++ HEADERS ==='
for f in \
include/xnu++/bootloader.h \
include/xnu++/bus.h \
include/xnu++/compat.h \
include/xnu++/device.h \
include/xnu++/diagnostics.h \
include/xnu++/features.h \
include/xnu++/isolation.h \
include/xnu++/quota.h \
include/xnu++/recovery.h \
include/xnu++/reliability.h \
include/xnu++/security.h \
include/xnu++/update.h \
include/xnu++/crypto.h; do
    [ -f "$f" ] && pass "$f" || fail "$f missing"
done

echo
echo '=== 3. BOOT SOURCES ==='
for f in \
boot/boot_entry.c \
boot/boot_platform.c \
boot/string.c \
boot/long_mode.S \
boot/multiboot2.S \
boot/multiboot2_64.S \
boot/linker.ld \
boot/linker64.ld; do
    [ -f "$f" ] && pass "$f" || fail "$f missing"
done

echo
echo '=== 4. KERNEL SOURCES ==='
for f in kernel/kernel.c kernel/kernel64.c; do
    [ -f "$f" ] && pass "$f" || fail "$f missing"
done

echo
echo '=== 5. SECURITY SOURCES ==='
for f in security/bootloader.c security/crypto.c; do
    [ -f "$f" ] && pass "$f" || fail "$f missing"
done

echo
echo '=== 6. USERSPACE ==='
[ -f userspace/orange_loader.c ] && pass 'Orange userspace loader' || fail 'Orange userspace loader missing'

echo
echo '=== 7. BUILD SYSTEM ==='
[ -f xnuxx.mk ] && pass 'xnuxx.mk' || fail 'xnuxx.mk missing'
[ -f Makefile ] && pass 'Makefile' || warn 'top-level Makefile missing'

echo
echo '=== 8. REQUIRED XNU SUBSYSTEMS ==='
for d in osfmk/kernel osfmk/ipc osfmk/vm osfmk/kern osfmk/sched osfmk/x86_64 bsd/vfs bsd/net iokit libkern libsyscall; do
    [ -d "$d" ] && pass "$d" || fail "$d missing"
done

echo
echo '=== 9. SOURCE INVENTORY ==='
TOTAL=$(find . \
    -path './.git' -prune -o \
    -path './build' -prune -o \
    -path './build64' -prune -o \
    -type f -print | wc -l)
CFILES=$(find . -path './.git' -prune -o -path './build' -prune -o -path './build64' -prune -o -name '*.c' -type f -print | wc -l)
HFILES=$(find . -path './.git' -prune -o -path './build' -prune -o -path './build64' -prune -o -name '*.h' -type f -print | wc -l)
ASMFILES=$(find . -path './.git' -prune -o -path './build' -prune -o -path './build64' -prune -o \( -name '*.S' -o -name '*.s' \) -type f -print | wc -l)
echo "Total source/project files: $TOTAL"
echo "C files: $CFILES"
echo "Headers: $HFILES"
echo "Assembly: $ASMFILES"

echo
echo '=== 10. XNU++ UNFINISHED MARKERS ==='
MARKERS=$(grep -RInE \
'TODO|FIXME|placeholder|not implemented|bring-up|temporary|replace with|XXX' \
include/xnu++ boot kernel userspace tools/xnu++ \
--include='*.c' --include='*.h' --include='*.S' --include='*.s' --include='*.sh' \
2>/dev/null || true)

if [ -n "$MARKERS" ]; then
    printf '%s\n' "$MARKERS"
    fail 'XNU++ unfinished markers found'
else
    pass 'No XNU++ unfinished markers'
fi

echo
echo '=== 11. XNU++ RETURN-FAIL PATHS ==='
RETURNS=$(grep -RInE 'return[[:space:]]+-1[[:space:]]*;' \
include/xnu++ boot kernel userspace tools/xnu++ \
--include='*.c' --include='*.h' 2>/dev/null || true)

if [ -n "$RETURNS" ]; then
    printf '%s\n' "$RETURNS"
    warn 'Explicit failure paths exist; they require semantic review'
else
    pass 'No explicit -1 paths'
fi

echo
echo '=== 12. SECURITY API ==='
grep -q 'xnuxx_ed25519_verify' include/xnu++/crypto.h \
    && pass 'Ed25519 API declared' || fail 'Ed25519 API missing'

grep -q 'xnuxx_ed25519_verify' security/crypto.c \
    && pass 'Ed25519 implementation linked in source tree' || fail 'Ed25519 implementation missing'

echo
echo '=== 13. SHA-256 ==='
grep -q 'sha256_k' boot/boot_platform.c \
    && pass 'SHA-256 implementation present' || fail 'SHA-256 implementation missing'

grep -q 'measure_sha256' boot/boot_platform.c \
    && pass 'SHA-256 measurement path present' || fail 'SHA-256 measurement path missing'

echo
echo '=== 14. BOOT VERIFICATION ==='
grep -q 'verify_signature' boot/boot_platform.c \
    && pass 'Signature verification callback present' || fail 'Signature verification missing'

grep -q 'expected_measurement' boot/boot_platform.c \
    && pass 'Expected measurement field used' || fail 'Expected measurement missing'

echo
echo '=== 15. RECOVERY / UPDATE / ROLLBACK ==='
for term in read_minimum_generation load_kernel enter_recovery; do
    grep -q "$term" boot/boot_platform.c \
        && pass "$term present" || fail "$term missing"
done

echo
echo '=== 16. PROVIDER / DEVICE / BUS ==='
for term in \
xnuxx_device \
xnuxx_bus \
xnuxx_security \
xnuxx_recovery \
xnuxx_update \
xnuxx_isolation \
xnuxx_quota; do
    if grep -Rqs "$term" include/xnu++ kernel boot security userspace; then
        pass "$term referenced"
    else
        warn "$term not found by symbol scan"
    fi
done

echo
echo '=== 17. 32-BIT BUILD ==='
if make -f xnuxx.mk clean >/dev/null 2>&1 && make -f xnuxx.mk >/dev/null 2>&1; then
    pass 'i386 build'
else
    fail 'i386 build'
fi

if [ -f build/xnuxx.elf ]; then
    file build/xnuxx.elf
    readelf -h build/xnuxx.elf >/dev/null 2>&1 \
        && pass 'i386 ELF readable' \
        || fail 'i386 ELF invalid'
else
    fail 'i386 ELF missing'
fi

echo
echo '=== 18. 64-BIT SOURCES / ARTIFACT ==='
if [ -f build64/xnuxx.elf ]; then
    file build64/xnuxx.elf
    file build64/xnuxx.elf | grep -q 'ELF 64-bit' \
        && pass 'x86-64 ELF' \
        || fail 'build64 artifact is not ELF64'
else
    warn 'build64/xnuxx.elf not present; source-level 64-bit checks continue'
fi

for f in boot/multiboot2_64.S kernel/kernel64.c boot/linker64.ld; do
    [ -f "$f" ] && pass "$f" || fail "$f missing"
done

echo
echo '=== 19. LINK SYMBOLS ==='
if [ -f build/xnuxx.elf ]; then
    nm -n build/xnuxx.elf >/dev/null 2>&1 \
        && pass 'ELF symbols readable' \
        || warn 'ELF symbols unavailable'
fi

echo
echo '=== 20. BOOT ENTRY ==='
grep -q '_start' boot/multiboot2.S \
    && pass 'Multiboot2 entry' || fail 'Multiboot2 entry missing'

grep -q '_start' boot/multiboot2_64.S \
    && pass '64-bit entry' || fail '64-bit entry missing'

echo
echo '=== 21. GIT STATE ==='
if git diff --check >/dev/null 2>&1; then
    pass 'Git whitespace check'
else
    fail 'Git whitespace errors'
fi

git status --short

echo
echo '=== 22. DOCUMENTATION ==='
for f in README.md XNU++_README.md XNU++_VERSION; do
    [ -f "$f" ] && pass "$f" || warn "$f missing"
done

echo
echo '============================================================'
echo "PASS : $PASS"
echo "WARN : $WARN"
echo "FAIL : $FAIL"
echo '============================================================'

if [ "$FAIL" -eq 0 ] && [ "$WARN" -eq 0 ]; then
    echo 'FINAL RESULT: XNU++ PASSED FULL AUDIT'
    exit 0
elif [ "$FAIL" -eq 0 ]; then
    echo 'FINAL RESULT: XNU++ PASSED REQUIRED CHECKS WITH WARNINGS'
    exit 2
else
    echo 'FINAL RESULT: XNU++ IS NOT COMPLETE'
    exit 1
fi
