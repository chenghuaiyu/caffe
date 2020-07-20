#ifndef _LINUX_
#include <io.h>
#include <time.h>
#define NOMINMAX
#include <windows.h>
#endif
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
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>
#include <utility>
#include <map>
#include <string>
#include <vector>
#include <iostream>  
#include <stdio.h> 
#include <stdlib.h>  
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "opencv2/opencv.hpp"
#include "common.h"
#include "runcaffe/run_caffe.hpp"
#include "detection.hpp"
#include "util/util.h"
#include "config/get_config.h"
#include "config/get_config_normal.h"
#include "config/get_config_xml.h"
#include "decryption/decryption.h"
#include "postprocess/postprocess.h"
#include "output/output.h"
#include "output/cjson.h"
#include "output/outputcjson.h"
#include "output/outputxml.h"
#include "SCWObjectDetectionV2.h"

#ifdef _DEBUG
#define SHOW_MESSAGE
#endif

enum {
	RSA_NONE = 0,
	RSA_SENTINEL = 1,
	RSA_TENDYRON = 2
};

//#define RSA		RSA_NONE
#define RSA		RSA_SENTINEL

#if		RSA == RSA_SENTINEL
#include "sentinel\hasp_api_cpp.h"
#include "sentinel\vendor_code.h"
#include "sentinel\errorprinter.h"
#elif	RSA == RSA_TENDYRON
#include "dongle/tendyron.h"

char * pszReaders = NULL;
wchar_t * pwszReaders = NULL;
//command 1: 00A4000002DF20
const unsigned char ucBuf1[] = { 0x00, 0xA4, 0x00, 0x00, 0x02, 0xDF, 0x20 };
//command 2: E02241B613010000 <message>
const unsigned char ucBuf2[] = { 0xE0, 0x22, 0x41, 0xB6, 0x13, 0x01, 0x00, 0x00 };
//command 3: E02A9E9A00
const unsigned char ucBuf3[] = { 0xE0, 0x2A, 0x9E, 0x9A, 0x00 };
const unsigned char ucBuf[] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f };

int tendyron(const unsigned char * pucBuf, unsigned long usBufLen, unsigned char ** ppucBufOut) {
	if (NULL == pucBuf || 0 >= usBufLen || NULL == ppucBufOut) {
		return FAIL;
	}

	if (NULL == pszReaders) {
		if (!TDR_GetFirstReaderName(&pszReaders)) {
			return FAIL;
		}
	}

	unsigned long ulLenOut = 0;
	unsigned char * pucBufOut = NULL;
	BOOL bRet = TDR_SendCommand(pszReaders, SCARD_PROTOCOL_T1, ucBuf1, sizeof(ucBuf1), &pucBufOut, &ulLenOut);
	if (!bRet) {
		return FAIL;
	}
	TDR_FreeMemory((void **)&pucBufOut);

	unsigned char ucBufIn[LEN_APDU] = { 0 };//buffer containing the data to write
	memcpy(ucBufIn, ucBuf2, sizeof(ucBuf2));
	memcpy(ucBufIn + sizeof(ucBuf2), pucBuf, usBufLen);
	//bRet = TDR_SendCommand(pszReaders, SCARD_PROTOCOL_T1, ucBufIn, sizeof(ucBuf2) + usBufLen, &pucBufOut, &ulLenOut);
	bRet = TDR_SendCommand(pszReaders, SCARD_PROTOCOL_T1, ucBufIn, sizeof(ucBuf2) + 16, &pucBufOut, &ulLenOut);
	if (!bRet) {
		return FAIL;
	}
	TDR_FreeMemory((void **)&pucBufOut);

	bRet = TDR_SendCommand(pszReaders, SCARD_PROTOCOL_T1, ucBuf3, sizeof(ucBuf3), &pucBufOut, &ulLenOut);
	if (!bRet) {
		return FAIL;
	}
	//TDR_FreeMemory((void **)&pucBufOut);
	*ppucBufOut = pucBufOut;
	return SUCCESS;
}
#elif	RSA != RSA_NONE
#error("unsupported RSA method")
#endif

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
using caffe::LabelMap;

using namespace std;
using namespace cv;

const char g_szObjectSep[] = { cObjectSep, '\0' };
const char g_szAliasSep[] = { cAliasSep, '\0' };
const char g_szSynonymSep[] = { cSynonymSep, '\0' };

vector<HScwAlg> g_vecScwAlgHandle;
std::map<HScwAlg, std::map<std::string, std::string>> g_mapAlgObjAlias;

typedef struct {
    MODEL_INFO_S stMainModel;
    MODEL_INFO_S stSubModel;
}DETECT_INFO_S;

cv::Mat imReadImage(const wchar_t* pwszImgFile) {
	FILE* fp = _wfopen(pwszImgFile, L"rb");
	if (!fp) {
		return Mat::zeros(1, 1, CV_8U);
	}
	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	char* buf = new char[sz];
	fseek(fp, 0, SEEK_SET);
	long n = fread(buf, 1, sz, fp);
	_InputArray arr(buf, sz);
	Mat img = imdecode(arr, CV_LOAD_IMAGE_COLOR);
	delete[] buf;
	fclose(fp);
	return img;
}

// Parse GPU ids or use all available devices
static void get_gpus(vector<int>* gpus, string strgpus) {
    if (strgpus == "all") {
        int count = 0;
#ifndef CPU_ONLY
        CUDA_CHECK(cudaGetDeviceCount(&count));
#else
        NO_GPU;
#endif
        for (int i = 0; i < count; ++i) {
            gpus->push_back(i);
        }
    } else if (strgpus.size()) {
        vector<string> strings;
        boost::split(strings, strgpus, boost::is_any_of(","));
        for (int i = 0; i < strings.size(); ++i) {
            gpus->push_back(boost::lexical_cast<int>(strings[i]));
        }
    } else {
        CHECK_EQ(gpus->size(), 0);
    }
}

