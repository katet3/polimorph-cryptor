void* __cdecl decRC4(char* pKey, DWORD dwKeyLen, char* pData, DWORD dwDataLen) {
    if (!pKey || !pData || dwKeyLen == 0 || dwDataLen == 0) {
        return nullptr;
    }

    unsigned char S[256];
    unsigned char temp;
    int i, j = 0, k;

    // Инициализация массива S
    for (i = 0; i < 256; ++i) {
        S[i] = i;
    }

    // Перемешивание массива S на основе ключа
    for (i = 0; i < 256; ++i) {
        j = (j + S[i] + pKey[i % dwKeyLen]) % 256;
        temp = S[i];
        S[i] = S[j];
        S[j] = temp;
    }

    // Дешифрование данных (процесс идентичен шифрованию)
    i = j = 0;
    for (DWORD idx = 0; idx < dwDataLen; ++idx) {
        i = (i + 1) % 256;
        j = (j + S[i]) % 256;

        // Обмен значениями S[i] и S[j]
        temp = S[i];
        S[i] = S[j];
        S[j] = temp;

        // Генерация псевдослучайного байта и его XOR с данными
        pData[idx] ^= S[(S[i] + S[j]) % 256];
    }
    return NULL;
}
