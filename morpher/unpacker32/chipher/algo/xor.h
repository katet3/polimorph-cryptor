void* __cdecl decXOR(char* pKey, DWORD dwKeyLen, char* pData, DWORD dwDataLen)
{
	if (!pKey || !pData || dwKeyLen == 0 || dwDataLen == 0) {
		return nullptr;
	}

	for (DWORD i = 0; i < dwDataLen; ++i) {
		pData[i] = pData[i] ^ pKey[i % dwKeyLen];
	}
	return NULL;
}