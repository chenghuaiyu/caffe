#ifndef CAFFE_RUN_HPP_
#define CAFFE_RUN_HPP_

#include <string>
#include <vector>

#include "opencv2/opencv.hpp"

#include "../common.h"

using namespace std;
using namespace cv;

vector<DETECT_BOX_S> run_caffe(cv::Mat& org_img, MODEL_INFO_S *pstInfo);

#endif  // CAFFE_RUN_HPP_
