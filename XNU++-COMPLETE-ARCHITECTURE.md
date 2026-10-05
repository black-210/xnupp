# XNU++ — Complete Architecture, Layers, Repository and File Guide

**Repository root:** `/data/data/com.termux/files/home/xnupp`  
**Project:** XNU++  
**Repository:** `https://github.com/black-210/xnupp`

---

## 1. Project Definition

XNU++ is a project-specific platform and engineering layer built around the XNU foundation.

**XNU++ is not intended to replace XNU.** XNU supplies the major kernel foundation—Mach, BSD, VM, IPC, scheduling, VFS, IOKit, kernel libraries, platform support, and related infrastructure. XNU++ adds its own boot integration, platform interfaces, security and policy integration, recovery, update, reliability, diagnostics, capability/compatibility model, device and bus abstractions, isolation, quotas, distribution profiles, tooling, tests, and userspace direction.

The core architectural idea is:

```text
                         XNU++
                           |
        +------------------+------------------+
        |                                     |
        |       XNU++ platform layer         |
        |                                     |
        | boot / security / recovery         |
        | update / reliability / diagnostics |
        | device / bus / isolation / quota   |
        | compatibility / capabilities       |
        +------------------+------------------+
                           |
                           v
                         XNU
        +------------------+------------------+
        |                  |                  |
       Mach               BSD               IOKit
        |                  |                  |
        +------------------+------------------+
                           |
                           v
                  Hardware / Platform
```

---

# 2. Repository Root

The supplied `pwd` is:

```text
/data/data/com.termux/files/home/xnupp
```

The supplied top-level tree is:

```text
xnupp/
├── .codeartsdoer/
├── .github/
├── .klaatai/
├── .rytora/
├── EXTERNAL_HEADERS/
├── SETUP/
├── boot/
├── bsd/
├── build/
├── build64/
├── config/
├── distribution/
├── doc/
├── include/
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
├── tools/
├── userspace/
├── Makefile
├── README.md
├── XNU++_VERSION
└── xnuxx.mk
```

This is a **mixed repository**: a large XNU foundation plus a smaller project-specific XNU++ layer and supporting engineering infrastructure.

---

# 3. The Layer Model

A practical architectural decomposition is:

```text
Layer 8 — Distribution / Userspace
    distribution/
    userspace/

Layer 7 — Tests / Engineering / Validation
    tests/
    tools/xnu++/

Layer 6 — XNU++ Platform Services
    security/
    kernel/
    policies and integration

Layer 5 — XNU++ Public Interfaces
    include/xnu++/

Layer 4 — XNU++ Boot / Machine Entry
    boot/

Layer 3 — XNU Integration
    kernel/ + selected integration points

Layer 2 — XNU Foundation
    osfmk/
    bsd/
    iokit/
    libkern/
    libsa/
    libsyscall/
    pexpert/
    EXTERNAL_HEADERS/

Layer 1 — Hardware / Firmware / Boot Environment
    CPU / memory / firmware / devices / buses
```

These are architectural layers, not a claim that every source file has exactly one dependency direction.

---

# 4. Layer 1 — Hardware and Firmware

At the bottom is the physical platform:

- CPU and execution modes.
- Physical memory.
- Paging hardware.
- Interrupt facilities.
- Timers.
- Devices and buses.
- Firmware.
- Boot environment.

The early XNU++ boot code bridges this environment to software.

Conceptually:

```text
Hardware
   |
   v
Firmware / boot environment
   |
   v
Bootloader
   |
   v
XNU++ boot entry
```

---

# 5. Layer 2 — XNU Foundation

The supplied XNU core directories are:

```text
osfmk/
bsd/
iokit/
libkern/
libsa/
libsyscall/
pexpert/
EXTERNAL_HEADERS/
```

These are the foundation on which XNU++ operates.

## 5.1 `osfmk/`

`osfmk/` contains the Mach-oriented and core kernel machinery.

Important conceptual areas include:

