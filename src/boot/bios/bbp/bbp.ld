/*OUTPUT_FORMAT(elf64-x86-64)*/
OUTPUT_FORMAT(binary)
ENTRY(start16)
SECTIONS {
	.text 0x7E00 : AT( 0x7E00 ) {
		*boot.o(.text); 
		*(.text);
	}
	.rodata : {
		*(.rodata);
	}
	.data : {
		*(.data);
	}
	.bss : {
		*(.bss);
	}
	/DISCARD/ : {
		*(.eh_frame);
	}
	. = ALIGN(4096);
}