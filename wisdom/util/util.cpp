#include "boost/filesystem.hpp" 
#include "boost/filesystem/operations.hpp"  
#include <string>
#include <vector>
#include <iostream>  
#include <stdio.h>  
#include "opencv2/opencv.hpp"

#ifdef _LINUX_
#include <unistd.h>  //access
#include <sys/stat.h>  //mkdir
#include <sys/types.h>  //mkdir
#include <dirent.h> //rmdir
#else
#include <io.h> //access
#include <direct.h> //mkdir
#include <windows.h>
#endif
#include "util.h"

#ifdef _LINUX_
#define SLASH '/' 
#define MKDIR(x) mkdir(x, 0777)
#else
#define SLASH '\\' 
#define MKDIR(x) mkdir(x)
#endif

using namespace std;

string GetAppPath() {
	string strFilePath = boost::filesystem::current_path().string();
    strFilePath += '/';
    return strFilePath;
}


bool IsDirExist(string strDirName) {
    boost::filesystem::path path(strDirName);
    bool result = boost::filesystem::is_directory(path);
    return result;
}

bool IsFileExist(string strFileName) {
    if (access(strFileName.c_str(), 0 ) == -1) {
        cout << "file doesn't exist: " << strFileName << endl;
        return false;
    }
    return true;
}

bool DoesFileExist(const char * pszFileName) {
	if (-1 != GetFileAttributesA(pszFileName))
		return true;
	else
		return false;
}

bool wDoesFileExist(const wchar_t * pwszFileName) {
	if (-1 != GetFileAttributesW(pwszFileName))
		return true;
	else
		return false;
}

#ifdef _LINUX_
void GetFiles(string path, vector<string>& files) {
    string tmp;
    // check if dir_name is a valid dir  
    struct stat s;  
    lstat( path.c_str() , &s );  
    if( ! S_ISDIR( s.st_mode ) ) {  
        cout<<"dir_name is not a valid directory !"<< path <<endl;  
        return;  
    }  
      
    struct dirent * filename;    // return value for readdir()  
    DIR * dir;                   // return value for opendir()  
    dir = opendir( path.c_str() );  
    if( NULL == dir ) {  
        cout<<"Can not open dir "<<path<<endl;  
        return;  
    }  

    /* read all the files in the dir ~ */  
    while( ( filename = readdir(dir) ) != NULL ) {  
        // get rid of "." and ".."  
        if( strcmp( filename->d_name , "." ) == 0 ||   
            strcmp( filename->d_name , "..") == 0    )  
            continue;  
        tmp = path;
        tmp += filename->d_name;
        files.push_back(tmp.c_str());
    }  
}