int GetConfig(DETECT_INFO_S *pstInfo, const char* ps8szConfigFile) {
	const char * pEnd4 = ps8szConfigFile + strlen(ps8szConfigFile) - 4;
	int nRet = 0;
	if (0 == strcmp(pEnd4, ".cfg")) {
		LOG(INFO) << "LoadFile2ModelInfo: " << ps8szConfigFile;
		wstring wstrConfigFile = MultiByte2Wide(ps8szConfigFile, CP_UTF8);
		nRet = wLoadCfgFile2ModelInfo(wstrConfigFile, & pstInfo->stMainModel);
		LOG(INFO) << "LoadFile2ModelInfo: " << nRet;
		if (SUCCESS != nRet) {
			return SCWERR_CONFIG;
		}
		if (pstInfo->stMainModel.postmode.find("classify") != string::npos) {
			LOG(INFO) << "LoadFile2ModelInfo: " << pstInfo->stMainModel.labelmapfile;
			nRet = wLoadCfgFile2ModelInfo(MultiByte2Wide(pstInfo->stMainModel.labelmapfile.c_str(), CP_UTF8), &pstInfo->stSubModel);
			LOG(INFO) << "LoadFile2ModelInfo: " << nRet;
			if (SUCCESS != nRet) {
				return SCWERR_CONFIG;
			}
		}
	} else if (0 == strcmp(pEnd4, ".xml")) {
		LOG(INFO) << "LoadFile2ModelInfoXml: " << ps8szConfigFile;
		nRet = LoadXmlFile2ModelInfo(ps8szConfigFile, & pstInfo->stSubModel);
		LOG(INFO) << "LoadFile2ModelInfoXml: " << nRet;
		if (SUCCESS != nRet) {
			return SCWERR_CONFIG;
		}
		if (pstInfo->stMainModel.postmode.find("classify") != string::npos) {
			LOG(INFO) << "LoadFile2ModelInfoXml: " << pstInfo->stMainModel.labelmapfile;
			nRet = LoadXmlFile2ModelInfo(pstInfo->stMainModel.labelmapfile, &pstInfo->stSubModel);
			LOG(INFO) << "LoadFile2ModelInfoXml: " << nRet;
			if (SUCCESS != nRet) {
				return SCWERR_CONFIG;
			}
		}
	} else {
		LOG(ERROR) << "not .cfg or .xml file: " << ps8szConfigFile;
		nRet = SCWERR_CONFIG;
	}

	return SCWERR_NOERROR;
}

int InitModel(MODEL_INFO_S *pstInfo) {
    // Set device id and mode
    vector<int> gpus;
    get_gpus(&gpus, pstInfo->gpus);
    if (gpus.size() != 0) {
        LOG(INFO) << "Use GPU with device ID " << gpus[0];
#ifndef CPU_ONLY
        cudaDeviceProp device_prop;
        cudaGetDeviceProperties(&device_prop, gpus[0]);
        LOG(INFO) << "GPU device name: " << device_prop.name;
        std::cout << "caffe: GPU device name: " << device_prop.name << "\n";
#endif
        Caffe::SetDevice(gpus[0]);
        Caffe::set_mode(Caffe::GPU);
    } else {
        //LOG(INFO) << "Use CPU.";
        //Caffe::set_mode(Caffe::CPU);
		LOG(ERROR) << "NO GPU.";
		return SCWERR_NOGPU;
    }

    // set caffe
    if (pstInfo->encry == "none") {
        pstInfo->caffe_net = new Net<float>(pstInfo->model, caffe::TEST);
        pstInfo->caffe_net->CopyTrainedLayersFrom(pstInfo->weight);
        return SCWERR_NOERROR;
    } else {
        char* modelbuf = NULL;
        char* weightbuf = NULL;
        int modellen;
        int weightlen;
		wstring weight = MultiByte2Wide(pstInfo->weight.c_str(), CP_UTF8);
		if (SUCCESS != VimDecrypt(weight.c_str(), &modelbuf, &weightbuf,
                &modellen, &weightlen, 
                pstInfo->encry.c_str(), pstInfo->key.c_str())) {
            return SCWERR_MODEL;
        }
        // load model
        caffe::NetParameter model_param;
        string strmodel(modelbuf, modelbuf + modellen);
		if (!google::protobuf::TextFormat::ParseFromString(strmodel, &model_param)) {
			if (modelbuf) {
				free(modelbuf);
			}
			if (weightbuf) {
				free(weightbuf);
			}
			return SCWERR_MODEL;
		}
        if (modelbuf) {
            free(modelbuf);
        }

        // load prototxt
        caffe::NetParameter weight_param;
        google::protobuf::io::CodedInputStream coded_input(
                            (google::protobuf::uint8 *)weightbuf, 
                            weightlen);
        coded_input.SetTotalBytesLimit(536870912, 536870912);
        weight_param.ParseFromCodedStream(&coded_input);
		if (weightbuf) {
			free(weightbuf);
		}

        // Instantiate the caffe net.
        pstInfo->caffe_net = new Net<float>(model_param);
        
        if (pstInfo->caffe_net) {
            pstInfo->caffe_net->CopyTrainedLayersFrom(weight_param);
            return SCWERR_NOERROR;
        } else {
            return SCWERR_MODEL;
        }
    }
	return SCWERR_NOERROR;
}

int isspace(unsigned char c) {
	return std::isspace(c);
}

// trim from start (in place)
static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<unsigned char, int>(isspace))));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<unsigned char, int>(isspace))).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
	ltrim(s);
	return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
	rtrim(s);
	return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
	return rtrim_copy(ltrim_copy(s));
}

vector<string> SplitBySeps(const char* pszWithSep, const char* pszSep) {
	char szTemp[_MAX_PATH << 10] = {};
	strcpy_s(szTemp, pszWithSep);
	vector<string> vec;
	char *next_token;
	char *token = strtok_s(szTemp, pszSep, &next_token);
	while (token != NULL) {
		string str = trim_copy(string(token));
		if (!str.empty()) {
			vec.push_back(str);
		}
		token = strtok_s(NULL, pszSep, &next_token);
	}
	return vec;
}

