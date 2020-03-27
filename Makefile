SHELL:=/bin/bash
CC:=gcc
LD:=$(CC)
AR:=ar
LIBS:=
FLAGS_release:=-O2 -flto
FLAGS_debug:=-Og -ggdb3 -DLITHIUM_TEST_BUILD
COMMONFLAGS:=-Werror -Wall
CFLAGS:=-std=gnu17 $(COMMONFLAGS) -Iinclude -fPIC
LDFLAGS:=$(CFLAGS)
OUTDIR:=build

# Recursive wildcard function, stackoverflow.com/questions/2483182
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Collect the source files
HEADERS:=$(call rwildcard,include,*.h)
CSRCS:=$(call rwildcard,src,*.c)
MAINCSRCS:=$(call rwildcard,mains,*.c)
OBJFILES_SRC:=$(patsubst %.c,%.o,$(CSRCS))
OBJFILES_SRC_debug:=$(foreach o,$(OBJFILES_SRC),$(OUTDIR)/debug/$(o))
OBJFILES_SRC_release:=$(foreach o,$(OBJFILES_SRC),$(OUTDIR)/release/$(o))
OBJFILES_MAINS:=$(patsubst %.c,%.o,$(MAINCSRCS))
OBJFILES_MAINS_debug:=$(foreach o,$(OBJFILES_MAINS),$(OUTDIR)/debug/$(o))
OBJFILES_MAINS_release:=$(foreach o,$(OBJFILES_MAINS),$(OUTDIR)/release/$(o))
BINS:=$(patsubst mains/%.c,bin/%,$(MAINCSRCS))
BINS_debug:=$(foreach f,$(BINS),$(OUTDIR)/debug/$(f))
BINS_release:=$(foreach f,$(BINS),$(OUTDIR)/release/$(f))
OUTPUTS_debug:=$(OBJFILES_SRC_debug) $(OBJFILES_MAINS_debug) $(BINS_debug)
OUTPUTS_release:=$(OBJFILES_SRC_release) $(OBJFILES_MAINS_release) \
	$(BINS_release)
OUTPUTS:=$(OUTPUTS_debug) $(OUTPUTS_release)
DIRS:=$(sort $(dir $(OUTPUTS)))

_create_dirs := $(foreach d,$(DIRS),$(shell [[ -d $(d) ]] || mkdir -p $(d)))

binpath = $(filter %/$(1),$(BINS))

ifeq ($(V),)
cmd = @printf '  %-8s %s\n' $(cmd_$(1)_name) $(if $(2),$(2),"$@") ; $(call cmd_$(1),$(2))
else
ifeq ($(V),1)
cmd = $(call cmd_$(1),$(2))
else
cmd = @$(call cmd_$(1),$(2))
endif
endif

get_target = $(word 2,$(subst /, ,$@))
target_flags = $(FLAGS_$(get_target))

DEPFLAGS = -MMD -MP -MF $@.d

cmd_c_to_o_name = CC
cmd_c_to_o = $(CC) $(CFLAGS) $(target_flags) $(DEPFLAGS) -c $< -o $@

cmd_o_to_elf_name = LD
cmd_o_to_elf = $(LD) $(LDFLAGS) $(target_flags) $^ $(LIBS) -o $@

cmd_o_to_ar_name = AR
cmd_o_to_ar = $(AR) rc $@ $^

cmd_o_to_so_name = LIB
cmd_o_to_so = $(LD) -shared -o $@ $^

cmd_clean_name = CLEAN
cmd_clean = rm -rf $(1)

cmd_gdb_name = GDB
cmd_gdb = $(MAKE) $(1) && gdb $(1)

cmd_run_name = RUN
cmd_run = $(MAKE) $(1) && ./$(1)

.SECONDARY:
.PHONY: all
all: $(BINS_debug) $(BINS_release) build/release/libithium.so build/release/libithium.a

-include $(call rwildcard,$(OUTDIR),*.d)

$(OUTDIR)/debug/bin/%: $(OUTDIR)/debug/mains/%.o $(OBJFILES_SRC_debug)
	$(call cmd,o_to_elf)
$(OUTDIR)/release/bin/%: $(OUTDIR)/release/mains/%.o $(OBJFILES_SRC_release)
	$(call cmd,o_to_elf)

$(OUTDIR)/debug/libithium.a: $(OBJFILES_SRC_debug)
	$(call cmd,o_to_ar)
$(OUTDIR)/release/libithium.a: $(OBJFILES_SRC_release)
	$(call cmd,o_to_ar)

$(OUTDIR)/debug/libithium.so: $(OBJFILES_SRC_debug)
	$(call cmd,o_to_so)
$(OUTDIR)/release/libithium.so: $(OBJFILES_SRC_release)
	$(call cmd,o_to_so)

$(OUTDIR)/debug/%.o: %.c
	$(call cmd,c_to_o)
$(OUTDIR)/release/%.o: %.c
	$(call cmd,c_to_o)

.PHONY: run-%
run-%:
	$(call cmd,run,$(OUTDIR)/release/$(call binpath,$(patsubst run-%,%,$@)))

.PHONY: debug-%
debug-%:
	$(call cmd,gdb,$(OUTDIR)/debug/$(call binpath,$(patsubst debug-%,%,$@)))

.PHONY: run-tests
run-tests:
	$(call cmd,run,$(OUTDIR)/debug/$(call binpath,run_tests))

.PHONY: clean
clean:
	$(call cmd,clean,$(OUTDIR))
