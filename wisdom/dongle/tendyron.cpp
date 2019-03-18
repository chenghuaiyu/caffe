#include <stdio.h>
#include <Winscard.h>
#include "tendyron.h"

BOOL TDR_GetFirstReaderName(char ** ppszReaders) {
	if (NULL == ppszReaders) {
		return FALSE;
	}
	//establish the context
	SCARDCONTEXT context;	//the context
	int iRv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context);
	if (SCARD_S_SUCCESS != iRv) {
		printf(("Fail:SCardListReaders 0x%X\n"), iRv);
		return FALSE;
	}

	//enu the reader
	CHAR tchReaders[LEN_APDU] = { 0 };
	DWORD dwLenReaders = LEN_APDU;
	iRv = SCardListReadersA(context,
		NULL,
		tchReaders,
		&dwLenReaders);
	if (SCARD_S_SUCCESS != iRv) {
		printf(("Fail:SCardListReaders 0x%X\n"), iRv);
		SCardReleaseContext(context);
		return FALSE;
	}

	//get the first reader status
	SCARDHANDLE scardHandle;	//the handle of the device
	DWORD dwAP = 0;
	iRv = SCardConnectA(context, tchReaders, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &scardHandle, &dwAP);
	if (SCARD_S_SUCCESS != iRv) {
		printf("Card not ready: %d", iRv);
		SCardReleaseContext(context);
		return FALSE;
	}
	SCardDisconnect(scardHandle, SCARD_LEAVE_CARD);

	//release the context
	SCardReleaseContext(context);

	*ppszReaders = new char[strlen(tchReaders) + 1]();
	strcpy(*ppszReaders, tchReaders);
	return TRUE;
}

BOOL TDR_GetFirstReaderNameW(wchar_t ** ppszReaders) {
	if (NULL == ppszReaders) {
		return FALSE;
	}
	//establish the context
	SCARDCONTEXT context;	//the context
	int iRv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context);
	if (SCARD_S_SUCCESS != iRv) {
		printf(("Fail:SCardListReaders 0x%X\n"), iRv);
		return FALSE;
	}

	//// states
	//SCARD_READERSTATEW readerStates = { 0 };
	//readerStates.szReader = L"\\\\?PNP?\\NOTIFICATION";
	//SCardGetStatusChangeW(context, INFINITE, &readerStates, 1);
	//readerStates.dwCurrentState = readerStates.dwEventState;


	//enu the reader
	WCHAR tchReaders[LEN_APDU] = { 0 };
	DWORD dwLenReaders = LEN_APDU;
	iRv = SCardListReadersW(context,
		NULL,
		tchReaders,
		&dwLenReaders);
	if (SCARD_S_SUCCESS != iRv) {
		printf(("Fail:SCardListReaders 0x%X\n"), iRv);
		SCardReleaseContext(context);
		return FALSE;
	}

	//get the first reader status
	SCARDHANDLE scardHandle;	//the handle of the device
	DWORD dwAP = 0;
	iRv = SCardConnectW(context, tchReaders, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &scardHandle, &dwAP);
	if (SCARD_S_SUCCESS != iRv) {
		printf("Card not ready: %d", iRv);
		SCardReleaseContext(context);
		return FALSE;
	}
	SCardDisconnect(scardHandle, SCARD_LEAVE_CARD);


	// states
	SCARD_READERSTATEW readerStates = { 0 };
	readerStates.szReader = tchReaders;
	iRv = SCardGetStatusChangeW(context, INFINITE, &readerStates, 1);
	if (SCARD_S_SUCCESS != iRv) {
		printf("Card not ready: %d", iRv);
		SCardReleaseContext(context);
		return FALSE;
	}
	//the pnp reader popup
	if (SCARD_E_NO_READERS_AVAILABLE == iRv) {
		printf("Card popup: %d", iRv);
		SCardReleaseContext(context);
		return FALSE;
	}

	DWORD dwChanges = (readerStates.dwEventState ^ readerStates.dwCurrentState) & MAXWORD;
	readerStates.dwCurrentState = readerStates.dwEventState;

	if (SCARD_STATE_PRESENT & dwChanges & readerStates.dwEventState /*&& !(SCARD_STATE_MUTE & readerStates.dwEventState*/) {//the readr insert
		printf("Card ready: %d", dwChanges);
	} else if (SCARD_STATE_EMPTY & dwChanges & readerStates.dwEventState) {//the card pop up
		printf("Card popup: %d", dwChanges);
		SCardReleaseContext(context);
		return FALSE;
	} else {
		printf("Card not ready: %d", dwChanges);
		SCardReleaseContext(context);
		return FALSE;
	}

	//release the context
	SCardReleaseContext(context);

	*ppszReaders = new WCHAR[wcslen(tchReaders) + 1]();
	wcscpy(*ppszReaders, tchReaders);
	return TRUE;
}