int generateMap(std::map<std::string, std::string> & mapObjAlias, std::string strObjectTypes, const char * pszObjectSep = g_szObjectSep, const char * pszAliasSep = g_szAliasSep, const char * pszSynonymSep = g_szSynonymSep) {
	vector<string> vecObj = SplitBySeps(strObjectTypes.c_str(), pszObjectSep);
	LOG(INFO) << "object type count: " << vecObj.size();
	if (0 == vecObj.size()) {
		LOG(WARNING) << "none object type found.";
		return SCWERR_NOERROR;
	}
	for (vector<string>::const_iterator citObj = vecObj.cbegin(); citObj < vecObj.cend(); ++citObj) {
		vector<string> vecAlias = SplitBySeps(citObj->c_str(), pszAliasSep);
		if (1 != vecAlias.size() && 2 != vecAlias.size()) {
			LOG(ERROR) << "more than one separator of " << pszAliasSep << " found in: " << citObj->c_str();
			return SCWERR_PARAMETER;
		}
		vector<string> vecSynonym = SplitBySeps(vecAlias.at(0).c_str(), pszSynonymSep);
		string alias = 2 == vecAlias.size() ? vecAlias.at(1) : vecSynonym.at(0);
		for (vector<string>::const_iterator citSyn = vecSynonym.cbegin(); citSyn < vecSynonym.cend(); ++citSyn) {
			if (mapObjAlias.find(*citSyn) != mapObjAlias.end()) {
				LOG(ERROR) << "object type exist already: " << *citSyn;
				return SCWERR_PARAMETER;
			}
			mapObjAlias[*citSyn] = alias;
		}
	}
	LOG(INFO) << "object type unique count: " << mapObjAlias.size();
	return SCWERR_NOERROR;
}

std::string getObject(std::string first, std::string second, const char * pszObjectSep, const char * pszAliasSep) {
	std::string str = first;
	if (0 != first.compare(second)) {
		str += pszAliasSep;
		str += second;
	}
	str += pszObjectSep;
	return str;
}

std::string getObjects(std::map<std::string, std::string> mapObjAlias, const char * pszObjectSep = g_szObjectSep, const char * pszAliasSep = g_szAliasSep) {
	std::string str;
	for (std::map<std::string, std::string>::iterator iter = mapObjAlias.begin(); iter != mapObjAlias.end(); ++iter) {
		str += getObject(iter->first, iter->second, pszObjectSep, pszAliasSep);
	}
	return str;
}

int combineObjects(std::string & strObjects, std::map<std::string, std::string> & mapObjAlias, const char * pszObj, const char * pszObjectSep = g_szObjectSep, const char * pszAliasSep = g_szAliasSep) {
	LOG(INFO) << "model support object type string: " << pszObj;
	vector<string> vecObj = SplitBySeps(pszObj, pszObjectSep);
	LOG(INFO) << "model support object type count: " << vecObj.size();
	if (0 == vecObj.size()) {
		LOG(ERROR) << "none object type found.";
		return SCWERR_CONFIG;
	}

	strObjects = "";
	for (vector<string>::iterator iter = vecObj.begin(); iter != vecObj.end(); ++iter) {
		if (mapObjAlias.find(*iter) == mapObjAlias.end()) {
			LOG(INFO) << "ignore the model object: " << *iter;
			continue;
		}
		strObjects += getObject(*iter, mapObjAlias.at(*iter), pszObjectSep, pszAliasSep);
	}
	if (strObjects.empty()) {
		LOG(INFO) << "ignore all model objects, so the model is not used.";
		return SCWERR_CONFIG;
	}

	LOG(INFO) << "model objects used: " << strObjects;
	return SCWERR_NOERROR;
}

int convert2MyImgRes(MyImgRes& myImgRes, DETECT_FILE_S& file, vector<string>& labels) {
	std::map<std::string, std::vector<std::pair<MyRect, int>>> mapObject;
	vector<DETECT_BOX_S> boxes = file.boxes;
	for (int j = 0; j < boxes.size(); j++) {
		int label = static_cast<int>(boxes[j].label);
		int score = static_cast<int>(boxes[j].score * 100);
		if (label >= labels.size()) {
			return FAIL;
		}
		string strObjecttype = labels[label];
		MyRect rc(boxes[j].x, boxes[j].y, boxes[j].x + boxes[j].w, boxes[j].y + boxes[j].h);
		if (mapObject.find(strObjecttype) == mapObject.end()) {
			//std::vector<std::pair<MyRect, int>> vecRectScore;
			mapObject[strObjecttype] = std::vector<std::pair<MyRect, int>>();
		}
		mapObject[strObjecttype].push_back(std::pair<MyRect, int>(rc, score));
	}

	myImgRes.strImgname = file.filename;
	myImgRes.width = file.width;
	myImgRes.height = file.height;
	myImgRes.mapObject = mapObject;

	return SUCCESS;
}

string CreateJsonV2(vector<MyImgRes>& vecImgRes, std::map<std::string, std::string > mapObjAlias) {
	cJSON *root, *images, *objs, *coords;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "ver", "2.0");
	cJSON_AddItemToObject(root, "images", images = cJSON_CreateArray());
	for (vector<MyImgRes>::iterator it = vecImgRes.begin(); it != vecImgRes.end(); ++it) {
		cJSON *image = cJSON_CreateObject();
		cJSON_AddItemToObject(images, "", image);
		cJSON_AddStringToObject(image, "name", it->strImgname.c_str());
		cJSON_AddNumberToObject(image, "w", it->width);
		cJSON_AddNumberToObject(image, "h", it->height);
		cJSON_AddItemToObject(image, "objs", objs = cJSON_CreateArray());
		std::map<std::string, std::vector<std::pair<MyRect, int>>> & mapObject = it->mapObject;
		for (map<string, vector<pair<MyRect, int>>>::iterator it2 = mapObject.begin(); it2 != mapObject.end(); ++it2) {
			cJSON *obj = cJSON_CreateObject();
			cJSON_AddItemToObject(objs, "", obj);
			string strObj = it2->first.c_str();
			if (mapObjAlias.end() != mapObjAlias.find(strObj)) {
				strObj += g_szAliasSep + mapObjAlias[strObj];
			}
			cJSON_AddStringToObject(obj, "obj", strObj.c_str());
			cJSON_AddItemToObject(obj, "locations", coords = cJSON_CreateArray());
			std::vector<std::pair<MyRect, int>> vecRect = it2->second;
			for (vector<pair<MyRect, int>>::iterator it3 = vecRect.begin(); it3 != vecRect.end(); ++it3) {
				cJSON *coord = cJSON_CreateObject();
				cJSON_AddItemToObject(coords, "", coord);
				cJSON_AddNumberToObject(coord, "conf", it3->second);
				cJSON *rect = cJSON_CreateObject();
				cJSON_AddItemToObject(coord, "rect", rect);
				cJSON_AddNumberToObject(rect, "left", it3->first.left);
				cJSON_AddNumberToObject(rect, "top", it3->first.top);
				cJSON_AddNumberToObject(rect, "right", it3->first.right);
				cJSON_AddNumberToObject(rect, "bottom", it3->first.bottom);
			}
		}
	}

	char *out = cJSON_Print(root);
	cJSON_Delete(root);
	string str = out;
	free(out);

	return str;
}

