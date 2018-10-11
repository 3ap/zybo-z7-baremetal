/*
 * Init.s
 *
 * Initial system initialization code (1st-stage boot-loader)
 */

    .syntax unified     /* Use the unified instruction syntax */
    .arm                /* Assemble ARM instruction set, not thumb */

/*
 *
 * References:
 * [1] Zynq-7000 Extensible Processing Platform - Technical Reference Manual UG585 (v1.0)
 *     May 8, 2012 http://www.xilinx.com
 *     Section 6.x "Boot and Configuration"
 *
 * [2] Zynq-7000 EPP Software Developers Guide - UG821 (v2.0) April 24, 2012
 *     http://www.xilinx.com
 *     Section 3.x "Boot and Configuration"
 *
 * [3] ZedBoard Hardware User Guide v1.1.  August 1st, 2012.
 *     Retrieved from Digilent website
 *     Page 29.
 *
 * [4] Cortex-A Series Programmer's Guide, Version 2.0.
 *     http:/www.arm.com
 *
 * The Zynq processor boot includes two mandatory stages: stage 0 and stage 1.
 * Stage 0 is the on-chip, immutable boot ROM.  This code is called on reset/
 * warm restart.
 *
 * The first-stage bootloader (FSBL) starts after boot.  It is either run from
 * on-chip memory starting at address 0x00000000 (populated by 0-th stage bootloader)
 * or it executes in place (XIP) from unencrypted memory-maped flash depending on the
 * BootROM header description. First-stage bootloader is responsible for initialization
 * of the PS (ARM core) using the PS data provided by XPS, programming the PL (FPGA) using
 * a bit-stream, loading second-stage bootloader or bare-metal application code into memory
 * and starting execution of the second-stage bootloader or bare-metal application.
 * NOTE: the default FSBL disables the MMU before starting the 2nd stage code.
 *
 * The files ps7_init.c and ps7_init.h are used to initialize CLK, DDR and MIO.  The same
 * initialization can be performed by ps7_init.tcl.  When debugging with XMD, the ps7_init.tcl
 * code is run by XMD, bypassing the need for a first-stage bootloader.
 *
 * For now, all development is done using XPS, with ps7_init.tcl utilized to perform the
 * 1st stage bootloader behavior.  When the program is moved to QSPI Flash or SD, refer
 * to [2] section 3.4 and 3.5 for details about how to generate boot images.
 *
 * When the JTAG boot source is selected, the CPU halts imediately after it disables
 * access to all security-related items and enables the JTAG port.  It is the user's
 * responsibility to manage downloading the boot image into OC RAM or DDR memory via
 * JTAG before waking up the CPU and continuing the boot process (See [1] pp 129).
 *
 * 5 mode pins (Boot_Mode[4:0] - mapped to physical pins MIO[6:2]) determing the boot source.
 * On the ZedBoard these are JP7-JP11 on the upper-right corner.
 *
 * MIO[2] (Boot_Mode[3]) determines JTAG configuration. Open (default) cascades both
 * the processor and FPGA resources - so they are both available from the same JTAG
 * interface.
 *
 * NOTE: the BootRom has two methods for communicating status information in the event
 * of a startup failure.  The primary metho is through the REBOOT_STATUS register.  Non-secure
 * boot failures result in the BootROM disabling access to the AES engine, clearing the PL
 * and enabling JTAG.  After JTAG is enabled, the REBOOT_STATUS register can be read to find the
 * source of boot failure.  See Table 6-9 in [1].
 *
 * MIO[5:3] (Boot_Mode[2:0]) determine boot device.  Three useful configurations are:
 * MIO[5]  MIO[4]  MIO[3]
 *  0        0       0       = Boot from JTAG-provided image
 *  1        0       0       = Boot from Q-SPI
 *  1        1       0       = Boot from SD Card
 *
 * MIO[6] (Boot-Mode[4]) enables or disables the PLLs during 0-stage bootloader.
 * The default is 0 (open), or enabled.
 *
 * After BootROM, the PS is in the following state:
 *   - MMU, Icache, Dcache, L2 cache are all disabled
 *   - Both processors are in the supervisor state
 *   - ROM code is masked and inaccessible
 *   - 192 kB of OCM is accessible starting at address 0x0
 *   - 64 kB of OCM is accessible starting at address 0xFFFF0000
 *   - CPU0 branches into the stage1 image if no failure occurs
 *   - CPU1 is in a WFE state while executing code at address 0xFFFFFE00 to 0xFFFFFFF0
 *
 * CPU0 is in charge of starting any addtional code on CPU1.  The BootROM keeps CPU1
 * in a reset-type state (WFE).  When CPU1 receives a system event it reads the contents
 * of address 0xFFFFFFF0 and jumps to that address.  Only ARM-32 ISA code is supported
 * at the jump target placed in 0xFFFFFFF0.  The destination address must be 32-bit
 * aligned and must be a valid ARM-32 instruction.
 */
    .equ PROC1_RESET_VECTOR_TABLE,  0xFFFFFFF0
