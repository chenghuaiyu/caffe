#include <fstream>
#include <iostream>

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
#ifndef _LINUX_
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <fstream>
#include <iostream>

#include "get_config.h"
#include "../common.h"
#include "../util/util.h"

using ::google::protobuf::Message;
using namespace std;

#define COMMENT_CHAR '#'

bool IsSpace(char c) {
	if (' ' == c || '\t' == c)
		return true;
	return false;
}

bool IsCommentChar(char c) {
	switch (c) {
	case COMMENT_CHAR:
		return true;
	default:
		return false;
	}
}

void Trim(string & str) {
	if (str.empty()) {
		return;
	}
	int i, start_pos, end_pos;
	for (i = 0; i < str.size(); ++i) {
		if (!IsSpace(str[i])) {
			break;
		}
	}
	if (i == str.size()) { // 全部是空白字符串
		str = "";
		return;
	}

	start_pos = i;

	for (i = str.size() - 1; i >= 0; --i) {
		if (!IsSpace(str[i])) {
			break;
		}
	}
	end_pos = i;

	str = str.substr(start_pos, end_pos - start_pos + 1);
}

bool AnalyseLine(const string & line, string & key, string & value) {
	if (line.empty())
		return false;
	int start_pos = 0, end_pos = line.size() - 1, pos;
	if ((pos = line.find(COMMENT_CHAR)) != -1) {
		if (0 == pos) {  // 行的第一个字符就是注释字符
			return false;
		}
		end_pos = pos - 1;
	}
	string new_line = line.substr(start_pos, end_pos + 1 - start_pos);  // 预处理，删除注释部分

	if ((pos = new_line.find('=')) == -1)
		return false;  // 没有=号

	key = new_line.substr(0, pos);
	value = new_line.substr(pos + 1, end_pos + 1 - (pos + 1));

	Trim(key);
	if (key.empty()) {
		return false;
	}
	Trim(value);
	return true;
}

bool ReadConfig(const string & filename, map<string, string> & m) {
	m.clear();
	ifstream infile(filename.c_str());
	if (!infile) {
		cout << "file open error" << endl;
		return false;
	}
	string line, key, value;
	while (getline(infile, line)) {
		if (AnalyseLine(line, key, value)) {
			m[key] = value;
		}
	}

	infile.close();
	return true;
}

void PrintConfig(const map<string, string> & m) {
	map<string, string>::const_iterator mite = m.begin();
	for (; mite != m.end(); ++mite) {
		cout << mite->first << "=" << mite->second << endl;
	}
}

void GetSizelimit(MODEL_INFO_S *pstInfo, string labelinfo) {
    string subinfo = labelinfo;
    int start_pos, end_pos, pos;
    vector<SIZE_LIMIT_S> sizelimits;
    
    sizelimits.clear();
    while((pos = subinfo.find('[')) != -1) {
        subinfo = subinfo.substr(pos);
        start_pos = 0;
        if ((pos = subinfo.find(']')) == -1) {
            break;
        }
        end_pos = pos;
        if ((pos = subinfo.find(',')) == -1) {
            break;
        }
        string sizemin = subinfo.substr(start_pos+1, pos-start_pos-1);
        printf("sizemin:%s\n",sizemin.c_str());
        string sizemax = subinfo.substr(pos+1, end_pos-pos-1);
        printf("sizemax:%s\n",sizemax.c_str());
        subinfo = subinfo.substr(end_pos+1);

        SIZE_LIMIT_S sizelimit;
        sizelimit.min = std::atoi(sizemin.c_str());
        sizelimit.max = std::atoi(sizemax.c_str());
        sizelimits.push_back(sizelimit);
    }
    pstInfo->sizelimits.push_back(sizelimits);
}

static void GetLabelMap(MODEL_INFO_S *pstInfo) {
    int fd = open(pstInfo->labelmapfile.c_str(), O_RDONLY);
    google::protobuf::io::FileInputStream* input = new google::protobuf::io::FileInputStream(fd);
    pstInfo->labelmap = new LabelMap();
    bool success = google::protobuf::TextFormat::Parse(input, pstInfo->labelmap);
    delete input;
    close(fd);
}

