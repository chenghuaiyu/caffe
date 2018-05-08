
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
#include <queue>
#ifndef _LINUX_
#include <io.h>
#include <direct.h>  
#else
#include <unistd.h>  //access
#include <sys/stat.h>  //mkdir
#include <sys/types.h>  //mkdir
#include <dirent.h> //rmdir
#endif
#include "opencv2/opencv.hpp"

#include "../common.h"
#include "../preprocess.hpp"

#include "../detection.hpp"
#include "../util/util.h"
//#include "../output/xml.h"
#include "postprocess_classify_uniform.h"

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

//#define DEBUG
#define SMOOTH

//之后会使用配置值
int crop_minW = 30;//30;
int crop_minH = 60;// 60;
float crop_wRatio = 0.25;
float crop_hRatio = 0.25;
float excludeIouVal = 0.2;// 0.2;
float expandExcludeIouVal = 0.2;
float hwRatio = 1.5;
static int boxId = 0;

static int maxClassTypeNum = 15;
static int newDistanceThreshold = 50;

static queue<int> classType;
static vector<int> classTypeNum;
static DETECT_BOX_S preBox = { 0, 0, 0, 0, 0, 0 };


//以后可能移到工具类文件中
static bool PairCompare(const std::pair<float, int>& lhs,
	const std::pair<float, int>& rhs) {
	return lhs.first > rhs.first;
}

//以后可能移到工具类文件中
/* Return the indices of the top N values of vector v. */
static std::vector<int> Argmax(const std::vector<float>& v, int N) {
	std::vector<std::pair<float, int> > pairs;
	for (size_t i = 0; i < v.size(); ++i)
		pairs.push_back(std::make_pair(v[i], static_cast<int>(i)));
	std::partial_sort(pairs.begin(), pairs.begin() + N, pairs.end(), PairCompare);

	std::vector<int> result;
	for (int i = 0; i < N; ++i)
		result.push_back(pairs[i].second);
	return result;
}

void IntersectBBox(const BBox& bbox1, const BBox& bbox2,
	BBox* intersect_bbox) 
{
	if (bbox2.xmin > bbox1.xmax || bbox2.xmax < bbox1.xmin ||
		bbox2.ymin > bbox1.ymax || bbox2.ymax < bbox1.ymin) {
		// Return [0, 0, 0, 0] if there is no intersection.
		intersect_bbox->xmin = 0;
		intersect_bbox->ymin = 0;
		intersect_bbox->xmax = 0;
		intersect_bbox->ymax = 0;
	}
	else {
		intersect_bbox->xmin = std::max(bbox1.xmin, bbox2.xmin);
		intersect_bbox->ymin = std::max(bbox1.ymin, bbox2.ymin);
		intersect_bbox->xmax = std::min(bbox1.xmax, bbox2.xmax);
		intersect_bbox->ymax = std::min(bbox1.ymax, bbox2.ymax);
	}
}

float BBoxSize(const BBox& bbox) {
	if (bbox.xmax < bbox.xmin || bbox.ymax < bbox.ymin)
	{
		// If bbox is invalid (e.g. xmax < xmin or ymax < ymin), return 0.
		return 0;
	}
	else 
	{
		float width = bbox.xmax - bbox.xmin;
		float height = bbox.ymax - bbox.ymin;

		// If bbox is not within range [0, 1].
		return (width + 1) * (height + 1);
	}
}

static float jaccardOverlap(const BBox& bbox1, const BBox& bbox2)
{
	BBox intersect_bbox;
	IntersectBBox(bbox1, bbox2, &intersect_bbox);
	float intersect_width, intersect_height;

	intersect_width = intersect_bbox.xmax - intersect_bbox.xmin + 1;
	intersect_height = intersect_bbox.ymax - intersect_bbox.ymin + 1;

	if (intersect_width > 0 && intersect_height > 0) {
		float intersect_size = intersect_width * intersect_height;
		float bbox1_size = BBoxSize(bbox1);
		float bbox2_size = BBoxSize(bbox2);
		return intersect_size / (bbox1_size + bbox2_size - intersect_size);
	}
	else {
		return 0.;
	}
}

static vector<bool>checkjaccardOverlap(const float& excludeIou, const vector<BBox>& bboxes1, const vector<BBox>& bboxes2)
{
	float iou = 0;
	bool exclude = false;
	vector<bool> iou_result;
	for (int i = 0; i < bboxes1.size(); i++)
	{
		exclude = false;
		for (int j = 0; j < bboxes2.size(); j++)
		{
			iou = jaccardOverlap(bboxes1[i], bboxes2[j]);
			if ((iou > excludeIou) && (iou != 1))
			{
				exclude = 1;
				continue;
			}
		}
		if (!exclude)
		{
			iou_result.push_back(false);
		}
		else
		{
			iou_result.push_back(true);
		}
	}
	return iou_result;
}