void TDR_FreeMemory(void ** ppMem) {
	if (NULL == ppMem) {
		return;
	}
	delete[] * ppMem;
	*ppMem = NULL;
}

BOOL TDR_SendCommand(LPCSTR readerName, int m_dwProtocal, const unsigned char * pucBufIn, unsigned long usLenIn, unsigned char ** ppucBufOut, unsigned long * pulLenOut) {
	if (0 == strlen(readerName) || NULL == pucBufIn || 0 == usLenIn || NULL == ppucBufOut || NULL == pulLenOut) {
		return FALSE;
	}

	//establish the context
	SCARDCONTEXT context;	//the context
	int iRv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context);
	if (SCARD_S_SUCCESS != iRv) {
		printf("Error: SCardEstablishContext: 0x%X \n\n", iRv);
		return FALSE;
	}

	SCARDHANDLE handle;	//the handle of the device
	DWORD dwAP = 0;
	iRv = SCardConnectA(context,
		readerName,
		SCARD_SHARE_SHARED,
		SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
		&handle,
		&dwAP);
	if (SCARD_S_SUCCESS != iRv) {
		if (0x8010000F == iRv) {
			printf(("Error: SCardConnect: 请求的协议与目前跟智能卡一起使用的协议不兼容:0x%X \n\n"), iRv);
		} else if (0x80100066 == iRv){
			printf(("Error: SCardConnect: 智能卡没有响应重置:0x%X \n\n"), iRv);
		} else {
			printf(("Error: SCardConnect: 0x%X \n\n"), iRv);
		}
		SCardReleaseContext(context);
		return FALSE;
	}
	//MakeUpper();
	unsigned char ucBufOut[LEN_APDU] = { 0 };//buffer containint the received data
	*pulLenOut = LEN_APDU;
	if (SCARD_PROTOCOL_T0 == m_dwProtocal){
		iRv = SCardTransmit(handle,
			SCARD_PCI_T0,
			pucBufIn,
			usLenIn,
			NULL,
			ucBufOut,
			pulLenOut
			);
	} else {
		iRv = SCardTransmit(handle,
			SCARD_PCI_T1,
			pucBufIn,
			usLenIn,
			NULL,
			ucBufOut,
			pulLenOut
			);
	}

	SCardDisconnect(handle, SCARD_LEAVE_CARD);
	SCardReleaseContext(context);
	if (SCARD_S_SUCCESS != iRv) {
		*pulLenOut = 0;
		if (0x8010000F == iRv) {
			printf(("Error: SCardTransmit: 请求的协议与目前跟智能卡一起使用的协议不兼容:0x%X \n\n"), iRv);
		} else if (0x80100066 == iRv){
			printf(("Error: SCardTransmit: 智能卡没有响应重置:0x%X \n\n"), iRv);
		} else {
			printf(("Error: SCardTransmit: 0x%X \n\n"), iRv);
		}
		return FALSE;
	}
	if (2 > *pulLenOut || 0x90 != ucBufOut[*pulLenOut - 2] || 0 != ucBufOut[*pulLenOut - 1]) {
		printf(("Error: SCardTransmit end: 0x%02X%02X \n\n"), ucBufOut[*pulLenOut - 2], ucBufOut[*pulLenOut - 1]);
		return FALSE;
	}

	*ppucBufOut = new unsigned char[*pulLenOut + 1]();
	memcpy(*ppucBufOut, ucBufOut, *pulLenOut);
	return TRUE;
}


