#ifndef __TENDYRON_H__
#define __TENDYRON_H__

#define LEN_APDU 4106

typedef int BOOL;

extern "C" {
	BOOL TDR_GetFirstReaderName(char ** ppszReaders);
	BOOL TDR_SendCommand(const char * readerName, int m_dwProtocal, const unsigned char * pucBufIn, unsigned long usLenIn, unsigned char ** ppucBufOut, unsigned long * pulLenOut);

	//BOOL TDR_GetFirstReaderNameW(wchar_t ** ppszReaders);
	//BOOL TDR_SendCommandW(LPCWSTR readerName, int m_dwProtocal, const unsigned char * pucBufIn, unsigned long usLenIn, unsigned char ** ppucBufOut, unsigned long * pulLenOut);

	void TDR_FreeMemory(void ** ppMem);

	BOOL tendyronRsa(unsigned char * pcRSAHEXdata, unsigned char ** ppcRSAresult);
}

#endif