int SCWInitObjectDetection(
	HScwAlg* phAlg,
	char** const ppszObjectTypeRes,
	const char* pszObjectTypes,
	const char* pszConfig,
	const unsigned char nKind
) {
	if (NULL == phAlg) {
		LOG(ERROR) << "NULL param 1.";
		return SCWERR_PARAMETER;
	}
	if (NULL != *phAlg && 0 < g_vecScwAlgHandle.size()) {
		if (std::find(g_vecScwAlgHandle.begin(), g_vecScwAlgHandle.end(), *phAlg) != g_vecScwAlgHandle.end()) {
			LOG(ERROR) << "This function should be invoked only once.";
			return SCWERR_INITIALIZED;
		}
	}

	LOG(INFO) << "SCWInitObjectDetection param 1: " << phAlg;
	if (NULL == ppszObjectTypeRes) {
		LOG(INFO) << "NULL param 2.";
	} else {
		LOG(INFO) << "SCWInitObjectDetection param 2: " << (void *)ppszObjectTypeRes;
	}

	if (NULL == pszObjectTypes) {
		LOG(WARNING) << "NULL param 3.";
	} else {
		LOG(INFO) << "SCWInitObjectDetection param 3: \"" << pszObjectTypes << "\"";
	}

	if (NULL == pszConfig) {
		LOG(INFO) << "SCWInitObjectDetection param 4: NULL";
	} else {
		LOG(INFO) << "SCWInitObjectDetection param 4: \"" << pszConfig << "\"";
	}

	int n1 = nKind & 1;
	int n2 = (nKind >> 4) & 1;
	LOG(INFO) << "SCWInitObjectDetection param 5: " << (int)nKind << ", so param 3 is utf8 string or file: " << n1 << ", and param 4 is utf8 file or string:" << n2;

	string s8ObjectTypes = NULL == pszObjectTypes ? "" : pszObjectTypes;
	std::map<std::string, std::string> g_mapObjAlias;
	if (1 == n1 && s8ObjectTypes.length() > 0) {
		// pszObjectTypes utf8 string denoting the file name, so read file content first;
		unsigned long nFileBinLen;
		unsigned char* pFileBins;
		if (!ReadFileLenAndDataA(nFileBinLen, &pFileBins, s8ObjectTypes.c_str())) {
			return SCWERR_FILE;
		}
		s8ObjectTypes = string((char*)pFileBins, nFileBinLen);
		delete[] pFileBins;
	}
	LOG(INFO) << "Split Objects with 3 symbols: " << g_szObjectSep << ", " << g_szAliasSep << ", " << g_szSynonymSep;
	int nRet = generateMap(g_mapObjAlias, s8ObjectTypes);
	if (SCWERR_NOERROR != nRet) {
		return nRet;
	}

	DETECT_INFO_S* pstInfo = new DETECT_INFO_S(){};
	if (0 == n2) {
		// TODO: pszConfig is not file, not implemented yet. 
		LOG(ERROR) << "pszConfig is not file, not implemented yet.";
		delete pstInfo;
		return SCWERR_NOTIMPLEMENT;
	} else {
		nRet = GetConfig(pstInfo, pszConfig);
		if (SCWERR_NOERROR != nRet) {
			delete pstInfo;
			return nRet;
		}
	}

	LOG(INFO) << "RSA: " << RSA;
#if		RSA == RSA_SENTINEL
	hasp_u32_t FeatureID = 1123u;
	ErrorPrinter errorPrinter;
	haspStatus status;

	//Demonstrates the login to the default feature of a key
	//Searches both locally and remotely for it
	cout << "login to default feature: " << FeatureID;
	Chasp hasp1(ChaspFeature::ChaspFeature(FeatureID));
	status = hasp1.login(vendorCode);
	errorPrinter.printError(status);
	if (!HASP_SUCCEEDED(status)) {
		LOG(ERROR) << "login error: " << status;
		delete pstInfo;
		return SCWERR_DONGLE;
	}

	SYSTEMTIME	curr_tm = {};
	GetLocalTime(&curr_tm);
	char szBuf[_MAX_PATH] = {};
	sprintf_s(szBuf, _MAX_PATH, "%03d%02d%03d%02d%02d%04d", curr_tm.wMilliseconds, curr_tm.wHour, rand(), curr_tm.wSecond, curr_tm.wMinute, clock());
	string str = string(szBuf);
	status = hasp1.encrypt(str);
	if (!HASP_SUCCEEDED(status)) {
		LOG(ERROR) << "hasp  encrypt: " << status;
		delete pstInfo;
		return SCWERR_DONGLE;
	}
	status = hasp1.decrypt(str);
	if (!HASP_SUCCEEDED(status)) {
		LOG(ERROR) << "hasp decrypt: " << status;
		return SCWERR_DONGLE;
	}
	if (str != string(szBuf)) {
		LOG(ERROR) << "hasp : " << status;
		delete pstInfo;
		return SCWERR_DONGLE;
	}

	hasp1.logout();

#elif	RSA == RSA_TENDYRON
	BOOL bRet = TDR_GetFirstReaderName(&pszReaders);
	if (!bRet) {
		delete pstInfo;
		return SCWERR_DONGLE;
	}

	SYSTEMTIME	curr_tm = {};
	GetLocalTime(&curr_tm);
	char szBuf[_MAX_PATH] = {};
	sprintf_s(szBuf, _MAX_PATH, "%03d%02d%03d%02d%02d%04d", curr_tm.wMilliseconds, curr_tm.wHour, rand(), curr_tm.wSecond, curr_tm.wMinute, clock());

	unsigned char * pucBufOut = NULL;
	//if (FAIL == tendyron(ucBuf, sizeof(ucBuf), &pucBufOut)) {
	if (FAIL == tendyron((unsigned char *)szBuf, strlen(szBuf), &pucBufOut)) {
		delete pstInfo;
		return SCWERR_DONGLE;
	}
	unsigned char * pucBufOut2 = NULL;
	tendyronRsa(pucBufOut, &pucBufOut2);
	TDR_FreeMemory((void **)&pucBufOut);
	//int n = memcmp(szBuf, pucBufOut2 + 128 - strlen(szBuf), strlen(szBuf));
	int n = memcmp(szBuf, pucBufOut2 + 112, 16);
	TDR_FreeMemory((void **)&pucBufOut2);
	if (0 != n) {
		delete pstInfo;
		return SCWERR_DONGLE;
	}
#endif

	LOG(INFO) << "Init Model: ";
	nRet = InitModel(&pstInfo->stMainModel);
	if (SCWERR_NOERROR != nRet) {
		delete pstInfo;
		return nRet;
	}
	if (pstInfo->stMainModel.postmode.find("classify") != string::npos) {
		LOG(INFO) << "Init Sub Model: ";
		nRet = InitModel(&pstInfo->stSubModel);
		if (SCWERR_NOERROR != nRet) {
			delete pstInfo;
			return nRet;
		}
	}

	string strObjUsed = "";
	string strObjUnused = "";
	std::vector<std::string> vecObjIn;
	vector<string> vecObj = pstInfo->stMainModel.labels;
	vector<string>::iterator iter = vecObj.begin();
	++iter;
	if (g_mapObjAlias.size() == 0) {
		for (; iter != vecObj.end(); ++iter) {
			strObjUsed += *iter;
			strObjUsed += g_szObjectSep;
		}
	} else {
	for (map<string, string>::iterator it = g_mapObjAlias.begin(); it != g_mapObjAlias.end(); ++it) {
		vecObjIn.push_back(it->first);
	}

	for (; iter != vecObj.end(); ++iter) {
		if (g_mapObjAlias.find(*iter) == g_mapObjAlias.end()) {
			strObjUnused += *iter;
			strObjUnused += g_szObjectSep;
		} else {
			strObjUsed += *iter;
			strObjUsed += g_szAliasSep;
			strObjUsed += g_mapObjAlias[*iter];
			strObjUsed += g_szObjectSep;
			
			if (ppszObjectTypeRes) {
				vecObjIn.erase(std::remove(vecObjIn.begin(), vecObjIn.end(), *iter), vecObjIn.end());
			}
		}
	}
	}

	if (0 == strObjUsed.size()) {
		LOG(ERROR) << "all objects unused: " << strObjUnused;
		return SCWERR_ALLOBJECTSUNUSED;
	}

	*phAlg = (HScwAlg)pstInfo;
	g_vecScwAlgHandle.push_back(*phAlg);
	g_mapAlgObjAlias[*phAlg] = g_mapObjAlias;


	// parse object kind: used, unused, invalid
	if (ppszObjectTypeRes) {
		string strObjInvalid = "";
		for (iter = vecObjIn.begin(); iter != vecObjIn.end(); ++iter) {
			strObjInvalid += *iter;
			strObjInvalid += g_szAliasSep;
			strObjInvalid += g_mapObjAlias[*iter];
			strObjInvalid += g_szObjectSep;
		}

		string strObjTypeRes = strObjUsed;
		strObjTypeRes += g_szObjectSep;
		if (0 == strObjUnused.size()) {
			strObjTypeRes += string(" ") + g_szObjectSep;
		} else {
			strObjTypeRes += strObjUnused;
		}
		strObjTypeRes += g_szObjectSep;
		if (0 != strObjInvalid.size()) {
			strObjTypeRes += strObjInvalid;
			strObjTypeRes += g_szObjectSep;
		}

		int nLen = (int)strObjTypeRes.length();
		*ppszObjectTypeRes = new char[nLen + 1]();
		memcpy_s(*ppszObjectTypeRes, nLen, strObjTypeRes.c_str(), nLen);
		LOG(INFO) << "return object kind result: " << *ppszObjectTypeRes;
	}

	::google::FlushLogFiles(0);
	return SCWERR_NOERROR;
}

