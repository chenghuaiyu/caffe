
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
#include <google/protobuf/io/zero_copy_stream_impl.h> 
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <utility>
#include <map>
#include <string>
#include <vector>
#include <iostream>  
#include <stdio.h> 
#include <stdlib.h>  

#include <vector>
#include <string>

#include "../common.h"
#include "../util/util.h"

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

static float BBoxSize(const DETECT_BOX_S& bbox) {
    float width = bbox.w;
    float height = bbox.h;

    return (width + 1) * (height + 1);
}

static float JaccardOverlap(const DETECT_BOX_S bbox1, const DETECT_BOX_S bbox2) {
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

static vector<DETECT_BOX_S> SortBox(vector<DETECT_BOX_S> box) 
{
    for (int i = 0; i < box.size(); i++)
    {
        for (int j = i; j < box.size(); j++)
        {
            if (box[i].score < box[j].score)
            {
                DETECT_BOX_S temp = box[i];
                box[i] = box[j];
                box[j] = temp;
            }
        }
    }

    return box;
}

static vector<DETECT_BOX_S> GetLabelBox(vector<DETECT_BOX_S> retbox, const vector<float> label) 
{
    vector<DETECT_BOX_S> labelBox;

    for (int i = 0; i < retbox.size(); ++i) 
    {
        for (int j = 0; j < label.size(); ++j)
        {
            if (retbox[i].label == label[j])
            {
                labelBox.push_back(retbox[i]);
            }
        }
    }

    labelBox = SortBox(labelBox);

    return labelBox;
}

static vector<DETECT_BOX_S> ApplyNMSFast(vector<DETECT_BOX_S> retbox, MODEL_INFO_S *pstInfo) 
{
    float adaptive_threshold = 0.30;
    vector<DETECT_BOX_S> newretbox;
    map<string, vector<float> > mainlabel;

    //"_"之前的字符表示大的类别
    mainlabel.clear();
    for (int i = 0; i < pstInfo->labels.size(); i++)
    {
        string labelname = pstInfo->labels[i];
        vector<string> result;
        result = Split(labelname, "_");
        mainlabel[result[0]].push_back(float(i));
        //printf("mainlabel:%s\n", result[0].c_str());
        //printf("sublabel:%s %d\n", labelname.c_str(), i);
    }
    
    for (map<string, vector<float> >::iterator it = mainlabel.begin();
         it != mainlabel.end(); ++it) 
    {
        const string labelname = it->first;
        //printf("labelname:%s\n", labelname.c_str());
        vector<DETECT_BOX_S> labelboxesold;
        vector<DETECT_BOX_S> labelboxesnew;
        labelboxesold = GetLabelBox(retbox, mainlabel[labelname]);
        labelboxesnew.clear();

        // Do nms.
        while(labelboxesold.size())
        {
            DETECT_BOX_S tmpbox = labelboxesold[0];
            bool keep = true;
            for (int k = 0; k < labelboxesnew.size(); ++k) 
            {
                if (keep) 
                {
                    float overlap = JaccardOverlap(tmpbox, labelboxesnew[k]);
                    keep = overlap <= adaptive_threshold;
                } else 
                {
                    break;
                }
            }
            if (keep) 
            {
                labelboxesnew.push_back(tmpbox);
            }
            labelboxesold.erase(labelboxesold.begin());
        }

        newretbox.insert(newretbox.end(), labelboxesnew.begin(), labelboxesnew.end());
    }

    newretbox = SortBox(newretbox);
    
    return newretbox;
}

vector<DETECT_BOX_S> postprocess_case1(vector<DETECT_BOX_S> inbox, MODEL_INFO_S *pstInfo)
{
    vector<DETECT_BOX_S> savebox;
    vector<DETECT_BOX_S> retbox;

    retbox = ApplyNMSFast(inbox,pstInfo);
    //retbox = inbox;
        
    for (int i = 0; i < retbox.size(); i++)
    {
        if (retbox[i].x < 0) retbox[i].x = 0;
        if (retbox[i].y < 0) retbox[i].y = 0;
        if (retbox[i].w < 0) retbox[i].w = 0;
        if (retbox[i].h < 0) retbox[i].h = 0;

        int label = int(retbox[i].label);

        if (retbox[i].w * retbox[i].h <= pstInfo->minarea[label])
            continue;

        int deleteflag = 0;
        vector<SIZE_LIMIT_S> sizelimits = pstInfo->sizelimits[label];
        for (int j = 0; j < sizelimits.size(); j++)
        {
            if ((min(retbox[i].w, retbox[i].h) < sizelimits[j].min)
                && (max(retbox[i].w, retbox[i].h) < sizelimits[j].max))
            {
                deleteflag = 1;
                break;
            }
        }
        if (deleteflag)
        {
            continue;
        }
        
        savebox.push_back(retbox[i]);
    }
    
    return savebox;
}

