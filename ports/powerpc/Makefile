include ../../py/mkenv.mk

CROSS = 0

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include $(TOP)/py/py.mk

ARCH = $(shell uname -m)
ifneq ("$(ARCH)", "ppc64")
ifneq ("$(ARCH)", "ppc64le")
	CROSS_COMPILE = powerpc64le-linux-
endif
endif

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -Iskiboot

CFLAGS = $(INC) -g -Wall -std=c99 $(COPT)
CFLAGS += -msoft-float -mno-string -mno-multiple -mno-vsx -mno-altivec -mlittle-endian -fno-stack-protector -mstrict-align -ffreestanding
CFLAGS += -Os
CFLAGS += -fdata-sections -ffunction-sections

LDFLAGS = -N -T powerpc.lds

LIBS =

SRC_C = \
	main.c \
	uart_core.c \
	lib/utils/printf.c \
	lib/utils/stdout_helpers.c \
	lib/utils/pyexec.c \
	lib/libc/string0.c \
	lib/mp-readline/readline.c \
	$(BUILD)/_frozen_mpy.c \

CLEAN_EXTRA = head.o

OBJ = $(PY_CORE_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

all: $(BUILD)/firmware.elf $(BUILD)/firmware.map $(BUILD)/firmware.bin

$(BUILD)/_frozen_mpy.c: frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode"
	$(Q)$(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=mpz $< > $@

head.o : head.S
	$(Q)$(CC) head.S -c -o head.o

$(BUILD)/firmware.elf: $(OBJ) head.o powerpc.lds
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(Q)$(OBJCOPY) -O binary $^ $(BUILD)/firmware.bin

$(BUILD)/firmware.map: $(BUILD)/firmware.elf
	$(Q)nm $^ | sort > $(BUILD)/firmware.map

include $(TOP)/py/mkrules.mk
