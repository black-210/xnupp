# XNU++ — Complete Project Guide

> A from-zero-to-architecture reference for the XNU++ project.

**Repository:** `https://github.com/black-210/xnupp`  
**Foundation:** XNU  
**Primary language:** C  
**Primary architectures:** i386 and x86-64  
**Boot protocol:** Multiboot2

---

## Table of Contents

1. [1. What XNU++ Is](#1.-what-xnu++-is)
2. [2. Repository Structure](#2.-repository-structure)
3. [3. XNU Foundation](#3.-xnu-foundation)
4. [4. Public XNU++ API](#4.-public-xnu++-api)
5. [5. boot/](#5.-boot)
6. [6. boot_entry.c](#6.-boot_entry.c)
7. [7. boot_platform.c](#7.-boot_platform.c)
8. [8. Cryptography](#8.-cryptography)
9. [9. security/](#9.-security)
10. [10. Kernel Layer](#10.-kernel-layer)
11. [11. Device and Bus Architecture](#11.-device-and-bus-architecture)
12. [12. Security, Recovery and Update](#12.-security,-recovery-and-update)
13. [13. Isolation, Quotas, Reliability and Diagnostics](#13.-isolation,-quotas,-reliability-and-diagnostics)
14. [14. Compatibility and Features](#14.-compatibility-and-features)
15. [15. tools/xnu++/](#15.-toolsxnu++)
16. [16. Userspace and Orange](#16.-userspace-and-orange)
17. [17. Boot Assembly](#17.-boot-assembly)
18. [18. Freestanding Support](#18.-freestanding-support)
19. [19. Build System](#19.-build-system)
20. [20. Why --target Is Needed](#20.-why---target-is-needed)
21. [21. x86-64](#21.-x86-64)
22. [22. Build Output](#22.-build-output)
23. [23. ELF Inspection](#23.-elf-inspection)
24. [24. Documentation](#24.-documentation)
25. [25. Other Major Directories](#25.-other-major-directories)
26. [26. Full Architecture](#26.-full-architecture)
27. [27. Complete Boot Flow](#27.-complete-boot-flow)
28. [28. Development and Testing](#28.-development-and-testing)
29. [29. Repository Workflow](#29.-repository-workflow)
30. [30. What 'Complete' Means](#30.-what-'complete'-means)
31. [31. File-by-File XNU++ Map](#31.-file-by-file-xnu++-map)
32. [32. The One-Sentence Definition](#32.-the-one-sentence-definition)
33. [33. Final Mental Model](#33.-final-mental-model)

---

## 1. 1. What XNU++ Is

XNU++ is an experimental operating-system platform and engineering layer built around XNU.

The key architectural rule is: XNU++ does NOT replace XNU. XNU remains the low-level foundation. XNU++ adds platform-level interfaces and policy for boot, security, cryptography, recovery, updates, rollback, devices, buses, isolation, quotas, diagnostics, reliability, compatibility, feature discovery, and userspace/distribution integration.

Conceptually:

```text
Applications
    ↓
Orange userspace / distribution
    ↓
XNU++ platform layer
    ↓
XNU (Mach + BSD + VM + IPC + IOKit + libkern + syscalls)
    ↓
Hardware
```

XNU++ therefore does not need to reimplement XNU's scheduler, VM, IPC, VFS, task/thread system, or IOKit.

---

## 2. 2. Repository Structure

The repository contains the XNU foundation plus XNU++ integration:

```text
xnupp/
├── EXTERNAL_HEADERS/
├── SETUP/
├── boot/
├── bsd/
├── build/
├── build64/
├── config/
├── distribution/
├── doc/
├── include/xnu++/
├── iokit/
├── iso/
├── kernel/
├── libkdd/
├── libkern/
├── libsa/
├── libsyscall/
├── makedefs/
├── osfmk/
├── pexpert/
├── san/
├── security/
├── tests/
├── tools/xnu++/
├── userspace/
├── Makefile
├── xnuxx.mk
├── README.md
├── XNU++_README.md
└── XNU++_VERSION
```

There are thousands of files because a substantial XNU source tree is present. Not every file in the repository was written specifically for XNU++.

---

## 3. 3. XNU Foundation

The major XNU areas are:

- `osfmk/` — Mach and core kernel internals.
- `osfmk/kern/` — core kernel mechanisms.
- `osfmk/ipc/` — Mach IPC.
- `osfmk/vm/` — virtual memory.
- `osfmk/x86_64/` — x86-64 architecture support.
- `bsd/` — BSD kernel services.
- `bsd/vfs/` — VFS/filesystem infrastructure.
- `bsd/net/` — networking.
- `iokit/` — device/driver framework.
- `libkern/` — kernel libraries.
- `libsa/` — standalone/support environment.
- `libsyscall/` — syscall/user boundary support.
- `pexpert/` — platform-specific XNU support.
- `libkdd/` — debugging/KDD support.
- `makedefs/` — XNU build definitions.
- `san/` — sanitizer-related material.
- `tests/` — testing material.

XNU++ uses these systems rather than implementing duplicate versions of them.

---

## 4. 4. Public XNU++ API

The public XNU++ interfaces are under `include/xnu++/`:

```text
bootloader.h   boot interface
bus.h          bus abstraction
compat.h       compatibility
crypto.h       cryptographic API
device.h       device abstraction
diagnostics.h  diagnostics
features.h     capabilities/features
isolation.h    isolation policy
quota.h        resource quotas
recovery.h     recovery policy
reliability.h  reliability
security.h     security policy
update.h       update policy
```

These headers are the vocabulary of the XNU++ platform. Higher-level code can depend on these interfaces without hard-coding one provider implementation.

---

## 5. 5. boot/

The XNU++ boot layer is:

```text
boot/
├── boot_entry.c
├── boot_platform.c
├── string.c
├── long_mode.S
├── multiboot2.S
├── multiboot2_64.S
├── linker.ld
└── linker64.ld
```

Its purpose is to bridge the boot protocol and early machine state into the C/platform layer.

---

## 6. 6. boot_entry.c

`boot/boot_entry.c` is the C-level early boot entry. Assembly performs architecture-specific setup and then transfers execution into the C initialization path.

Conceptually:

```text
Multiboot2
    ↓
assembly entry
    ↓
boot_entry.c
    ↓
platform initialization
```

Keeping the C entry separate from the assembly transition makes the architecture-specific boundary explicit.

---

## 7. 7. boot_platform.c

`boot/boot_platform.c` is a central XNU++ platform file. It contains early platform/security integration such as measurement, verification interfaces, kernel loading, generation/rollback handling, and recovery decisions.

The conceptual security path is:

```text
payload
  ↓
measurement
  ↓
SHA-256 digest
  ↓
signature verification
  ↓
generation/rollback policy
  ↓
continue OR recovery
```

SHA-256 measurement and digital-signature verification are different operations. A hash alone is not a signature.

---

## 8. 8. Cryptography

XNU++ exposes Ed25519 verification through `include/xnu++/crypto.h`.

The API uses:

```text
32-byte public key
64-byte Ed25519 signature
message + message size
```

The intended operation is:

```text
trusted public key + message + signature
                 ↓
        Ed25519 verification
                 ↓
             valid/invalid
```

`security/crypto.c` is the implementation side and is compiled as `build/crypto.o` in the i386 build.

Security-critical documentation must distinguish between an API existing and a cryptographic algorithm actually being implemented and validated. A fail-closed stub is not a real cryptographic implementation.

---

## 9. 9. security/

Important XNU++ security files include:

```text
security/bootloader.c
security/crypto.c
```

`security/bootloader.c` connects bootloader-related operations to platform security policy.

`security/crypto.c` provides the implementation behind the public crypto API.

The same directory also contains XNU security infrastructure, so not every security source file is an XNU++-owned file.

---

## 10. 10. Kernel Layer

The XNU++ kernel-side directory is:

```text
kernel/
├── kernel.c
├── kernel64.c
└── README.md
```

`kernel/kernel.c` is the i386 platform-side implementation.

`kernel/kernel64.c` is the x86-64 counterpart.

These are XNU++ platform implementations; they do not represent a replacement for all of XNU's kernel internals.

---

## 11. 11. Device and Bus Architecture

`include/xnu++/device.h` defines a device abstraction.

`include/xnu++/bus.h` defines a bus abstraction.

The intended provider model is:

```text
XNU++ consumer
      ↓
XNU++ device/bus API
      ↓
provider
      ↓
XNU / IOKit / native implementation
      ↓
hardware
```

This prevents higher-level platform code from being tightly coupled to one device implementation.

---

## 12. 12. Security, Recovery and Update

The relevant interfaces are:

```text
security.h
recovery.h
update.h
crypto.h
```

A secure update conceptually follows:

```text
candidate image
    ↓
measure
    ↓
verify signature
    ↓
check compatibility
    ↓
check generation
    ↓
install/activate
    ↓
boot validation
    ↓
success
```

A failed validation can enter recovery or rollback.

Rollback protection conceptually compares a candidate generation with a trusted minimum generation:

```text
candidate < minimum → reject
candidate >= minimum → continue
```

A production rollback counter needs secure persistent storage; the policy interface alone does not create persistent storage.

---

## 13. 13. Isolation, Quotas, Reliability and Diagnostics

The interfaces are:

```text
isolation.h
quota.h
reliability.h
diagnostics.h
```

Isolation expresses separation policy.

Quotas express resource limits.

Reliability and diagnostics provide ways to describe and inspect platform state.

The project also contains:

```text
tools/xnu++/reliability-matrix.json
tools/xnu++/validate-reliability.sh
tools/xnu++/support-matrix.json
```

These make support/reliability information machine-readable.

---

## 14. 14. Compatibility and Features

`compat.h` contains compatibility-related interfaces.

`features.h` represents platform capabilities.

This allows the project to distinguish states such as:

```text
supported
unsupported
experimental
architecture-specific
provider-dependent
```

That is preferable to pretending every platform has identical capabilities.

---

## 15. 15. tools/xnu++/

The XNU++ tooling directory contains:

```text
arch-probe.sh
check-x86.sh
doctor.sh
probe.sh
reliability-matrix.json
support-matrix.json
validate-profile.sh
validate-reliability.sh
```

Their roles are:

- `arch-probe.sh` — inspect architecture/platform information.
- `check-x86.sh` — x86-specific checks.
- `doctor.sh` — environment/project diagnostics.
- `probe.sh` — general capability/environment probing.
- `reliability-matrix.json` — reliability data.
- `support-matrix.json` — support data.
- `validate-profile.sh` — profile validation.
- `validate-reliability.sh` — reliability validation.

---

## 16. 16. Userspace and Orange

`userspace/orange_loader.c` is the bridge toward the Orange userspace/distribution layer.

The intended relationship is:

```text
XNU
 ↓
XNU++
 ↓
Orange loader
 ↓
Orange init
 ↓
services
 ↓
applications
```

Orange is userspace/distribution functionality, not a replacement for XNU.

---

## 17. 17. Boot Assembly

`boot/multiboot2.S` is the i386 Multiboot2 entry.

`boot/multiboot2_64.S` is the x86-64 Multiboot2 entry. It performs early machine setup such as temporary stack setup, paging preparation, long-mode activation, GDT setup, and transfer to C.

`boot/long_mode.S` contains long-mode support.

The conceptual x86-64 transition is:

```text
32-bit entry
  ↓
paging preparation
  ↓
long mode
  ↓
64-bit C entry
```

`boot/linker.ld` and `boot/linker64.ld` control the final ELF layouts.

---

## 18. 18. Freestanding Support

`boot/string.c` supplies minimal memory/string functionality needed by early freestanding code.

Kernel/boot code cannot assume that a normal hosted libc is present. This is why small primitives such as `memcpy` may exist locally.

---

## 19. 19. Build System

`xnuxx.mk` is the focused XNU++ build file.

The i386 build uses:

```text
clang
--target=i386-unknown-elf
-m32
-ffreestanding
-fno-pic
-fno-stack-protector
-fno-builtin
-fno-strict-aliasing
```

The linker uses:

```text
ld -m elf_i386
```

The current i386 build sequence is:

```text
multiboot2.S
boot_entry.c
boot_platform.c
string.c
kernel.c
security/bootloader.c
security/crypto.c
        ↓
object files
        ↓
build/xnuxx.elf
```

`Makefile` is the top-level make interface.

---

## 20. 20. Why --target Is Needed

The development environment is Termux on ARM64. The host compiler target is therefore AArch64 Android, while the XNU++ i386 image must target i386 ELF.

The distinction is:

```text
host:   aarch64-unknown-linux-android
target: i386-unknown-elf
```

Therefore the build explicitly uses:

```bash
clang --target=i386-unknown-elf
```

This is why the build can produce an i386 ELF from an ARM64 development device.

---

## 21. 21. x86-64

The repository also contains a working x86-64 path using:

```text
boot/multiboot2_64.S
boot/linker64.ld
kernel/kernel64.c
```

The x86-64 ELF has been verified as:

```text
ELF64
x86-64
statically linked
```

The x86-64 boot path is separate from the i386 path where architecture-specific assembly and linker details require it.

---

## 22. 22. Build Output

Generated output normally lives in:

```text
build/
build64/
```

Examples:

```text
build/xnuxx.elf
build/*.o
build64/xnuxx.elf
```

Generated objects such as `build/crypto.o` are build artifacts, not source files. Whether ELF artifacts are committed is a repository-policy decision.

---

## 23. 23. ELF Inspection

Useful inspection commands are:

```bash
file build/xnuxx.elf
readelf -h build/xnuxx.elf
readelf -S build/xnuxx.elf
readelf -s build/xnuxx.elf
```

These answer different questions:

- `file` — basic artifact type.
- `readelf -h` — ELF header and architecture.
- `readelf -S` — sections.
- `readelf -s` — symbols.

---

## 24. 24. Documentation

Important documentation files are:

```text
README.md
XNU++_README.md
XNU++_VERSION
kernel/README.md
doc/
```

`README.md` is the public project overview.

`XNU++_README.md` focuses on XNU++ architecture and operation.

`XNU++_VERSION` identifies the project version.

`doc/` provides additional documentation.

---

## 25. 25. Other Major Directories

The remaining major directories have these roles:

```text
EXTERNAL_HEADERS/ → external header material
SETUP/            → setup-related project material
config/           → configuration material
distribution/     → distribution integration
iso/              → ISO/image integration
libkdd/            → XNU debugging/KDD support
libkern/           → XNU kernel libraries
libsa/             → XNU standalone support
libsyscall/        → syscall boundary
makedefs/          → XNU build definitions
pexpert/           → platform-specific XNU support
san/               → sanitizer-related code
tests/             → tests
```

These areas are part of the broader project/source foundation and should not all be described as newly written XNU++ components.

---

## 26. 26. Full Architecture

The complete conceptual architecture is:

```text
┌─────────────────────────────────────────────────────────────┐
│                        Applications                         │
├─────────────────────────────────────────────────────────────┤
│                 Orange Userspace / Distribution             │
│              init / services / packages / UI                │
├─────────────────────────────────────────────────────────────┤
│                           XNU++                             │
│                                                             │
│ Boot | Security | Crypto | Recovery | Update | Rollback    │
│ Device | Bus | Isolation | Quota | Diagnostics | Reliability│
│ Compatibility | Features | Provider Interfaces              │
├─────────────────────────────────────────────────────────────┤
│                            XNU                              │
│                                                             │
│ Mach | IPC | VM | Tasks | Threads | Scheduler               │
│ BSD | VFS | Networking | IOKit | libkern | Syscalls         │
├─────────────────────────────────────────────────────────────┤
│                         Hardware                            │
└─────────────────────────────────────────────────────────────┘
```

---

## 27. 27. Complete Boot Flow

A simplified boot flow is:

```text
Bootloader
    ↓
Multiboot2
    ↓
multiboot2.S / multiboot2_64.S
    ↓
architecture initialization
    ↓
boot_entry.c
    ↓
boot_platform.c
    ↓
measurement
    ↓
signature verification
    ↓
generation / rollback checks
    ↓
XNU initialization
    ↓
XNU++ platform initialization
    ↓
Orange/userspace integration
```

A failure in validation can route into recovery rather than blindly continuing.

---

## 28. 28. Development and Testing

A successful compilation proves that the source can be compiled and linked into the expected artifact. It does not by itself prove runtime correctness, cryptographic correctness, hardware correctness, or recovery correctness.

A serious validation pipeline should separate:

```text
1. source/syntax validation
2. compilation
3. linking
4. ELF inspection
5. boot testing
6. subsystem testing
7. security testing
8. recovery testing
9. architecture testing
10. documentation/status validation
```

The project's audit script is useful as a project check, but an audit script itself must also be syntactically and semantically correct before its final verdict can be trusted.

---

## 29. 29. Repository Workflow

Typical development workflow:

```bash
cd ~/xnupp
git status
make -f xnuxx.mk clean
make -f xnuxx.mk
file build/xnuxx.elf
git diff
git add ...
git commit -m "..."
git push origin main
```

Generated build artifacts should generally be ignored unless the repository deliberately distributes them.

A meaningful integration commit can be:

```text
Complete xnu++ platform integration
```

---

## 30. 30. What 'Complete' Means

XNU++ being "complete" must be understood relative to its defined project scope.

It does NOT mean:

- every possible operating-system feature exists;
- XNU has been replaced;
- every XNU subsystem was rewritten;
- every future device is supported;
- every cryptographic or recovery scenario is automatically proven correct.

It means the current XNU++ platform architecture and implementation have been integrated for the project's declared scope.

Documentation should distinguish:

```text
implemented
integrated
experimental
planned
unsupported
```

That distinction is especially important for security-sensitive features.

---

## 31. 31. File-by-File XNU++ Map

The core XNU++-owned map is:

```text
include/xnu++/bootloader.h   → boot API
include/xnu++/bus.h          → bus API
include/xnu++/compat.h       → compatibility API
include/xnu++/crypto.h       → crypto API
include/xnu++/device.h       → device API
include/xnu++/diagnostics.h  → diagnostics API
include/xnu++/features.h     → feature/capability API
include/xnu++/isolation.h    → isolation API
include/xnu++/quota.h        → quota API
include/xnu++/recovery.h     → recovery API
include/xnu++/reliability.h  → reliability API
include/xnu++/security.h     → security API
include/xnu++/update.h       → update API

boot/boot_entry.c            → C boot entry
boot/boot_platform.c         → platform boot/security logic
boot/string.c                → freestanding memory/string support
boot/long_mode.S             → long-mode support
boot/multiboot2.S            → i386 Multiboot2 entry
boot/multiboot2_64.S         → x86-64 Multiboot2 entry
boot/linker.ld               → i386 linker layout
boot/linker64.ld             → x86-64 linker layout

kernel/kernel.c              → i386 XNU++ platform code
kernel/kernel64.c            → x86-64 XNU++ platform code
kernel/README.md             → kernel documentation

security/bootloader.c        → boot security integration
security/crypto.c            → crypto implementation

userspace/orange_loader.c    → Orange userspace bridge

tools/xnu++/arch-probe.sh    → architecture probing
tools/xnu++/check-x86.sh     → x86 checks
tools/xnu++/doctor.sh        → diagnostics
tools/xnu++/probe.sh         → capability probing
tools/xnu++/reliability-matrix.json → reliability data
tools/xnu++/support-matrix.json     → support data
tools/xnu++/validate-profile.sh      → profile validation
tools/xnu++/validate-reliability.sh  → reliability validation
```

---

## 32. 32. The One-Sentence Definition

The most compact technically accurate description is:

> **XNU++ is a platform engineering layer built around XNU that adds explicit boot, security, cryptographic, recovery, update, rollback, device, bus, isolation, quota, reliability, diagnostics, compatibility, and distribution interfaces while continuing to use XNU's Mach, VM, IPC, BSD, IOKit, libkern, syscall, and architecture foundations.**

---

## 33. 33. Final Mental Model

Remember the project as:

```text
                    XNU++
                      │
        ┌─────────────┼─────────────┐
        │             │             │
      Policy       Interfaces     Integration
        │             │             │
 Security/Boot    Device/Bus     Orange/userspace
 Recovery/Update  Crypto         Distribution
 Isolation/Quota  Diagnostics    Profiles
        │             │             │
        └─────────────┼─────────────┘
                      ↓
                     XNU
                      ↓
            Mach / BSD / IOKit / VM
                      ↓
                   Hardware
```

XNU is the foundation. XNU++ is the platform layer around it. Orange is the userspace/distribution direction. The repository contains both the XNU foundation and the XNU++ integration needed to build the project.

---

## Important Scope Note

This guide documents the XNU++ architecture and the project files known from the current repository structure. The repository contains thousands of XNU source files, so "every file" in the literal sense would require enumerating every upstream XNU source file individually. Those files are therefore documented by subsystem/directory here, while the XNU++-owned files are mapped individually.

For security-sensitive features, source presence and successful compilation must not be confused with proof of a production-grade security implementation. In particular, a cryptographic API or object file is not itself evidence that an Ed25519 implementation has been independently validated.

## Final Architecture

```text
                         Applications
                              │
                              ▼
                    Orange Userspace
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│                            XNU++                             │
│                                                              │
│ Boot / Security / Crypto / Recovery / Update / Rollback      │
│ Device / Bus / Isolation / Quota / Diagnostics / Reliability │
│ Compatibility / Features / Provider Interfaces               │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│                             XNU                              │
│                                                              │
│ Mach / IPC / VM / Tasks / Threads / Scheduler                 │
│ BSD / VFS / Networking / IOKit / libkern / Syscalls           │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
                           Hardware
```

**XNU++ is the platform layer. XNU remains the kernel foundation.**