/*
 * See sectoin 6.3.7 of [1] for more information on PS (CPU0,1) state after BootROM exit
 *
 * The ARM Cortex-A series processors have 7 standard modes (an 8th mode
 * is utilized by the TrustZone security architecure and in systems with
 * virtualization extensions a 9th mode is utilized by the hypervisor)
 * associated with the standard modes.
 *
 * MODE_USER and MODE_SYSTEM are the normal operating modes.  The former
 * is for unprivileged user code and the latter for privileged execution.
 * They share a common stack and register set (thus 7 modes, 6 stacks).
 *
 * MODE_SVC (Supervisor) is used to handle software-generated exceptions,
 * such as system requests generated by user code.
 *
 * MODE_ABORT and MODE_UNDEFINED support fault and undefined instruction
 * states.
 *
 * Two interrupt request signals, IRQ and FIQ, are used to interrupt the
 * processor.  The procesor has two modes, MODE_IRQ and MODE_FIQ, for
 * handling these interupts.  The FIQ ("Fast Interrupt Request") has
 * additional hardware support and placement optimizations to increase
 * its responsiveness.
 *
 * Mode is determined by the value in the MODE field of the CPSR register.
 * These can be set using the "CPS" instruction, ie "CPS CPSR_MODE_USER":
 */
 /* TODO THESE ARE MACROS DEFINING THE HARDWARE MODE OF THE PROCESSOR */
    .equ CPSR_MODE_USER,       0x10
    .equ CPSR_MODE_FIQ,        0x11
    .equ CPSR_MODE_IRQ,        0x12
    .equ CPSR_MODE_SVC,        0x13
    .equ CPSR_MODE_ABORT,      0x17
    .equ CPSR_MODE_UNDEFINED,  0x8B
    .equ CPSR_MODE_SYSTEM,     0x1F

/*
 * Stack placement is handled in the linker script - see Zynq.ld.  The
 * stack size for each is defined by __stack_size, below. Stacks are
 * placed in the high 64k block of OCM.  There are 14 stacks and
 * we can't step on the last ~256 bytes of this block (as it is used
 * by proc1 on start-up - see below) so limited to ~4k stack size.
 */
    .equ __stack_size, 0x1000  /* See linker script */
    .global __stack_size

/*
 * The IRQ and FIQ exceptions are enabled/disabled by flags in the CPSR.
 * They are enabled/disabled using the CPSIE/CPSID instructions followed by
 * an argument describing which interupt to alter, 'i' for IRQ and 'f' for FIQ:
 *
 * cpsie i   - enables IRQ interrupts
 * cpsid d   - disables IRQ interupts // TODO should this be cpsid i?
 * cpsie f   - enables FIQ interrupts
 * cpsid f   - disables FIQ interrupts
 *
 * These can also be set by writing directly to the CPSR register - which is done
 * below during initialization to set up stacks and interrupt masks for the
 * multiple processor modes
 */
    .equ CPSR_IRQ, 0x80 /* disable IRQ interrupts */
    .equ CPSR_FIQ, 0x40 /* disable FIQ interrupts */

