
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

#include "../detection.hpp"
#include "../util/util.h"
//#include "../output/xml.h"

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

#define MAX_SIZE (160)
#define MIN_SIZE (32)
#define BOTTOM_RATIO (1.0/2)

int getsize(int posw, int posh, float sratio)
{
    int size = max(posw, posh);
	printf("size:%d\n", size);
    float sratio_top = sratio;
    float sratio_bottom = sratio * BOTTOM_RATIO;
    float sratio_new = sratio;
    if (size < MIN_SIZE)
    {
        sratio_new = sratio_top;
    }
    else if (size >= MAX_SIZE)
    {
        sratio_new = sratio_bottom;
    }
    else
    {
        sratio_new = sratio_bottom + ((sratio_top - sratio_bottom)/(MAX_SIZE-MIN_SIZE)*(MAX_SIZE-size));
    }
	printf("sratio_new:%f\n", sratio_new);
    size = int(size*sratio_new);
    
    return size;
}

cv::Mat getimage(int x, int y, int w, int h, float sratio, cv::Mat& org_img)
{
    int xmin,xmax,ymin,ymax;
    int xmin_src,xmax_src,ymin_src,ymax_src;
    int xmin_dst,xmax_dst,ymin_dst,ymax_dst;
    printf("x:%d,y:%d,w:%d,h:%d\n",x, y, w, h);
    
	int size = getsize(w, h, sratio);
	printf("size:%d\n", size);
	int xcenter = (x + w / 2);
	int ycenter = (y + h / 2);
    
    xmin = int(xcenter - size/2);
    ymin = int(ycenter - size/2);
    xmax = int(xcenter + size/2);
    ymax = int(ycenter + size/2);
    printf("x1:%d,x2:%d,y1:%d,yw:%d\n",xmin,xmax,ymin,ymax);
    
    if (xmin < 0)
    {
      xmin_src = 0;
      xmin_dst = 0-xmin;
    }
    else
    {
      xmin_src = xmin;
      xmin_dst = 0;
    }
    
    if (xmax > org_img.cols)
    {
      xmax_src = org_img.cols;
    }
    else
    {
      xmax_src = xmax;
    }
    
    xmax_dst = xmax_src-xmin_src+xmin_dst;
    
    if (ymin < 0)
    {
      ymin_src = 0;
      ymin_dst = 0-ymin;
    }
    else
    {
      ymin_src = ymin;
      ymin_dst = 0;
    }
    
    if (ymax > org_img.rows)
    {
      ymax_src = org_img.rows;
    }
    else
    {
      ymax_src = ymax;
    }
    
	ymax_dst = ymax_src - ymin_src + ymin_dst;
    printf("x1:%d,x2:%d,y1:%d,yw:%d\n",xmin_src,xmax_src,ymin_src,ymax_src);
    printf("x1:%d,x2:%d,y1:%d,yw:%d\n",xmin_dst,xmax_dst,ymin_dst,ymax_dst);

	cv::Mat img_pad;

	//from crop to pad
	img_pad.create(size, size, org_img.type());
	img_pad.setTo(cv::Scalar(0));

	//根据给定矩形设置图像的ROI(Region of Interesting)
	cv::Rect roiSrc(xmin_src, ymin_src, xmax_src-xmin_src, ymax_src-ymin_src);
	cv::Rect roiDst(xmin_dst, ymin_dst, xmax_dst-xmin_dst, ymax_dst-ymin_dst);

	Mat imageROI = img_pad(roiDst);

	org_img(roiSrc).copyTo(imageROI);

    return imageROI;
}

vector<DETECT_BOX_S> postprocess_classify(vector<DETECT_BOX_S> retbox, MODEL_INFO_S *pstInfo, cv::Mat& org_img)
{
    int j;
    vector<DETECT_BOX_S> savebox;
    Net<float> *caffe_net = pstInfo->caffe_net;
    map<int, int> cnt;

    for(int i = 0; i < pstInfo->labels.size(); i++)
    {
        cnt[i] = 0;
    }
    
    for (int i = 0; i < retbox.size(); i++)
    {
        float score = retbox[i].score;
        int label = retbox[i].label;
        if (cnt[label]>= pstInfo->maxcnts[label])
        {
            continue;
        }
        
        float scorelow = pstInfo->scoreslow[label];
        float scorehigh = pstInfo->scoreshigh[label];
		printf("score:%f, scorelow:%f, scorehigh:%f, cropscale:%f\n", score, scorelow, scorehigh, pstInfo->cropscale);
        if ((score > scorelow) && (score < scorehigh))
        {
            float x = retbox[i].x;
            float y = retbox[i].y;
            float width = retbox[i].w;
            float height = retbox[i].h;

            cv::Mat img_tmp = getimage(x,y,width,height,pstInfo->cropscale,org_img);
            
            cv::Mat img = Preprocess(img_tmp);

            std::vector<cv::Mat> input_channels;
			WrapInputLayer(img, &input_channels, pstInfo);
            
            caffe_net->Forward();
            Blob<float>* outblob = caffe_net->output_blobs()[0];
            const float* outdata = outblob->cpu_data();
            for (j = 0; j < outblob->count(); j++)
            {
                if (outdata[j] > outdata[label])
                {
                    break;
                }
            }
            
            if (j != outblob->count())
            {
                continue;
            }
        }
        else if (score < scorelow)
        {
            continue;
        }

        cnt[label] += 1; 
        savebox.push_back(retbox[i]);
    }
    
    return savebox;
}

