#ifndef _RUN_CAFFE_CASE2_HPP_
#define _RUN_CAFFE_CASE2_HPP_

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

vector<DETECT_BOX_S> run_caffe_case2(cv::Mat& org_img, MODEL_INFO_S *pstInfo);

#endif  // _RUN_CAFFE_CASE2_HPP_