static vector<BBox>excludeClosePerson(const float& excludeIou, const vector<BBox>& bboxes1, const vector<BBox>& bboxes2)
{
	vector<bool> iou_result;
	vector<BBox> box_result;

	iou_result = checkjaccardOverlap(excludeIou, bboxes1, bboxes2);

	for (int i = 0; i < iou_result.size(); i++)
	{
		if (!iou_result[i])
			box_result.push_back(bboxes1[i]);
	}
	return box_result;
}

static vector<BBox>expandPersonBox(const float& wRatio, const float& hRatio, vector<BBox>& bboxes)
{
	vector<BBox> box_result;
	BBox box;
	int width, height, size, dx, dy;

	for (int i = 0; i < bboxes.size(); i++)
	{
		width = bboxes[i].xmax - bboxes[i].xmin + 1;
		height = bboxes[i].ymax - bboxes[i].ymin + 1;

		size = max(width*(1 + 2 * wRatio), height*(1 + 2 * hRatio));

		if (size % 2)
		{
			size = size + 1;
		}
		dx = (size - width) / 2;
		dy = (size - height) / 2;

		box.xmin = bboxes[i].xmin - dx;
		box.ymin = bboxes[i].ymin - dy;
		box.xmax = bboxes[i].xmax + dx;
		box.ymax = bboxes[i].ymax + dy;

		box_result.push_back(box);
	}
	return box_result;
}

static void computeLocationMiddle(const BBox bbox, const int targetW, const int targetH, BBox& srcbox, BBox& dstbox)
{
	dstbox.xmin = (targetW - (bbox.xmax - bbox.xmin + 1)) / 2;
	dstbox.xmax = dstbox.xmin + (bbox.xmax - bbox.xmin + 1) - 1;
	dstbox.ymin = (targetH - (bbox.ymax - bbox.ymin + 1)) / 2;
	dstbox.ymax = dstbox.ymin + (bbox.ymax - bbox.ymin + 1) - 1;

	srcbox = bbox;
}

static void computeLocationCorner(const int width, const int height, const BBox& bbox, BBox& srcbox, BBox& dstbox)
{
	if (bbox.xmin < 0)
	{
		srcbox.xmin = 0;
		dstbox.xmin = 0 - bbox.xmin;
	}
	else
	{
		srcbox.xmin = bbox.xmin;
		dstbox.xmin = 0;
	}

	if (bbox.xmax > (width - 1))
	{
		srcbox.xmax = width - 1;
	}
	else
	{
		srcbox.xmax = bbox.xmax;
	}

	dstbox.xmax = srcbox.xmax - srcbox.xmin + dstbox.xmin;

	if (bbox.ymin < 0)
	{
		srcbox.ymin = 0;
		dstbox.ymin = 0 - bbox.ymin;
	}
	else
	{ 
		srcbox.ymin = bbox.ymin;
		dstbox.ymin = 0;
	}
	
	if (bbox.ymax > (height - 1))
	{
		srcbox.ymax = height - 1;
	}
	else
	{ 
		srcbox.ymax = bbox.ymax;
	}
	dstbox.ymax = srcbox.ymax - srcbox.ymin + dstbox.ymin;
}

//将输入的box 处理一下, 返回处理过后的box
static void process_box(const vector<BBox>& allPerson, cv::Mat& org_img, vector<BBox>&checkBoxes, vector<BBox>& expandBoxes, vector<BBox>& srcBoxes, vector<BBox>& dstBoxes)
{
	cv::Size input_geometry;
	int width, height;
	vector<BBox> check, expand;
	vector<bool> isOverlap;

	input_geometry = org_img.size();
	width = input_geometry.width;
	height = input_geometry.height;

	//exclude too close persons
	check = excludeClosePerson(excludeIouVal, allPerson, allPerson);

	expand = expandPersonBox(crop_wRatio, crop_hRatio, check);

	isOverlap = checkjaccardOverlap(expandExcludeIouVal, expand, expand);

	for (int i = 0; i < expand.size(); i++)
	{
		int cWidth = check[i].xmax - check[i].xmin + 1;
		int cHeight = check[i].ymax - check[i].ymin + 1;
		BBox srcBox, dstBox;

		//org must bigger enough
		if ((cWidth < crop_minW) || (cHeight < crop_minH))
			continue;

		int eWidth = expand[i].xmax - expand[i].xmin + 1;
		int eHeight = expand[i].ymax - expand[i].ymin + 1;

		//use not expand person
		if (isOverlap[i])
		{
			computeLocationMiddle(check[i], eWidth, eHeight, srcBox, dstBox);
		}
		else
		{
			computeLocationCorner(width, height, expand[i], srcBox, dstBox);
		}

		checkBoxes.push_back(check[i]);
		expandBoxes.push_back(expand[i]);
		srcBoxes.push_back(srcBox);
		dstBoxes.push_back(dstBox);
	}
}

static int compute_box_distance(const DETECT_BOX_S& pre, const DETECT_BOX_S& now)
{
	float preXCenter = pre.x + pre.w / 2;
	float preYCenter = pre.y + pre.h / 2;
	float nowXCenter = now.x + now.w / 2;
	float nowYCenter = now.y + now.h / 2;

	return sqrt(pow(nowXCenter - preXCenter, 2) + pow(nowYCenter - preYCenter, 2));
}