```text
osfmk/kern/     kernel core
osfmk/ipc/      Mach IPC
osfmk/vm/       virtual memory
osfmk/sched/    scheduling
osfmk/x86_64/   architecture-specific support
```

XNU already provides major mechanisms such as:

- tasks;
- threads;
- scheduler infrastructure;
- synchronization;
- kernel objects;
- Mach IPC;
- VM;
- low-level kernel services;
- architecture-specific execution support.

XNU++ therefore does **not** need to reimplement those merely to have its own platform layer.

## 5.2 `bsd/`

BSD-side XNU functionality, including areas such as:

- processes;
- POSIX interfaces;
- system calls;
- VFS;
- filesystems;
- networking;
- sockets;
- credentials;
- BSD kernel services.

Conceptually:

```text
Userspace
   |
   v
BSD interfaces
   |
   v
XNU
```

## 5.3 `iokit/`

The I/O/device subsystem.

It provides the underlying architecture for:

- device objects;
- services;
- drivers;
- device matching;
- I/O communication;
- hardware-facing infrastructure.

This is directly relevant to XNU++'s `device.h` and `bus.h` abstractions.

## 5.4 `libkern/`

Kernel support infrastructure and utility facilities used by XNU.

It should be treated as foundation code rather than automatically as XNU++-authored code.

## 5.5 `libsa/`

Standalone/boot-support functionality used by relevant XNU build paths.

## 5.6 `libsyscall/`

The userspace/system-call boundary and associated library infrastructure.

Conceptually:

```text
Userspace
   |
   v
libsyscall
   |
   v
system call boundary
   |
   v
XNU
```

## 5.7 `pexpert/`

Platform-expert code that connects generic XNU mechanisms with platform-specific details.

```text
Generic XNU
    |
    v
Platform Expert
    |
    v
Hardware/platform
```

## 5.8 `EXTERNAL_HEADERS/`

External/public header material used by the XNU source tree.

It is part of the repository's XNU foundation and should not automatically be treated as newly authored XNU++ code.

---

# 6. Layer 3 — XNU++ Kernel Integration

The project-specific kernel directory is:

```text
kernel/
├── README.md
├── kernel.c
└── kernel64.c
```

## `kernel/README.md`

Documentation for the project-specific kernel/platform layer.

## `kernel/kernel.c`

The project-specific 32-bit/i386-oriented kernel/platform implementation.

It should be understood as XNU++ integration/platform code, not a replacement for the complete XNU kernel.

## `kernel/kernel64.c`

The project-specific 64-bit implementation/integration layer.

The relationship is:

```text
kernel/kernel.c
       |
       +---- XNU++ 32-bit/platform integration

kernel/kernel64.c
       |
       +---- XNU++ 64-bit/platform integration

both
       |
       v
      XNU
```

---

# 7. Layer 4 — Boot System

The supplied boot files are:

```text
boot/boot_entry.c
boot/boot_platform.c
boot/grub.cfg
boot/kernal.c
boot/linker.ld
boot/linker64.ld
boot/long_mode.S
boot/multiboot2.S
boot/multiboot2_64.S
boot/string.c
```

> `boot/kernal.c` is spelled exactly this way in the supplied repository listing. It must not be assumed to be the same file as `kernel/kernel.c` without inspecting its contents.

## 7.1 `boot/boot_entry.c`

C-level entry point after the earliest assembly setup.

Typical conceptual flow:

```text
Multiboot entry
      |
      v
boot_entry.c
      |
      v
platform initialization
      |
      v
XNU++ integration
```

## 7.2 `boot/boot_platform.c`

A major XNU++ boot/platform component.

It is associated with platform initialization and boot security/measurement/verification paths.

Conceptual secure boot flow:

```text
Boot artifact
     |
     v
Measure
     |
     v
Digest
     |
     v
Signature / policy verification
     |
     v
Accept or reject
```

A crypto API or verification hook alone does not prove production-grade cryptography; the implementation, trusted key source, signed data format, rollback handling, and failure path must all be verified.

