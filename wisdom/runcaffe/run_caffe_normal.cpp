
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
#define GLOG_NO_ABBREVIATED_SEVERITIES
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

vector<DETECT_BOX_S> run_caffe_once_normal(Net<float> *caffe_net, cv::Mat& org_img, cv::Rect& padrect, vector<float> scorethr) {
    vector<DETECT_BOX_S> retbox;
    /* run caffe */
    caffe_net->Forward();

    /* draw in image */
    Blob<float>* rect = caffe_net->output_blobs()[0];
    int num_det = rect->height();
    std::cout << "num_det:" << num_det << endl;
    const float* result_vec = rect->cpu_data();
    for (int i = 0; i < num_det; i++) {
        DETECT_BOX_S tmpbox;
        int label = result_vec[i * 7 + 1]; 
        float score = result_vec[i * 7 + 2];
        if (label < 0) {
            continue;
        }
        if (score < scorethr[label]) {
            continue;
        }

        cv::Rect cvrect;
		cvrect.x = max(min(static_cast<int>(result_vec[i * 7 + 3] * padrect.width),padrect.width), 0);
		cvrect.y = max(min(static_cast<int>(result_vec[i * 7 + 4] * padrect.height), padrect.height), 0);
		cvrect.width = max(min(static_cast<int>(result_vec[i * 7 + 5] * padrect.width), padrect.width), 0) - cvrect.x;
		cvrect.height = max(min(static_cast<int>(result_vec[i * 7 + 6] * padrect.height), padrect.height), 0) - cvrect.y;
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
        tmpbox.x = max(min(cvrect.x, org_img.cols), 0);
        tmpbox.y = max(min(cvrect.y, org_img.rows), 0);
        tmpbox.w = max(min(cvrect.width, org_img.cols), 0);
        tmpbox.h = max(min(cvrect.height, org_img.rows), 0);
        retbox.push_back(tmpbox);
    }
    return retbox;
}

vector<DETECT_BOX_S> run_caffe_normal(cv::Mat& org_img, MODEL_INFO_S *pstInfo) {
    vector<DETECT_BOX_S> retbox;
    std::cout << "run_caffe_others" << endl;
    cv::Mat img = Preprocess(org_img);

    std::vector<cv::Mat> input_channels;
	WrapInputLayer(img, &input_channels, pstInfo);

    cv::Rect padrect = cv::Rect(0, 0, img.cols, img.rows);
	retbox = run_caffe_once_normal(pstInfo->caffe_net, org_img, padrect, pstInfo->scores);
    return retbox;
}
        

