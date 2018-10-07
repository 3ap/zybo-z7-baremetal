CROSS_COMPILE ?= arm-none-eabi-

AS      = $(CROSS_COMPILE)as
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
SZ      = $(CROSS_COMPILE)size
GDB     = $(CROSS_COMPILE)gdb
OOCD    = openocd

CFLAGS  = -I$(BSPDIR) -mcpu=cortex-a9 -O0 -g3 -Wall -Werror -Wpedantic
LDFLAGS = -nostdlib -T$(BSPDIR)/Zynq.ld
ASFLAGS = -g -mcpu=cortex-a9

BSP     = $(BSPDIR)/boot.S \
          $(BSPDIR)/ps7_init_gpl.c \
          $(BSPDIR)/ps7_spl_init.c

OBJECTS = $(patsubst %.S,%.o,$(patsubst %.c,%.o,$(SOURCES) $(BSP)))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^
	@$(SZ) $(TARGET)

%.o: %.S
	$(AS) $(ASFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJECTS)

openocd:
	$(OOCD) -f interface/ftdi/digilent-hs1.cfg \
	        -f target/zynq_7000.cfg

debug: $(TARGET)
	$(GDB)  -iex "target remote localhost:3333" \
	        -iex "monitor halt" \
		-ex "load" \
		-ex "continue" "$(TARGET)"

run: $(TARGET)
	$(GDB)  -iex "target remote localhost:3333" \
	        -iex "monitor halt" \
		-ex "load" \
		-ex "continue" \
		-ex "set confirm no" \
		-ex "quit" "$(TARGET)"

.PHONY: clean debug openocd