BOOL TDR_SendCommandW(LPCWSTR readerName, int m_dwProtocal, const unsigned char * pucBufIn, unsigned long usLenIn, unsigned char ** ppucBufOut, unsigned long * pulLenOut) {
	if (0 == wcslen(readerName) || NULL == pucBufIn || 0 == usLenIn || NULL == ppucBufOut || NULL == pulLenOut) {
		return FALSE;
	}

	//establish the context
	SCARDCONTEXT context;	//the context
	int iRv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context);
	if (SCARD_S_SUCCESS != iRv) {
		printf("Error: SCardEstablishContext: 0x%X \n\n", iRv);
		return FALSE;
	}

	SCARDHANDLE handle;	//the handle of the device
	DWORD dwAP = 0;
	iRv = SCardConnectW(context,
		readerName,
		SCARD_SHARE_SHARED,
		SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
		&handle,
		&dwAP);
	if (SCARD_S_SUCCESS != iRv) {
		if (0x8010000F == iRv) {
			printf(("Error: SCardConnect: 请求的协议与目前跟智能卡一起使用的协议不兼容:0x%X \n\n"), iRv);
		} else if (0x80100066 == iRv){
			printf(("Error: SCardConnect: 智能卡没有响应重置:0x%X \n\n"), iRv);
		} else {
			printf(("Error: SCardConnect: 0x%X \n\n"), iRv);
		}
		SCardReleaseContext(context);
		return FALSE;
	}
	//MakeUpper();
	unsigned char ucBufOut[LEN_APDU] = { 0 };//buffer containint the received data
	*pulLenOut = LEN_APDU;
	if (SCARD_PROTOCOL_T0 == m_dwProtocal){
		iRv = SCardTransmit(handle,
			SCARD_PCI_T0,
			pucBufIn,
			usLenIn,
			NULL,
			ucBufOut,
			pulLenOut
			);
	} else {
		iRv = SCardTransmit(handle,
			SCARD_PCI_T1,
			pucBufIn,
			usLenIn,
			NULL,
			ucBufOut,
			pulLenOut
			);
	}

	SCardDisconnect(handle, SCARD_LEAVE_CARD);
	SCardReleaseContext(context);
	if (SCARD_S_SUCCESS != iRv) {
		*pulLenOut = 0;
		if (0x8010000F == iRv) {
			printf(("Error: SCardTransmit: 请求的协议与目前跟智能卡一起使用的协议不兼容:0x%X \n\n"), iRv);
		} else if (0x80100066 == iRv){
			printf(("Error: SCardTransmit: 智能卡没有响应重置:0x%X \n\n"), iRv);
		} else {
			printf(("Error: SCardTransmit: 0x%X \n\n"), iRv);
		}
		return FALSE;
	}

	*ppucBufOut = new unsigned char[*pulLenOut + 1]();
	memcpy(*ppucBufOut, ucBufOut, *pulLenOut);
	return TRUE;
}