int GetModelInfoNormal(string configfile, MODEL_INFO_S *pstInfo, string strpath) {
    string strModelFileName;
    
    map<string, string> m_Config;
    ReadConfig(configfile, m_Config);
    PrintConfig(m_Config);

    map<string, string>::iterator iter_configMap;

    //Get gpus
    iter_configMap = m_Config.find("gpus");
    if (iter_configMap != m_Config.end()) {
        pstInfo->gpus = iter_configMap->second;
    } else {
		std::cout << "Warning! No gpus in " << configfile << endl;
        std::cout << "use gpus = all" << endl;
        pstInfo->gpus = "all";
    }
    
    //Get enEncryType
    iter_configMap = m_Config.find("encry");
    if (iter_configMap != m_Config.end()) {
        pstInfo->encry = iter_configMap->second;
    } else {
		std::cout << "Warning! No encry in " << configfile << endl;
        std::cout << "use encry = rc4" << endl;
        pstInfo->encry = "rc4";
    }

    //Get weight
    iter_configMap = m_Config.find("weight");
    if (iter_configMap != m_Config.end()) {
		pstInfo->weight = strpath + iter_configMap->second;
        if (!IsFileExist(pstInfo->weight)) {
            std::cout << "Error! file not exist " << pstInfo->weight << endl;
            return FAIL;
        }
    } else {
		std::cout << "Error! No weight in " << configfile << endl;
        return FAIL;
    }
    
    if (pstInfo->encry == "none") {
        //Get model
        iter_configMap = m_Config.find("model");
        if (iter_configMap != m_Config.end()) {
			pstInfo->model = strpath + iter_configMap->second;
            if (!IsFileExist(pstInfo->model)) {
                std::cout << "Error! file not exist " << pstInfo->model << endl;
                return FAIL;
            }
        } else {
			std::cout << "Error! No model in " << configfile << endl;
            return FAIL;
        }
    } else {
        //Get key
        iter_configMap = m_Config.find("key");
        if (iter_configMap != m_Config.end()) {
            pstInfo->key = iter_configMap->second;
        } else {
            pstInfo->key = "12345678901234567890";
        }
    }

    //Get LabelMap
    iter_configMap = m_Config.find("labelmapfile");
    if (iter_configMap != m_Config.end()) {
		pstInfo->labelmapfile = strpath + iter_configMap->second;
        if (!IsFileExist(pstInfo->labelmapfile)) {
            std::cout << "Error! file not exist " << pstInfo->labelmapfile << endl;
            return FAIL;
        }
        std::cout << "labelmapfile:" << pstInfo->labelmapfile << endl;
        
        GetLabelMap(pstInfo);

        for (int i = 0; i < pstInfo->labelmap->item_size(); ++i) {
            char tmpname[64];
            const string& name = pstInfo->labelmap->item(i).display_name();
            const int label = pstInfo->labelmap->item(i).label();

            pstInfo->labels.push_back(name.c_str());
            printf("labels:%s, ",name.c_str());

            sprintf(tmpname, "%s_scores", name.c_str());
            iter_configMap = m_Config.find(tmpname);
            if (iter_configMap != m_Config.end()) {
                float score = atof(iter_configMap->second.c_str());
                printf("score:%f, ", score);
                pstInfo->scores.push_back(score);
            } else {
                pstInfo->scores.push_back(1.0);
                std::cout << "Warning! No scores for " << name << endl;
            }
            

            sprintf(tmpname, "%s_scorelow", name.c_str());
            iter_configMap = m_Config.find(tmpname);
            if (iter_configMap != m_Config.end()) {
                float scorelow = atof(iter_configMap->second.c_str());
                printf("scorelow:%f, ", scorelow);
                pstInfo->scoreslow.push_back(scorelow);
            } else {
                pstInfo->scoreslow.push_back(0.0);
                std::cout << "Warning! No scorelow for " << name << endl;
            }
            
            sprintf(tmpname, "%s_scorehigh", name.c_str());
            iter_configMap = m_Config.find(tmpname);
            if (iter_configMap != m_Config.end()) {
                float scorehigh = atof(iter_configMap->second.c_str());
                printf("scorehigh:%f, ", scorehigh);
                pstInfo->scoreshigh.push_back(scorehigh);
            } else {
                pstInfo->scoreshigh.push_back(1.0);
                std::cout << "Warning! No scorehigh for " << name << endl;
            }
            
            sprintf(tmpname, "%s_maxcnt", name.c_str());
            iter_configMap = m_Config.find(tmpname);
            if (iter_configMap != m_Config.end()) {
                int maxcnt = atoi(iter_configMap->second.c_str());
                printf("maxcnt:%d, ", maxcnt);
                pstInfo->maxcnts.push_back(maxcnt);
            } else {
                pstInfo->maxcnts.push_back(0);
                std::cout << "Warning! No maxcnt for " << name << endl;
            }
            
            sprintf(tmpname, "%s_color", name.c_str());
            iter_configMap = m_Config.find(tmpname);
            if (iter_configMap != m_Config.end()) {
                int color = strtol(iter_configMap->second.c_str(), NULL, 16);
				printf("color:0x%x, ", color);
                pstInfo->colors.push_back(color);
            } else {
                pstInfo->colors.push_back(0xFF0000);
                std::cout << "Warning! No color for " << name << endl;
            }
            printf("\n");

            sprintf(tmpname, "%s_sizelimit", name.c_str());
            string sizelimit;
            iter_configMap = m_Config.find(tmpname);
            if (iter_configMap != m_Config.end()) {
                sizelimit = iter_configMap->second.c_str();
                printf("sizelimit:%s\n", sizelimit.c_str());
                GetSizelimit(pstInfo, sizelimit);
            } else {
                std::cout << "Warning! No sizelimit for " << name << endl;
                GetSizelimit(pstInfo, sizelimit);
            }
            
            //Get minarea
            sprintf(tmpname, "%s_minarea", name.c_str());
            float minarea;
            iter_configMap = m_Config.find(tmpname);
            if (iter_configMap != m_Config.end()) {
                minarea = atof(iter_configMap->second.c_str());
				printf("minarea:%f\n", minarea);
                pstInfo->minarea.push_back(minarea);
            } else {
                std::cout << "Warning! No minarea in " << configfile << endl;
                std::cout << "use minarea = 360" << endl;
                pstInfo->minarea.push_back(360);
            }
        }
    } else {
		std::cout << "Warning! No labelmapfile in " << configfile << endl;
        char labels[64];
        for (int i = 0; i < 21; ++i) {
            sprintf(labels, "label_%d", i);
            printf("%s\n",labels);
            pstInfo->scores.push_back(0.90);
            pstInfo->labels.push_back(labels);
        }
    }

    //Get postmode
    iter_configMap = m_Config.find("postmode");
    if (iter_configMap != m_Config.end()) {
        pstInfo->postmode = iter_configMap->second;
    } else {
		std::cout << "Warning! No postmode in " << configfile << endl;
        std::cout << "use postmode = none" << endl;
        pstInfo->postmode = "none";
    }

    //Get runmode
    iter_configMap = m_Config.find("runmode");
    if (iter_configMap != m_Config.end()) {
        pstInfo->runmode = iter_configMap->second;
    } else {
		std::cout << "Warning! No runmode in " << configfile << endl;
        std::cout << "use runmode = normal" << endl;
        pstInfo->runmode = "normal";
    }

    //Get outmode
    iter_configMap = m_Config.find("outmode");
    if (iter_configMap != m_Config.end()) {
        pstInfo->outmode = iter_configMap->second;
    } else {
		std::cout << "Warning! No outmode in " << configfile << endl;
        std::cout << "use outmode = json" << endl;
        pstInfo->outmode = "json";
    }

    //Get mean
    iter_configMap = m_Config.find("meannum");
    if (iter_configMap != m_Config.end()) {
        int meannum = atoi(iter_configMap->second.c_str());
        for (int i = 0; i < meannum; ++i) {
            char means[64];
            sprintf(means, "mean_%d", i);
            iter_configMap = m_Config.find(means);
            if (iter_configMap != m_Config.end()) {
                float mean = atof(iter_configMap->second.c_str());
                printf("%s:%f\n",means, mean);
                pstInfo->means.push_back(mean);
            } else {
                std::cout << "Warning! No mean for " << means << endl;
            }
            // set default mean
            if (pstInfo->means.size() == 0) {
                pstInfo->means.push_back(104.0);
                pstInfo->means.push_back(117.0);
                pstInfo->means.push_back(123.0);
            }
        }
    } else {
		pstInfo->means.push_back(104.0);
		pstInfo->means.push_back(117.0);
		pstInfo->means.push_back(123.0);
		std::cout << "Warning! No mean in " << configfile << endl;
    }

    //Get cropscale
    iter_configMap = m_Config.find("cropscale");
    if (iter_configMap != m_Config.end()) {
        pstInfo->cropscale = atof(iter_configMap->second.c_str());
    } else {
		std::cout << "Warning! No cropscale in " << configfile << endl;
        std::cout << "use cropscale = 1.0" << endl;
        pstInfo->cropscale = 1.0;
    }

	
	//Get outputexpand
	iter_configMap = m_Config.find("outputexpandratio");
	if (iter_configMap != m_Config.end()) {
		pstInfo->outputexpandratio = atof(iter_configMap->second.c_str());
	} else {
		std::cout << "Warning! No outputexpandratio in " << configfile << endl;
		std::cout << "use outputexpandratio = 0" << endl;
		pstInfo->outputexpandratio = 0;
	}

    return SUCCESS;
}

