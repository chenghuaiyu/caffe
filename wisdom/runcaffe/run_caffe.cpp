
#include <string>
#include <vector>

#include "opencv2/opencv.hpp"

#include "../common.h"

#include "run_caffe_normal.hpp"
#include "run_caffe_case1.hpp"
#include "run_caffe_case2.hpp"
#include "run_caffe_case3.hpp"
#include "run_caffe_case4.hpp"

using namespace std;
using namespace cv;

vector<DETECT_BOX_S> run_caffe(cv::Mat& org_img, MODEL_INFO_S *pstInfo)
{
    vector<DETECT_BOX_S> retbox;
    if (pstInfo->runmode == "normal")
    {
        retbox = run_caffe_normal(org_img, pstInfo);
    }
    else if (pstInfo->runmode == "case1")
    {
        retbox = run_caffe_case1(org_img, pstInfo);
    }
    else if (pstInfo->runmode == "case2")
    {
        retbox = run_caffe_case2(org_img, pstInfo);
    }
    else if (pstInfo->runmode == "case3")
    {
        retbox = run_caffe_case3(org_img, pstInfo);
    }
    else if (pstInfo->runmode == "case4")
    {
        retbox = run_caffe_case4(org_img, pstInfo);
    }
    else
    {
        retbox = run_caffe_normal(org_img, pstInfo);
    }

    return retbox;
} 

