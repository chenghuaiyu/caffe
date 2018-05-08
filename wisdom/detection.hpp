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


    // ��ʼ��
    //DllExport int DetectionInit(string objectName, string DeviceName);
    //objectName��ʾ������ͣ����磺apple����apple; gun
    //DeviceName��ʾ�豸�ͺţ����� : GD_AT10080B

    // ����㷨
    //DllExport string Detection(string imagesPath, int missErrorRatio);
    //imagesPath��ʾͼƬ·��, ����:D : \1.jpg����D:\1.jpg; D:\2.jpg
    //missErrorRatio��ʾ���ƶ�

    // ��ʼ��
    DllExport int DetectionInit(char *objectName, char *DeviceName);
    //objectName��ʾ������ͣ����磺apple����apple; gun
    //DeviceName��ʾ�豸�ͺţ����� : GD_AT10080B

    // ����㷨
    DllExport char* Detection(char *imagesPath, int missErrorRatio);
    //imagesPath��ʾͼƬ·��, ����:D : \1.jpg����D:\1.jpg; D:\2.jpg
    //missErrorRatio��ʾ���ƶ�

	// cv::Mat������
	DllExport char* DetectionMat(cv::Mat& org_img, int missErrorRatio);
	//org_img��ʾ����ͼƬ
	//���ظ��ָ�ʽ���ַ���������json,xml
    
	// �ѽ������ͼƬ��
	DllExport bool DetectionDraw(cv::Mat& org_img, char* pBox);
	// org_img������������ͼƬ��
	// pBox�� DetectionMat���صĽ��

    // ����voc�ĸ�ʽ���������н��������JPEGImages/Annotations/AnnotationJpegs
	DllExport void DetectionSave(string src, string dst, string fn, char* pBox, int savenobox);
    // src ԴĿ¼
    // dst Ŀ��Ŀ¼
    // fn �ļ���
    // pBox, Detection���صĽ��
    //savenobox��û�п�ʱ���Ƿ񱣴�AnnotationJpegs��Ϣ

    // �ͷ�
    DllExport void DetectionUnInit();

}  // namespace caffe


#endif  // CAFFE_DETECTION_HPP_
