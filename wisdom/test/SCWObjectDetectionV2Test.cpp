#ifndef _LINUX_
#include "windows.h"
#include <tchar.h>
#include <io.h>
#else
#endif

#include <string>
#include <vector>
#include <iostream>  
#include <stdio.h>  
#include <cstring>
#include <fstream>
#include "../util/util.h"
#include "opencv2/opencv.hpp"
#include <ctime>
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"
#include "../SCWObjectDetectionV2.h"

using namespace std;
using namespace cv;

bool IsImageFileExt(const char * pszExt) {
	if (_stricmp(pszExt, "jpg") == 0 ||
		_stricmp(pszExt, "jpeg") == 0 ||
		_stricmp(pszExt, "png") == 0 ||
		_stricmp(pszExt, "bmp") == 0) {
		return true;
	}
	else {
		return false;
	}
}

#ifdef _WIN32

void SearchImageFiles(vector<string> & fileList, const char * pszImagePath, bool bFullPath) {
	//LOG_FUNCTION;
	if (NULL == pszImagePath)
	{
		return;
	}
	char szSearchPath[_MAX_PATH];
	strcpy_s(szSearchPath, pszImagePath);

	strcat_s(szSearchPath, "/*.*");

	HANDLE hFind;
	WIN32_FIND_DATAA ffd;

	hFind = FindFirstFileA(szSearchPath, &ffd);
	DWORD errorcode = 0;
	if (hFind == INVALID_HANDLE_VALUE) {
		printf("Can not find any file\n");
		return;
	}

	while (true)
	{
		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			int nLen = (int)strlen(ffd.cFileName);
			if (nLen >= 5)
			{
				char * pszExt = strrchr(ffd.cFileName, '.');

				if (pszExt != NULL && IsImageFileExt(pszExt + 1))
				{
					if (bFullPath)
					{
						string strFile = pszImagePath;
						strFile += "/";
						strFile += ffd.cFileName;
						fileList.push_back(strFile);
					}
					else
					{
						fileList.push_back(string(ffd.cFileName));
					}
				}
			}
		}

		if (FindNextFileA(hFind, &ffd) == FALSE)
			break;
	}

	FindClose(hFind);
}
#else
void SearchImageFiles(vector<string> & fileList, const char * pszImagePath, bool bFullPath = false)
{
	LOG_FUNCTION;
	char szSearchPath[_MAX_PATH];
	strcpy(szSearchPath, pszImagePath);

	DIR *dir = opendir(szSearchPath);
	if (dir == NULL) {
		printf("Can not find any file\n");
		return;
	}

	while (true)
	{
		struct dirent *s_dir = readdir(dir);
		if (s_dir == NULL)
			break;

		if ((strcmp(s_dir->d_name, ".") == 0) || (strcmp(s_dir->d_name, "..") == 0))
			continue;

		char currfile[_MAX_PATH];
		sprintf(currfile, "%s/%s", szSearchPath, s_dir->d_name);
		struct stat file_stat;
		stat(currfile, &file_stat);
		if (!S_ISDIR(file_stat.st_mode))
		{
			int nLen = strlen(s_dir->d_name);
			if (nLen >= 5)
			{
				char * pszExt = strrchr(s_dir->d_name, '.');

				if (pszExt != NULL && IsImageFileExt(pszExt + 1))
				{
					if (bFullPath)
					{
						string strFile = pszImagePath;
						strFile += "/";
						strFile += s_dir->d_name;
						fileList.push_back(strFile);
					}
					else
					{
						fileList.push_back(string(s_dir->d_name));
					}

					printf("%s %s\n", currfile, s_dir->d_name);
				}
			}
		}
	}

	closedir(dir);
}
#endif

bool DoesFileExist(const char * pszFileName) {
	if (-1 != GetFileAttributesA(pszFileName))
		return true;
	else
		return false;
}

std::wstring MultiByte2Wide(const char * pszMultiByte, int cp) {
	int nLen = MultiByteToWideChar(cp, 0, pszMultiByte, -1, NULL, 0);
	if (0 == nLen) {
		return L"";
	}
	wchar_t* pwszBuf = new wchar_t[nLen + 1]();
	MultiByteToWideChar(cp, 0, pszMultiByte, (int)strlen(pszMultiByte), pwszBuf, nLen);

	std::wstring wstr(pwszBuf);
	delete[] pwszBuf;

	return wstr;
}

std::string Wide2MultiByte(const wchar_t * pwszBuf, int cp) {
	int nLen = WideCharToMultiByte(cp, 0, pwszBuf, -1, NULL, NULL, NULL, NULL);
	if (0 == nLen) {
		return "";
	}

	char* pszMultiByte = new char[nLen + 1]();
	WideCharToMultiByte(cp, 0, pwszBuf, (int)wcslen(pwszBuf), pszMultiByte, nLen, NULL, NULL);

	std::string str(pszMultiByte);
	delete[] pszMultiByte;

	return str;
}

std::string MultiByte2UTF8(const char * pszMultiByte, int cp) {
	int nLen = MultiByteToWideChar(cp, 0, pszMultiByte, -1, NULL, 0);
	if (0 == nLen) {
		return "";
	}
	wchar_t* pwszBuf = new wchar_t[nLen + 1]();
	MultiByteToWideChar(cp, 0, pszMultiByte, (int)strlen(pszMultiByte), pwszBuf, nLen);
	nLen = WideCharToMultiByte(CP_UTF8, 0, pwszBuf, -1, NULL, NULL, NULL, NULL);
	char* pszUTF8 = new char[nLen + 1]();
	WideCharToMultiByte(CP_UTF8, 0, pwszBuf, (int)wcslen(pwszBuf), pszUTF8, nLen, NULL, NULL);
	delete[] pwszBuf;

	std::string strUTF8(pszUTF8);
	delete[] pszUTF8;

	return strUTF8;
}

