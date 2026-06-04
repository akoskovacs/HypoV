# High-level kbuild Makefile imported from Linux
# modified for HypoV. Under GPL 2 LICENSE.
#
# Do not:
# o  use make's built-in rules and variables
#    (this increases performance and avoids hard-to-debug behaviour);
# o  print "Entering directory ...";
MAKEFLAGS += -rR --no-print-directory

# Shared C flags across 32bit and 64bit 
SHARED_FLAGS := -std=c99 -nostdinc -fno-builtin -fno-stack-protector -fno-unwind-tables
SHARED_FLAGS += -fno-asynchronous-unwind-tables -ffreestanding -mno-sse -mno-avx
SHARED_FLAGS += -nostartfiles -nodefaultlibs -nostdlib -static -mno-80387 -mno-fp-ret-in-387 -ggdb 

BINARY_TARGET := hypov.bin
TESTFS 		  := testfs.img

# To put more focus on warnings, be less verbose as default
# Use 'make V=1' to see the full commands

ifeq ("$(origin V)", "command line")
  KBUILD_VERBOSE = $(V)
endif
ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif

# kbuild supports saving output files in a separate directory.
# To locate output files in a separate directory two syntaxes are supported.
# In both cases the working directory must be the root of the kernel src.
# 1) O=
# Use "make O=dir/to/store/output/files/"
#
# 2) Set KBUILD_OUTPUT
# Set the environment variable KBUILD_OUTPUT to point to the directory
# where the output files shall be placed.
# export KBUILD_OUTPUT=dir/to/store/output/files/
# make
#
# The O= assignment takes precedence over the KBUILD_OUTPUT environment
# variable.

# Our default target
PHONY := _all
_all:

# KBUILD_SRC is set on invocation of make in OBJ directory
# KBUILD_SRC is not intended to be used by the regular user (for now)
ifeq ($(KBUILD_SRC),)

# OK, Make called in directory where kernel src resides
# Do we want to locate output files in a separate directory?
ifeq ("$(origin O)", "command line")
  KBUILD_OUTPUT := $(O)
endif

ifeq ("$(origin W)", "command line")
  export KBUILD_ENABLE_EXTRA_GCC_CHECKS := $(W)
endif

# Cancel implicit rules on top Makefile
$(CURDIR)/Makefile Makefile: ;

ifneq ($(KBUILD_OUTPUT),)
# Invoke a second make in the output directory, passing relevant variables
# check that the output directory actually exists
saved-output := $(KBUILD_OUTPUT)
KBUILD_OUTPUT := $(shell cd $(KBUILD_OUTPUT) && /bin/pwd)
$(if $(KBUILD_OUTPUT),, \
     $(error output directory "$(saved-output)" does not exist))

PHONY += $(MAKECMDGOALS) sub-make

$(filter-out _all sub-make $(CURDIR)/Makefile, $(MAKECMDGOALS)) _all: sub-make
	$(Q)@:

sub-make: FORCE
	$(if $(KBUILD_VERBOSE:1=),@)$(MAKE) -C $(KBUILD_OUTPUT) \
	KBUILD_SRC=$(CURDIR) \
	-f $(CURDIR)/Makefile \
	$(filter-out _all sub-make,$(MAKECMDGOALS))

# Leave processing to above invocation of make
skip-makefile := 1
endif # ifneq ($(KBUILD_OUTPUT),)
endif # ifeq ($(KBUILD_SRC),)

# We process the rest of the Makefile if this is the final invocation of make
ifeq ($(skip-makefile),)

# If building an external module we do not care about the all: rule
# but instead _all depend on modules
PHONY += all
_all: all

srctree		:= $(if $(KBUILD_SRC),$(KBUILD_SRC),$(CURDIR))
objtree		:= $(CURDIR)
src		:= $(srctree)
obj		:= $(objtree)

VPATH		:= $(srctree)

export srctree objtree VPATH

CROSS_COMPILE	?= $(CONFIG_CROSS_COMPILE:"%"=%)

KCONFIG_CONFIG	?= .config
export KCONFIG_CONFIG

# SHELL used by kbuild
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

HOSTCC       = gcc
HOSTCXX      = g++
HOSTCFLAGS   = -Wall -Wmissing-prototypes -Wstrict-prototypes -O2 -fomit-frame-pointer
HOSTCXXFLAGS = -O2