## 7.3 `boot/multiboot2.S`

32-bit Multiboot2 entry path.

It establishes low-level state and transfers execution into the C boot layer.

## 7.4 `boot/multiboot2_64.S`

64-bit-oriented Multiboot2 entry path.

The conceptual transition is:

```text
32-bit entry
   |
   v
page tables
   |
   v
CR3 / CPU feature setup
   |
   v
EFER long-mode enable
   |
   v
paging enable
   |
   v
GDT / far jump
   |
   v
64-bit execution
   |
   v
boot_entry()
```

## 7.5 `boot/long_mode.S`

Low-level x86-64 long-mode transition machinery.

Relevant concepts:

- GDT;
- page tables;
- CR3;
- CR4;
- EFER;
- CR0;
- stack setup;
- 32-bit to 64-bit transition;
- 64-bit entry.

## 7.6 `boot/linker.ld`

32-bit linker layout.

## 7.7 `boot/linker64.ld`

64-bit linker layout.

A linker script determines section placement and entry layout in the final ELF.

## 7.8 `boot/grub.cfg`

Boot configuration for the relevant boot path.

## 7.9 `boot/string.c`

Minimal string support suitable for freestanding/early boot code.

---

# 8. Layer 5 — XNU++ Public Interface Layer

The project-specific public headers are:

```text
include/xnu++/
├── bootloader.h
├── bus.h
├── compat.h
├── crypto.h
├── device.h
├── diagnostics.h
├── features.h
├── isolation.h
├── quota.h
├── recovery.h
├── reliability.h
├── security.h
└── update.h
```

This directory is one of the clearest representations of the XNU++ architecture.

## `bootloader.h`

Bootloader/platform interface.

It defines the contract between boot-related code and higher XNU++ logic.

## `bus.h`

Project-level bus abstraction.

```text
XNU++ service
     |
     v
XNU++ bus API
     |
     +---- XNU/IOKit-backed provider
     +---- another provider
     +---- future/native provider
```

## `compat.h`

Compatibility interface for centralizing platform/implementation compatibility decisions.

## `crypto.h`

Cryptographic interface, including the project's Ed25519-oriented API surface.

Conceptually:

```text
public key + message + signature
              |
              v
       signature verification
              |
          valid/invalid
```

The declaration is not itself proof that the cryptographic implementation is complete.

## `device.h`

Project-level device abstraction.

It lets higher layers reason about devices without exposing every provider-specific implementation detail.

## `diagnostics.h`

Diagnostic interface for system/platform state and failure reporting.

## `features.h`

Feature/capability model.

```text
hardware/platform
       |
       v
capability detection
       |
       v
features
       |
       v
policy decision
```

## `isolation.h`

Isolation interface for component/resource/policy boundaries.

## `quota.h`

Resource quota interface.

Possible resources include memory, devices, execution resources, storage, or other project-defined resources.

## `recovery.h`

Recovery and rollback interface.

```text
normal operation
      |
      v
failure
      |
      v
recovery
      +---- restart
      +---- rollback
      +---- safe state
```

## `reliability.h`

Reliability/health policy interface.

## `security.h`

Security policy/interface boundary.

## `update.h`

Update lifecycle interface.

```text
candidate
   |
   v
validate -> verify -> stage -> activate -> health check
                                      |
                         +------------+------------+
                         |                         |
                       success                   failure
                         |                         |
                       commit                  rollback
```

---

# 9. Layer 6 — XNU++ Security

The supplied security tree is:

