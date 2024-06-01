void * __cdecl memset (
        void *dst,
        int val,
        unsigned int count
        )
{
        void *start = dst;

        while (count--) {
                *(char *)dst = (char)val;
                dst = (char *)dst + 1;
        }

        return(start);
}


/*
#include <stddef.h>

void* memset(void* dest, int val, size_t count)
{
    char* dst = (char*)dest;

    while (count--)
    {
        *dst++ = (char)val;
    }

    return dest;
}
*/