std::string UTF82MultiByte(const char * pszUTF8, int cp) {
	int nLen = MultiByteToWideChar(CP_UTF8, 0, pszUTF8, -1, NULL, 0);
	if (0 == nLen) {
		return "";
	}
	wchar_t* pwszBuf = new wchar_t[nLen + 1]();
	MultiByteToWideChar(CP_UTF8, 0, pszUTF8, (int)strlen(pszUTF8), pwszBuf, nLen);
	nLen = WideCharToMultiByte(cp, 0, pwszBuf, -1, NULL, NULL, NULL, NULL);
	char* pszMultiByte = new char[nLen + 1]();
	WideCharToMultiByte(cp, 0, pwszBuf, (int)wcslen(pwszBuf), pszMultiByte, nLen, NULL, NULL);
	delete[] pwszBuf;

	std::string str(pszMultiByte);
	delete[] pszMultiByte;
	return str;
}

void BatchProcess(void * hAlg, string strImageDirS8, int missErrorRatio) {
	LOG(INFO) << "proceed image dir: " << strImageDirS8;
	string strImageDir = UTF82MultiByte(strImageDirS8.c_str(), CP_ACP);
	std::cout << "proceed image dir: " << strImageDir << endl;
	if (!DoesFileExist(strImageDir.c_str())) {
		std::cout << "src file directory doesn't exist: " << strImageDir << endl;
		return;
	}

	vector<string> fileList;
	SearchImageFiles(fileList, strImageDir.c_str(), true);
	if (0 == fileList.size()) {
		std::cout << "image file doesn't exist" << endl;
		return;
	}
	LOG(INFO) << "image file count: " << fileList.size();
	std::cout << "image file count: " << fileList.size() << endl;

	for (vector<string>::iterator it = fileList.begin(); it != fileList.end(); ++it) {
		printf(">>> proceed: %s\n", it->c_str());
		string strUtf8 = MultiByte2UTF8(it->c_str(), CP_ACP);
		LOG(INFO) << ">>> proceed: " << strUtf8;
		char* pszRes = NULL;
		int nRet = SCWDetectObjectByFile(&pszRes, hAlg, strUtf8.c_str(), missErrorRatio);
		if (SCWERR_NOERROR != nRet) {
			LOG(INFO) << "detection return: " << nRet;
			continue;
		}
		printf("--->proceed result:\n %s\n<---\n", UTF82MultiByte(pszRes, CP_ACP).c_str());
		SCWRelease(&pszRes);
	}
}

//int main(int argc, char** argv) {
int wmain(int argc, wchar_t *argv[], wchar_t *envp[]) {
	string strArgv0 = Wide2MultiByte(argv[0], CP_ACP);
	::google::InitGoogleLogging(strArgv0.c_str());
	std::cout << "argc: " << argc << endl;
	if (5 > argc) {
		std::cout << "format: objectTypes\t configFile\t dstDir imgDir [imgDir2 ...]\n"
			<< "for example:\n\t\"knife;scissor\" \"d:/SCWObjectDetectionConf.ini\" \"D:\\SCWObjectDetectionResult\" \"D:\\imagesA\" \"D:\\imagesB\"" << endl;
		return -1;
	}

	//wchar_t wszObjectType[] = L" knife : 刀  ;   scissors=scissor :   剪刀;liquid:液体;battery:锂电池 ";
	//string strObjectType = Wide2MultiByte(wszObjectType, CP_UTF8);
	string strObjectType = Wide2MultiByte(argv[1], CP_UTF8);
	LOG(INFO) << "objectTypes: \"" << strObjectType.c_str() << "\"";
	string strConfigFile = Wide2MultiByte(argv[2], CP_UTF8);
	LOG(INFO) << "configFile: \"" << strConfigFile << "\"";

	HScwAlg hAlg = NULL;
	char* pszObjects = NULL;
	int nRet = SCWInitObjectDetection(&hAlg, &pszObjects, strObjectType.c_str(), strConfigFile.c_str(), 16);
	if (0 != nRet) {
		printf("exit due to failure invoking DetectionInit: %d", nRet);
		return nRet;
	} else {
		//std::wcout << L"object types result: " << MultiByte2Wide(pszObjects, CP_UTF8) << L"\n";
		SCWRelease(&pszObjects);
	}

	string strDstDir = Wide2MultiByte(argv[3], CP_UTF8);
	LOG(INFO) << "dstDir: " << strDstDir << endl;

	//char* pszRes = NULL;
	//const char szImagePath[] = "D:/images/xRayImg/images/01.jpg; D:/images/xRayImg/images/02.jpg";
	//LOG(INFO) << ">>> proceed: " << szImagePath;
	//nRet = SCWDetectObjectByFile(&pszRes, hAlg, szImagePath, 0);
	//if (SCWERR_NOERROR != nRet) {
	//	LOG(INFO) << "detection return: " << nRet;
	//} else {
	//	SCWRelease(&pszRes);
	//}

	for (int i = 4; i < argc; ++i) {
		string strImgDir = Wide2MultiByte(argv[i], CP_UTF8);
		LOG(INFO) << "imgDir: " << strImgDir;
		BatchProcess(hAlg, strImgDir, 0);
	}

	nRet = SCWUninitObjectDetection(&hAlg);

	::google::FlushLogFiles(0);
	::google::ShutdownGoogleLogging();
	printf("job finished\n");
	LOG(INFO) << "job finished.";
	return 0;
}