int SCWUninitObjectDetection(
	HScwAlg* phAlg
) {
	if (NULL == phAlg) {
		LOG(ERROR) << "NULL param 1.";
		return SCWERR_PARAMETER;
	}
	if (NULL == *phAlg) {
		LOG(ERROR) << "param 1 point to NULL.";
		return SCWERR_NOINIT;
	}
	if (0 == g_vecScwAlgHandle.size()) {
		LOG(ERROR) << "no invoke SCWInitObjectDetection function before.";
		return SCWERR_NOINIT;
	}
	if (std::find(g_vecScwAlgHandle.begin(), g_vecScwAlgHandle.end(), *phAlg) == g_vecScwAlgHandle.end()) {
		LOG(ERROR) << "no invoke SCWInitObjectDetection function before.";
		return SCWERR_NOINIT;
	}
	g_vecScwAlgHandle.erase(std::remove(g_vecScwAlgHandle.begin(), g_vecScwAlgHandle.end(), *phAlg), g_vecScwAlgHandle.end());
	g_mapAlgObjAlias.erase(g_mapAlgObjAlias.find(*phAlg));

    DETECT_INFO_S *pstInfo = (DETECT_INFO_S *)* phAlg;
    if (pstInfo->stMainModel.caffe_net) {
        delete pstInfo->stMainModel.caffe_net;
    }
    if (pstInfo->stSubModel.caffe_net) {
        delete pstInfo->stSubModel.caffe_net;
	}
	delete pstInfo;
	* phAlg = nullptr;

#if		RSA == RSA_SENTINEL

#elif	RSA == RSA_TENDYRON
	if (pszReaders) {
		TDR_FreeMemory((void **)&pszReaders);
	}
#else
#endif
	return SCWERR_NOERROR;
}