/*
 * The linker script (Zynq.ld) places text in the "bootimage" section at the
 * start of the image.  The interrupt jump table (vector) lives at address 0x00000000 and
 * contains branch instructions to appropriate handlers.  Single-instruction
 * branches cannot jump further than 2^24 bits, so common practice is to
 * branch to a locally placed handler, or branch to a locally placed
 * verneer that allows a longer jump to a handler. Note that in general
 * the FIQ is the last entry, allowing the FIQ handler to begin execution
 * at the jump table address (rather than jumping to the handler) as no
 * addtional jump table entries exist after it.  In BEAR we take the
 * performance hit and jump to a handler.
 *
 * Exception Modes and handling are described in [4] Section 10.  Note there
 * are different recommended return instructions for each mode; these are
 * captured in the comments in the jump table below.
 *
 * Unimplemented handlers halt by looping in the jump table below.
 *
 * _init (reset handler), svc_Handler (supervisor call) and fiq_handler (interrupts)
 * are assumed to live in the OCM, well within the 2^24 max jump distance restriction
 * on the "b <target>" instruction.
 *
 * FIQ_handler and SVC_handler are defined in interrupts.S
 */
    .section .vectortable

    /* Exception                Processor Mode          Event                    Return Sequence              */
    b reset_handler             /* MODE_SVC          System Reset                n/a                          */
    b undef_handler             /* MODE_UNDEFINED    Undefined Instruction       MOVS PC, LR (if emulating)   */
    b SVC_handler               /* MODE_SVC          SVC instruction             MOVS PC, LR                  */
    b prefetch_abort_handler    /* MODE_ABORT        Invalid inst. address       SUBS PC, LR, #4              */
    b data_abort_handler        /* MODE_ABORT        R/W to invalid address      SUBS PC, LR, #8 (to retry)   */
    b hypervisor_handler        /* MODE_HYP          Hypervisor entry            ERET                         */
    b IRQ_handler               /* MODE_IRQ          IRQ Input Asserted          SUBS PC, LR, #4              */
    b FIQ_handler               /* MODE_FIQ          FIQ Input Asserted          SUBS PC, LR, #4              */

    .text

reset_handler:
proc0_init:
    /* Switch between the various modes and initialize stack pointers */
    /* When finished with this block, we'll be in the supervisor (privledged user) mode */
    /* set all the stack pointers */
    /* NOTE: ONLY NEED TO SET STACK POINTERS FOR THE MODE IM RUNNING IN (SVC and IRQ)
          Jason reccomends only using the last 2 lines of code
          NOTE: this also disables interrupts in each of these modes as the default setting */
    msr CPSR, #(CPSR_IRQ | CPSR_FIQ | CPSR_MODE_IRQ)        /* switch to MODE_IRQ */
    ldr sp, =__proc0_irq_stack                              /* load IRQ mode stack pointer */
    msr CPSR, #(CPSR_IRQ | CPSR_FIQ | CPSR_MODE_FIQ)        /* switch to MODE_FIQ */
    ldr sp, =__proc0_fiq_stack                              /* load FIQ mode stack pointer */
    msr CPSR, #(CPSR_IRQ | CPSR_FIQ | CPSR_MODE_ABORT)      /* switch to MODE_ABT */
    ldr sp, =__proc0_abt_stack                              /* load ABT mode stack pointer */
    msr CPSR, #(CPSR_IRQ | CPSR_FIQ | CPSR_MODE_UNDEFINED)  /* switch to MODE_UND */
    ldr sp, =__proc0_und_stack                              /* load UND mode stack pointer */
    msr CPSR, #(CPSR_IRQ | CPSR_FIQ | CPSR_MODE_SYSTEM)     /* switch to MODE_SYS */
    ldr sp, =__proc0_sys_stack                              /* load SYS mode stack pointer */
    msr CPSR, #(CPSR_IRQ | CPSR_FIQ | CPSR_MODE_SVC)        /* switch to MODE_SVC */
    ldr sp, =__proc0_svc_stack                              /* load SVC mode stack pointer */
    /* We are now in SVC mode */


    /* Clear the .bss section (zero inits) */
    ldr r1, =__bss_start__ // where in memory is start of bss (get from linker script)
    ldr r2, =__bss_end__ // where in memory is end of bss (get from linker script)
    mov r3, #0
clearbss:
    cmp r1, r2
    stmialt r1!,{r3}
    blt clearbss

    bl ps7_init
    /* Finally, transfer control to the user program */
    b main

undef_handler:
    b undef_handler

SVC_handler:
    b SVC_handler

prefetch_abort_handler:
    b prefetch_abort_handler

data_abort_handler:
    b data_abort_handler

hypervisor_handler:
    b hypervisor_handler

IRQ_handler:
	/* IRQ only used for scheduling */
    b IRQ_handler

FIQ_handler:
    b FIQ_handler


.end
