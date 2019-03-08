
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
    std::cout << "dll directory: " << strpath <<endl;
    if (IsFileExist(configfile + ".cfg"))
    {
		ret = GetModelInfoNormal(configfile + ".cfg", pstInfo, strpath);
		std::cout << "GetModelInfoNormal(" << configfile + ".cfg): " << ret << endl;
    }
    else if(IsFileExist(configfile + ".xml"))
    {
		ret = GetModelInfoXml(configfile + ".xml", pstInfo, strpath);
		std::cout << "GetModelInfoNormal(" << configfile + ".xml): " << ret << endl;
    }
    else
    {
        ret = FAIL;
    }
    
    return ret;
} 
