/*
 * Copyright (c) 2012-2019
 * See LICENSE for details.
 *
 * Joe Fenton <jlfenton65@gmail.com>
 * Israel Jacquez <mrkotfw@gmail.com>
 */

OUTPUT_FORMAT ("elf32-sh")
OUTPUT_ARCH (sh)
EXTERN (_start)
ENTRY (_start)
SEARCH_DIR ("$YAUL_INSTALL_ROOT/$YAUL_ARCH_SH_PREFIX/lib");

MEMORY {
  hiram (RWx) : ORIGIN = 0x06004000, LENGTH = 0x000FC000 /* 984 KB */
  loram (RWx) : ORIGIN = 0x00204000, LENGTH = 0x000FC000 /* 984 KB */
  vdp1ram (RWx) : ORIGIN = 0x05C00000, LENGTH = 0x80000 /* 512KB */
}

SECTIONS
{
  .text :
  {
     *(.text)
     *(.text.*)
     *(.gnu.linkonce.t.*)

     INCLUDE ldscripts/yaul-c++.x

     . = ALIGN (4);
  } > hiram

  .rodata :
  {
     . = ALIGN (16);

     *(.rdata)
     *(.rodata)
     *(.rodata.*)
     *(.gnu.linkonce.r.*)
  } > hiram

  .data :
  {
     . = ALIGN (16);

     *(.data)
     *(.data.*)
     *(.gnu.linkonce.d.*)
     SORT (CONSTRUCTORS)
     *(.sdata)
     *(.sdata.*)
     *(.gnu.linkonce.s.*)
  } > hiram

  .bss :
  {
     . = ALIGN (16);
     PROVIDE (___bss_start = .);

     *(.bss)
     *(.bss.*)
     *(.gnu.linkonce.b.*)
     *(.sbss)
     *(.sbss.*)
     *(.gnu.linkonce.sb.*)
     *(.scommon)
     *(COMMON)

     . = ALIGN (16);
     PROVIDE (___bss_end = .);
  } > hiram

  .uncached (0x20000000 | ___bss_end) : AT (___bss_end)
  {
     *(.uncached)
     *(.uncached.*)

     . = ALIGN (4);
  }

  /* Back to cached addresses */
  ___end = ___bss_end + SIZEOF (.uncached);


  .hiram : {
    . = ALIGN(4);
    _shiram = .;
    *(.hiram)
    *(.hiram*)
    . = ALIGN(4);
    _ehiram = .;
  }

  .vdp1ram : {
    . = ALIGN(4);
    _svdp1ram = .;
    *(.vdp1ram)
    *(.vdp1ram*)
    . = ALIGN(4);
    _evdp1ram = .;
  }
}
