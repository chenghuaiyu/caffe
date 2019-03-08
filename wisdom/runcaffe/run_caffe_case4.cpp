
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
#include "opencv2/opencv.hpp"

#include "../common.h"
#include "../preprocess.hpp"


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

using namespace std;
using namespace cv;

#define IMG_SCALE (1.5)

cv::Mat PreprocessNew(const cv::Mat &img)
{
    cv::Mat img_resize;
    cv::resize(img, img_resize, cv::Size(int(img.cols*IMG_SCALE),int(img.rows*IMG_SCALE)));
    
    /* Convert the input image to the input image format of the network. */
    cv::Mat sample;
    if (img_resize.channels() == 4)
        cv::cvtColor(img_resize, sample, cv::COLOR_BGRA2BGR);
    else if (img_resize.channels() == 1)
        cv::cvtColor(img_resize, sample, cv::COLOR_GRAY2BGR);
    else
        sample = img_resize;

    cv::Mat sample_float;
    sample.convertTo(sample_float, CV_32FC3);

    return sample_float;
}

vector<DETECT_BOX_S> run_caffe_once_case4(cv::Mat& org_img, cv::Rect& padrect, MODEL_INFO_S *pstInfo)
{
    Net<float> *caffe_net = pstInfo->caffe_net;
    vector<DETECT_BOX_S> retbox;
    /* run caffe */
    caffe_net->Forward();

    /* draw in image */
    Blob<float>* rect = caffe_net->output_blobs()[0];
    int num_det = rect->height();
    std::cout << "num_det:" << num_det << endl;
    const float* result_vec = rect->cpu_data();
    for (int i = 0; i < num_det; i++)
    {
        DETECT_BOX_S tmpbox;
        int label = result_vec[i * 7 + 1]; 
        float score = result_vec[i * 7 + 2];
        if (label < 0)
        {
            continue;
        }
		if (score < pstInfo->scores[label])
        {
            continue;
        }
        cv::Rect cvrect;
        cvrect.x = static_cast<int>(result_vec[i * 7 + 3] * padrect.width);
        cvrect.y = static_cast<int>(result_vec[i * 7 + 4] * padrect.height);
        cvrect.width = static_cast<int>(result_vec[i * 7 + 5] * padrect.width) - cvrect.x;
        cvrect.height = static_cast<int>(result_vec[i * 7 + 6] * padrect.height) - cvrect.y;
        cvrect.x += padrect.x;
        cvrect.y += padrect.y;
        //std::cout<<"cols:"<<org_img.cols<<endl;
        //std::cout<<"rows:"<<org_img.rows<<endl;
        //std::cout<<"score:"<<score<<endl;
        //std::cout<<"x:"<<cvrect.x<<endl;
        //std::cout<<"y:"<<cvrect.y<<endl;
        //std::cout<<"width:"<<cvrect.width<<endl;
        //std::cout<<"height:"<<cvrect.height<<endl;
        tmpbox.label = result_vec[i * 7 + 1];
        tmpbox.score = result_vec[i * 7 + 2];
        tmpbox.x = max(min(int(cvrect.x/IMG_SCALE), org_img.cols), 0);
        tmpbox.y = max(min(int(cvrect.y/IMG_SCALE), org_img.rows), 0);
        tmpbox.w = max(min(int(cvrect.width/IMG_SCALE), org_img.cols), 0);
        tmpbox.h = max(min(int(cvrect.height/IMG_SCALE), org_img.rows), 0);
        retbox.push_back(tmpbox);
    }
    return retbox;
}

