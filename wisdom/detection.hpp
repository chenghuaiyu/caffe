#ifndef CAFFE_DETECTION_HPP_
#define CAFFE_DETECTION_HPP_

#include <string>
#include <cstring>
#include <stdlib.h>  
#include <string.h>
#include <vector>
#include "opencv2/opencv.hpp"

using namespace std;

#ifndef _LINUX_
#define DllExport __declspec(dllexport)
#else
#define DllExport 
#endif

extern "C" {


    DllExport int DetectionAll(string model, string srcdir, string dstdir, string strgpus, int knifeScore, int scissorScore);


    // 初始化
    //DllExport int DetectionInit(string objectName, string DeviceName);
    //objectName表示检测类型，例如：apple或者apple; gun
    //DeviceName表示设备型号：例如 : GD_AT10080B

    // 检测算法
    //DllExport string Detection(string imagesPath, int missErrorRatio);
    //imagesPath表示图片路径, 例如:D : \1.jpg或者D:\1.jpg; D:\2.jpg
    //missErrorRatio表示疑似度

    // 初始化
    DllExport int DetectionInit(char *objectName, char *DeviceName);
    //objectName表示检测类型，例如：apple或者apple; gun
    //DeviceName表示设备型号：例如 : GD_AT10080B

    // 检测算法
    DllExport char* Detection(char *imagesPath, int missErrorRatio);
    //imagesPath表示图片路径, 例如:D : \1.jpg或者D:\1.jpg; D:\2.jpg
    //missErrorRatio表示疑似度

	// cv::Mat输入检测
	DllExport char* DetectionMat(cv::Mat& org_img, int missErrorRatio);
	//org_img表示输入图片
	//返回各种格式的字符串，比如json,xml
    
	// 把结果画到图片上
	DllExport bool DetectionDraw(cv::Mat& org_img, char* pBox);
	// org_img，结果就在这个图片上
	// pBox， DetectionMat返回的结果

    // 按照voc的格式，保存所有结果，包括JPEGImages/Annotations/AnnotationJpegs
	DllExport void DetectionSave(string src, string dst, string fn, char* pBox, int savenobox);
    // src 源目录
    // dst 目标目录
    // fn 文件名
    // pBox, Detection返回的结果
    //savenobox，没有框时，是否保存AnnotationJpegs信息

    // 释放
    DllExport void DetectionUnInit();

}  // namespace caffe


#endif  // CAFFE_DETECTION_HPP_
