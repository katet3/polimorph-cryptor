    __asm
    {
        mov eax, info_copy.original_entry_point;
        add eax, original_image_base;
        leave;
        jmp eax;
    }