//void OnBnClickedButtonUpdatereader()
//{
//	//Display(_T("读卡器更新 "));
//	m_listReader.DeleteAllItems();
//	CString cstrDisplay;
//
//	//establish the context
//	SCARDCONTEXT context;	//the context
//	int iRv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context);
//	if (SCARD_S_SUCCESS != iRv)
//	{
//		DWORD nErrorNo = iRv;
//		LPSTR lpBuffer;
//		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
//			NULL,
//			nErrorNo,
//			LANG_NEUTRAL,
//			(LPTSTR)& lpBuffer,
//			0,
//			NULL);
//		cstrDisplay.Format(_T("Fail:SCardListReaders 0x%X %s\n"), nErrorNo, lpBuffer);
//		Display(cstrDisplay);
//		LocalFree(lpBuffer);
//		Display(cstrDisplay);
//		return;
//	}
//
//	//enu the reader
//	TCHAR tchReaders[LEN_APDU] = { 0 };
//	DWORD dwLenReaders = LEN_APDU;
//	iRv = SCardListReaders(context,
//		NULL,
//		tchReaders,
//		&dwLenReaders);
//	if (SCARD_S_SUCCESS != iRv)
//	{
//		DWORD nErrorNo = iRv;
//		LPSTR lpBuffer;
//		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
//			NULL,
//			nErrorNo,
//			LANG_NEUTRAL,
//			(LPTSTR)& lpBuffer,
//			0,
//			NULL);
//		cstrDisplay.Format(_T("Fail:SCardListReaders 0x%X %s\n"), nErrorNo, lpBuffer);
//		Display(cstrDisplay);
//		LocalFree(lpBuffer);
//		return;
//	}
//
//	DWORD dwIndex = 0;
//	int iNum = 0;
//	CString cstrReader;
//	while (dwIndex + 1 < dwLenReaders)
//	{
//		//get the reader name
//		cstrReader.Format(_T("%s"), &tchReaders[dwIndex]);
//		m_listReader.InsertItem(iNum, cstrReader);
//
//		//get the reader status
//		SCARDCONTEXT hCx;	//the context
//		SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hCx);
//		SCARDHANDLE scardHandle;	//the handle of the device
//		DWORD dwAP = 0;
//		iRv = SCardConnect(hCx, cstrReader, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &scardHandle, &dwAP);
//		if (SCARD_W_REMOVED_CARD == iRv)
//		{
//			m_listReader.SetItemText(iNum, 1, _T("No Card"));
//		}
//		else
//		{
//			m_listReader.SetItemText(iNum, 1, _T("Ready"));
//		}//if - else
//		SCardDisconnect(scardHandle, SCARD_LEAVE_CARD);
//		SCardReleaseContext(hCx);
//
//		dwIndex += cstrReader.GetLength() + 1;
//		iNum++;
//		cstrReader.Empty();
//	}
//	//release the context
//	SCardReleaseContext(context);
//
//	//cstrReader.Empty();
//	//cstrReader.Format(_T("OK：系统中有%d个读卡器\n"), iNum);
//	Display(cstrReader);
//}


//-----------------
#include "polarssl\rsa.h"

#define LEN_1024 1024
#define LEN_1536 1536
#define LEN_2048 2048
#define LEN_4096 4096
#define LEN_INPUT_1024 128
#define LEN_INPUT_1536 192
#define LEN_INPUT_2048 256
#define LEN_INPUT_4096 512

#define iNum 2048
char cRsakey_N[iNum] = "84FC1CC83CEFE7927535AA9A130B43BDB78544C21A9167FF47BE761BDD4E5D665D5B336EC4BF08619B8E84F73A43959F934AEA291ADB7EB93EB6F3482F36591A85422B7BE48B6E0A3A642ED9531E5E0C49DE12375A998320370B652ED8EE10577B5BA98B9A0DADBD8B8E358D8F9E99D249FF1CAD8726C95BBBF4075860DAAFB9";
char cRsakey_E[iNum] = "00010001";

BOOL tendyronRsa(unsigned char * pcRSAHEXdata, unsigned char ** ppcRSAresult) {
	if (NULL == pcRSAHEXdata || NULL == ppcRSAresult) {
		return FALSE;
	}

	rsa_context rsa_MFC;
	rsa_init(&rsa_MFC, RSA_PKCS_V15, 0);
	rsa_MFC.len = LEN_INPUT_1024;

	int irv = mpi_read_string(&rsa_MFC.N, 16, cRsakey_N);
	if (0 != irv) {
		printf("ReadString error");
		return FALSE;
	}
	irv = mpi_read_string(&rsa_MFC.E, 16, cRsakey_E);
	if (0 != irv) {
		printf("ReadString error");
		return FALSE;
	}

	*ppcRSAresult = new unsigned char[rsa_MFC.len];
	irv = rsa_public(&rsa_MFC, pcRSAHEXdata, *ppcRSAresult);
	if (0 != irv) {
		printf("rsa error");
		return FALSE;
	}

	return TRUE;
}