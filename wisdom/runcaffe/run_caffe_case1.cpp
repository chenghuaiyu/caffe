
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

float BBoxSize(const DETECT_BOX_S& bbox) {
	float width = bbox.w;
	float height = bbox.h;

	return (width + 1) * (height + 1);
}

float JaccardOverlap(const DETECT_BOX_S bbox1, const DETECT_BOX_S bbox2) {
  if (bbox2.x > (bbox1.w+bbox1.x) || (bbox2.w+bbox2.x) < bbox1.x ||
      bbox2.y > (bbox1.h+bbox1.y) || (bbox2.h+bbox2.y) < bbox1.y) {
    return float(0.);
  } else {
    const float inter_xmin = std::max(bbox1.x, bbox2.x);
    const float inter_ymin = std::max(bbox1.y, bbox2.y);
    const float inter_xmax = std::min(bbox1.w+bbox1.x, bbox2.w+bbox2.x);
    const float inter_ymax = std::min(bbox1.h+bbox1.y, bbox2.h+bbox2.y);

    const float inter_width = inter_xmax - inter_xmin;
    const float inter_height = inter_ymax - inter_ymin;
    const float inter_size = inter_width * inter_height;

    const float bbox1_size = BBoxSize(bbox1);
    const float bbox2_size = BBoxSize(bbox2);

    return inter_size / (bbox1_size + bbox2_size - inter_size);
  }
}

vector<DETECT_BOX_S> SortBox(vector<DETECT_BOX_S> box) {
    for (int i = 0; i < box.size(); i++) {
        for (int j = i; j < box.size(); j++) {
            if (box[i].score < box[j].score) {
                DETECT_BOX_S temp = box[i];
                box[i] = box[j];
                box[j] = temp;
            }
        }
    }

    return box;
}

vector<DETECT_BOX_S> GetLabelBox(vector<DETECT_BOX_S> retbox, const float label) {
    vector<DETECT_BOX_S> labelBox;

    for (int i = 0; i < retbox.size(); ++i) {
        if (retbox[i].label == label) {
            labelBox.push_back(retbox[i]);
        }
    }

    labelBox = SortBox(labelBox);

    return labelBox;
}

vector<DETECT_BOX_S> ApplyNMSFast(vector<DETECT_BOX_S> retbox, MODEL_INFO_S *pstInfo) {
    float adaptive_threshold = 0.30;
    vector<DETECT_BOX_S> newretbox;
    
    for (int i = 0; i < pstInfo->labels.size(); i++) {
        vector<DETECT_BOX_S> labelboxesold;
        vector<DETECT_BOX_S> labelboxesnew;
        labelboxesold = GetLabelBox(retbox, (float)i);
        labelboxesnew.clear();

        // Do nms.
        while(labelboxesold.size()) {
            DETECT_BOX_S tmpbox = labelboxesold[0];
            bool keep = true;
            for (int k = 0; k < labelboxesnew.size(); ++k) {
                if (keep) {
                    float overlap = JaccardOverlap(tmpbox, labelboxesnew[k]);
                    keep = overlap <= adaptive_threshold;
                } else {
                    break;
                }
            }
            if (keep) {
                labelboxesnew.push_back(tmpbox);
            }
            labelboxesold.erase(labelboxesold.begin());
        }

        newretbox.insert(newretbox.end(), labelboxesnew.begin(), labelboxesnew.end());
    }

    newretbox = SortBox(newretbox);
    
    return newretbox;
}

vector<DETECT_BOX_S> run_caffe_once_case1(cv::Mat& org_img, cv::Rect& padrect, MODEL_INFO_S *pstInfo) {
    Net<float> *caffe_net = pstInfo->caffe_net;
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
		if (score < pstInfo->scores[label]) {
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
        tmpbox.x = max(min(cvrect.x, org_img.cols), 0);
        tmpbox.y = max(min(cvrect.y, org_img.rows), 0);
        tmpbox.w = max(min(cvrect.width, org_img.cols), 0);
        tmpbox.h = max(min(cvrect.height, org_img.rows), 0);
        retbox.push_back(tmpbox);
    }
    return retbox;
}

