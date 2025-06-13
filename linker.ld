ENTRY(_reset)

MEMORY {
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 64M
}

SECTIONS {
    . = 0x80000000;
    .vectors : { KEEP(*(.vectors)) } > RAM
    .os_text : { root.o(.text*) os.o(.text*) stdio.o(.text*) root.o(.rodata*) os.o(.rodata*) stdio.o(.rodata*) } > RAM
    .os_data : { root.o(.data*) os.o(.data*) stdio.o(.data*) } > RAM
    .os_bss : {
        _bss_start = .;
        root.o(.bss*) os.o(.bss*) stdio.o(.bss*) root.o(COMMON) os.o(COMMON) stdio.o(COMMON)
        _bss_end = .;
    } > RAM

    . = 0x80010000;
    .proc1_text : { process1.o(.text*) process1.o(.rodata*) } > RAM
    .proc1_data : { process1.o(.data*) } > RAM
    .proc1_bss : { process1.o(.bss*) process1.o(COMMON) } > RAM

    . = 0x80020000;
    .proc2_text : { process2.o(.text*) process2.o(.rodata*) } > RAM
    .proc2_data : { process2.o(.data*) } > RAM
    .proc2_bss : { process2.o(.bss*) process2.o(COMMON) } > RAM

    _os_stack_top = 0x80008000;
    _proc1_stack_top = 0x80018000;
    _proc2_stack_top = 0x80028000;

    . = ALIGN(4);
}