GRUB_MKRESCUE = i686-elf-grub-mkrescue

# Beautify output
# ---------------------------------------------------------------------------
#
# Normally, we echo the whole command before executing it. By making
# that echo $($(quiet)$(cmd)), we now have the possibility to set
# $(quiet) to choose other forms of output instead, e.g.
#
#         quiet_cmd_cc_o_c = Compiling $(RELDIR)/$@
#         cmd_cc_o_c       = $(CC) $(c_flags) -c -o $@ $<
#
# If $(quiet) is empty, the whole command will be printed.
# If it is set to "quiet_", only the short version will be printed. 
# If it is set to "silent_", nothing will be printed at all, since
# the variable $(silent_cmd_cc_o_c) doesn't exist.
#
# A simple variant is to prefix commands with $(Q) - that's useful
# for commands that shall be hidden in non-verbose mode.
#
#	$(Q)ln $@ :<
#
# If KBUILD_VERBOSE equals 0 then the above command will be hidden.
# If KBUILD_VERBOSE equals 1 then the above command is displayed.

ifeq ($(KBUILD_VERBOSE),1)
  quiet =
  Q =
else
  quiet=quiet_
  Q = @
endif

# If the user is running make -s (silent mode), suppress echoing of
# commands

ifneq ($(findstring s,$(MAKEFLAGS)),)
  quiet=silent_
endif

export quiet Q KBUILD_VERBOSE


# Look for make include files relative to root of kernel src
MAKEFLAGS += --include-dir=$(srctree)

# We need some generic definitions (do not try to remake the file).
$(srctree)/scripts/Kbuild.include: ;
include $(srctree)/scripts/Kbuild.include

# Make variables (CC, etc...)

AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
YASM    = yasm
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
AWK			= awk
XZ      	= xz
RUBY      	= ruby
INSTALLHVISOR  := installkernel
PERL		= perl
QEMU32	= qemu-system-i386
QEMU64	= qemu-system-x86_64
BOCHS   = bochs
CHECK_MBOOT = grub-file --is-x86-multiboot
CPREPROC = $(CROSS_COMPILE)cpp

MAPFILE = HypoV.map

CHECKFLAGS     := -D__HypoV__ -Dhypov -D__STDC__ -Dunix -D__unix__ \
		  -Wbitwise -Wno-return-void $(CF)
CFLAGS_HVISOR	=
AFLAGS_HVISOR	=


# Use LINUXINCLUDE when you must reference the include/ directory.
# Needed to be compatible with the O= option
LINUXINCLUDE    := -Iinclude \
                   $(if $(KBUILD_SRC), -I$(srctree)/include) \
                   -include include/generated/autoconf.h

KBUILD_CPPFLAGS := -D__HVISOR__

KBUILD_CFLAGS   := -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs \
		   -fno-strict-aliasing -fno-common \
		   -Werror-implicit-function-declaration \
		   -Wno-format-security \
		   -fno-delete-null-pointer-checks
KBUILD_AFLAGS_HVISOR :=
KBUILD_CFLAGS_HVISOR :=
KBUILD_AFLAGS   := -D__ASSEMBLY__

# Read HVISORRELEASE from include/config/kernel.release (if it exists)
HVISORRELEASE = $(shell cat include/config/kernel.release 2> /dev/null)
HVISORVERSION = $(VERSION)$(if $(PATCHLEVEL),.$(PATCHLEVEL)$(if $(SUBLEVEL),.$(SUBLEVEL)))$(EXTRAVERSION)

export ARCH SRCARCH CONFIG_SHELL HOSTCC HOSTCFLAGS CROSS_COMPILE AS LD CC CPREPROC
export CPP AR NM STRIP OBJCOPY OBJDUMP YASM XZ RUBY
export MAKE AWK GENKSYMS INSTALLHVISOR PERL UTS_MACHINE
export HOSTCXX HOSTCXXFLAGS SHARED_FLAGS

export KBUILD_CPPFLAGS NOSTDINC_FLAGS LINUXINCLUDE OBJCOPYFLAGS LDFLAGS
export KBUILD_CFLAGS CFLAGS_HVISOR
export KBUILD_AFLAGS AFLAGS_HVISOR
export KBUILD_AFLAGS_HVISOR KBUILD_CFLAGS_HVISOR
export KBUILD_ARFLAGS

