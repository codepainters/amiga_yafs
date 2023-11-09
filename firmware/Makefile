# vim: noexpandtab

CROSS_COMPILE = avr-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc
SIZE = $(CROSS_COMPILE)size
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

#
# Project files
#

# Note: 'at' prefix is added where needed
MCU_TYPE = tiny404
# MCU_TYPE = tiny416
OUTFILE = yafs

OBJS = $(addprefix objs/,main.o timer.o)
INCLUDE_PATHS = -Isrc 
vpath %.c	src
vpath %.h	src

#
# Compiler settings, parameters and flags
#
COMMON_FLAGS = -c -g3 -Os $(INCLUDE_PATHS) -std=gnu99 -Wall -Wno-unknown-pragmas \
	-ffunction-sections -fdata-sections -fmessage-length=0 \
	-mmcu=at$(MCU_TYPE) \
	-D__$(MCU_TYPE)__ -DDEBUG \
	-B ./dev/at$(MCU_TYPE) \
	-Iinclude

CFLAGS  = $(COMMON_FLAGS) -DTARGET=$(TARGET) -fno-builtin

LDFLAGS = -Wl,--nostdlib -mmcu=at$(MCU_TYPE) -Wl,--gc-sections -Wl,-Map=out/$(OUTFILE).map -g \
		-B "./dev/at$(MCU_TYPE)" -L"./dev/at$(MCU_TYPE)/avrxmega3/short-calls"  \
		-l:libat$(MCU_TYPE).a 

LDLIBS  = -lm
OCFLAGS = --strip-unneeded

all: out/$(OUTFILE).bin

$(OBJS): | dirs

dirs:
	mkdir -p out
	mkdir -p objs

objs/%.o : %.c 
	$(CC) $(CFLAGS) -o $@ $<

out/$(OUTFILE).elf: $(OBJS) $(SYS_OBJS)
	$(LD) $(LDFLAGS) -o out/$(OUTFILE).elf $(OBJS) $(LDLIBS)
	-@echo ""
	$(SIZE) out/$(OUTFILE).elf
	-@echo ""
	$(OBJCOPY) $(OCFLAGS) -O ihex out/$(OUTFILE).elf out/$(OUTFILE).hex
	
out/$(OUTFILE).bin: out/$(OUTFILE).elf
	$(OBJCOPY) $(OCFLAGS) -O binary out/$(OUTFILE).elf out/$(OUTFILE).bin

clean:
	rm -Rf objs
	rm -f out/$(OUTFILE).*

