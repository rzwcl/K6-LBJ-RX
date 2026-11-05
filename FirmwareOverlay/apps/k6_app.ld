ENTRY(k6_app_main)

MEMORY
{
    APP_RAM (rwx) : ORIGIN = 0x20001800, LENGTH = 0x1800
}

SECTIONS
{
    . = ORIGIN(APP_RAM);

    .text :
    {
        KEEP(*(.text.k6_app_entry))
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        *(.glue_7)
        *(.glue_7t)
        *(.ARM.extab*)
        *(.ARM.exidx*)
        . = ALIGN(4);
    } > APP_RAM

    .data :
    {
        . = ALIGN(4);
        *(.data)
        *(.data.*)
        . = ALIGN(4);
    } > APP_RAM

    __k6_app_image_end = .;

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __k6_app_bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        __k6_app_bss_end = .;
    } > APP_RAM

    __k6_app_end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }

    ASSERT(__k6_app_image_end <= ORIGIN(APP_RAM) + LENGTH(APP_RAM),
           "K6 APP image exceeds overlay SRAM")
    ASSERT(__k6_app_end <= ORIGIN(APP_RAM) + LENGTH(APP_RAM),
           "K6 APP image+BSS exceeds overlay SRAM")
}
