
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

#include "common.h"

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

void WrapInputLayer(const cv::Mat& img, std::vector<cv::Mat> *input_channels, MODEL_INFO_S *pstInfo)
{
    Blob<float>* input_layer = pstInfo->caffe_net->input_blobs()[0];
    int width = input_layer->width();
    int height = input_layer->height();
    float* input_data = input_layer->mutable_cpu_data();

    //resize
    cv::Size input_geometry;
    cv::Mat sample_resize;
    input_geometry = cv::Size(width, height);
    if (img.size() != input_geometry)
    {
        cv::resize(img, sample_resize, input_geometry);
    }
    else
    {
        sample_resize = img;
    }

    //split to input_layer
    for (int j = 0; j < input_layer->channels(); ++j)
    {
        cv::Mat channel(height, width, CV_32FC1, input_data);
        input_channels->push_back(channel);
        input_data += width * height;
    }
    cv::split(sample_resize, *input_channels);

    //sub mean
    input_data = input_layer->mutable_cpu_data();
    for (int c = 0; c < 3; ++c) {
        for (int h = 0; h < height; ++h){
            for (int w = 0; w < width; ++w){
                *(input_data + c * height* width + h * width + w) -= pstInfo->means[c];
            }
        }
    }
}

cv::Mat Preprocess(const cv::Mat &img)
{
    /* Convert the input image to the input image format of the network. */
    cv::Mat sample;
    if (img.channels() == 4)
        cv::cvtColor(img, sample, cv::COLOR_BGRA2BGR);
    else if (img.channels() == 1)
        cv::cvtColor(img, sample, cv::COLOR_GRAY2BGR);
    else
        sample = img;

    cv::Mat sample_float;
    sample.convertTo(sample_float, CV_32FC3);

    return sample_float;
}