```text
security/
├── Makefile
├── _label.h
├── bootloader.c
├── conf/
├── crypto.c
├── mac.h
├── mac_audit.c
├── mac_base.c
├── mac_data.c
├── mac_data.h
├── mac_file.c
├── mac_framework.h
├── mac_internal.h
├── mac_iokit.c
├── mac_kext.c
├── mac_label.c
├── mac_mach.c
├── mac_mach_internal.h
├── mac_necp.c
├── mac_pipe.c
├── mac_policy.h
├── mac_posix_sem.c
├── mac_posix_shm.c
├── mac_priv.c
├── mac_process.c
├── mac_pty.c
├── mac_skywalk.c
├── mac_socket.c
├── mac_system.c
├── mac_sysv_msg.c
├── mac_sysv_sem.c
├── mac_sysv_shm.c
├── mac_vfs.c
└── mac_vfs_subr.c
```

This directory must be read with provenance in mind. The names `mac_*` are associated with XNU's security/MAC architecture; their presence does not automatically mean every file is newly authored XNU++ code.

## `security/bootloader.c`

Security-side boot integration.

## `security/crypto.c`

Cryptographic implementation/integration corresponding to the XNU++ crypto interface.

Security claims must be based on the actual implementation, not merely the existence of this file.

## `security/conf/`

Contains:

```text
Makefile
Makefile.arm64
Makefile.template
Makefile.x86_64
copyright.nai
files
files.arm64
files.x86_64
```

These describe/build the security subsystem and architecture-specific source composition.

## MAC/security files

Conceptually, the major groups include:

```text
mac_file.c       file/VFS security
mac_process.c    process security
mac_mach.c       Mach security
mac_iokit.c      IOKit security
mac_socket.c     socket/network security
mac_vfs.c        VFS security
mac_pipe.c       pipe security
mac_priv.c       privilege security
mac_system.c     system security
mac_label.c      security labels
mac_audit.c      audit-related security
```

Exact provenance must follow each file's headers/history.

---

# 10. Distribution Layer

The supplied distribution files are:

```text
distribution/
├── README.md
├── default.profile
├── recovery.policy
├── reliability.profile
├── security.profile
└── update.policy
```

This layer turns generic platform mechanisms into configured distribution behavior.

## `default.profile`

Default configuration/profile.

## `security.profile`

Security policy profile.

## `recovery.policy`

Recovery behavior/policy.

## `reliability.profile`

Reliability configuration.

## `update.policy`

Update behavior/policy.

## `README.md`

Documentation for the distribution/profile system.

The relationship is:

```text
XNU++ mechanisms
       |
       v
profiles / policies
       |
       v
specific distribution behavior
```

---

# 11. Userspace Layer

The supplied XNU++ userspace file is:

```text
userspace/orange_loader.c
```

This represents the bridge toward the project's Orange OS/userspace direction.

Conceptually:

```text
XNU
 |
v
XNU++ platform
 |
v
orange_loader
 |
v
Orange OS userspace
 |
v
services / applications
```

A loader/module existing does not by itself mean that a complete production userspace exists; the actual handoff and service model must be implemented and validated.

---

# 12. Tools Layer

The project-specific tools are:

```text
tools/xnu++/
├── arch-probe.sh
├── check-x86.sh
├── doctor.sh
├── probe.sh
├── reliability-matrix.json
├── support-matrix.json
├── validate-profile.sh
└── validate-reliability.sh
```

## `arch-probe.sh`

Architecture capability detection.

## `check-x86.sh`

x86 platform/build checks.

## `doctor.sh`

Environment/repository diagnostic tool.

Conceptually:

```text
repo + toolchain + architecture + configuration
                         |
                         v
                      doctor
                         |
                         v
                    diagnostics
```

## `probe.sh`

General capability/environment probing.

## `support-matrix.json`

Machine-readable supported/unsupported platform combinations.

## `reliability-matrix.json`

Machine-readable reliability/support state information.

## `validate-profile.sh`

Distribution profile validation.

## `validate-reliability.sh`

Reliability configuration validation.

---

# 13. Tests Layer

The supplied `tests/` directory is very large and contains many XNU tests. It includes areas such as:

```text
tests/ipc/
tests/vm/
tests/sched/
tests/vfs/
tests/iokit/
tests/skywalk/
tests/ktrace/
tests/unit/
tests/memorystatus/
tests/workq/
tests/nvram_tests/
```

