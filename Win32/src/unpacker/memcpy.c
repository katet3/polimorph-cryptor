void* __cdecl memcpy(
        void * dst,
        const void * src,
        unsigned int count
        )
{
        void * ret = dst;

        while (count--) {
                *(char *)dst = *(char *)src;
                dst = (char *)dst + 1;
                src = (char *)src + 1;
        }

        return(ret);
}


/*
#include <stddef.h>

void* memcpy(void* dest, const void* src, size_t count)
{
    char* dst = (char*)dest;
    const char* source = (const char*)src;

    while (count--)
    {
        *dst++ = *source++;
    }

    return dest;
}
*/