void SCWRelease(
	char** const ppszBuf
) {
	if (NULL == ppszBuf || NULL == *ppszBuf) {
		return;
	}

	delete[] * ppszBuf;
	*ppszBuf = NULL;
}

int Detect(cv::Mat& org_img, std::wstring wstrImgPath, vector<DETECT_FILE_S>& totalfiles, MODEL_INFO_S* pstMainInfo, MODEL_INFO_S* pstPostInfo) {
#if	RSA == RSA_SENTINEL
#if CALC_SENTINEL_TIME
	union {
		ULARGE_INTEGER uli; // QuadPart 100 ns
		FILETIME       ft;
	}u1, u2;
	SYSTEMTIME st_start = {};
	GetLocalTime(&st_start);
	SystemTimeToFileTime(&st_start, &u1.ft);
	SYSTEMTIME st_end = {};
#endif

	//Demonstrates the login to the default feature of a key
	//Searches both locally and remotely for it
	//cout << "login to default feature: " << 1123u;
	Chasp hasp1(ChaspFeature::ChaspFeature(1123u));
	haspStatus status = hasp1.login(vendorCode);
	if (!HASP_SUCCEEDED(status)) {
		ErrorPrinter errorPrinter;
		errorPrinter.printError(status);
		LOG(ERROR) << "login error: " << status;
		return SCWERR_DONGLE;
	}
#if CALC_SENTINEL_TIME
	GetLocalTime(&st_end);
	SystemTimeToFileTime(&st_end, &u2.ft);
	int uld = (u2.uli.QuadPart - u1.uli.QuadPart) * 1.E-4; // milliseconds
	cout << "login time duration(ms): " << uld << endl;

	SYSTEMTIME	curr_tm = {};
	GetLocalTime(&curr_tm);
	char szBuf[_MAX_PATH] = {};
	sprintf_s(szBuf, _MAX_PATH, "%03d%02d%03d%02d%02d%04d", curr_tm.wMilliseconds, curr_tm.wHour, rand(), curr_tm.wSecond, curr_tm.wMinute, clock());
	string str = string(szBuf);
	status = hasp1.encrypt(str);
	if (!HASP_SUCCEEDED(status)) {
		LOG(ERROR) << "hasp  encrypt: " << status;
		return SCWERR_DONGLE;
	}
	GetLocalTime(&st_end);
	SystemTimeToFileTime(&st_end, &u2.ft);
	uld = (u2.uli.QuadPart - u1.uli.QuadPart) * 1.E-4; // milliseconds
	cout << "encrypt time duration(ms): " << uld << endl;

	status = hasp1.decrypt(str);
	if (!HASP_SUCCEEDED(status)) {
		LOG(ERROR) << "hasp decrypt: " << status;
		return SCWERR_DONGLE;
	}
	if (str != string(szBuf)) {
		LOG(ERROR) << "hasp : " << status;
		return SCWERR_DONGLE;
	}
	GetLocalTime(&st_end);
	SystemTimeToFileTime(&st_end, &u2.ft);
	uld = (u2.uli.QuadPart - u1.uli.QuadPart) * 1.E-4; // milliseconds
	cout << "decrypt time duration(ms): " << uld << endl;
#endif

	hasp1.logout();

#if CALC_SENTINEL_TIME
	GetLocalTime(&st_end);
	SystemTimeToFileTime(&st_end, &u2.ft);
	int ms = (u2.uli.QuadPart - u1.uli.QuadPart) * 1.E-4; // milliseconds
	cout << "time duration(ms): " << ms << endl;
#endif
#elif	RSA == RSA_TENDYRON
	SYSTEMTIME	curr_tm = {};
	GetLocalTime(&curr_tm);
	char szBuf[_MAX_PATH] = {};
	sprintf_s(szBuf, _MAX_PATH, "%03d%02d%03d%02d%02d%04d", curr_tm.wMilliseconds, curr_tm.wHour, rand(), curr_tm.wSecond, curr_tm.wMinute, clock());

	unsigned char* pucBufOut = NULL;
	if (FAIL == tendyron((unsigned char*)szBuf, strlen(szBuf), &pucBufOut)) {
		return SCWERR_DONGLE;
	}
	unsigned char* pucBufOut2 = NULL;
	tendyronRsa(pucBufOut, &pucBufOut2);
	TDR_FreeMemory((void**)&pucBufOut);
	int n = memcmp(szBuf, pucBufOut2 + 112, 16);
	TDR_FreeMemory((void**)&pucBufOut2);
	if (0 != n) {
		return SCWERR_DONGLE;
	}
#endif

	vector<DETECT_BOX_S> retbox = run_caffe(org_img, pstMainInfo);
	vector<DETECT_BOX_S> savebox = postprocess(retbox, pstMainInfo->postmode, pstPostInfo, org_img);
	DETECT_FILE_S tmpfile = {};
	tmpfile.filename = Wide2MultiByte(wstrImgPath.c_str(), CP_UTF8);
	tmpfile.wfilename = wstrImgPath;
	tmpfile.boxes.clear();
	tmpfile.boxes.insert(tmpfile.boxes.end(), savebox.begin(), savebox.end());
	tmpfile.width = org_img.cols;
	tmpfile.height = org_img.rows;

	totalfiles.push_back(tmpfile);
	return SCWERR_NOERROR;
}

