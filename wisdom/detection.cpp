
#include "boost/algorithm/string.hpp"
#include "caffe/caffe.hpp"
#include "caffe/util/signal_handler.h"
#include "caffe/util/bbox_util.hpp"
#include "caffe/blob.hpp"
#include "caffe/common.hpp"
#include "caffe/layer.hpp"
#include "caffe/util/db.hpp"
#include "caffe/util/io.hpp"
#include "caffe/util/upgrade_proto.hpp"
#include "google/protobuf/text_format.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h> 
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <utility>
#include <map>
#include <string>
#include <vector>
#include <iostream>  
#include <stdio.h> 
#include <stdlib.h>  
#ifndef _LINUX_
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "opencv2/opencv.hpp"

#include "common.h"
#include "runcaffe/run_caffe.hpp"
#include "detection.hpp"
#include "util/util.h"
#include "config/get_config.h"
#include "decryption/decryption.h"
#include "postprocess/postprocess.h"
#include "output/output.h"



using caffe::Blob;
using caffe::Caffe;
using caffe::Net;
using caffe::Layer;
using caffe::Solver;
using caffe::shared_ptr;
using caffe::string;
using caffe::Timer;
using caffe::vector;
using std::ostringstream;
using ::google::protobuf::Message;
using caffe::LabelMap;


using namespace std;
using namespace cv;

typedef struct {
    MODEL_INFO_S stMainModel;
    MODEL_INFO_S stSubModel;
}DETECT_INFO_S;

static DETECT_INFO_S g_stInfo;

#define CONFIG_FILENAME "config/config"
#define SUBCONFIG_FILENAME "config/subconfig"

#ifdef _WIN32
	#include <windows.h>
	#include <tchar.h>

	#ifndef _delayimp_h
		extern"C"IMAGE_DOS_HEADER __ImageBase;
	#endif

	char* ConvertLPWSTRToLPSTR(LPWSTR lpwszStrIn)
	{
		LPSTR pszOut = NULL;
		if (lpwszStrIn != NULL)
		{
			int nInputStrLen = wcslen(lpwszStrIn);

			// Double NULL Termination
			int nOutputStrLen = WideCharToMultiByte(CP_ACP, 0, lpwszStrIn, nInputStrLen, NULL, 0, 0, 0) + 2;
			pszOut = new char[nOutputStrLen];

			if (pszOut)
			{
				memset(pszOut, 0x00, nOutputStrLen);
				WideCharToMultiByte(CP_ACP, 0, lpwszStrIn, nInputStrLen, pszOut, nOutputStrLen, 0, 0);
			}
		}
		return pszOut;
	}
	string GetDLLPath()
	{
		char *strpath;
		TCHAR result[MAX_PATH];
		HMODULE hModule = reinterpret_cast<HMODULE>(&__ImageBase);

		if (GetModuleFileName(hModule, (LPWSTR)result, MAX_PATH))
		{
			(_tcsrchr(result, _T('\\')))[1] = 0;
		}
		strpath = ConvertLPWSTRToLPSTR(result);
		return std::string(strpath);
	}
#endif


int GetConfig(DETECT_INFO_S *pstInfo)
{
	int ret;
	string strDLLPath;
	string strConfigFilePath;
	string strSubConfigFilePath;
#ifdef _WIN32
	strDLLPath = GetDLLPath();
	strConfigFilePath = strDLLPath;
	strSubConfigFilePath = strDLLPath;
#else
	strDLLPath = GetAppPath();
	strConfigFilePath = strDLLPath;
	strSubConfigFilePath = strDLLPath;
#endif
	std::cout << "DLL Path = " << strConfigFilePath.c_str() << endl;
	strConfigFilePath.append(CONFIG_FILENAME);

	ret = GetModelInfo(strConfigFilePath, &pstInfo->stMainModel, strDLLPath);

	if (pstInfo->stMainModel.postmode.find("classify") != string::npos)
	{
		strSubConfigFilePath.append(SUBCONFIG_FILENAME);
		ret = GetModelInfo(strSubConfigFilePath, &pstInfo->stSubModel, strDLLPath);
	}

	return ret;
}