void GetFiles(string path, vector<string>& files, vector<string>& pathfiles) {
    string tmp;
    // check if dir_name is a valid dir  
    struct stat s;  
    lstat( path.c_str() , &s );  
    if( ! S_ISDIR( s.st_mode ) ) {  
        cout<<"dir_name is not a valid directory !"<<endl;  
        return;  
    }  
      
    struct dirent * filename;    // return value for readdir()  
    DIR * dir;                   // return value for opendir()  
    dir = opendir( path.c_str() );  
    if( NULL == dir ) {  
        cout<<"Can not open dir "<<path<<endl;  
        return;  
    }
      
    /* read all the files in the dir ~ */  
    while( ( filename = readdir(dir) ) != NULL ) {  
        // get rid of "." and ".."  
        if( strcmp( filename->d_name , "." ) == 0 ||   
            strcmp( filename->d_name , "..") == 0    )  
            continue;  
        files.push_back(filename->d_name);
        tmp = path;
        tmp += filename->d_name;
        pathfiles.push_back(tmp.c_str());
    }  
}
#else
void GetFiles(string path, vector<string>& files) {
    intptr_t   hFile = 0;
    struct _finddata_t fileinfo;
    string p;

    if ((hFile = _findfirst(p.assign(path).append("*").c_str(), &fileinfo)) != -1L) {
        do
        {
            if ((fileinfo.attrib &  _A_SUBDIR)) {
            } else {
                files.push_back(p.assign(path).append(fileinfo.name));
            }
        } while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
}

void GetFiles(string path, vector<string>& files, vector<string>& pathfiles) {
    intptr_t   hFile = 0;
    struct _finddata_t fileinfo;
    string p;

    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1L) {
        do
        {
            if ((fileinfo.attrib &  _A_SUBDIR)) {
            } else {
                files.push_back(fileinfo.name);
                pathfiles.push_back(p.assign(path).append("\\").append(fileinfo.name));
            }
        } while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
}
#endif


#ifdef _WIN32
#include <windows.h>
#include <tchar.h>

#ifndef _delayimp_h
extern"C"IMAGE_DOS_HEADER __ImageBase;
#endif

char* ConvertLPWSTRToLPSTR(LPWSTR lpwszStrIn) {
	LPSTR pszOut = NULL;
	if (lpwszStrIn != NULL) {
		int nInputStrLen = wcslen(lpwszStrIn);

		// Double NULL Termination
		int nOutputStrLen = WideCharToMultiByte(CP_ACP, 0, lpwszStrIn, nInputStrLen, NULL, 0, 0, 0) + 2;
		pszOut = new char[nOutputStrLen];

		if (pszOut) {
			memset(pszOut, 0x00, nOutputStrLen);
			WideCharToMultiByte(CP_ACP, 0, lpwszStrIn, nInputStrLen, pszOut, nOutputStrLen, 0, 0);
		}
	}
	return pszOut;
}

string GetDLLPath() {
	char *strpath;
	TCHAR result[MAX_PATH];
	HMODULE hModule = reinterpret_cast<HMODULE>(&__ImageBase);

	if (GetModuleFileName(hModule, (LPWSTR)result, MAX_PATH)) {
		(_tcsrchr(result, _T('\\')))[1] = 0;
	}
	return Wide2MultiByte(result, CP_ACP);
}

wstring wGetDLLPath() {
	wchar_t result[MAX_PATH];
	HMODULE hModule = reinterpret_cast<HMODULE>(&__ImageBase);

	if (GetModuleFileNameW(hModule, (LPWSTR)result, MAX_PATH)) {
		(_tcsrchr(result, _T('\\')))[1] = 0;
	}
	return std::wstring(result);
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

#endif


string GetFileNameWithNoSuffix(const string filePathName) {
	std::string filename;
	if (filePathName.size() < 0)
		return "";
	string::size_type is = filePathName.find_last_of('/');
    if (is == string::npos)
	    is = filePathName.find_last_of('\\');
	string::size_type ie = filePathName.find_last_of('.');

	if ((is != string::npos) && (ie != string::npos))
		filename = filePathName.substr(is + 1, ie - is - 1);

	return filename;
}

string GetFileNameByFilePathName(const string filePathName) {
    std::string filename;
    if (filePathName.size() < 0)
        return "";
	string::size_type ix = filePathName.find_last_of('/');
    if (ix == string::npos)
	    ix = filePathName.find_last_of('\\');

    if (ix != string::npos)
        filename = filePathName.substr(ix + 1, filePathName.size() - ix);

    return filename;
}

string GetFilePathByFilePathName(const string filePathName) {
    std::string filePath;
    if (filePathName.size() < 0)
        return "";
	string::size_type ix = filePathName.find_last_of('/');
    if (ix == string::npos)
	    ix = filePathName.find_last_of('\\');

    if (ix != string::npos)
        filePath = filePathName.substr(0, ix + 1);

    return filePath;
}

std::vector<std::string> Split(std::string str, std::string pattern) {
    std::string::size_type pos;
    std::vector<std::string> result;
    str += pattern;
    int size = str.size();

    for (int i = 0; i<size; i++) {
        pos = str.find(pattern, i);
        if (pos<size) {
            std::string s = str.substr(i, pos - i);
            result.push_back(s);
            i = pos + pattern.size() - 1;
        }
    }
    return result;
}

void MkDstDir(const char * dstdir) {
    int ret = -1;
    string cmd;

    if (access(dstdir, 0) != -1) {
#ifdef _LINUX_
        cmd = "rm -rf ";
        cmd += dstdir;
        system(cmd.c_str());
#else
		cmd = "rd /s/q ";
		cmd += dstdir;
		system(cmd.c_str());
#endif
    }


    //create dst
    //struct _stat fileStat;
    //_stat(dstdir, &fileStat);
    //if (fileStat.st_mode & _S_IFDIR != 0)
    if (access(dstdir, 0) == -1) {
        ret = MKDIR(dstdir);
        printf("%s ret:%d\n", dstdir, ret);
    }

    //create dst/Annotations/
    std::string Ann_dir = dstdir;
    Ann_dir += "/Annotations/";
    if (access(Ann_dir.c_str(), 0) == -1) {
        ret = MKDIR(Ann_dir.c_str());
        printf("%s ret:%d\n", Ann_dir.c_str(), ret);
    }

    //create dst/AnnotationJpegs/
    std::string AnnJpg_dir = dstdir;
    AnnJpg_dir += "/AnnotationJpegs/";
    if (access(AnnJpg_dir.c_str(), 0) == -1) {
        ret = MKDIR(AnnJpg_dir.c_str());
        printf("%s ret:%d\n", AnnJpg_dir.c_str(), ret);
    }

    //create dst/JPEGImages/
    std::string Jpg_dir = dstdir;
    Jpg_dir += "/JPEGImages/";
    if (access(Jpg_dir.c_str(), 0) == -1) {
        ret = MKDIR(Jpg_dir.c_str());
        printf("%s ret:%d\n", Jpg_dir.c_str(), ret);
    }
}

//get a buffer and chang ping pang
Mat PingPangBuffer::getBuffer() {
	if (isPing) {
		isPing = false;
		return pingBuffer;
	} else {
		isPing = true;
		return pangBuffer;
	}
}

PingPangBuffer::PingPangBuffer() {
	isPing = true;
}


#ifdef _LINUX_
void paDrawString(Mat& dst, const char* str, Point org, Scalar color, int fontSize, bool italic, bool underline) {
}
#else
void GetStringSize(HDC hDC, const char* str, int* w, int* h) {
	SIZE size;
	GetTextExtentPoint32A(hDC, str, strlen(str), &size);
	if (w != 0) *w = size.cx;
	if (h != 0) *h = size.cy;
}

void paDrawString(Mat& dst, const char* str, Point org, Scalar color, int fontSize, bool italic, bool underline) {
	CV_Assert(dst.data != 0 && (dst.channels() == 1 || dst.channels() == 3));

	int x, y, r, b;
	if (org.x > dst.cols || org.y > dst.rows) return;
	x = org.x < 0 ? -org.x : 0;
	y = org.y < 0 ? -org.y : 0;

	LOGFONTA lf;
	lf.lfHeight = -fontSize;
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = 5;
	lf.lfItalic = italic;  //斜体  
	lf.lfUnderline = underline;   //下划线  
	lf.lfStrikeOut = 0;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = 0;
	lf.lfClipPrecision = 0;
	lf.lfQuality = PROOF_QUALITY;
	lf.lfPitchAndFamily = 0;
	//strcpy(lf.lfFaceName, "华文行楷");
	strcpy(lf.lfFaceName, "微软雅黑");

	HFONT hf = CreateFontIndirectA(&lf);
	HDC hDC = CreateCompatibleDC(0);
	HFONT hOldFont = (HFONT)SelectObject(hDC, hf);

	int strBaseW = 0, strBaseH = 0;
	int singleRow = 0;
	char buf[1 << 12];
	strcpy(buf, str);

	//处理多行  
	{
		int nnh = 0;
		int cw, ch;
		const char* ln = strtok(buf, "\n");
		while (ln != 0) {
			GetStringSize(hDC, ln, &cw, &ch);
			strBaseW = max(strBaseW, cw);
			strBaseH = max(strBaseH, ch);

			ln = strtok(0, "\n");
			nnh++;
		}
		singleRow = strBaseH;
		strBaseH *= nnh;
	}

	if (org.x + strBaseW < 0 || org.y + strBaseH < 0) {
		SelectObject(hDC, hOldFont);
		DeleteObject(hf);
		DeleteObject(hDC);
		return;
	}

	r = org.x + strBaseW > dst.cols ? dst.cols - org.x - 1 : strBaseW - 1;
	b = org.y + strBaseH > dst.rows ? dst.rows - org.y - 1 : strBaseH - 1;
	org.x = org.x < 0 ? 0 : org.x;
	org.y = org.y < 0 ? 0 : org.y;

	BITMAPINFO bmp = { 0 };
	BITMAPINFOHEADER& bih = bmp.bmiHeader;
	int strDrawLineStep = strBaseW * 3 % 4 == 0 ? strBaseW * 3 : (strBaseW * 3 + 4 - ((strBaseW * 3) % 4));

	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = strBaseW;
	bih.biHeight = strBaseH;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = BI_RGB;
	bih.biSizeImage = strBaseH * strDrawLineStep;
	bih.biClrUsed = 0;
	bih.biClrImportant = 0;

	void* pDibData = 0;
	HBITMAP hBmp = CreateDIBSection(hDC, &bmp, DIB_RGB_COLORS, &pDibData, 0, 0);

	CV_Assert(pDibData != 0);
	HBITMAP hOldBmp = (HBITMAP)SelectObject(hDC, hBmp);

	//color.val[2], color.val[1], color.val[0]  
	SetTextColor(hDC, RGB(255, 255, 255));
	SetBkColor(hDC, 0);
	//SetStretchBltMode(hDC, COLORONCOLOR);  

	strcpy(buf, str);
	const char* ln = strtok(buf, "\n");
	int outTextY = 0;
	while (ln != 0) {
		TextOutA(hDC, 0, outTextY, ln, strlen(ln));
		outTextY += singleRow;
		ln = strtok(0, "\n");
	}
	uchar* dstData = (uchar*)dst.data;
	int dstStep = dst.step / sizeof(dstData[0]);
	unsigned char* pImg = (unsigned char*)dst.data + org.x * dst.channels() + org.y * dstStep;
	unsigned char* pStr = (unsigned char*)pDibData + x * 3;
	for (int tty = y; tty <= b; ++tty) {
		unsigned char* subImg = pImg + (tty - y) * dstStep;
		unsigned char* subStr = pStr + (strBaseH - tty - 1) * strDrawLineStep;
		for (int ttx = x; ttx <= r; ++ttx) {
			for (int n = 0; n < dst.channels(); ++n){
				double vtxt = subStr[n] / 255.0;
				int cvv = vtxt * color.val[n] + (1 - vtxt) * subImg[n];
				subImg[n] = cvv > 255 ? 255 : (cvv < 0 ? 0 : cvv);
			}

			subStr += 3;
			subImg += dst.channels();
		}
	}

	SelectObject(hDC, hOldBmp);
	SelectObject(hDC, hOldFont);
	DeleteObject(hf);
	DeleteObject(hBmp);
	DeleteDC(hDC);
}
#endif