vector<DETECT_BOX_S> run_caffe_small_case1(cv::Mat& org_img, MODEL_INFO_S *pstInfo) {
    Net<float> *caffe_net = pstInfo->caffe_net;
    vector<DETECT_BOX_S> retbox;
    std::cout << "run_caffe_small" << endl;
    cv::Mat img = Preprocess(org_img);
    int srch = img.rows;
    int srcw = img.cols;

    Blob<float>* input_layer = caffe_net->input_blobs()[0];
    int width = input_layer->width();
    int height = input_layer->height();
    int w = width / 2;
    int h = height / 2;
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

    std::cout << "img_pad.rows:" << img_pad.rows << endl;
    std::cout << "img_pad.cols:" << img_pad.cols << endl;
    std::vector<cv::Mat> input_channels;
    WrapInputLayer(img_pad, &input_channels, pstInfo);

    cv::Rect padrect = cv::Rect(-w_off, -h_off, w, h);//w_off,h_off对框进行修正
    retbox = run_caffe_once_case1(org_img, padrect, pstInfo);
    return retbox;
}
vector<DETECT_BOX_S> run_caffe_pad_case1(cv::Mat& org_img, MODEL_INFO_S *pstInfo) {
    Net<float> *caffe_net = pstInfo->caffe_net;
    vector<DETECT_BOX_S> retbox;
    std::cout << "run_caffe_pad" << endl;
    cv::Mat img = Preprocess(org_img);
    int srch = img.rows;
    int srcw = img.cols;

    int w = (std::max)(srch, srcw);
    int h = (std::max)(srch, srcw);
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

    std::cout << "img_pad.rows:" << img_pad.rows << endl;
    std::cout << "img_pad.cols:" << img_pad.cols << endl;
    std::vector<cv::Mat> input_channels;
    WrapInputLayer(img_pad, &input_channels, pstInfo);

    cv::Rect padrect = cv::Rect(-w_off, -h_off, w, h);//w_off,h_off对框进行修正
    retbox = run_caffe_once_case1(org_img, padrect, pstInfo);
    return retbox;
}

vector<DETECT_BOX_S> run_caffe_largeW_case1(cv::Mat& org_img, MODEL_INFO_S *pstInfo) {
    Net<float> *caffe_net = pstInfo->caffe_net;
    vector<DETECT_BOX_S> retbox1;
    vector<DETECT_BOX_S> retbox2;
    std::cout << "run_caffe_largeW" << endl;
    std::vector<cv::Mat> input_channels;
    cv::Mat img = Preprocess(org_img);
    int overlap = 50;
    int x;
    int srch = img.rows;
    int srcw = img.cols;
    int step = max((srcw+overlap+1)/2, srch);
    std::cout << "step:" << step << endl;

    for (int i = 0; i < srcw; i += (step - overlap)) {
        if ((i + step) >= srcw) {
            x = srcw - step;
        } else {
            x = i;
        }
        int h_valid = min(step,srch);
        cv::Rect src_roi(x, 0, step, h_valid);
        cv::Mat img_once;
        img_once.create(h_valid, step, img.type());
        img_once.setTo(cv::Scalar(pstInfo->means[0],pstInfo->means[1],pstInfo->means[2]));
        img(src_roi).copyTo(img_once);

        retbox2 = run_caffe_pad_case1(img_once, pstInfo);
        for (int j = 0; j < retbox2.size(); j++) {
            retbox2[j].x += x;
        }
        retbox1.insert(retbox1.end(), retbox2.begin(), retbox2.end());

        if ((i + step) >= srcw) {
            break;
        }
    }
    
    retbox1 = ApplyNMSFast(retbox1, pstInfo);

    return retbox1;
}

vector<DETECT_BOX_S> run_caffe_other_case1(cv::Mat& org_img, MODEL_INFO_S *pstInfo) {
	Net<float> *caffe_net = pstInfo->caffe_net;
    vector<DETECT_BOX_S> retbox;
    std::cout << "run_caffe_others" << endl;
    cv::Mat img = Preprocess(org_img);

    std::vector<cv::Mat> input_channels;
    WrapInputLayer(img, &input_channels, pstInfo);

    cv::Rect padrect = cv::Rect(0, 0, img.cols, img.rows);
    retbox = run_caffe_once_case1(org_img, padrect, pstInfo);
    return retbox;
}

vector<DETECT_BOX_S> run_caffe_case1(cv::Mat& org_img, MODEL_INFO_S *pstInfo) {
    vector<DETECT_BOX_S> retbox;
    Blob<float>* input_layer = pstInfo->caffe_net->input_blobs()[0];
    int width = input_layer->width();
    int height = input_layer->height();
    
    if ((org_img.rows < height / 2) && (org_img.cols < width / 2)) {
        retbox = run_caffe_small_case1(org_img, pstInfo);
    } else if ((org_img.rows < height) && (org_img.cols < width)) {
        retbox = run_caffe_pad_case1(org_img, pstInfo);
    } else if ((org_img.cols >= width) && (org_img.cols > org_img.rows)) {
        retbox = run_caffe_largeW_case1(org_img, pstInfo);
    } else {
        retbox = run_caffe_other_case1(org_img, pstInfo);
    }

	return retbox;
}