# Files to ignore in find ... statements

RCS_FIND_IGNORE := \( -name SCCS -o -name BitKeeper -o -name .svn -o -name CVS -o -name .pc -o -name .hg -o -name .git \) -prune -o

# ===========================================================================
# Rules shared between *config targets and build targets

# Basic helpers built in scripts/
PHONY += scripts_basic install_stage0 fat32_read
scripts_basic:
	$(Q)$(MAKE) $(build)=scripts/basic

# To avoid any implicit rule to kick in, define an empty command.
scripts/basic/%: scripts_basic ;

install_stage0:
	$(Q)$(MAKE) $(build)=scripts/install_stage0

fat32_read:
	$(Q)$(MAKE) $(build)=scripts/fat32_read

scripts/install_stage0/%: scripts_basic ;

PHONY += outputmakefile
# outputmakefile generates a Makefile in the output directory, if using a
# separate output directory. This allows convenient use of make in the
# output directory.
outputmakefile:
ifneq ($(KBUILD_SRC),)
	$(Q)ln -fsn $(srctree) source
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/mkmakefile \
	    $(srctree) $(objtree) $(VERSION) $(PATCHLEVEL)
endif


# To make sure we do not include .config for any of the *config targets
# catch them early, and hand them over to scripts/kconfig/Makefile
# It is allowed to specify more targets when calling make, including
# mixing *config targets and build targets.
# For example 'make oldconfig all'.
# Detect when mixed targets is specified, and make a second invocation
# of make so .config is not included in this case either (for *config).

no-dot-config-targets := clean mrproper distclean \
			 cscope gtags TAGS tags help %docs check% coccicheck \
			 include/linux/version.h headers_% \
			 kernelversion %src-pkg

config-targets := 0
mixed-targets  := 0
dot-config     := 1

ifneq ($(filter $(no-dot-config-targets), $(MAKECMDGOALS)),)
	ifeq ($(filter-out $(no-dot-config-targets), $(MAKECMDGOALS)),)
		dot-config := 0
	endif
endif

ifneq ($(filter config %config,$(MAKECMDGOALS)),)
        config-targets := 1
        ifneq ($(filter-out config %config,$(MAKECMDGOALS)),)
                mixed-targets := 1
        endif
endif

ifeq ($(mixed-targets),1)
# ===========================================================================
# We're called with mixed targets (*config and build targets).
# Handle them one by one.

%:: FORCE
	$(Q)$(MAKE) -C $(srctree) KBUILD_SRC= $@

else
ifeq ($(config-targets),1)
# ===========================================================================
# *config targets only - make sure prerequisites are updated, and descend
# in scripts/kconfig to make the *config target

# Read arch specific Makefile to set KBUILD_DEFCONFIG as needed.
# KBUILD_DEFCONFIG may point out an alternative default configuration
# used for 'make defconfig'
#include $(srctree)/arch/$(SRCARCH)/Makefile
export KBUILD_DEFCONFIG KBUILD_KCONFIG

config: scripts_basic outputmakefile FORCE
	$(Q)mkdir -p include/config # include/linux 
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

%config: scripts_basic outputmakefile FORCE
	$(Q)mkdir -p include/config # include/linux 
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

else


ifeq ($(dot-config),1)
# Read in config
-include include/config/auto.conf


# Read in dependencies to all Kconfig* files, make sure to run
# oldconfig if changes are detected.
-include include/config/auto.conf.cmd

# To avoid any implicit rule to kick in, define an empty command
$(KCONFIG_CONFIG) include/config/auto.conf.cmd: ;

# If .config is newer than include/config/auto.conf, someone tinkered
# with it and forgot to run make oldconfig.
# if auto.conf.cmd is missing then we are probably in a cleaned tree so
# we execute the config step to be sure to catch updated Kconfig files
include/config/%.conf: $(KCONFIG_CONFIG) include/config/auto.conf.cmd
	$(Q)$(MAKE) -f $(srctree)/Makefile silentoldconfig
else
# Dummy target needed, because used as prerequisite
include/config/auto.conf: ;
endif # $(dot-config)

