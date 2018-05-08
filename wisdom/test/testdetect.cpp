#ifndef _LINUX_
#include "windows.h"
#include <tchar.h>
#include <io.h>
#else
#include "../detection.hpp"
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

using namespace std;
using namespace cv;

#ifndef _LINUX_

typedef int(_cdecl *LPDETECTIONINIT)(char* objectName, char* DeviceName);
typedef char*(_cdecl *LPDETECTION)(char* imagesPath, int missErrorRatio);
typedef void (_cdecl *LPDETECTIONUNINIT)();
typedef char*(_cdecl *LPDETECTIONMAT)(cv::Mat& org_img, int missErrorRatio);
typedef bool(_cdecl *LPDETECTIONDRAW)(cv::Mat& org_img, char* pBox);
typedef void(_cdecl *LPDETECTIONSAVE)(string src, string dst, string fn, char* pBox, int savenobox);

LPDETECTIONINIT 		m_lpDetectionInit;
LPDETECTION				m_lpDetection;
LPDETECTIONUNINIT		m_lpDetectionUnInit;
LPDETECTIONMAT			m_lpDetectionMat;
LPDETECTIONDRAW			m_lpDetectionDraw;
LPDETECTIONSAVE			m_lpDetectionSave;

HINSTANCE hmod;

#endif

int OsInitEnv(void)
{
#ifndef _LINUX_
	hmod = ::LoadLibrary(_TEXT("detection.dll"));
	if (NULL == hmod)
	{
	    std::cout << "Load DLL fail"  << endl;
	    return -1;
	}
	
	m_lpDetectionInit = (LPDETECTIONINIT)GetProcAddress(hmod, "DetectionInit");        
	m_lpDetection = (LPDETECTION)GetProcAddress(hmod, "Detection");                  
	m_lpDetectionUnInit = (LPDETECTIONUNINIT)GetProcAddress(hmod, "DetectionUnInit");
	m_lpDetectionMat = (LPDETECTIONMAT)GetProcAddress(hmod, "DetectionMat");
	m_lpDetectionDraw = (LPDETECTIONDRAW)GetProcAddress(hmod, "DetectionDraw");
	m_lpDetectionSave = (LPDETECTIONSAVE)GetProcAddress(hmod, "DetectionSave");
	
	if (NULL == m_lpDetectionInit || NULL == m_lpDetection || NULL == m_lpDetectionUnInit 
		|| NULL == m_lpDetectionMat || NULL == m_lpDetectionDraw || NULL == m_lpDetectionSave)
		return -1;
	else
		return 0;			
#else
	return 0;
#endif	
}

void OsUninitEnv(void)
{
#ifndef _LINUX_
	FreeLibrary(hmod);
#else

#endif		
}


int OsDetectionInit(char *objectName, char *DeviceName)
{
#ifndef _LINUX_
	return m_lpDetectionInit(objectName, DeviceName);
#else
	return DetectionInit(objectName, DeviceName);
#endif
}

char* OsDetectionMat(cv::Mat& org_img, int missErrorRatio)
{
#ifndef _LINUX_
	return m_lpDetectionMat(org_img, missErrorRatio);
#else
	return DetectionMat(org_img, missErrorRatio);
#endif	
}

char* OsDetection(char *imagesPath, int missErrorRatio)
{
#ifndef _LINUX_
	return m_lpDetection(imagesPath, missErrorRatio);
#else
	return Detection(imagesPath, missErrorRatio);
#endif	
}

void OsDetectionUnInit(void)
{
#ifndef _LINUX_
	m_lpDetectionUnInit();
#else
	DetectionUnInit();
#endif	
}

bool OsDetectionDraw(cv::Mat& org_img, char* pBox)
{
#ifndef _LINUX_
	return m_lpDetectionDraw(org_img, pBox);
#else
	return DetectionDraw(org_img, pBox);
#endif		
}

void OsDetectionSave(string src, string dst, string fn, char* pBox, int savenobox)
{
#ifndef _LINUX_
	m_lpDetectionSave(src, dst, fn, pBox, savenobox);
#else
	DetectionSave(src, dst, fn, pBox, savenobox);
#endif	
}