It also contains hundreds of individual regression, architecture, security, memory, networking, filesystem, process, Mach, and VM tests.

The key distinction is:

> The existence of a test under `tests/` does not automatically mean that the corresponding subsystem was implemented by XNU++.

Many tests belong to the underlying XNU ecosystem.

A useful test architecture is:

```text
XNU / XNU++ implementation
          |
          v
      test harness
          |
   +------+------+------+------+ 
   |      |      |      |      |
   VM    IPC   sched   VFS   IOKit
   |      |      |      |      |
   +------+------+------+------+ 
          |
          v
      regression
          |
          v
       unit tests
```

---

# 14. Build Infrastructure

The repository contains multiple build systems and build components:

```text
Makefile
xnuxx.mk
EXTERNAL_HEADERS/Makefile
SETUP/Makefile
bsd/Makefile
config/Makefile
iokit/Makefile
libkern/Makefile
libsa/Makefile
osfmk/Makefile
pexpert/Makefile
san/Makefile
security/Makefile
tests/Makefile
tools/Makefile
```

These do not all mean that every part of XNU can be built on every host with one command.

The upstream XNU build has platform/toolchain assumptions. The XNU++ project-specific `xnuxx.mk` provides its own freestanding/cross-target build path.

---

# 15. `xnuxx.mk`

This is the dedicated XNU++ build makefile.

On an ARM64 Termux development host, the i386 target can be explicitly selected using a target such as:

```text
--target=i386-unknown-elf
```

The conceptual process is:

```text
C / ASM
   |
   v
Clang cross compilation
   |
   v
object files
   |
   v
ld + linker script
   |
   v
ELF
```

This is cross compilation: the host architecture and generated target architecture can differ.

---

# 16. `Makefile`

Top-level build orchestration for the repository/XNU build environment.

It should be distinguished from the smaller project-specific `xnuxx.mk` path.

---

# 17. `config/`

Contains configuration/build material, including:

```text
config/Makefile
config/README.DEBUG-kernel.txt
```

It bridges source configuration and selected build targets.

---

# 18. `makedefs/`

XNU build-system definitions and make infrastructure.

This is build infrastructure rather than an XNU++ runtime layer.

---

# 19. `libkdd/`

Kernel debug/data support infrastructure.

It belongs to the broader XNU engineering environment.

---

# 20. `san/`

Sanitizer-related support and validation infrastructure.

It is primarily an engineering/debugging facility, not a replacement for the XNU++ security layer.

---

# 21. `SETUP/`

Repository setup/build support.

The supplied tree contains:

```text
SETUP/Makefile
```

---

# 22. `doc/`

General documentation area.

Documentation should be distinguished from runtime implementation.

---

# 23. Hidden and Automation Directories

The supplied repository also contains:

```text
.codeartsdoer/
.github/
.klaatai/
.rytora/
```

These are project/automation/metadata areas. They are not automatically kernel layers.

---

# 24. Build Artifacts

The supplied tree contains:

```text
build/
build64/
iso/
```

These should normally be considered generated/output areas rather than authoritative source.

A clean project workflow distinguishes:

```text
source        -> authoritative
configuration -> authoritative
build/        -> generated
build64/      -> generated
iso/          -> generated/distribution artifact
```

---

# 25. Complete Boot Flow

The intended conceptual path is:

```text
Firmware / boot environment
          |
          v
Bootloader
          |
          v
Multiboot2 metadata
          |
          v
boot/multiboot2*.S
          |
          v
CPU / page tables / stack setup
          |
          v
boot_entry.c
          |
          v
boot_platform.c
          |
          v
security / measurement / verification
          |
          v
kernel/kernel.c or kernel64.c
          |
          v
XNU
          |
    +-----+-----+-----+
    |     |     |     |
   Mach   VM    BSD  IOKit
    |     |     |     |
    +-----+-----+-----+
          |
          v
XNU++ platform services
          |
          v
userspace loader
          |
          v
Orange OS / userspace
          |
          v
services
          |
          v
applications
```