stage0/stage0: stage0/stage0.asm
	$(Q)$(MAKE) -C stage0

loader: stage0/stage0

# The all: target is the default when no target is given on the
# command line.

all: sys/core/boot_stub.h hypov loader

sys/core/boot_stub.h: sys/core/boot_stub.asm
	$(Q)$(YASM) -f bin -o sys/core/boot_stub.bin $<
	$(Q)xxd -i sys/core/boot_stub.bin | sed 's/sys_core_boot_stub_bin/boot_stub_bytes/g' > $@
	@echo "  GEN     $@"
# 32bit parts
objs-y		:= sys boot 
# Separate compilations of the libraries for 32 and 64 bit code
libs32-y    := lib/lib32
libs64-y    := lib/lib64
libs-y		:= $(libs32-y) $(libs64-y)
# 64bit part (core hypervisor functionality)
hvobjs-y    := sys/core

hypov-dirs	:= $(objs-y) $(libs-y) $(hvobjs-y)
hypov-objs	:= $(patsubst %,%/built-in.o, $(objs-y))
hvcore-objs	:= $(patsubst %,%/built-in.o, $(hvobjs-y))
hypov-libs	:= $(patsubst %,%/lib.a, $(libs-y))
hypov-all	:= $(hypov-objs) $(hypov-libs) $(hvcore-objs)

HVCORE_DIR    := sys/core
HVCORE_TARGET := hvcore.elf64
HVCORE_LIB64  := $(libs64-y)/lib.a
HVCORE_ELF64  := $(HVCORE_DIR)/$(HVCORE_TARGET)	   # The main 64 bit hypervisor ELF
HVCORE_XZ     := $(HVCORE_DIR)/$(HVCORE_TARGET).xz # compressed image of the object
HVCORE_OBJ	  := $(HVCORE_DIR)/$(HVCORE_TARGET).o  # 32bit ELF binary container object

# Link the main ELF32 with the loader, debug console, and ELF64 container
quiet_cmd_hypov = LD      $@
      cmd_hypov = $(CC) $(LDFLAGS) -o $(BINARY_TARGET) \
      -Wl,--start-group $(libs32-y)/lib.a $(hypov-objs) $(HVCORE_OBJ) -Wl,--end-group \
	  -Wl,-T boot/linker.lds $(SHARED_FLAGS) -Wl,-Map $(MAPFILE) -ggdb

# Link the core ELF64
CC64 = x86_64-elf-gcc
quiet_cmd_hvcore = LD      $@
      cmd_hvcore = $(CC64) -Wl,-ehv_entry_64 -nostdlib -o $(HVCORE_DIR)/$(HVCORE_TARGET) \
	  -Wl,-T $(HVCORE_DIR)/hvcore.lds -Wl,--start-group $(HVCORE_LIB64) $(hvcore-objs) -Wl,--end-group -ggdb

# Compress the target ELF64 with XZ
quiet_cmd_hvcore_xz = XZ      $@
      cmd_hvcore_xz = $(XZ) --check=crc32 --x86 --lzma2=dict=32MiB -kf $(HVCORE_ELF64)

# Embed the compressed ELF64 in a regular ELF32 object file
quiet_cmd_hvobj  = OBJCOPY $@
  	  cmd_hvobj  = $(OBJCOPY) -I binary -O elf32-i386 \
	  --rename-section .data=.hvcore_payload,alloc,load,readonly,data,contents \
	  --redefine-sym _binary_sys_core_hvcore_elf64_xz_end=__hvcore_end \
	  --redefine-sym _binary_sys_core_hvcore_elf64_xz_start=__hvcore_start \
	  --redefine-sym _binary_sys_core_hvcore_elf64_xz_size=__hvcore_size \
	  --binary-architecture i386 $(HVCORE_XZ) $(HVCORE_OBJ)

# Create hvcore.elf64
$(HVCORE_ELF64): $(HVCORE_LIB64) $(hvcore-objs)
	$(call if_changed,hvcore)

# Compress into hvcore.elf64.xz
$(HVCORE_XZ): $(HVCORE_ELF64)
	$(call if_changed,hvcore_xz)

# Bundle the image into hvcore.elf64.o
$(HVCORE_OBJ): $(HVCORE_XZ)
	$(call if_changed,hvobj)