int Test_Detection_video(string dstDir, string srcFilename, int missErrorRatio)
{
	VideoCapture cap;
	int total_frames, processed_frames = 0;
	Mat frame;
	VideoWriter videoWriter;
	float fps;
	clock_t start_clock;
	float time = 0;
	float avgFps = 0;
	char* out;
	
	//初始化解码器
	if (!cap.open(srcFilename))
	{
		std::cout << "Can't open video file " << srcFilename << endl;
		return -1;
	}
	
	total_frames = cap.get(CV_CAP_PROP_FRAME_COUNT);
	fps = cap.get(CV_CAP_PROP_FPS);
	processed_frames = 0;
	string fileNameOnly = GetFileNameWithNoSuffix(srcFilename.c_str());
	string dst_fn = "";
	dst_fn += dstDir;
	dst_fn += "/AnnotationJpegs/";
	dst_fn += fileNameOnly;
	dst_fn += ".avi";

	//to get width and height
	cap >> frame;
	//初始化视频生成器
	videoWriter.open(dst_fn.c_str(), CV_FOURCC('M', 'J', 'P', 'G'), fps, cvSize(frame.size().width, frame.size().height));
	cap.set(CV_CAP_PROP_POS_FRAMES, 0);
	start_clock = clock();
	time = 0;
	avgFps = 0;
	
	while (1)
	{	
		//解码video
		cap >> frame;
		
		if (!frame.empty())
		{
			//AI处理
			out = OsDetectionMat(frame, missErrorRatio);
			if (NULL != out)
			{
				//绘制框
				OsDetectionDraw(frame, out);
			}
			//if (!result)
			{
				//计算平均帧率
				time = (static_cast<double>(clock() - start_clock) /
					CLOCKS_PER_SEC);
	
				avgFps = processed_frames / time;
				int fontface = FONT_HERSHEY_SIMPLEX;
				double scale = 1;
				int thickness = 2;
				int baseline = 0;
				char buffer[100];
				memset(buffer, 0, 100 * sizeof(char));
				sprintf(buffer, "Time:%.2f s, Frames:%d, AVG_FPS: %.2f", time, processed_frames, avgFps);
				Size text = getTextSize(buffer, fontface, scale, thickness,
					&baseline);
				//绘制帧率信息
				rectangle(frame, Point(0, 0),
					Point(text.width, text.height + baseline),
					CV_RGB(255, 255, 255), CV_FILLED);
				putText(frame, buffer, Point(0, text.height + baseline / 2.),
					fontface, scale, CV_RGB(0, 0, 0), thickness, 8);
				
				//显示
				imshow("video", frame);
				waitKey(1);
				//写文件
				videoWriter.write(frame);
			}
		}
		
		++processed_frames;
		
		if (processed_frames == total_frames)
		{
			//释放解码器和视频生成器
			cap.release();
			videoWriter.release();
			break;
		}
	}
	return 0;
}

void Test_Detection_image(string srcdir, string dstDir, string srcFilename, int missErrorRatio)
{
	char* out;
	
	//解码image文件，AI处理
	out = OsDetection((char *)srcFilename.c_str(), missErrorRatio);
	if (NULL != out)
	{
		//绘制框，写文件
		OsDetectionSave(srcdir, dstDir, srcFilename, out, 0);
	}
}

void Test_Detection(string srcdir, string dstdir, int missErrorRatio)
{
	int i;
	char* objectName = "test";
	char* DeviceName = "gpu0";
	vector<string> files;

	if (!IsDirExist(srcdir))
	{
		std::cout << "srcdir file is not exist" << endl;
		return;
	}

	MkDstDir((const char *)dstdir.c_str());

	//初始化OS环境
	if (OsInitEnv())
	{
			std::cout << "OsInitEnv Error" << endl;
	    return;
	}

	//初始化AI
	if (OsDetectionInit(objectName, DeviceName) == 0)
	{
		GetFiles(srcdir, files);
		
		for (i = 0; i < files.size(); i++)
		{
			printf("%s\n", files[i].c_str());
			string suffixStr = files[i].substr(files[i].find_last_of('.') + 1);
			if ((suffixStr == "mp4") || (suffixStr == "avi"))
			{
				//解码video，AI处理，绘制框，显示，写文件
				Test_Detection_video(dstdir, files[i], missErrorRatio);
			}
			else
			{
				//解码image文件，AI处理，绘制框，写文件
				Test_Detection_image(srcdir, dstdir, files[i], missErrorRatio);
			}
		}
		OsDetectionUnInit();
	}
	
	OsUninitEnv(); 
}


int main(int argc, char** argv)
{
	string srcdir, dstdir;
	if (argc < 3)
	{
		std::cout << "parameter is not enough:" << endl;
	}
	else
	{
		srcdir = argv[1];
		dstdir = argv[2];
		
		std::cout << "srcdir:" << srcdir << endl;
		std::cout << "dstdir:" << dstdir << endl;
	
		Test_Detection(srcdir, dstdir, 0);
	}
}



