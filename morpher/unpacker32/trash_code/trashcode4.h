// trashcode4.h
{
    int array[10];
    int* ptr;
    ptr = array;
    int i;
    for (i = 0; i < 10; ++i) {
        *(ptr + i) = i;
    }
}
{
    #define DUMMY_MACRO(x) ((x) * (x))
    int value;
    value = DUMMY_MACRO(5);
    char dummyVar4;
    dummyVar4 = 'A';
}
