/* memmap_app_ram.ld — APP runs entirely from RAM with code overlay */
MEMORY {
  RAM (rwx) : ORIGIN = 0x20030000, LENGTH = 0x00070000
  CODE_OVERLAY (rx) : ORIGIN = 0x20040000, LENGTH = 0x00040000  /* 256KB code overlay window */
}

SECTIONS {
  /* Header must be first at ORIGIN so kernel can read it */
  .app_hdr ORIGIN(RAM) : {
    KEEP(*(.app_hdr))
    . = ALIGN(4);
  } > RAM

  /* Code and read-only data - goes into overlay window */
  /* All code pages will load into this fixed region, overlaying each other */
  .text : {
    *(.text*)
    *(.rodata*)
    *(.srodata*)
    *(.gnu.linkonce.t.*)
    *(.gnu.linkonce.r.*)
    . = ALIGN(4);
  } > CODE_OVERLAY

  /* Initialized data */
  .data : {
    __data_start__ = .;
    *(.data*)
    *(.sdata*)
    *(.gnu.linkonce.d.*)
    . = ALIGN(4);
    __data_end__ = .;
  } > RAM

  /* Uninitialized data - ONE unified BSS section containing all variants */
  /* CRITICAL: All uninitialized data must end up here for proper zeroing */
  .bss (NOLOAD) : {
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)                    /* Common symbols (old GCC behavior) */
    *(.sbss*)                    /* Small BSS - must be in .bss, not separate */
    *(.gnu.linkonce.b*)          /* Linkonce BSS sections */
    *(.scommon)                  /* Small common symbols */
    . = ALIGN(8);
    __bss_end__ = .;
  } > RAM

  /* Heap starts after unified .bss - single source of truth */
  . = ALIGN(8);
  __heap_start__ = .;
  __heap_end__ = ORIGIN(RAM) + LENGTH(RAM);
  PROVIDE(end = __heap_start__);
  __StackTop = ORIGIN(RAM) + LENGTH(RAM);

  /* Discard unused sections */
  /DISCARD/ : {
    *(.comment*)
    *(.note*)
    *(.debug*)
    *(.ARM.attributes*)
    *(.eh_frame*)
    *(.eh_frame_hdr*)
    *(.gnu.warning*)
    *(.iplt*)
    *(.igot*)
    *(.rel.dyn*)
    *(.rela.dyn*)
    *(.rel.plt*)
    *(.rela.plt*)
    *(.init*)
    *(.fini*)
    *(.preinit_array*)
    *(.init_array*)
    *(.fini_array*)
    *(.ctors*)
    *(.dtors*)
    *(.jcr*)
    *(.got*)
    *(.got.plt*)
    *(.dynamic*)
    *(.dynsym*)
    *(.dynstr*)
    *(.hash*)
    *(.gnu.version*)
    *(.gnu.version_d*)
    *(.gnu.version_r*)
  }
}