int SCWDetectObjectByFile(
	char** const ppszDetectionResult,
	HScwAlg hAlg,
	const char*	pszImagePath,
	const int	nConfidentialThreshold
) {
	LOG(INFO) << "SCWDetectObjectByFile(): " << pszImagePath << ", missErrorRatio:" << nConfidentialThreshold;
	if (NULL == ppszDetectionResult) {
		LOG(ERROR) << "NULL param 1.";
		return SCWERR_PARAMETER;
	}

	if (NULL == hAlg) {
		LOG(ERROR) << "NULL param 2.";
		return SCWERR_NOINIT;
	}
	if (0 == g_vecScwAlgHandle.size()) {
		LOG(ERROR) << "no invoke SCWInitObjectDetection function before.";
		return SCWERR_NOINIT;
	}
	if (std::find(g_vecScwAlgHandle.begin(), g_vecScwAlgHandle.end(), hAlg) == g_vecScwAlgHandle.end()) {
		LOG(ERROR) << "no invoke SCWInitObjectDetection function before.";
		return SCWERR_NOINIT;
	}

	if (NULL == pszImagePath) {
		LOG(ERROR) << "NULL param 3.";
		return SCWERR_PARAMETER;
	}
	if (0 > nConfidentialThreshold || 100 < nConfidentialThreshold) {
		LOG(ERROR) << "param 4 not in the range [0, 100]: " << nConfidentialThreshold;
		return SCWERR_PARAMETER;
	}

	DETECT_INFO_S *pstInfo = (DETECT_INFO_S *)hAlg;

	MODEL_INFO_S *pstPostInfo = NULL;
    vector<string> labels;
	if (pstInfo->stMainModel.postmode.find("classify") != string::npos) {
		labels = pstInfo->stSubModel.labels;
        pstPostInfo = &pstInfo->stSubModel;
	} else {
		labels = pstInfo->stMainModel.labels;
        pstPostInfo = &pstInfo->stMainModel;
	}

	std::vector<std::string> vecImgPathS8 = SplitBySeps(pszImagePath, g_szObjectSep);
	if (0 == vecImgPathS8.size()) {
		LOG(ERROR) << "no image path found.";
		return SCWERR_PARAMETER;
	}

	int nCount = vecImgPathS8.size();
	if (1 < nCount) {
		sort(vecImgPathS8.begin(), vecImgPathS8.end());
		vecImgPathS8.erase(unique(vecImgPathS8.begin(), vecImgPathS8.end()), vecImgPathS8.end());
		if (nCount > vecImgPathS8.size()) {
			LOG(ERROR) << "duplicate image path found.";
			return SCWERR_PARAMETER;
		}
	}

	// path encoding utf8 to Unicode, and check image path existence
	std::vector<std::wstring> vecImgPath;
	for (vector<string>::iterator it  = vecImgPathS8.begin(); it != vecImgPathS8.end(); ++it) {
		wstring wstrImgFile = MultiByte2Wide(it->c_str(), CP_UTF8);
		if (! wDoesFileExist(wstrImgFile.c_str())) {
			LOG(ERROR) << "image file not found: " << *it;
			return SCWERR_FILENOTFOUND;
		}
		vecImgPath.push_back(wstrImgFile);
	}
 
    Net<float> *caffe_net = pstInfo->stMainModel.caffe_net;
    Blob<float>* input_layer = caffe_net->input_blobs()[0];
    int width = input_layer->width();
    int height = input_layer->height();
    
    MODEL_INFO_S *pstMainInfo = &pstInfo->stMainModel;
    vector<DETECT_FILE_S> totalfiles;
    for (int i = 0; i < vecImgPath.size(); ++i) {
		if (!wDoesFileExist(vecImgPath[i].c_str())) {
			DETECT_FILE_S tmpfile = {};
			tmpfile.filename = Wide2MultiByte(vecImgPath[i].c_str(), CP_UTF8);
			tmpfile.wfilename = vecImgPath[i];
			tmpfile.width = 0;
			tmpfile.height = 0;
			tmpfile.boxes.clear();
			totalfiles.push_back(tmpfile);
			continue;
		}
        
		cv::Mat org_img = imReadImage(vecImgPath[i].c_str());
		if (org_img.empty() || org_img.cols == 0 || org_img.rows == 0) {
			DETECT_FILE_S tmpfile = {};
			tmpfile.filename = Wide2MultiByte(vecImgPath[i].c_str(), CP_UTF8);
			tmpfile.wfilename = vecImgPath[i];
			tmpfile.width = 0;
			tmpfile.height = 0;
			tmpfile.boxes.clear();
			totalfiles.push_back(tmpfile);
			continue;
		}

		int nRet = Detect(org_img, vecImgPath[i], totalfiles, pstMainInfo, pstPostInfo);
		if (SCWERR_NOERROR != nRet) {
			return nRet;
		}
	}
    
	string str;
	if (pstMainInfo->outmode == "json") {
		vector<MyImgRes> vecImgRes;
		for (vector<DETECT_FILE_S>::iterator it = totalfiles.begin(); it != totalfiles.end(); ++it) {
			MyImgRes myImgRes = {};
			int n = convert2MyImgRes(myImgRes, *it, labels);
			if (n == 0) {
				vecImgRes.push_back(myImgRes);
			}
		}
		str = CreateJsonV2(vecImgRes, g_mapAlgObjAlias[hAlg]); // str utf8 encoding
	} else if (pstMainInfo->outmode == "jsonspec") {
		str = JsonOutputSpec(totalfiles, labels);
	} else if (pstMainInfo->outmode == "xml") {
		str = XmlOutput(totalfiles, labels);
	} else {
		str = JsonOutput(totalfiles, labels);
	}
#ifdef SHOW_MESSAGE
	printf("\n------>\n%s\n<------\n", str.c_str());
#endif

	*ppszDetectionResult = new char[str.length() + 1]();
	memcpy_s(*ppszDetectionResult, str.length(), str.c_str(), str.length());
	LOG(INFO) << "detection result:------>\n" << *ppszDetectionResult << "\n<------";
	LOG(INFO);

	::google::FlushLogFiles(0);
	return SCWERR_NOERROR;
}