// Parse GPU ids or use all available devices
static void get_gpus(vector<int>* gpus, string strgpus)
{
    if (strgpus == "all") 
    {
        int count = 0;
#ifndef CPU_ONLY
        CUDA_CHECK(cudaGetDeviceCount(&count));
#else
        NO_GPU;
#endif
        for (int i = 0; i < count; ++i) 
        {
            gpus->push_back(i);
        }
    } 
    else if (strgpus.size())
    {
        vector<string> strings;
        boost::split(strings, strgpus, boost::is_any_of(","));
        for (int i = 0; i < strings.size(); ++i) 
        {
            gpus->push_back(boost::lexical_cast<int>(strings[i]));
        }
    } 
    else 
    {
        CHECK_EQ(gpus->size(), 0);
    }
}

int InitModel(MODEL_INFO_S *pstInfo)
{
    // Set device id and mode
    vector<int> gpus;
    get_gpus(&gpus, pstInfo->gpus);
    if (gpus.size() != 0) {
        LOG(INFO) << "Use GPU with device ID " << gpus[0];
#ifndef CPU_ONLY
        cudaDeviceProp device_prop;
        cudaGetDeviceProperties(&device_prop, gpus[0]);
        LOG(INFO) << "GPU device name: " << device_prop.name;
        std::cout << "caffe: GPU device name: " << device_prop.name << "\n";
#endif
        Caffe::SetDevice(gpus[0]);
        Caffe::set_mode(Caffe::GPU);
    }
    else {
        LOG(INFO) << "Use CPU.";
        Caffe::set_mode(Caffe::CPU);
    }

    // set caffe
    if (pstInfo->encry == "none")
    {
        pstInfo->caffe_net = new Net<float>(pstInfo->model, caffe::TEST);
        pstInfo->caffe_net->CopyTrainedLayersFrom(pstInfo->weight);
        return SUCCESS;
    }
    else
    {
        char* modelbuf;
        char* weightbuf;
        int modellen;
        int weightlen;
		if (SUCCESS != VimDecrypt(pstInfo->weight.c_str(), &modelbuf, &weightbuf,
                &modellen, &weightlen, 
                pstInfo->encry.c_str(), pstInfo->key.c_str()))
        {
            return FAIL;
        }
        // load model
        caffe::NetParameter model_param;
        string strmodel(modelbuf, modelbuf + modellen);
        CHECK(google::protobuf::TextFormat::ParseFromString(strmodel, &model_param));

        // load prototxt
        caffe::NetParameter weight_param;
        google::protobuf::io::CodedInputStream coded_input(
                            (google::protobuf::uint8 *)weightbuf, 
                            weightlen);
        coded_input.SetTotalBytesLimit(536870912, 536870912);
        weight_param.ParseFromCodedStream(&coded_input);

        // Instantiate the caffe net.
        pstInfo->caffe_net = new Net<float>(model_param);
        
        if (weightbuf)
        {
            free(weightbuf);
        }
        if (modelbuf)
        {
            free(modelbuf);
        }
        
        if (pstInfo->caffe_net)
        {
            pstInfo->caffe_net->CopyTrainedLayersFrom(weight_param);
            return SUCCESS;
        }
        else
        {
            return FAIL;
        }
    }
}

int InitDetector(DETECT_INFO_S *pstInfo)
{
    int ret;
    
    ret = InitModel(&pstInfo->stMainModel);

    if (pstInfo->stMainModel.postmode.find("classify") != string::npos)
    {
        ret = InitModel(&pstInfo->stSubModel);
    }
    
    return ret;
}

int DetectionInit(char *objectName, char *DeviceName)
{
    DETECT_INFO_S *pstInfo = &g_stInfo;
    memset(pstInfo, 0, sizeof(DETECT_INFO_S));

    // get config
    if (GetConfig(pstInfo))
    {
        return FAIL;
    }

    if (InitDetector(pstInfo))
    {
        return FAIL;
    }
	return SUCCESS;
} 