---

# 26. XNU++ Security Flow

A high-level secure boot concept is:

```text
boot artifact
     |
     v
measure
     |
     v
cryptographic digest
     |
     v
signature verification
     |
     v
security policy
     |
     +---- valid ----> continue
     |
     +---- invalid --> reject/recovery
```

A complete production security claim requires all of the following to be verified:

1. Actual cryptographic implementation.
2. Trusted public-key source.
3. Exact signed data format.
4. Signature verification integration.
5. Failure-closed behavior.
6. Anti-rollback state.
7. Key rotation/revocation strategy.
8. Boot-chain trust.
9. Memory-safety properties.
10. Correct handling of malformed input.

---

# 27. Update + Recovery + Reliability

These subsystems form one lifecycle:

```text
                    +------------------+
                    |  candidate image |
                    +--------+---------+
                             |
                             v
                        validation
                             |
                             v
                       verification
                             |
                             v
                           stage
                             |
                             v
                         activate
                             |
                             v
                       health check
                        /          \
                       /            \
                    healthy       failure
                       |              |
                       v              v
                    commit         diagnostics
                                      |
                                      v
                                   rollback
                                      |
                                      v
                                   recovery
```

The interfaces are represented by:

```text
update.h
recovery.h
reliability.h
diagnostics.h
```

Policies are represented in:

```text
distribution/update.policy
distribution/recovery.policy
distribution/reliability.profile
```

---

# 28. Device and Bus Architecture

The project-level abstraction is:

```text
XNU++ service
       |
       v
device.h / bus.h
       |
       v
provider abstraction
       |
       v
XNU / IOKit / hardware
```

This makes it possible for higher layers to depend on a project-level contract instead of every consumer directly depending on one provider implementation.

---

# 29. Capability Architecture

The feature/compatibility path is:

```text
Hardware
   |
   v
Capability detection
   |
   v
features.h
   |
   v
compat.h
   |
   v
policy
   |
   +---- supported
   +---- degraded
   +---- rejected
```

For security-sensitive behavior, an unknown capability should not silently be treated as safe.

---

# 30. Isolation and Quotas

The intended conceptual relationship is:

```text
XNU resource
     |
     v
XNU++ isolation
     |
     v
XNU++ quota
     |
     v
policy enforcement
```

Potential resource classes include memory, devices, execution resources, storage, or project-specific resource handles.

---

# 31. Diagnostics

Diagnostics connect implementation state with policy and recovery.

```text
system state
    |
    v
health/error detection
    |
    v
diagnostics
    |
    v
classification
    |
    +---- normal
    +---- degraded
    +---- recovery
    +---- rollback
    +---- unsupported
```

---

# 32. Provider-Neutral Design

A major architectural objective is to keep higher-level XNU++ interfaces provider-neutral:

```text
                  XNU++ service
                       |
                       v
               provider-neutral API
                  /          \
                 /            \
                v              v
          XNU/IOKit        future/native
           provider          provider
```

This is an interface design goal, not a claim that every provider implementation already exists.

---

# 33. 32-bit and 64-bit Split

The supplied project has explicit x86 boot artifacts:

```text
32-bit
  boot/multiboot2.S
  boot/linker.ld
  kernel/kernel.c

64-bit
  boot/multiboot2_64.S
  boot/long_mode.S
  boot/linker64.ld
  kernel/kernel64.c
```

The split exists because CPU mode, registers, ABI, paging, linker layout, and boot transition requirements differ.

---

# 34. Freestanding Environment

Early boot/kernel code cannot assume a normal hosted operating-system C runtime.

Therefore the project can require:

- explicit memory setup;
- explicit stack setup;
- minimal string support;
- linker scripts;
- freestanding compiler flags;
- explicit CPU setup;
- no ordinary hosted libc assumption.