hypov: $(hypov-all) $(HVCORE_OBJ)
	$(call if_changed,hypov)
	@echo -n "  LOADER  "
	$(Q)$(CHECK_MBOOT) $@.bin && echo "OK" || echo "FAIL"

qemu: hypov
	$(Q)$(QEMU64) -serial stdio -kernel $(BINARY_TARGET)

qemufs: hypov
	$(Q)$(QEMU64) -serial stdio -hda testfs.img

qemuiso: hypov.iso
	$(Q)$(QEMU64) -serial stdio -cdrom $^

# TCG with Intel VMX emulation
qemutcg: hypov.iso
	$(Q)$(QEMU64) -cpu qemu64,+vmx -m 512 -serial stdio -cdrom $^

qemutcgdbg: hypov.iso
	$(Q)$(QEMU64) -cpu qemu64,+vmx -m 512 -serial stdio -cdrom $^ -S -s

# TCG with AMD SVM emulation (works on macOS without KVM)
qemusvm: hypov.iso
	$(Q)$(QEMU64) -cpu EPYC -m 512 -serial stdio -cdrom $^

qemusvmdbg: hypov.iso
	$(Q)$(QEMU64) -cpu EPYC -m 512 -serial stdio -cdrom $^ -S -s

# Guest proof: hyp_check compiled as a Linux init, runs inside the SVM guest.
# GRUB shows a 3s menu — host auto-boots HypoV; in the guest, press 1.
# Works with: make qemuproof (QEMU/SVM) or make bochsproof (Bochs/VMX)
#
PROOF_KERNEL_URL := http://tinycorelinux.net/15.x/x86_64/release/distribution_files/vmlinuz64

tools/guest/hyp_check: tools/guest/hyp_check.c
	$(Q)gcc -static -o $@ $< 2>/dev/null && echo "  CC (native)  $@" || \
	    x86_64-linux-musl-gcc -static -o $@ $< 2>/dev/null && echo "  CC (musl)    $@" || \
	    docker run --rm --platform linux/amd64 -v "$(CURDIR)/tools/guest:/work" alpine:latest \
	        sh -c "apk add -q gcc musl-dev && gcc -static -o /work/hyp_check /work/hyp_check.c" \
	        && echo "  CC (docker)  $@" || \
	    { echo "ERROR: need gcc, x86_64-linux-musl-gcc, or Docker"; exit 1; }

proof-initrd.img: tools/guest/hyp_check
	$(Q)rm -rf /tmp/hypv_root && mkdir -p /tmp/hypv_root/dev /tmp/hypv_root/proc /tmp/hypv_root/sys
	$(Q)cp tools/guest/hyp_check /tmp/hypv_root/hyp_check
	$(Q)printf '#!/bin/sh\nmount -t devtmpfs devtmpfs /dev 2>/dev/null || true\n/hyp_check\nwhile true; do sleep 60; done\n' \
	    > /tmp/hypv_root/init && chmod +x /tmp/hypv_root/init
	$(Q)cd /tmp/hypv_root && find . | cpio -o -H newc 2>/dev/null | gzip > $(CURDIR)/$@
	@echo "  INITRD  $@"

vmlinuz64:
	wget -O $@ $(PROOF_KERNEL_URL)

hypov-proof.iso: hypov proof-initrd.img vmlinuz64
	$(Q)mkdir -p /tmp/hproof/boot/grub
	$(Q)cp $(BINARY_TARGET) /tmp/hproof/boot/hypov.bin
	$(Q)cp vmlinuz64 /tmp/hproof/boot/
	$(Q)cp proof-initrd.img /tmp/hproof/boot/
	$(Q)printf 'set timeout=3\nset default=0\n\nmenuentry "HypoV" {\n    multiboot /boot/hypov.bin\n    boot\n}\n\nmenuentry "hyp_check (run inside HypoV guest)" {\n    linux /boot/vmlinuz64 quiet console=ttyS0\n    initrd /boot/proof-initrd.img\n    boot\n}\n' \
	    > /tmp/hproof/boot/grub/grub.cfg
	$(Q)$(GRUB_MKRESCUE) -o $@ /tmp/hproof 2>/dev/null
	@echo "  ISO     $@"
	@echo "  Usage:  make qemuproof"
	@echo "  Host:   GRUB auto-boots HypoV after 3s"
	@echo "  Guest:  press 1 to run hyp_check inside the hypervisor"