char* DetectionMat(cv::Mat& org_img, int missErrorRatio)
{
    MODEL_INFO_S *pstMainInfo = &g_stInfo.stMainModel;
    MODEL_INFO_S *pstSubInfo = &g_stInfo.stSubModel; 
    MODEL_INFO_S *pstPostInfo = NULL;
	vector<DETECT_BOX_S> retbox;
	vector<DETECT_BOX_S> savebox;

	vector<string> labels;
	if (pstMainInfo->postmode.find("classify") != string::npos)
	{
		labels = pstSubInfo->labels;
        pstPostInfo = &g_stInfo.stSubModel; 
	}
	else
	{
		labels = pstMainInfo->labels;
        pstPostInfo = &g_stInfo.stMainModel; 
	}

    vector<DETECT_FILE_S> totalfiles;
    DETECT_FILE_S tmpfile;
    tmpfile.filename = "name";
    tmpfile.width = 0;
    tmpfile.height = 0;
    tmpfile.boxes.clear();
                
    if (org_img.empty())
    {
        goto out; 
    }
    
    if (org_img.cols == 0 || org_img.rows == 0)
    { 
        goto out; 
    }
    
    tmpfile.width = org_img.cols;
    tmpfile.height = org_img.rows;
        
	retbox = run_caffe(org_img, pstMainInfo);

	savebox = postprocess(retbox, pstMainInfo->postmode, pstPostInfo, org_img);

    if (savebox.size() == 0)
    {
        std::cout << "no box:" << endl;
    }
    else
    {
        tmpfile.boxes.insert(tmpfile.boxes.end(), savebox.begin(), savebox.end());
    }
    
out:
    totalfiles.push_back(tmpfile);
    return Output(totalfiles, pstMainInfo->outmode, labels);
}

bool DetectionDraw(cv::Mat& org_img, char* pOutput)
{
    MODEL_INFO_S *pstMainInfo = &g_stInfo.stMainModel;
    MODEL_INFO_S *pstSubInfo = &g_stInfo.stSubModel; 

    vector<string> labels;
    vector<int> colors;
    if (pstMainInfo->postmode.find("classify") != string::npos)
    {
		labels = pstSubInfo->labels;
		colors = pstSubInfo->colors;
    }
    else
    {
		labels = pstMainInfo->labels;
		colors = pstMainInfo->colors;
    }
    
	vector<DETECT_FILE_S> totalfiles = OutputParse(pOutput, pstMainInfo->outmode, labels);

    for (int i = 0; i < totalfiles.size(); i++)
    {
        vector<DETECT_BOX_S> savebox = totalfiles[i].boxes;
        for (int j = 0; j < savebox.size(); j++)
        {
            int label = static_cast<int>(savebox[j].label);
            float score = static_cast<float>(savebox[j].score);
            int color = colors[label];
    		unsigned int r = (color & 0xff0000) >> 16;
    		unsigned int g = (color & 0x00ff00) >> 8;
    		unsigned int b = (color & 0x0000ff) >> 0;
            cv::Rect cvrect;
            cvrect.x = static_cast<int>(savebox[j].x);
            cvrect.y = static_cast<int>(savebox[j].y);
            cvrect.width = static_cast<int>(savebox[j].w);
            cvrect.height = static_cast<int>(savebox[j].h);
			rectangle(org_img, cvrect, cvScalar(b, g, r), 2);
            int x, y;
            x = (std::min)(cvrect.x, org_img.cols - 20);
			y = (std::max)(cvrect.y, 10);

#ifdef _LINUX_
			cv::putText(org_img, labels[label] + ' ' + std::to_string((int)(score)),
				cvPoint(x, y), cv::FONT_HERSHEY_PLAIN, 1, cvScalar(255, 0, 0), 1, CV_AA);
#else
			string chineseLabel;
			y = (std::max)(cvrect.y - 30, 10);
			if (labels[label] == "none")
			{
				chineseLabel = "异常闯入";
				paDrawString(org_img, chineseLabel.c_str(), cvPoint(x, y), cvScalar(0, 0, 255), 30, false, false);
//				y = (std::max)(cvrect.y - 35, 10);
//				cv::putText(org_img, std::to_string((int)(score)),
//					cvPoint(x, y), cv::FONT_HERSHEY_PLAIN, 1, cvScalar(255, 0, 0), 1, CV_AA);
			}
			else if (labels[label] == "clothe")
			{
				chineseLabel = "未戴头盔";
				paDrawString(org_img, chineseLabel.c_str(), cvPoint(x, y), cvScalar(255, 0, 0), 30, false, false);
//				y = (std::max)(cvrect.y - 35, 10);
//				cv::putText(org_img, std::to_string((int)(score)),
//					cvPoint(x, y), cv::FONT_HERSHEY_PLAIN, 1, cvScalar(255, 0, 0), 1, CV_AA);
			}
			else if (labels[label] == "full")
			{
				chineseLabel = "正常";
				paDrawString(org_img, chineseLabel.c_str(), cvPoint(x, y), cvScalar(255, 0, 0), 30, false, false);
//				y = (std::max)(cvrect.y - 35, 10);
//				cv::putText(org_img, std::to_string((int)(score)),
//					cvPoint(x, y), cv::FONT_HERSHEY_PLAIN, 1, cvScalar(255, 0, 0), 1, CV_AA);
			}
			else
				cv::putText(org_img, labels[label] + ' ' + std::to_string((int)(score)),
				cvPoint(x, y), cv::FONT_HERSHEY_PLAIN, 1, cvScalar(255, 0, 0), 1, CV_AA);
#endif
        }
    }
    
    return true;
}