int LoadFile2ModelInfo(string configfile, MODEL_INFO_S *pstInfo) {
	string strpath;
	size_t found = configfile.find_last_of("/\\");
	if (-1 == found) {
		strpath = GetDLLPath();
	} else {
		strpath = configfile.substr(0, found + 1);
	}
    
    map<string, string> m_Config;
    ReadConfig(configfile, m_Config);
    PrintConfig(m_Config);

    map<string, string>::iterator iter_configMap;

    //Get gpus
    iter_configMap = m_Config.find("gpus");
    if (iter_configMap != m_Config.end())  {
        pstInfo->gpus = iter_configMap->second;
    } else {
		std::cout << "Warning! No gpus in " << configfile << endl;
        pstInfo->gpus = "all";
    }
	std::cout << "gpus:" << pstInfo->gpus << endl;
    
    //Get enEncryType
    iter_configMap = m_Config.find("encry");
    if (iter_configMap != m_Config.end()) {
        pstInfo->encry = iter_configMap->second;
    } else {
		std::cout << "Warning! No encry in " << configfile << endl;
        pstInfo->encry = "rc4";
    }
	std::cout << "encry: " << pstInfo->encry << endl;

    //Get weight
    iter_configMap = m_Config.find("weight");
    if (iter_configMap != m_Config.end()) {
		pstInfo->weight = strpath + iter_configMap->second;
        if (!IsFileExist(pstInfo->weight)) {
            std::cout << "Error! file not exist " << pstInfo->weight << endl;
            return FAIL;
        }
    } else {
		std::cout << "Error! No weight in " << configfile << endl;
        return FAIL;
    }
	std::cout << "weight: " << pstInfo->weight << endl;
    
    if (pstInfo->encry == "none") {
        //Get model
        iter_configMap = m_Config.find("model");
        if (iter_configMap != m_Config.end())  {
			pstInfo->model = strpath + iter_configMap->second;
            if (!IsFileExist(pstInfo->model)) {
                std::cout << "Error! file not exist " << pstInfo->model << endl;
                return FAIL;
            }
        } else {
			std::cout << "Error! No model in " << configfile << endl;
            return FAIL;
        }
		std::cout << "model: " << pstInfo->model << endl;
    } else {
        //Get key
        iter_configMap = m_Config.find("key");
        if (iter_configMap != m_Config.end()) {
            pstInfo->key = iter_configMap->second;
        } else {
            pstInfo->key = "12345678901234567890";
        }
    }

    //Get LabelMap
    iter_configMap = m_Config.find("labelmapfile");
    if (iter_configMap != m_Config.end()) {
		pstInfo->labelmapfile = strpath + iter_configMap->second;
        if (!IsFileExist(pstInfo->labelmapfile)) {
            std::cout << "Error! file not exist " << pstInfo->labelmapfile << endl;
            return FAIL;
        }
        std::cout << "labelmapfile:" << pstInfo->labelmapfile << endl;
        
        GetLabelMap(pstInfo);

        for (int i = 0; i < pstInfo->labelmap->item_size(); ++i) {
            char tmpname[64];
            const string& name = pstInfo->labelmap->item(i).display_name();
            const int label = pstInfo->labelmap->item(i).label();

            pstInfo->labels.push_back(name.c_str());
            printf("label %d:%s, ", label, name.c_str());

            sprintf(tmpname, "%s_scores", name.c_str());
            iter_configMap = m_Config.find(tmpname);
            if (iter_configMap != m_Config.end()) {
                float score = atof(iter_configMap->second.c_str());
                printf("score:%f, ", score);
                pstInfo->scores.push_back(score);
            } else {
                pstInfo->scores.push_back(1.0);
                std::cout << "Warning! No scores for " << name << endl;
            }
            
            sprintf(tmpname, "%s_scorelow", name.c_str());
            iter_configMap = m_Config.find(tmpname);
            if (iter_configMap != m_Config.end()) {
                float scorelow = atof(iter_configMap->second.c_str());
                printf("scorelow:%f, ", scorelow);
                pstInfo->scoreslow.push_back(scorelow);
            } else {
                pstInfo->scoreslow.push_back(0.0);
                std::cout << "Warning! No scorelow for " << name << endl;
            }
            
            sprintf(tmpname, "%s_scorehigh", name.c_str());
            iter_configMap = m_Config.find(tmpname);
            if (iter_configMap != m_Config.end()) {
                float scorehigh = atof(iter_configMap->second.c_str());
                printf("scorehigh:%f, ", scorehigh);
                pstInfo->scoreshigh.push_back(scorehigh);
            } else {
                pstInfo->scoreshigh.push_back(1.0);
                std::cout << "Warning! No scorehigh for " << name << endl;
            }
            
            sprintf(tmpname, "%s_maxcnt", name.c_str());
            iter_configMap = m_Config.find(tmpname);
            if (iter_configMap != m_Config.end()) {
                int maxcnt = atoi(iter_configMap->second.c_str());
                printf("maxcnt:%d, ", maxcnt);
                pstInfo->maxcnts.push_back(maxcnt);
            } else {
                pstInfo->maxcnts.push_back(0);
                std::cout << "Warning! No maxcnt for " << name << endl;
            }
            
            sprintf(tmpname, "%s_color", name.c_str());
            iter_configMap = m_Config.find(tmpname);
            if (iter_configMap != m_Config.end()) {
                int color = strtol(iter_configMap->second.c_str(), NULL, 16);
				printf("color:0x%x, ", color);
                pstInfo->colors.push_back(color);
            } else {
                pstInfo->colors.push_back(0xFF0000);
                std::cout << "Warning! No color for " << name << endl;
            }

            sprintf(tmpname, "%s_sizelimit", name.c_str());
            string sizelimit;
            iter_configMap = m_Config.find(tmpname);
            if (iter_configMap != m_Config.end()) {
                sizelimit = iter_configMap->second.c_str();
                printf("sizelimit:%s\n", sizelimit.c_str());
                GetSizelimit(pstInfo, sizelimit);
            } else {
                std::cout << "Warning! No sizelimit for " << name << endl;
                GetSizelimit(pstInfo, sizelimit);
            }
            
            //Get minarea
            sprintf(tmpname, "%s_minarea", name.c_str());
            float minarea;
            iter_configMap = m_Config.find(tmpname);
            if (iter_configMap != m_Config.end()) {
                minarea = atof(iter_configMap->second.c_str());
				printf("minarea:%f\n", minarea);
                pstInfo->minarea.push_back(minarea);
            } else {
                std::cout << "Warning! No minarea in " << configfile << endl;
                std::cout << "use minarea = 360" << endl;
                pstInfo->minarea.push_back(360);
            }
            printf("\n");
        }
    } else {
		std::cout << "Warning! No labelmapfile in " << configfile << endl;
        char labels[64];
        for (int i = 0; i < 21; ++i) {
            sprintf(labels, "label_%d", i);
            printf("%s\n",labels);
            pstInfo->scores.push_back(0.90);
            pstInfo->labels.push_back(labels);
        }
    }

    //Get postmode
    iter_configMap = m_Config.find("postmode");
    if (iter_configMap != m_Config.end()) {
        pstInfo->postmode = iter_configMap->second;
    } else {
		std::cout << "Warning! No postmode in " << configfile << endl;
        pstInfo->postmode = "none";
    }
	std::cout << "postmode:" << pstInfo->postmode << endl;

    //Get runmode
    iter_configMap = m_Config.find("runmode");
    if (iter_configMap != m_Config.end()) {
        pstInfo->runmode = iter_configMap->second;
    } else {
		std::cout << "Warning! No runmode in " << configfile << endl;
        pstInfo->runmode = "normal";
    }
	std::cout << "runmode:" << pstInfo->runmode << endl;

    //Get outmode
    iter_configMap = m_Config.find("outmode");
    if (iter_configMap != m_Config.end()) {
        pstInfo->outmode = iter_configMap->second;
    } else {
		std::cout << "Warning! No outmode in " << configfile << endl;
        pstInfo->outmode = "json";
    }
	std::cout << "outmode:" << pstInfo->outmode << endl;

    //Get mean
    iter_configMap = m_Config.find("meannum");
    if (iter_configMap != m_Config.end()) {
        int meannum = atoi(iter_configMap->second.c_str());
        for (int i = 0; i < meannum; ++i) {
            char means[64];
            sprintf(means, "mean_%d", i);
            iter_configMap = m_Config.find(means);
            if (iter_configMap != m_Config.end()) {
                float mean = atof(iter_configMap->second.c_str());
                printf("%s:%f\n",means, mean);
                pstInfo->means.push_back(mean);
            } else {
                std::cout << "Warning! No mean for " << means << endl;
            }
            // set default mean
            if (pstInfo->means.size() == 0) {
                pstInfo->means.push_back(104.0);
                pstInfo->means.push_back(117.0);
                pstInfo->means.push_back(123.0);
            }
        }
    } else {
		pstInfo->means.push_back(104.0);
		pstInfo->means.push_back(117.0);
		pstInfo->means.push_back(123.0);
		std::cout << "Warning! No mean in " << configfile << endl;
    }

    //Get cropscale
    iter_configMap = m_Config.find("cropscale");
    if (iter_configMap != m_Config.end()) {
        pstInfo->cropscale = atof(iter_configMap->second.c_str());
    } else {
		std::cout << "Warning! No cropscale in " << configfile << endl;
        pstInfo->cropscale = 1.0;
    }
	std::cout << "cropscale:" << pstInfo->cropscale << endl;
	
	//Get outputexpand
	iter_configMap = m_Config.find("outputexpandratio");
	if (iter_configMap != m_Config.end()) {
		pstInfo->outputexpandratio = atof(iter_configMap->second.c_str());
	} else {
		std::cout << "Warning! No outputexpandratio in " << configfile << endl;
		pstInfo->outputexpandratio = 0;
	}
	std::cout << "outputexpandratio:" << pstInfo->outputexpandratio << endl;

	if (pstInfo->postmode.find("classify") != string::npos) {
		//Get modelInfoFile, reuse the labelmapfile field
		std::cout << "reuse the labelmapfile field to save modelInfoFile" << endl;
		iter_configMap = m_Config.find("subconfigfile");
		if (iter_configMap != m_Config.end()) {
			pstInfo->labelmapfile = iter_configMap->second;
		} else {
			pstInfo->labelmapfile = "";
		}
		std::cout << "modelInfoFile:" << pstInfo->labelmapfile << endl;
	}

	return SUCCESS;
}