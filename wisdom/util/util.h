
#ifndef _VIM_UTIL__h
#define _VIM_UTIL__h


#include <string>
#include <vector>
#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;

#ifdef _WIN32
#include <winnls.h>

char* ConvertLPWSTRToLPSTR(wchar_t * lpwszStrIn);

wstring wGetDLLPath();
string GetDLLPath();

std::wstring MultiByte2Wide(const char * pszMultiByte, int cp);
std::string Wide2MultiByte(const wchar_t * pwszBuf, int cp);

std::string MultiByte2UTF8(const char * pszMultiByte, int cp);
std::string UTF82MultiByte(const char * pszUTF8, int cp);

#endif

string GetAppPath();
bool IsDirExist(string strDirName);
bool IsFileExist(string strFileName);
bool DoesFileExist(const char * pszFileName);
bool wDoesFileExist(const wchar_t * pwszFileName);
void GetFiles(string path, vector<string>& files);
void GetFiles(string path, vector<string>& files, vector<string>& pathfiles);

string GetFileNameWithNoSuffix(const string filePathName);

string GetFileNameByFilePathName(const std::string filePathName);

string GetFilePathByFilePathName(const std::string filePathName);

std::vector<std::string> Split(std::string str, std::string pattern);

void MkDstDir(const char * dstdir);

void paDrawString(Mat& dst, const char* str, Point org, Scalar color, int fontSize, bool italic, bool underline);

class PingPangBuffer
{
private:
	Mat pingBuffer;
	Mat pangBuffer;
	bool isPing;

public:
	PingPangBuffer();
	Mat getBuffer(void);
};

#endif
