# HypoV VMX Roadmap — Transparent Guest OS Virtualization

## Goal

Boot Windows 7 (x64) as a fully transparent guest VM under HypoV. The guest must have no reliable way to detect it is virtualized. A VMCALL-based proof-of-concept tool running inside Windows 7 confirms the guest is running under HypoV.

**Target CPU requirement**: Intel with VT-x + "unrestricted guest" support (Nehalem 2008+). Unrestricted guest mode is required to run the guest in real mode during Windows boot.

## Existing infrastructure

The following is already in place and ready to use:

| Component | Location |
|---|---|
| MSR read/write | `include/system.h`: `msr_read()`, `msr_write()` |
| CR0/CR3/CR4 access | `include/system.h` |
| CPUID | `sys/system.c` |
| VMX feature flag | `include/cpu.h`: `CPU_FEATURE_VMX`, `CR4_VMXE` |
| 4K heap allocator | `sys/memory.c`: `mm_expand_heap_4k()` |
| Interrupt dispatch | `sys/core/host_int.c` |
| Debug output | serial + VGA + `hv_printf()` |
| 64-bit entry point | `sys/core/hvisor.c`: `hv_start()` |

---

## Phase 1 — VMX host infrastructure

### `include/vmx.h` (new)
- VMX MSR numbers: `IA32_VMX_BASIC` (0x480), `IA32_VMX_PINBASED_CTLS` (0x481), `IA32_VMX_PROCBASED_CTLS` (0x482), `IA32_VMX_EXIT_CTLS` (0x483), `IA32_VMX_ENTRY_CTLS` (0x484), `IA32_VMX_MISC` (0x485), `IA32_VMX_PROCBASED_CTLS2` (0x48B)
- VMCS field encodings for all used fields (host state, guest state, control fields)
- `struct VmxCapabilities` — parsed capability MSRs (allowed-0 / allowed-1 bitmasks)
- `struct Vmcs` — 4KB aligned VMCS region (first 4 bytes = revision ID from `IA32_VMX_BASIC`)
- `struct VmxState` — per-vCPU state: VMXON region, VMCS, guest register snapshot

### `sys/core/vmx_ll.asm` (new, `-felf64`)
Low-level VMX instruction wrappers:
- `vmx_on(uint64_t *vmxon_pa)` — VMXON
- `vmx_off()` — VMXOFF
- `vmx_clear(uint64_t *vmcs_pa)` — VMCLEAR
- `vmx_ptr_load(uint64_t *vmcs_pa)` — VMPTRLD
- `vmx_read(uint32_t field)` → `uint64_t` — VMREAD
- `vmx_write(uint32_t field, uint64_t value)` — VMWRITE
- `vmx_launch()` — VMLAUNCH (first VM entry)
- `vmx_resume()` — VMRESUME (re-entry after VM exit)
- `vmx_exit_trampoline` — saves all GPRs, calls `vmx_exit_handler()`, restores, calls `vmx_resume()`

### `sys/core/vmx.c` (new)
- `vmx_check_support()` — checks `CPU_FEATURE_VMX` in `cpu_info->ci_features`
- `vmx_read_capabilities()` — reads all VMX MSRs, fills `VmxCapabilities`
- `vmx_adjust_controls(uint32_t requested, uint32_t msr)` — applies allowed-0/allowed-1 masks from capability MSRs
- `vmx_enable()` — sets `CR4_VMXE`, writes revision ID to VMXON region, calls `vmx_on()`

---

## Phase 2 — VMCS initialization

### `sys/core/vmcs.c` (new)

**Host-state fields** — our 64-bit hypervisor environment:
- `HOST_CR0/CR3/CR4` — from `cr0_read()`, `cr3_read()`, `cr4_read()`
- `HOST_CS/SS/DS/ES/FS/GS/TR` — current GDT selectors (from `host_cpu.c`)
- `HOST_GDTR_BASE`, `HOST_IDTR_BASE` — hypervisor GDT and IDT base addresses
- `HOST_RSP` — hypervisor stack for VM exit landing
- `HOST_RIP` — `vmx_exit_trampoline` (VM exit handler entry in `vmx_ll.asm`)

**Guest-state fields** — Windows 7 real-mode boot initial state:
- `GUEST_CR0` — PE=0, PG=0 (real mode), plus required fixed bits from capability MSRs
- `GUEST_CR3` — 0 (paging disabled)
- `GUEST_CR4` — required fixed bits only
- `GUEST_CS/DS/SS/ES/FS/GS` — real-mode segments (base = selector×16, limit = 0xFFFF, AR = real-mode attributes)
- `GUEST_RIP` — `0x7C00` (MBR entry point)
- `GUEST_RSP` — `0x7C00`
- `GUEST_RFLAGS` — `0x2` (reserved bit always set)
- `GUEST_GDTR_BASE/LIMIT` — real-mode IVT: base=0, limit=0x3FF
- `GUEST_IDTR_BASE/LIMIT` — same
- `GUEST_EFER` — 0 (no long mode)

**Control fields:**
- **Pin-based**: external interrupt exiting, NMI exiting
- **Primary proc-based**: RDTSC exiting, activate secondary controls; I/O passthrough initially
- **Secondary proc-based**: EPT, unrestricted guest (required for real mode), VPID, RDTSCP
- **VM-exit**: host address-space size (64-bit host), save/load EFER
- **VM-entry**: IA-32e guest = 0 (real mode entry), load EFER

