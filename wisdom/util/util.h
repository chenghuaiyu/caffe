
#ifndef _VIM_UTIL__h
#define _VIM_UTIL__h


#include <string>
#include <vector>
#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;


string GetAppPath();
bool IsDirExist(string strDirName);
bool IsFileExist(string strFileName);
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