This explains the presence of `boot/string.c`, linker scripts, and cross-target compiler configuration.

---

# 35. Why XNU++ Does Not Reimplement XNU

XNU already contains the major kernel machinery:

```text
Mach IPC
VM
scheduler
threads/tasks
VFS
BSD services
networking
IOKit
kernel support libraries
```

XNU++ instead provides a project-level architecture around those mechanisms.

This is one of the most important distinctions in the entire project.

---

# 36. Project-Owned vs Foundation Material

A useful repository classification is:

```text
XNU foundation / derived areas:
    osfmk/
    bsd/
    iokit/
    libkern/
    libsa/
    libsyscall/
    pexpert/
    EXTERNAL_HEADERS/
    substantial portions of tests/

XNU++ project layer:
    boot/
    kernel/
    include/xnu++/
    tools/xnu++/
    distribution/
    userspace/
    project-specific integration

Mixed / provenance requires file-level inspection:
    security/
    tests/
    configuration/build areas
```

For legal licensing and authorship, always follow the actual file header, upstream notice, Git history, and applicable license.

---

# 37. Licensing Architecture

Because the repository contains XNU-derived material and project-original material, licensing must be understood per component.

Conceptually:

```text
XNU-derived material
        |
        v
original applicable XNU licenses

XNU++ original material
        |
        v
GNU AGPLv3 + valid additional terms

Third-party material
        |
        v
its own applicable license
```

A root `LICENSE` file does not automatically relicense every upstream file in the repository.

---

# 38. Contributor and Attribution Layer

The project may use:

```text
LICENSE
CLA.md
XNU++-ADDITIONAL-TERMS.md
```

Their roles differ:

- `LICENSE`: project licensing information.
- `CLA.md`: contributor agreement for submitted contributions.
- `XNU++-ADDITIONAL-TERMS.md`: additional terms applicable only where the relevant rights holder can legally impose them.

A contribution can receive accurate contributor credit without making that contributor the founder or originator of the whole project.

---

# 39. Contribution Principle

The project's central attribution principle can be summarized as:

```text
Contribution
     !=
Foundership

Contribution
     !=
Project ownership

Contribution
     !=
Project origin

Contribution
     !=
Overall project authorship

Contribution
     !=
Authority to represent XNU++
```

A contributor can accurately claim authorship of their own original work where legally applicable.

---

# 40. Repository History

Git history is valuable evidence for:

- original project creation;
- feature chronology;
- commit authorship;
- releases;
- architectural evolution;
- contributor activity.

Historical records should not be intentionally manipulated to create a false impression of project origin.

---

# 41. Project Identity

The project identity is separate from individual contribution authorship.

```text
Original project origin
          |
          v
        XNU++
          ^
          |
     Contributor B
          |
       contribution
```

A later contributor's work can be important without becoming the historical origin of the project.

Project names, logos, branding, and official representation can also involve rights separate from copyright.

---

# 42. Development Status Vocabulary

Use precise status terms:

```text
IMPLEMENTED
    Functionality exists and is integrated.

INTEGRATED
    Existing foundation functionality is connected to XNU++.

ADAPTED
    Existing functionality is adapted for XNU++.

EXPERIMENTAL
    Functionality exists but requires further validation.

PARTIAL
    Some components exist but the feature is incomplete.

PLANNED
    Architecture/documentation exists but implementation is not complete.

INHERITED
    Functionality is primarily supplied by the XNU foundation.
```

An interface declaration is not automatically equivalent to a complete implementation.

---

# 43. What "Complete" Means Here

For this project, completeness should be evaluated against the declared XNU++ scope.

It does **not** mean:

- replacing Mach;
- replacing BSD;
- replacing IOKit;
- rewriting XNU's scheduler;
- rewriting XNU's VM;
- becoming an unrelated independent kernel;
- implementing every hardware device on earth.

It means that the defined XNU++ platform architecture and its declared integration scope are present, documented, buildable where supported, and honestly classified by implementation status.

---