---

## Phase 3 — EPT (Extended Page Tables)

### `sys/core/ept.c` (new)

4-level EPT mapping guest-physical → host-physical 1:1 for all RAM from the multiboot memory map. Use 2MB pages where possible (matching the host page table granularity).

- `ept_build(struct PhysicalMMapping *maps)` → `uint64_t eptp`
- EPTP: memory type WB (6), page-walk length 4, EPT PML4 base address
- All RAM regions: RWX; MMIO holes: unmapped (EPT violation on access)
- HypoV's own memory (≥16MB, where `hvcore.elf64` lives): excluded or read-only — guest cannot modify the hypervisor

---

## Phase 4 — VM exit handling

### `sys/core/vmx_exit.c` (new)

`vmx_exit_handler(struct TrapFrame *frame)` — called from `vmx_exit_trampoline` in `vmx_ll.asm`.

Dispatches on `VM_EXIT_REASON` VMCS field:

| Exit reason | Action |
|---|---|
| CPUID (10) | Mask `ECX[31]` (hypervisor present bit) and `CPUID[0x40000000]` vendor leaf; pass through otherwise |
| RDTSC / RDTSCP (16 / 51) | Apply TSC offset to compensate for VM overhead |
| VMCALL (18) | Hypercall interface (see Phase 5) |
| I/O instruction (30) | Pass through to real hardware via `inb`/`outb` |
| External interrupt (1) | Handle host interrupt, re-enter guest |
| EPT violation (48) | Log + halt (unmapped MMIO — extend as needed) |
| Triple fault (31) | Log full guest state to serial, halt |

After each handler: `vmx_resume()` to re-enter guest.

Modify `sys/core/host_int.c` to route the VM exit interrupt vector to `vmx_exit_handler`.

---

## Phase 5 — Proof of virtualization

### Hypercall interface (in `vmx_exit.c`)

When the guest executes `VMCALL` with `RAX = 0x48594F56` ("HYOV"):
- HypoV sets `RBX = 0xDEADBEEF` (magic signature)
- HypoV sets `RCX` = total VM exit counter
- Returns to guest

### Guest-side tool: `tools/guest/hypv_check.c`

Minimal Windows 7 x64 `.exe` (no CRT dependency):
```c
// executes VMCALL, reads RBX/RCX, prints result
```
- "Running under HypoV — exit count: N" if `RBX == 0xDEADBEEF`
- "Not running under HypoV" otherwise
- Cross-compiled with `x86_64-w64-mingw32-gcc`

---

## Phase 6 — Integration into `hv_start()`

`sys/core/hvisor.c::hv_start()` call sequence after existing init:

```
vmx_check_support()      // abort with error if no VT-x
vmx_read_capabilities()  // parse VMX MSRs
vmx_enable()             // CR4.VMXE + VMXON
ept_build(sys_info->maps) // build guest physical memory map
vmcs_init(sys_info)      // set all VMCS fields
vmx_launch()             // enter Windows 7 real-mode boot — does not return
```

---

## Phase 7 — Windows 7 boot media

The guest starts at `CS:IP = 0000:7C00` (real mode MBR entry). Two options:

1. **MBR stub first** — embed a tiny 512-byte real-mode "Hello from guest" stub at physical address `0x7C00` in the hypervisor. Confirms unrestricted guest + real-mode execution before wiring up disk I/O.

2. **Disk passthrough** — intercept IDE/SATA I/O port exits and forward to a disk image or physical disk. Windows 7 MBR and bootloader run normally.

Start with option 1 to validate the VMX stack end-to-end, then extend to option 2.

---

## Files summary

| File | Status | Purpose |
|---|---|---|
| `include/vmx.h` | Create | VMX constants, VMCS encodings, structs |
| `sys/core/vmx.c` | Create | VMXON, capabilities, enable |
| `sys/core/vmx_ll.asm` | Create | VMXON/VMREAD/VMWRITE/VMLAUNCH/VMRESUME/VMCALL |
| `sys/core/vmcs.c` | Create | VMCS host/guest/control field initialization |
| `sys/core/ept.c` | Create | EPT builder |
| `sys/core/vmx_exit.c` | Create | VM exit dispatch and handlers |
| `sys/core/Makefile` | Modify | Add new source files |
| `sys/core/hvisor.c` | Modify | Call VMX init from `hv_start()` |
| `sys/core/host_int.c` | Modify | Route VM exit vector |
| `tools/guest/hypv_check.c` | Create | Windows 7 VMCALL proof tool |

---

## Verification milestones

1. **VMXON succeeds** — `hv_printf` confirms no `#UD` or `#GP` on VMXON
2. **VMLAUNCH returns via exit** — first VM exit logged to serial (should be CPUID or I/O from MBR stub)
3. **Real-mode guest executes** — MBR stub prints to VGA from inside the VM
4. **Windows 7 boot progresses** — BIOS POST and bootloader messages visible
5. **Hypercall proof** — `hypv_check.exe` running in Windows 7 prints HypoV signature and exit counter
6. **Transparency** — Windows 7 Task Manager, `msinfo32`, and Device Manager show no virtualization indicators
