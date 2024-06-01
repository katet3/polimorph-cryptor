// trashcode1.h
{
    int a, i;
    a = 0;
    for (i = 0; i < 1000; ++i) {
        a += i;
        a -= i;
    }
}
{
    int b, i;
    b = 1;
    for (i = 0; i < 1000; ++i) {
        if (i % 2 == 0) {
            b *= 2;
        } else {
            b /= 2;
        }
    }
    int dummyVar1;
    dummyVar1 = 123456;
}