void DetectionSave(string src, string dst, string fn, char* pBox, int savenobox)
{
	MODEL_INFO_S *pstMainInfo = &g_stInfo.stMainModel;

	OutputSave(src, dst, fn, pBox, savenobox, pstMainInfo->outmode);
}

char* Detection(char *imagesPath, int missErrorRatio)
{
    string dstImagePath;
    string dstImageName;
    std::vector<cv::Mat> input_channels;
    std::vector<std::string> result;
    MODEL_INFO_S *pstMainInfo = &g_stInfo.stMainModel;
    MODEL_INFO_S *pstSubInfo = &g_stInfo.stSubModel;
    MODEL_INFO_S *pstPostInfo = NULL;
    Net<float> *caffe_net = pstMainInfo->caffe_net;

    vector<string> labels;
	if (pstMainInfo->postmode.find("classify") != string::npos)
	{
		labels = pstSubInfo->labels;
        pstPostInfo = &g_stInfo.stSubModel; 
	}
	else
	{
		labels = pstMainInfo->labels;
        pstPostInfo = &g_stInfo.stMainModel; 
	}

    result = Split(imagesPath, ";");
 
    Blob<float>* input_layer = caffe_net->input_blobs()[0];
    int width = input_layer->width();
    int height = input_layer->height();
    
    vector<DETECT_FILE_S> totalfiles;
    for (int j = 0; j < result.size(); j++)
    {
        DETECT_FILE_S tmpfile;
        tmpfile.filename = result[j];
        tmpfile.width = 0;
        tmpfile.height = 0;
        tmpfile.boxes.clear();
        
        if (result[j] == "") //如果文件名是空的，应该不需要什么处理，特别是JSON
        {
            totalfiles.push_back(tmpfile);
            continue;  
        }
        
        cv::Mat org_img = imread(result[j].c_str());
        if (org_img.cols == 0 || org_img.rows == 0)
        { 
            totalfiles.push_back(tmpfile);
            continue;
        }
        tmpfile.width = org_img.cols;
        tmpfile.height = org_img.rows;

        vector<DETECT_BOX_S> retbox;
        retbox = run_caffe(org_img, pstMainInfo);

        vector<DETECT_BOX_S> savebox;
        savebox = postprocess(retbox, pstMainInfo->postmode, pstPostInfo, org_img);
        
        tmpfile.boxes.insert(tmpfile.boxes.end(), savebox.begin(), savebox.end());
        totalfiles.push_back(tmpfile);
    }
    
    return Output(totalfiles, pstMainInfo->outmode, labels);
} 

void DetectionUnInit(void)
{   
    DETECT_INFO_S *pstInfo = &g_stInfo;
    MODEL_INFO_S *pstMainInfo = &g_stInfo.stMainModel;
    MODEL_INFO_S *pstSubInfo = &g_stInfo.stSubModel;
    
    if (pstMainInfo->caffe_net)
    {
        delete pstMainInfo->caffe_net;
    }
    
    if (pstSubInfo->caffe_net)
    {
        delete pstSubInfo->caffe_net;
    }
}