# 44. Recommended Reading Order

A developer should not start by reading thousands of upstream XNU tests.

Recommended order:

```text
1. README.md
2. XNU++_VERSION
3. xnuxx.mk
4. boot/
5. include/xnu++/
6. kernel/
7. security/
8. distribution/
9. tools/xnu++/
10. userspace/
11. selected XNU subsystem
12. tests/
```

Then follow specific XNU++ interfaces down into XNU only where necessary.

---

# 45. How to Read Any File in This Repository

Ask these questions:

```text
1. Is this XNU foundation code?
2. Is this XNU++ project code?
3. Is this third-party code?
4. Is it generated output?
5. Is it configuration?
6. Is it a test?
7. Is it an interface or implementation?
8. Is it policy or mechanism?
9. What layer does it belong to?
10. What depends on it?
11. What does it depend on?
```

This method is more accurate than judging ownership or architecture from filenames alone.

---

# 46. End-to-End Architecture

```text
                           XNU++
                             |
        +--------------------+--------------------+
        |                    |                    |
        v                    v                    v
      Boot                Policy              Interfaces
        |                    |                    |
        +--------------------+--------------------+
                             |
                             v
                    XNU++ integration
                             |
                             v
                            XNU
             +---------------+---------------+
             |               |               |
            Mach            BSD             IOKit
             |               |               |
             +---------------+---------------+
                             |
                             v
                         Hardware
```

Above XNU, XNU++ provides:

```text
Boot integration
Security
Crypto interface/integration
Recovery
Rollback
Update
Reliability
Diagnostics
Isolation
Quota
Compatibility
Capabilities
Devices
Buses
Distribution profiles
Tooling
Userspace direction
```

Inside XNU, the foundation provides:

```text
Mach
IPC
VM
Scheduler
Tasks/threads
BSD
VFS
Networking
IOKit
libkern
syscall boundary
platform support
```

---

# 47. One-Sentence Definition

> **XNU++ is a project-specific platform and engineering layer built around the XNU foundation, adding its own boot integration, interfaces, security, recovery, update, reliability, diagnostics, device/bus abstractions, capability model, distribution policies, tooling, and userspace direction without requiring XNU itself to be replaced.**

---

# 48. Repository Reference Summary

## XNU foundation

```text
osfmk/
bsd/
iokit/
libkern/
libsa/
libsyscall/
pexpert/
EXTERNAL_HEADERS/
```

## XNU++ core

```text
boot/
kernel/
include/xnu++/
security/
distribution/
userspace/
tools/xnu++/
```

## Engineering and build

```text
Makefile
xnuxx.mk
config/
SETUP/
makedefs/
san/
libkdd/
tools/
```

## Testing

```text
tests/
```

## Documentation

```text
README.md
XNU++_VERSION
doc/
distribution/README.md
kernel/README.md
```

## Generated/output areas

```text
build/
build64/
iso/
```

---

# 49. Final Mental Model

Remember XNU++ like this:

```text
                 XNU++ PROJECT
                       |
        +--------------+--------------+
        |              |              |
      Boot          Policies       Interfaces
        |              |              |
        +--------------+--------------+
                       |
                       v
               Platform Integration
                       |
                       v
                      XNU
        +--------------+--------------+
        |              |              |
       Mach           BSD           IOKit
        |              |              |
        +--------------+--------------+
                       |
                       v
                    Hardware
```

**XNU is the foundation. XNU++ is the platform layer built around that foundation.**

---

## Scope and Accuracy Note

This document is based on the repository tree supplied for `/data/data/com.termux/files/home/xnupp`. It deliberately does not invent a unique role for every upstream XNU test/source file. Where a directory is primarily an upstream subsystem, it is explained as a subsystem and its relationship to XNU++ is described.

For exact legal ownership, copyright, and provenance of an individual file, inspect the file's copyright/license header and Git history. For exact implementation behavior, inspect the source rather than inferring behavior from filenames or interfaces alone.
