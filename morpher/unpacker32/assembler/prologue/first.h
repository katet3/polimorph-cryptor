__asm
    {
        jmp next;
        ret 0xC;
    next:
        push ebp;
        mov ebp, esp;
        sub esp, 4096;
    }

    unsigned int original_image_base;
    unsigned int rva_of_first_section;
    unsigned int original_image_base_no_fixup;

    __asm
    {
        mov original_image_base, 0x11111111;
        mov rva_of_first_section, 0x22222222;
        mov original_image_base_no_fixup, 0x33333333;
    }