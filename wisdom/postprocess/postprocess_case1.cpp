
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

vector<DETECT_BOX_S> postprocess_case1(vector<DETECT_BOX_S> retbox, MODEL_INFO_S *pstInfo)
{
    vector<DETECT_BOX_S> savebox;
    
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

