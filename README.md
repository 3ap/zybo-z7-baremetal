# Digilent Zybo Z7-10 (bare metal)

This repository is aimed to simplify the interaction with Digilent
Zybo Z7-10 by using standard Linux open source utilities (such as
gcc, gdb, openocd) instead of proprietary Xilinx SDK. It is something
like [libopencm3][3] but for Xilinx Zynq 7010.

## Quick Start

  1. Check if you have these apps installed:

     - `openocd`
     - `arm-none-eabi-gdb`
     - `arm-none-eabi-gcc`
     - `arm-none-eabi-newlib`

  2. Check whether the jumper JP5 set in JTAG mode on board;

  3. Connect your Zybo Z7 via USB (PROG/UART) to PC and turn the
     switch "ON/OFF" on;

  4. Open terminal and check for `/dev/ttyUSB0` and `/dev/ttyUSB1`:

```bash
$ ls -al /dev/ttyUSB*
crw-rw---- 1 root uucp 188, 0 Oct 10 15:50 /dev/ttyUSB0
crw-rw---- 1 root uucp 188, 1 Oct 10 15:50 /dev/ttyUSB1
```

  5. Add your user to the group that owns ttyUSB1 (usually `dialout` or
  `uucp`) for getting access to UART without root permissions:

```bash
sudo usermod -aG uucp "${LOGNAME}"
newgrp uucp
```

  6. Create file /etc/udev/rules.d/49-digilent.rules (change the
     group from p.5) for getting access to JTAG without root
     permissions:

```
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6010", GROUP="uucp", MODE:="0660"
```

  7. Turn off and on board;

  8. Now you can run blink example for testing purpose:

```bash
cd examples/blink
make
make openocd &
make run
```

## Credits

  - `bsp/boot.S` & `bsp/Zynq.ld` from [bigbrett/zybo-baremetal][1]
  - `bsp/ps7_init_gpl.{c,h}` & `bsp/ps7_spl_init.c` from [Das U-boot][2]

[1]: https://github.com/bigbrett/zybo-baremetal
[2]: http://git.denx.de/?p=u-boot.git;a=summary
[3]: https://github.com/libopencm3/libopencm3