int SCWDetectObjectByFileBuf(
	char** const ppszDetectionResult,
	HScwAlg hAlg,
	const struct SCWImgBufStruct * pScwImgStruct,
	const int		nScwImgStructCount
) {
	LOG(INFO) << "SCWDetectObjectByFileBuf(): ";
	if (NULL == ppszDetectionResult) {
		LOG(ERROR) << "NULL param 1.";
		return SCWERR_PARAMETER;
	}

	if (NULL == hAlg) {
		LOG(ERROR) << "NULL param 2.";
		return SCWERR_NOINIT;
	}
	if (0 == g_vecScwAlgHandle.size()) {
		LOG(ERROR) << "no invoke SCWInitObjectDetection function before.";
		return SCWERR_NOINIT;
	}
	if (std::find(g_vecScwAlgHandle.begin(), g_vecScwAlgHandle.end(), hAlg) == g_vecScwAlgHandle.end()) {
		LOG(ERROR) << "no invoke SCWInitObjectDetection function before.";
		return SCWERR_NOINIT;
	}
	if (NULL == pScwImgStruct) {
		LOG(ERROR) << "NULL param 3.";
		return SCWERR_PARAMETER;
	}
	if (0 >= nScwImgStructCount) {
		LOG(ERROR) << "param 4 should be positive.";
		return SCWERR_PARAMETER;
	}
	for (int i = 0; i < nScwImgStructCount; ++i) {
		if (NULL == pScwImgStruct[i].pImageBuf) {
			LOG(ERROR) << "NULL image file buffer in No. " << i;
			return SCWERR_PARAMETER;
		}
		if (0 >= pScwImgStruct[i].nImageBufLen) {
			LOG(ERROR) << "image file buffer size error in No. " << i;
			return SCWERR_PARAMETER;
		}
		if (0 >= pScwImgStruct[i].nImgType) {
			LOG(ERROR) << "image type error in No. " << i;
			return SCWERR_PARAMETER;
		}
		if (0 > pScwImgStruct[i].nConfidentialThreshold || 100 < pScwImgStruct[i].nConfidentialThreshold) {
			LOG(ERROR) << "nConfidentialThreshold not in the range [0, 100] in No. " << i;
			return SCWERR_PARAMETER;
		}
	}
	if (1 < nScwImgStructCount) {
		std::vector<std::string> vecImgPathS8;
		for (int i = 0; i < nScwImgStructCount; ++i) {
			vecImgPathS8.push_back(pScwImgStruct[i].pszImagePath);
		}
		sort(vecImgPathS8.begin(), vecImgPathS8.end());
		vecImgPathS8.erase(unique(vecImgPathS8.begin(), vecImgPathS8.end()), vecImgPathS8.end());
		if (nScwImgStructCount > vecImgPathS8.size()) {
			LOG(ERROR) << "duplicate image path found.";
			return SCWERR_PARAMETER;
		}
	}

	DETECT_INFO_S *pstInfo = (DETECT_INFO_S *)hAlg;

	MODEL_INFO_S *pstPostInfo = NULL;
    vector<string> labels;
	if (pstInfo->stMainModel.postmode.find("classify") != string::npos) {
		labels = pstInfo->stSubModel.labels;
        pstPostInfo = &pstInfo->stSubModel;
	} else {
		labels = pstInfo->stMainModel.labels;
        pstPostInfo = &pstInfo->stMainModel;
	}

    Net<float> *caffe_net = pstInfo->stMainModel.caffe_net;
    Blob<float>* input_layer = caffe_net->input_blobs()[0];
    int width = input_layer->width();
    int height = input_layer->height();
    
    MODEL_INFO_S *pstMainInfo = &pstInfo->stMainModel;
    vector<DETECT_FILE_S> totalfiles;
    for (int i = 0; i < nScwImgStructCount; ++i) {
		vector<uchar> vecImg;
		for (int i = 0; i < pScwImgStruct->nImageBufLen; ++i) {
			vecImg.push_back(pScwImgStruct->pImageBuf[i]);
		}
		cv::Mat org_img = cv::imdecode(vecImg, cv::IMREAD_COLOR);
		if (org_img.empty() || org_img.cols == 0 || org_img.rows == 0) {
			DETECT_FILE_S tmpfile = {};
			tmpfile.filename = pScwImgStruct->pszImagePath;
			tmpfile.wfilename = MultiByte2Wide(pScwImgStruct->pszImagePath, CP_UTF8);
			tmpfile.width = 0;
			tmpfile.height = 0;
			tmpfile.boxes.clear();
			totalfiles.push_back(tmpfile);
			continue;
		}

		int nRet = Detect(org_img, MultiByte2Wide(pScwImgStruct->pszImagePath, CP_UTF8), totalfiles, pstMainInfo, pstPostInfo);
		if (SCWERR_NOERROR != nRet) {
			return nRet;
		}
	}
    
	string str;
	if (pstMainInfo->outmode == "json") {
		vector<MyImgRes> vecImgRes;
		for (vector<DETECT_FILE_S>::iterator it = totalfiles.begin(); it != totalfiles.end(); ++it) {
			MyImgRes myImgRes = {};
			int n = convert2MyImgRes(myImgRes, *it, labels);
			if (n == 0) {
				vecImgRes.push_back(myImgRes);
			}
		}
		str = CreateJsonV2(vecImgRes, g_mapAlgObjAlias[hAlg]); // str utf8 encoding
	} else if (pstMainInfo->outmode == "jsonspec") {
		str = JsonOutputSpec(totalfiles, labels);
	} else if (pstMainInfo->outmode == "xml") {
		str = XmlOutput(totalfiles, labels);
	} else {
		str = JsonOutput(totalfiles, labels);
	}
#ifdef SHOW_MESSAGE
	printf("\n------>\n%s\n<------\n", str.c_str());
#endif

	*ppszDetectionResult = new char[str.length() + 1]();
	memcpy_s(*ppszDetectionResult, str.length(), str.c_str(), str.length());
	LOG(INFO) << "detection result:------>\n" << *ppszDetectionResult << "\n<------";
	LOG(INFO);

	::google::FlushLogFiles(0);
	return SCWERR_NOERROR;
}