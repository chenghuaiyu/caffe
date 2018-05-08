
#include <string>
#include <vector>

#include "../common.h"
#include "../util/util.h"

#include "get_config_normal.h"
#include "get_config_xml.h"

using namespace std;

int GetModelInfo(string configfile, MODEL_INFO_S *pstInfo, string strpath)
{
    int ret;
    std::cout << "strpath:" << strpath <<endl;
    std::cout << "configfile:" << configfile <<endl;
    if (IsFileExist(configfile+".cfg"))
    {
        ret = GetModelInfoNormal(configfile+".cfg", pstInfo, strpath);
    }
    else if(IsFileExist(configfile+".xml"))
    {
        ret = GetModelInfoXml(configfile+".xml", pstInfo, strpath);
    }
    else
    {
        ret = FAIL;
    }
    
    return ret;
} 