vector<DETECT_BOX_S> postprocess_classify_uniform(vector<DETECT_BOX_S> retbox, MODEL_INFO_S *pstInfo, cv::Mat& org_img)
{
	vector<DETECT_BOX_S> savebox;
	Net<float> *caffe_net = pstInfo->caffe_net;
	vector<BBox> allPerson, checkBoxes, expandBoxes, srcBoxes, dstBoxes;

	if (classTypeNum.size() == 0)
	{
		for (int i = 0; i < (pstInfo->labels).size(); i++)
		{
			classTypeNum.push_back(0);
		}
	}

	for (int i = 0; i < retbox.size(); i++)
	{
		BBox tempBox;
		tempBox.xmin = retbox[i].x;
		tempBox.ymin = retbox[i].y;
		tempBox.xmax = tempBox.xmin + retbox[i].w - 1;
		tempBox.ymax = tempBox.ymin + retbox[i].h - 1;

		//width and height must be even
		if (int(retbox[i].w) % 2)
		{
			tempBox.xmax--;
		}

		if (int(retbox[i].h) % 2)
		{
			tempBox.ymax--;
		}

		allPerson.push_back(tempBox);
	}

	process_box(allPerson, org_img, checkBoxes, expandBoxes, srcBoxes, dstBoxes);

	for (int i = 0; i < checkBoxes.size(); i ++)
	{
		cv::Mat img_pad, mask;

		//from crop to pad
		int expandWidth = expandBoxes[i].xmax - expandBoxes[i].xmin + 1;
		int expandHeight = expandBoxes[i].ymax - expandBoxes[i].ymin + 1;
		img_pad.create(expandHeight, expandWidth, org_img.type());
		img_pad.setTo(cv::Scalar(0));

		//根据给定矩形设置图像的ROI(Region of Interesting)
		cv::Rect roiDst(dstBoxes[i].xmin, dstBoxes[i].ymin, dstBoxes[i].xmax - dstBoxes[i].xmin + 1, dstBoxes[i].ymax - dstBoxes[i].ymin + 1);
		cv::Rect roiSrc(srcBoxes[i].xmin, srcBoxes[i].ymin, srcBoxes[i].xmax - srcBoxes[i].xmin + 1, srcBoxes[i].ymax - srcBoxes[i].ymin + 1);

		Mat imageROI = img_pad(roiDst);

		org_img(roiSrc).copyTo(imageROI);

		//imshow("ROI", imageROI);
		//imshow("Classify", img_pad);
		//waitKey(1);
		
		cv::Mat img = Preprocess(img_pad);

		std::vector<cv::Mat> input_channels;
		WrapInputLayer(img, &input_channels, pstInfo);

		caffe_net->Forward();

		Blob<float>* outblob = caffe_net->output_blobs()[0];
		const float* begin = outblob->cpu_data();
		const float* end = begin + outblob->channels();

		std::vector<float> output = std::vector<float>(begin, end);

		std::vector<int> maxN = Argmax(output, 1);

		int idx = maxN[0];

		DETECT_BOX_S tmpbox;
		tmpbox.label = idx;

		tmpbox.score = output[idx];
		tmpbox.x = checkBoxes[i].xmin;
		tmpbox.y = checkBoxes[i].ymin;
		tmpbox.w = checkBoxes[i].xmax - checkBoxes[i].xmin + 1;
		tmpbox.h = checkBoxes[i].ymax - checkBoxes[i].ymin + 1;

#ifdef DEBUG
		string dirName = "classifierInput";
		if (!boxId)
			mkdir(dirName.c_str());

		string name = dirName + "/" + to_string(boxId) + "_" + pstInfo->labels[idx] + ".jpg";
		imwrite(name, img_pad);	
		boxId++;
#endif

		//skip background
		if (0 == tmpbox.label)
			continue;


#ifdef SMOOTH
		if (tmpbox.h / tmpbox.w < hwRatio)
			continue;
		//to process images
		int distance = compute_box_distance(preBox, tmpbox);
		//cout <<"distance: " << distance;
		//clear history
		if (distance > newDistanceThreshold)
		{
			while (!classType.empty())
				classType.pop();
			for (int i = 0; i < (pstInfo->labels).size(); i++)
			{
				classTypeNum[i] = 0;
			}
		}

		preBox.w = tmpbox.w;
		preBox.h = tmpbox.h;
		preBox.x = tmpbox.x; 
		preBox.y = tmpbox.y;

		if (classType.size() > maxClassTypeNum)
		{
			int type = classType.front();
			classTypeNum[type]--;
			classType.pop();
		}
		classType.push(tmpbox.label);
		classTypeNum[tmpbox.label]++;

		vector<int>::iterator biggest = std::max_element(std::begin(classTypeNum), std::end(classTypeNum));
		int index = std::distance(std::begin(classTypeNum), biggest);
		tmpbox.label = index;
#endif
		savebox.push_back(tmpbox);
	}
	return savebox;
}