qemuproof: hypov hypov-proof.iso
	@echo "When GRUB appears the SECOND time (inside the guest), press 1."
	$(Q)$(QEMU64) -cpu EPYC -m 512 \
	    -cdrom hypov-proof.iso \
	    -serial stdio

# CirrOS: tiny cloud-testing image with known default credentials.
# Login: cirros / gocubsgo  (no setup, no cloud-init, works immediately)
CIRROS_IMG ?= cirros.img
CIRROS_URL := https://download.cirros-cloud.net/0.6.2/cirros-0.6.2-x86_64-disk.img

download-cirros:
	wget -O $(CIRROS_IMG) $(CIRROS_URL)

qemucirros: hypov $(CIRROS_IMG)
	$(Q)$(QEMU64) -cpu EPYC -m 512 \
	    -drive file=$(CIRROS_IMG),if=ide,index=0 \
	    -drive file=hypov.iso,media=cdrom,if=ide,index=1 \
	    -serial stdio

# Alpine 3.19 guest (~100MB) — login: vagrant / vagrant
# Run 'make setup-guest' once to install gcc (persisted to disk).
GUEST_BOX_URL := https://vagrantcloud.com/generic/boxes/alpine319/versions/4.3.12/providers/libvirt/amd64/download/vagrant.box
GUEST_IMG     ?= alpine-guest.qcow2

$(GUEST_IMG):
	@echo "Downloading Alpine 3.19 guest (~100MB)..."
	wget -O /tmp/guest.box $(GUEST_BOX_URL)
	tar -xf /tmp/guest.box box.img
	qemu-img convert -O qcow2 box.img $@
	rm /tmp/guest.box box.img
	@echo "  IMG     $@ (login: vagrant / vagrant)"

SSH_OPTS := -p 2222 -o StrictHostKeyChecking=no -o ConnectTimeout=2

# One-time setup: install gcc and copy hyp_check.c, then open SSH shell
setup-guest:
	@test -f /tmp/vagrant_key || \
	    curl -sL https://raw.githubusercontent.com/hashicorp/vagrant/main/keys/vagrant \
	    -o /tmp/vagrant_key && chmod 600 /tmp/vagrant_key
	@echo "Waiting for SSH..."
	@until ssh $(SSH_OPTS) -i /tmp/vagrant_key vagrant@localhost true 2>/dev/null; do sleep 2; done
	@echo "Fixing DNS (QEMU internal DNS unreachable through HypoV; using 8.8.8.8)..."
	ssh $(SSH_OPTS) -i /tmp/vagrant_key vagrant@localhost \
	    "echo 'nameserver 8.8.8.8' | sudo tee /etc/resolv.conf > /dev/null"
	@echo "Installing gcc (one-time, persisted to disk)..."
	ssh $(SSH_OPTS) -i /tmp/vagrant_key vagrant@localhost "sudo apk add gcc musl-dev"
	scp $(SSH_OPTS) -i /tmp/vagrant_key tools/guest/hyp_check.c vagrant@localhost:~
	@echo "Done. Connecting..."
	ssh $(SSH_OPTS) -i /tmp/vagrant_key vagrant@localhost

qemuguest: hypov $(GUEST_IMG)
	@if [ "$$(uname)" = "Darwin" ]; then \
	    echo "ERROR: qemuguest requires Linux with KVM (real SVM/VMX hardware)."; \
	    echo "       Use 'make qemusvm' on macOS to test basic SVM interception."; \
	    exit 1; \
	fi
	@pkill -f "$(GUEST_IMG)" 2>/dev/null || true
	@echo "┌─────────────────────────────────────────┐"
	@echo "│  HypoV guest: Alpine Linux              │"
	@echo "│  Login:  vagrant  │  Password: vagrant  │"
	@echo "│  SSH:    ssh -p 2222 vagrant@localhost  │"
	@echo "└─────────────────────────────────────────┘"
	$(Q)$(QEMU64) -enable-kvm -cpu host -m 512 \
	    -boot order=dc \
	    -drive file=$(GUEST_IMG),if=ide,index=0 \
	    -drive file=hypov.iso,media=cdrom,if=ide,index=1 \
	    -nic user,model=e1000,hostfwd=tcp::2222-:22 \
	    -serial stdio

