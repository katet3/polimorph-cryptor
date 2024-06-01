{
typedef LPVOID(__stdcall* virtual_alloc_func)(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
#define TOO_MUCH_MEM 100000000
#define TOO_MEM_IDX 33331
#define TOO_OP(i) i / 3 * 3 - i + (i * 321)
	char* memdmp = NULL;

	// Выделяем память с помощью переданной функции
	memdmp = (char*)a(NULL, TOO_MUCH_MEM, MEM_COMMIT, PAGE_READWRITE);

	if (memdmp != NULL)
	{
		// Можно выполнить некоторые действия с памятью здесь
		// Например, заполнить её нулями или какими-либо данными
		memset(memdmp, 0, TOO_MUCH_MEM);

		// Заполняем часть памяти данными
		for (size_t i = 0; i < TOO_MEM_IDX; ++i)
		{
			memdmp[i] = (char)TOO_OP(i);
		}

		// Освобождаем память с помощью переданной функции
	}
}