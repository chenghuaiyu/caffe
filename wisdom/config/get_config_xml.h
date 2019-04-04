
#ifndef _GET_CONFIG_XML_H_
#define _GET_CONFIG_XML_H_

#include <string>
#include "../common.h"

using namespace std;

int GetModelInfoXml(string configfile, MODEL_INFO_S *pstInfo, string strpath);
int LoadXmlFile2ModelInfo(string configfile, MODEL_INFO_S *pstInfo);

#endif