vector<DETECT_BOX_S> run_caffe_pad_case4(cv::Mat& org_img, MODEL_INFO_S *pstInfo)
{
    Blob<float>* input_layer = pstInfo->caffe_net->input_blobs()[0];
    int width = input_layer->width();
    int height = input_layer->height();
    Net<float> *caffe_net = pstInfo->caffe_net;
    vector<DETECT_BOX_S> retbox;
    std::cout << "run_caffe_pad_case4" << endl;
    cv::Mat img = PreprocessNew(org_img);
    int srch = img.rows;
    int srcw = img.cols;

    int w = width;
    int h = height;
    int w_off = (w - srcw) / 2;
    int h_off = (h - srch) / 2;
    std::cout << "w_off:" << w_off << endl;
    std::cout << "w:" << w << endl;
    std::cout << "h_off:" << h_off << endl;
    std::cout << "h:" << h << endl;

    cv::Mat img_pad;
    img_pad.create(h, w, img.type());
    img_pad.setTo(cv::Scalar(pstInfo->means[0],pstInfo->means[1],pstInfo->means[2]));
    cv::Rect bbox_roi(w_off, h_off, srcw, srch);
    img.copyTo(img_pad(bbox_roi));//拷贝到中间
    //imwrite("tmp.png",img_pad);

    std::cout << "img_pad.rows:" << img_pad.rows << endl;
    std::cout << "img_pad.cols:" << img_pad.cols << endl;
    std::vector<cv::Mat> input_channels;
    WrapInputLayer(img_pad, &input_channels, pstInfo);

    cv::Rect padrect = cv::Rect(-w_off, -h_off, w, h);//w_off,h_off对框进行修正
    retbox = run_caffe_once_case4(org_img, padrect, pstInfo);
    return retbox;
}

vector<DETECT_BOX_S> run_caffe_other_case4(cv::Mat& org_img, MODEL_INFO_S *pstInfo)
{
    Blob<float>* input_layer = pstInfo->caffe_net->input_blobs()[0];
    int width = input_layer->width();
    int height = input_layer->height();
    Net<float> *caffe_net = pstInfo->caffe_net;
    vector<DETECT_BOX_S> retbox;
    std::cout << "run_caffe_other_case4" << endl;
    cv::Mat img = PreprocessNew(org_img);
    int srch = img.rows;
    int srcw = img.cols;

    float ratiow = float(srcw)/width;
    float ratioh = float(srch)/height;
    float ratio = ratiow > ratioh ? ratiow : ratioh;
    
    int w = width*ratio;
    int h = height*ratio;
    w = max(srcw, w);
    h = max(srch, h);
    int w_off = (w - srcw) / 2;
    int h_off = (h - srch) / 2;
    std::cout << "w_off:" << w_off << endl;
    std::cout << "w:" << w << endl;
    std::cout << "h_off:" << h_off << endl;
    std::cout << "h:" << h << endl;

    cv::Mat img_pad;
    img_pad.create(h, w, img.type());
    img_pad.setTo(cv::Scalar(pstInfo->means[0],pstInfo->means[1],pstInfo->means[2]));
    cv::Rect bbox_roi(w_off, h_off, srcw, srch);
    img.copyTo(img_pad(bbox_roi));//拷贝到中间
    //imwrite("tmp.png",img_pad);

    std::cout << "img_pad.rows:" << img_pad.rows << endl;
    std::cout << "img_pad.cols:" << img_pad.cols << endl;
    std::vector<cv::Mat> input_channels;
    WrapInputLayer(img_pad, &input_channels, pstInfo);

    cv::Rect padrect = cv::Rect(-w_off, -h_off, w, h);//w_off,h_off对框进行修正
    retbox = run_caffe_once_case4(org_img, padrect, pstInfo);
    return retbox;
}

vector<DETECT_BOX_S> run_caffe_case4(cv::Mat& org_img, MODEL_INFO_S *pstInfo)
{
    vector<DETECT_BOX_S> retbox;
    Blob<float>* input_layer = pstInfo->caffe_net->input_blobs()[0];
    input_layer->Reshape(1, input_layer->shape(1),
                       (int(org_img.rows*IMG_SCALE)+127)&(~127), 
                       (int(org_img.cols*IMG_SCALE)+127)&(~127));
    /* Forward dimension change to all layers. */
    pstInfo->caffe_net->Reshape();
  
    int width = input_layer->width();
    int height = input_layer->height();
    
    if ((org_img.rows < height) && (org_img.cols < width))
    {
        retbox = run_caffe_pad_case4(org_img, pstInfo);
    }
    else
    {
        retbox = run_caffe_other_case4(org_img, pstInfo);
    }

	return retbox;
}