qemudbg: hypov.iso
	$(Q)$(QEMU64) -serial stdio -cdrom $^ -S -s

bochs: hypov
	$(Q)$(BOCHS)

# Proof via Bochs (Intel VMX path — works on macOS without KVM).
# When GRUB appears the second time (inside the guest), press 1.
# Serial output in debug_serial.txt
bochsproof: hypov-proof.iso
	@sed 's|hypov\.iso|hypov-proof.iso|g' bochsrc > /tmp/bochsrc-proof
	$(Q)$(BOCHS) -q -f /tmp/bochsrc-proof

# The resulting ISO image could be both used for CD and pendrive installs
# with ISO to pendrive image converters
hypov.iso: hypov
	$(Q)cp $(BINARY_TARGET) etc/grub/boot/
	$(Q)$(GRUB_MKRESCUE) -o $@ etc/grub/

$(TESTFS): hypov
	./scripts/mktestfs.sh

iso: hypov.iso
mkfs: $(TESTFS)
upfs:
	./scripts/updatefs.sh

# The actual objects are generated when descending, 
# make sure no implicit rule kicks in
$(sort $(hypov-all)): $(hypov-dirs) ;

# Handle descending into subdirectories listed in $(vmlinux-dirs)
# Preset locale variables to speed up the build process. Limit locale
# tweaks to this spot to avoid wrong language settings when running
# make menuconfig etc.
# Error messages still appears in the original language

#PHONY += $(vmlinux-dirs)
#$(vmlinux-dirs): prepare scripts
PHONY += $(hypov-dirs) iso bochs bochsproof qemu qemufs qemutcg qemutcgdbg qemusvm qemusvmdbg qemuproof qemucirros download-cirros qemuguest setup-guest mkfs upfs
$(hypov-dirs): scripts_basic
	$(Q)$(MAKE) $(build)=$@

###
# Cleaning is done on three levels.
# make clean     Delete most generated files
#                Leave enough to build external modules
# make mrproper  Delete the current configuration, and all generated files
# make distclean Remove editor backup files, patch leftover files and the like

# Directories & files removed with 'make clean'
CLEAN_DIRS  += 
CLEAN_FILES +=	$(BINARY_TARGET) hypov.iso etc/grub/boot/$(BINARY_TARGET) $(HVCORE_XZ)

# Directories & files removed with 'make mrproper'
MRPROPER_DIRS  += include/config include/generated
MRPROPER_FILES += .config .config.old tags TAGS cscope* GPATH GTAGS GRTAGS GSYMS

# clean - Delete most, but leave enough to build external modules
#
clean: rm-dirs  := $(CLEAN_DIRS)
clean: rm-files := $(CLEAN_FILES)
clean-dirs      := $(addprefix _clean_, $(hypov-dirs))

PHONY += $(clean-dirs) clean archclean
$(clean-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _clean_%,%,$@)

clean: $(clean-dirs)
	$(call cmd,rmdirs)
	$(call cmd,rmfiles)
	@find . $(RCS_FIND_IGNORE) \
		\( -name '*.[oas]' -o -name '.*.cmd' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \
		-o -name modules.builtin -o -name '.tmp_*.o.*' \
		-o -name '*.gcno' \) -type f -print | xargs rm -f

# mrproper - Delete all generated files, including .config
#
mrproper: rm-dirs  := $(wildcard $(MRPROPER_DIRS))
mrproper: rm-files := $(wildcard $(MRPROPER_FILES))
mrproper-dirs      := $(addprefix _mrproper_, scripts)

PHONY += $(mrproper-dirs) mrproper
$(mrproper-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _mrproper_%,%,$@)

mrproper: clean $(mrproper-dirs)
	$(call cmd,rmdirs)
	$(call cmd,rmfiles)

# distclean
#
PHONY += distclean
distclean: mrproper
	@find $(srctree) $(RCS_FIND_IGNORE) \
		\( -name '*.orig' -o -name '*.rej' -o -name '*~' \
		-o -name '*.bak' -o -name '#*#' -o -name '.*.orig' \
		-o -name '.*.rej' -o -size 0 \
		-o -name '*%' -o -name '.*.cmd' -o -name 'core' \) \
		-type f -print | xargs rm -f


