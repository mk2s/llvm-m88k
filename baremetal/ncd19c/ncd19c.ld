/*
 * Linker script for NCD 19c bare-metal images (ld.lld, ELF output; elf2ncd.py converts to COFF).
 * The boot ROM accepts exactly three sections: .text, .data, .bss.
 *
 * Overrides: -Wl,--defsym=__ram_top=0x08200000  -Wl,--defsym=__stack_size=0x20000
 * (real unit: 2 MB data RAM at 0x08000000; MAME ncd19c: 8 MB data, 4 MB code.)
 */
ENTRY(_start)

PROVIDE(__ram_top = 0x08200000);
PROVIDE(__stack_size = 0x20000);

SECTIONS
{
	. = 0x04020000;
	.text : {
		KEEP(*(.text.ncdhdr))
		*(.text .text.*)
		*(.rodata .rodata.*)
		*(.srodata .srodata.*)
		. = ALIGN(4);
		PROVIDE_HIDDEN(__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN(__preinit_array_end = .);
		PROVIDE_HIDDEN(__init_array_start = .);
		KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) .init_array))
		PROVIDE_HIDDEN(__init_array_end = .);
		PROVIDE_HIDDEN(__fini_array_start = .);
		KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) .fini_array))
		PROVIDE_HIDDEN(__fini_array_end = .);
		. = ALIGN(4);
	}

	. = 0x08010000;
	.data : {
		KEEP(*(.data.ncdmon))
		*(.data .data.*)
		*(.sdata .sdata.*)
		. = ALIGN(4);
	}

	.bss (NOLOAD) : {
		. = ALIGN(4);
		__bss_start = .;
		*(.bss .bss.*)
		*(.sbss .sbss.*)
		*(COMMON)
		. = ALIGN(8);
		__bss_end = .;
	}

	__heap_start = ALIGN(__bss_end, 8);
	__stack_top = __ram_top;
	__heap_end = __stack_top - __stack_size;

	/DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.m88k.attributes) }
}

ASSERT(SIZEOF(.text) + 0x04020000 <= 0x04400000, ".text does not fit in MAME's 4 MB code RAM")
ASSERT(__heap_start + 0x10000 <= __heap_end, "no room for heap")
