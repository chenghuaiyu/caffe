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

bool wIsImageFileExt(const wchar_t * pwszExt) {
	if (_wcsicmp(pwszExt, L"jpg") == 0 ||
		_wcsicmp(pwszExt, L"jpeg") == 0 ||
		_wcsicmp(pwszExt, L"png") == 0 ||
		_wcsicmp(pwszExt, L"bmp") == 0) {
		return true;
	}
	else {
		return false;
	}
}

#ifdef _WIN32
void wSearchImageFiles(vector<wstring> & fileList, const wchar_t * pwszImagePath, bool bFullPath) {
	//LOG_FUNCTION;
	if (NULL == pwszImagePath) {
		return;
	}
	wchar_t szSearchPath[_MAX_PATH];
	wcscpy_s(szSearchPath, pwszImagePath);

	wcscat_s(szSearchPath, L"/*.*");

	HANDLE hFind;
	WIN32_FIND_DATAW ffd;

	hFind = FindFirstFileW(szSearchPath, &ffd);
	DWORD errorcode = 0;
	if (hFind == INVALID_HANDLE_VALUE) {
		printf("Can not find any file\n");
		return;
	}

	while (true) {
		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			int nLen = (int)wcslen(ffd.cFileName);
			if (nLen >= 2) {
				wchar_t * pszExt = wcsrchr(ffd.cFileName, L'.');

				if (pszExt != NULL && wIsImageFileExt(pszExt + 1)) {
					if (bFullPath) {
						wstring strFile = pwszImagePath;
						strFile += L"/";
						strFile += ffd.cFileName;
						fileList.push_back(strFile);
					}
					else {
						fileList.push_back(wstring(ffd.cFileName));
					}
				}
			}
		}

		if (FindNextFileW(hFind, &ffd) == FALSE)
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

void string_replace(std::wstring&str, const std::wstring&strMark, const std::wstring&strReplace)
{
	std::wstring::size_type pos = 0;
	std::wstring::size_type a = strMark.size();
	std::wstring::size_type b = strReplace.size();
	while ((pos = str.find(strMark, pos)) != std::wstring::npos)
	{
		str.replace(pos, a, strReplace);
		pos += b;
	}
}

std::wstring wConvertPath2Name(const wchar_t * pszImgName) {
	wchar_t szFullPath[_MAX_PATH];
	GetFullPathNameW(pszImgName, _MAX_PATH, szFullPath, NULL);
	std::wstring strFile = szFullPath;

	string_replace(strFile, L"/", L"_");
	string_replace(strFile, L"\\", L"_");
	string_replace(strFile, L":", L"__");
	return strFile;
}

void BatchProcess(void * hAlg, wstring wstrDstDir, wstring wstrImageDir, int missErrorRatio) {
	LOG(INFO) << "proceed image dir: " << Wide2MultiByte(wstrImageDir.c_str(), CP_UTF8);
	std::cout << "proceed image dir: " << Wide2MultiByte(wstrImageDir.c_str(), CP_UTF8) << endl;
	if (!wDoesFileExist(wstrImageDir.c_str())) {
		std::cout << "directory doesn't exist." << endl;
		return;
	}

	vector<wstring> fileList;
	wSearchImageFiles(fileList, wstrImageDir.c_str(), true);
	if (0 == fileList.size()) {
		std::cout << "image file doesn't exist" << endl;
		return;
	}
	LOG(INFO) << "image file count: " << fileList.size();
	std::cout << "image file count: " << fileList.size() << endl;

	for (vector<wstring>::iterator it = fileList.begin(); it != fileList.end(); ++it) {
		string strUtf8 = Wide2MultiByte(it->c_str(), CP_UTF8);
		printf(">>> proceed file: %s\n", strUtf8.c_str());
		LOG(INFO) << ">>> proceed: " << strUtf8;
		char* pszRes = NULL;
		int nRet = SCWDetectObjectByFile(&pszRes, hAlg, strUtf8.c_str(), missErrorRatio);
		if (SCWERR_NOERROR != nRet) {
			LOG(INFO) << "detection return: " << nRet;
			continue;
		}
		printf("--->alg result:\n %s\n<---\n", pszRes);
		wstring filePathName = wstrDstDir + L"/";
		filePathName+=wConvertPath2Name(it->c_str()) + L".txt";
		std::ofstream ofs(filePathName, std::ofstream::binary);
		ofs << pszRes;
		ofs.close();

		SCWRelease(&pszRes);
	}
}

//int main(int argc, char** argv) {
int wmain(int argc, wchar_t *argv[], wchar_t *envp[]) {
	system("chcp 65001");
	string strArgv0 = Wide2MultiByte(argv[0], CP_ACP);
	::google::InitGoogleLogging(strArgv0.c_str());
	std::cout << "[Test]argc: " << argc << endl;
	if (5 > argc) {
		std::cout << "format: objectTypes\t configFile\t dstDir imgDir [imgDir2 ...]\n"
			<< "for example:\n\t\"knife;scissor\" \"d:/SCWObjectDetectionConf.ini\" \"D:\\SCWObjectDetectionResult\" \"D:\\imagesA\" \"D:\\imagesB\"" << endl;
		return -1;
	}

	//wchar_t wszObjectType[] = L" knife : 刀  ;   scissors=scissor :   剪刀;liquid:液体;battery:锂电池 ";
	//string strObjectType = Wide2MultiByte(wszObjectType, CP_UTF8);
	//std::wcout << L"[Test]objectTypes: " << argv[1] << endl;
	string strObjectType = Wide2MultiByte(argv[1], CP_UTF8);
	std::cout << "[Test]objectTypes: " << strObjectType << endl;
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
		std::cout << "[Test]object types result: " << pszObjects << endl;
		SCWRelease(&pszObjects);
	}

	wstring strDstDir = argv[3];
	LOG(INFO) << "dstDir: " << Wide2MultiByte(argv[3], CP_UTF8) << endl;
	nRet = _wmkdir(strDstDir.c_str());
	if (0 != nRet) {
		printf("fail to invoke _mkdir(): %s.", Wide2MultiByte(strDstDir.c_str(), CP_UTF8).c_str());
		LOG(ERROR) << "return: " << nRet << " invoking _wmkdir() ";
	}

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
		LOG(INFO) << "imgDir: " << Wide2MultiByte(argv[i], CP_UTF8).c_str();
		wstring strImgDir = argv[i];
		BatchProcess(hAlg, strDstDir, strImgDir, 0);
	}

	nRet = SCWUninitObjectDetection(&hAlg);

	::google::FlushLogFiles(0);
	::google::ShutdownGoogleLogging();
	printf("job finished\n");
	LOG(INFO) << "job finished.";
	return 0;
}