# FIXME Should go into a make.lib or something
# ===========================================================================

quiet_cmd_rmdirs = $(if $(wildcard $(rm-dirs)),CLEAN   $(wildcard $(rm-dirs)))
      cmd_rmdirs = rm -rf $(rm-dirs)

quiet_cmd_rmfiles = $(if $(wildcard $(rm-files)),CLEAN   $(wildcard $(rm-files)))
      cmd_rmfiles = rm -f $(rm-files)

# Shorthand for $(Q)$(MAKE) -f scripts/Makefile.clean obj=dir
# Usage:
# $(Q)$(MAKE) $(clean)=dir
clean := -f $(if $(KBUILD_SRC),$(srctree)/)scripts/Makefile.clean obj

help:
	@echo  'Cleaning targets:'
	@echo  '  clean		  - Remove most generated files but keep the config and'
	@echo  '                    enough build support to build external modules'
	@echo  '  mrproper	  - Remove all generated files + config + various backup files'
	@echo  '  distclean	  - mrproper + remove editor backup and patch files'
	@echo  ''
	@echo  'Configuration targets:'
	@$(MAKE) -f $(srctree)/scripts/kconfig/Makefile help
	@echo  ''
	@echo  'Other generic targets:'
	@echo  '  all		  - Build all targets marked with [*]'
	@echo  '* hypov	  	  - Build the hypervisor'
	@echo  '  iso  	  	  - Create a bootable ISO image for CDs and pendrives'
	@echo  '  mkfs 	  	  - Create a FAT32 test filesystem to use with SYSLINUX'
	@echo  '  upfs 	  	  - Update the FAT32 test filesystem if something is changed'
	@echo  '  dir/            - Build all files in dir and below'
	@echo  '  dir/file.[oisS] - Build specified target only'
	@echo  '  dir/file.lst    - Build specified mixed source/assembly target only'
	@echo  '                    (requires a recent binutils and recent build (System.map))'
	@echo  '  tags/TAGS	  - Generate tags file for editors'
	@echo  '  cscope	  - Generate cscope index'
	@echo  '  gtags           - Generate GNU GLOBAL index'
	@echo  '  kernelrelease	  - Output the release version string'
	@echo  '  kernelversion	  - Output the version stored in Makefile'
	 echo  ''
	@echo  'Static analysers'
	@echo  '  checkstack      - Generate a list of stack hogs'
	@echo  '  namespacecheck  - Name space analysis on compiled kernel'
	@echo  '  versioncheck    - Sanity check on version.h usage'
	@echo  '  includecheck    - Check for duplicate included header files'
	@echo  '  export_report   - List the usages of all exported symbols'
	@echo  '  headers_check   - Sanity check on exported headers'
#	@$(MAKE) -f $(srctree)/scripts/Makefile.help checker-help
	@echo  ''
#	@echo  'Kernel packaging:'
#	@$(MAKE) $(build)=$(package-dir) help
	@echo  ''
#	@echo  'Documentation targets:'
#	@$(MAKE) -f $(srctree)/Documentation/DocBook/Makefile dochelp
	@echo  ''
	@echo  '  make V=0|1 [targets] 0 => quiet build (default), 1 => verbose build'
	@echo  '  make V=2   [targets] 2 => give reason for rebuild of target'
	@echo  '  make O=dir [targets] Locate all output files in "dir", including .config'
	@echo  '  make W=n   [targets] Enable extra gcc checks, n=1,2,3 where'
	@echo  '		1: warnings which may be relevant and do not occur too often'
	@echo  '		2: warnings which occur quite often but may still be relevant'
	@echo  '		3: more obscure warnings, can most likely be ignored'
	@echo  '		Multiple levels can be combined with W=12 or W=123'
	@echo  '  make RECORDMCOUNT_WARN=1 [targets] Warn about ignored mcount sections'
	@echo  ''
	@echo  'Execute "make" or "make all" to build all targets marked with [*] '
	@echo  'For further info see the ./README file'


endif #ifeq ($(config-targets),1)
endif #ifeq ($(mixed-targets),1)

endif	# skip-makefile

PHONY += FORCE
FORCE:

# Declare the contents of the .PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)
