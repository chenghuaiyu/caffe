
#ifndef _GET_CONFIG_NORMAL_H_
#define _GET_CONFIG_NORMAL_H_

#include <string>
#include "../common.h"

using namespace std;

int GetModelInfoNormal(string configfile, MODEL_INFO_S *pstInfo, string strpath);
int LoadFile2ModelInfo(string configfile, MODEL_INFO_S *pstInfo);

int wGetModelInfoNormal(wstring configfile, MODEL_INFO_S *pstInfo, wstring wstrpath);
int wLoadCfgFile2ModelInfo(wstring configfile, MODEL_INFO_S *pstInfo);

#endif