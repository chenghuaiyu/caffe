
#include <vector>
#include <string>

#include "opencv2/opencv.hpp"

#include "../common.h"
#include "postprocess_normal.h"
#include "postprocess_case1.h"
#include "postprocess_classify.h"
#include "postprocess_classify_uniform.h"

using namespace std;
using namespace cv;

#define MIN_EXPAND  8
#define MAX_EXPAND  16
void OutputExpand(vector<DETECT_BOX_S>& retbox, MODEL_INFO_S *pstInfo, cv::Mat& org_img)
{
	float expand_ratio = pstInfo->outputexpandratio;
	float expand_width, expand_heigth;
	if (expand_ratio > 1)
	{
		float temp = (expand_ratio - 1)*0.5;
		for (int i = 0; i < retbox.size(); i++)
		{
			expand_width = (retbox[i].w * temp) > MAX_EXPAND ? MAX_EXPAND : (retbox[i].w * temp);
			expand_heigth = (retbox[i].h * temp) > MAX_EXPAND ? MAX_EXPAND : (retbox[i].h * temp);

			expand_width = (retbox[i].w * temp) < MIN_EXPAND ? MIN_EXPAND : (retbox[i].w * temp);
			expand_heigth = (retbox[i].h * temp) < MIN_EXPAND ? MIN_EXPAND : (retbox[i].h * temp);

			retbox[i].x = retbox[i].x - expand_width;
			retbox[i].y = retbox[i].y - expand_heigth;

			retbox[i].w = retbox[i].w + expand_width * 2;
			retbox[i].h = retbox[i].h + expand_heigth * 2;
			if (retbox[i].x < 0) retbox[i].x = 0;
			if (retbox[i].y < 0) retbox[i].y = 0;

			if (retbox[i].x + retbox[i].w > org_img.cols)
			{
				retbox[i].w = org_img.cols - retbox[i].x;
				//printf("org_img.cols == %d", org_img.cols);
			}
			if (retbox[i].y + retbox[i].h > org_img.rows)
			{
				retbox[i].h = org_img.rows - retbox[i].y;
				//printf("org_img.rows == %d", org_img.rows);
			}

		}
	}
}

vector<DETECT_BOX_S> postprocess(vector<DETECT_BOX_S> inbox, string postmode, MODEL_INFO_S *pstInfo, cv::Mat& org_img)
{
	vector<DETECT_BOX_S> retbox;
	vector<DETECT_BOX_S> retbox_temp;
    if (postmode == "normal")
    {
		retbox = postprocess_normal(inbox);
    }
    else if (postmode == "case1")
    {
		retbox = postprocess_case1(inbox, pstInfo);
    }
    else if (postmode == "classify")
    {
		retbox_temp = postprocess_case1(inbox, pstInfo);
		retbox = postprocess_classify(retbox_temp, pstInfo, org_img);
    }
	else if (postmode == "classify_uniform")
	{
		retbox = postprocess_classify_uniform(inbox, pstInfo, org_img);
	}
	else 
    {
		retbox = postprocess_normal(inbox);
    }

	OutputExpand(retbox, pstInfo, org_img);

	return retbox;
}

