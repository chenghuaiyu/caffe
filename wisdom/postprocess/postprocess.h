#ifndef _VIM_PROCESS_H_
#define _VIM_PROCESS_H_

#include <string>
#include <vector>

#include "opencv2/opencv.hpp"

#include "../common.h"

using namespace std;
using namespace cv;

vector<DETECT_BOX_S> postprocess(vector<DETECT_BOX_S> retbox, string postmode, MODEL_INFO_S *pstInfo, cv::Mat& org_img);

#endif //_VIM_PROCESS_H_