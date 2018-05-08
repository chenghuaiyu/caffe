
#ifndef _VIM_COMMON_H_
#define _VIM_COMMON_H_
 
#include <string>
#include <vector>

#include "caffe/caffe.hpp"

#define SUCCESS 0
#define FAIL 1

using caffe::Net;
using caffe::LabelMap;

using namespace std;
 
 typedef struct {
   float label;
   float score;
   float x;
   float y;
   float w;
   float h;
 } DETECT_BOX_S;
 
typedef struct {
  string filename;
  int width;
  int height;
  vector<DETECT_BOX_S> boxes;
} DETECT_FILE_S;

typedef struct {
  int min;
  int max;
} SIZE_LIMIT_S;

typedef struct {
  Net<float> *caffe_net;
  string gpus;
  string encry;
  string key;
  string model;
  string weight;
  string labelmapfile;
  LabelMap *labelmap;
  vector<float> scores;
  vector<float> scoreslow;
  vector<float> scoreshigh;
  vector<float> maxcnts;
  vector<int> colors;
  vector<string> labels;
  vector<float> means;
  vector<vector<SIZE_LIMIT_S>> sizelimits;
  string postmode;
  string runmode;
  string outmode;
  float cropscale;
  float outputexpandratio;
  vector<float> minarea;
} MODEL_INFO_S;

#endif
