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
#include "../output/tinyxml2.h"

using ::google::protobuf::Message;
using namespace std;

static void GetLabelMap(MODEL_INFO_S *pstInfo) {
    int fd = open(pstInfo->labelmapfile.c_str(), O_RDONLY);
    google::protobuf::io::FileInputStream* input = new google::protobuf::io::FileInputStream(fd);
    pstInfo->labelmap = new LabelMap();
    bool success = google::protobuf::TextFormat::Parse(input, pstInfo->labelmap);
    delete input;
    close(fd);
}

int GetIdxByName(MODEL_INFO_S *pstInfo, string label) {
    for (int i = 0; i < pstInfo->labelmap->item_size(); ++i) {
        if (pstInfo->labelmap->item(i).display_name() == label) {
            return pstInfo->labelmap->item(i).label();
        }
    }
    return -1;
}
int GetModelInfoXml(string configfile, MODEL_INFO_S *pstInfo, string strpath) {
    tinyxml2::XMLDocument doc;
	tinyxml2::XMLError err = doc.LoadFile(configfile.c_str());
    tinyxml2::XMLElement* cfg = doc.FirstChildElement( "cfg" );
    tinyxml2::XMLElement* tmpElement;
    
    //Get gpus
    tmpElement = cfg->FirstChildElement( "gpus" );
    if (tmpElement) {
        pstInfo->gpus = tmpElement->GetText();
        std::cout << "gpus:" << pstInfo->gpus <<endl;
    } else {
		std::cout << "Warning! No gpus in " << configfile << endl;
        std::cout << "use gpus = all" << endl;
        pstInfo->gpus = "all";
    }
    
    //Get enEncryType
    tmpElement = cfg->FirstChildElement( "encry" );
    if (tmpElement) {
        pstInfo->encry = tmpElement->GetText();
        std::cout << "encry:" << pstInfo->encry <<endl;
    } else {
		std::cout << "Warning! No encry in " << configfile << endl;
        std::cout << "use encry = rc4" << endl;
        pstInfo->encry = "rc4";
    }

    //Get weight
    tmpElement = cfg->FirstChildElement( "weight" );
    if (tmpElement) {
		pstInfo->weight = strpath + tmpElement->GetText();
        std::cout << "weight:" << pstInfo->weight <<endl;
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
        tmpElement = cfg->FirstChildElement( "model" );
        if (tmpElement) {
			pstInfo->model = strpath + tmpElement->GetText();
            std::cout << "model:" << pstInfo->model <<endl;
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
        tmpElement = cfg->FirstChildElement( "key" );
        if (tmpElement) {
            pstInfo->key = tmpElement->GetText();
        } else {
            pstInfo->key = "12345678901234567890";
        }
    }
    
    //Get postmode
    tmpElement = cfg->FirstChildElement( "postmode" );
    if (tmpElement) {
        pstInfo->postmode = tmpElement->GetText();
        std::cout << "postmode:" << pstInfo->postmode <<endl;
    } else {
        std::cout << "Warning! No postmode in " << configfile << endl;
        std::cout << "use postmode = none" << endl;
        pstInfo->postmode = "none";
    }

    //Get runmode
    tmpElement = cfg->FirstChildElement( "runmode" );
    if (tmpElement) {
        pstInfo->runmode = tmpElement->GetText();
        std::cout << "runmode:" << pstInfo->runmode <<endl;
    } else {
        std::cout << "Warning! No runmode in " << configfile << endl;
        std::cout << "use runmode = normal" << endl;
        pstInfo->runmode = "normal";
    }

    //Get outmode
    tmpElement = cfg->FirstChildElement( "outmode" );
    if (tmpElement) {
        pstInfo->outmode = tmpElement->GetText();
        std::cout << "outmode:" << pstInfo->outmode <<endl;
    } else {
        std::cout << "Warning! No outmode in " << configfile << endl;
        std::cout << "use outmode = json" << endl;
        pstInfo->outmode = "json";
    }

    //Get cropscale
    tmpElement = cfg->FirstChildElement( "cropscale" );
    if (tmpElement) {
        pstInfo->cropscale = atof(tmpElement->GetText());
        std::cout << "cropscale:" << pstInfo->cropscale <<endl;
    } else {
        std::cout << "Warning! No cropscale in " << configfile << endl;
        std::cout << "use cropscale = 1.0" << endl;
        pstInfo->cropscale = 1.0;
    }

    
    //Get outputexpand
    tmpElement = cfg->FirstChildElement( "outputexpandratio" );
    if (tmpElement) {
        pstInfo->outputexpandratio = atof(tmpElement->GetText());
        std::cout << "outputexpandratio:" << pstInfo->outputexpandratio <<endl;
    } else {
        std::cout << "Warning! No outputexpandratio in " << configfile << endl;
        std::cout << "use outputexpandratio = 0" << endl;
        pstInfo->outputexpandratio = 0;
    }

    //Get mean
    tmpElement = cfg->FirstChildElement( "mean" );
    while(tmpElement) {
        float mean = atof(tmpElement->GetText());
        pstInfo->means.push_back(mean);
        tmpElement = tmpElement->NextSiblingElement("mean");
    }
    // set default mean
    if (pstInfo->means.size() == 0) {
        pstInfo->means.push_back(104.0);
        pstInfo->means.push_back(117.0);
        pstInfo->means.push_back(123.0);
    }

    //Get LabelMap
    tmpElement = cfg->FirstChildElement( "labelmapfile" );
    if (tmpElement) {
		pstInfo->labelmapfile = strpath + tmpElement->GetText();
        std::cout << "labelmapfile:" << pstInfo->labelmapfile <<endl;
        if (!IsFileExist(pstInfo->labelmapfile)) {
            std::cout << "Error! file not exist " << pstInfo->labelmapfile << endl;
            return FAIL;
        }
        std::cout << "labelmapfile:" << pstInfo->labelmapfile << endl;
        
        GetLabelMap(pstInfo);

        //init
        pstInfo->scores.resize(pstInfo->labelmap->item_size());
        pstInfo->scoreslow.resize(pstInfo->labelmap->item_size());
        pstInfo->scoreshigh.resize(pstInfo->labelmap->item_size());
        pstInfo->maxcnts.resize(pstInfo->labelmap->item_size());
        pstInfo->colors.resize(pstInfo->labelmap->item_size());
        pstInfo->labels.resize(pstInfo->labelmap->item_size());
        pstInfo->sizelimits.resize(pstInfo->labelmap->item_size());
        pstInfo->minarea.resize(pstInfo->labelmap->item_size());
        
        for (int i = 0; i < pstInfo->labelmap->item_size(); ++i) {
            int idx = pstInfo->labelmap->item(i).label();
            if (idx >= pstInfo->labelmap->item_size()) {
                std::cout << "Error label in labelmapfile" << endl;
                return FAIL;
            }
            pstInfo->scores[idx] = 1.0;
            pstInfo->scoreslow[idx] = 0.0;
            pstInfo->scoreshigh[idx] = 1.0;
            pstInfo->maxcnts[idx] = 0.0;
            pstInfo->colors[idx] = 0xFF0000;
            pstInfo->labels[idx] = pstInfo->labelmap->item(i).display_name();
            pstInfo->minarea[idx] = 360;
        }
        
        tmpElement = cfg->FirstChildElement( "labelinfo" );
        while(tmpElement) {
            tinyxml2::XMLElement* subElement;
            int label;
            
            subElement = tmpElement->FirstChildElement( "name" );
            if (subElement) {
				string name = subElement->GetText();
				label = GetIdxByName(pstInfo, name);
				pstInfo->labels[label] = name;
                std::cout << "labels:" << pstInfo->labels[label] << endl;
                if (label < 0) {
                    std::cout << "Error! name not in labelmapfile" << endl;
                    return FAIL;
                }
            } else {
                std::cout << "Error! No name " << endl;
                return FAIL;
            }

            subElement = tmpElement->FirstChildElement( "scores" );
            if (subElement) {
                float score = atof(subElement->GetText());
                std::cout << "score: " << score << endl;
                pstInfo->scores[label] = score;
            } else {
                std::cout << "Warning! No scores " << endl;
                pstInfo->scores[label] = 1.0;
            }
            
            subElement = tmpElement->FirstChildElement( "scorelow" );
            if (subElement) {
                float scorelow = atof(subElement->GetText());
                std::cout << "scorelow: " << scorelow << endl;
                pstInfo->scoreslow[label] = scorelow;
            } else {
                std::cout << "Warning! No scorelow " << endl;
                pstInfo->scoreslow[label] = 0.0;
            }
            
            subElement = tmpElement->FirstChildElement( "scorehigh" );
            if (subElement) {
                float scorehigh = atof(subElement->GetText());
                std::cout << "scorehigh: " << scorehigh << endl;
                pstInfo->scoreshigh[label] = scorehigh;
            } else {
                std::cout << "Warning! No scorehigh " << endl;
                pstInfo->scoreshigh[label] = 1.0;
            }
            
            subElement = tmpElement->FirstChildElement( "maxcnt" );
            if (subElement) {
                int maxcnt = atoi(subElement->GetText());
                std::cout << "maxcnt: " << maxcnt << endl;
                pstInfo->maxcnts[label] = maxcnt;
            } else {
                std::cout << "Warning! No maxcnt "  << endl;
                pstInfo->maxcnts[label] = 0;
            }
            
            subElement = tmpElement->FirstChildElement( "color" );
            if (subElement) {
                int color = strtol(subElement->GetText(), NULL, 16);
                std::cout << "color: " << color << endl;
                pstInfo->colors[label] = color;
            } else {
                std::cout << "Warning! No color " << endl;
                pstInfo->colors[label] = 0xFF0000;
            }

            subElement = tmpElement->FirstChildElement( "minarea" );
            if (subElement) {
                int minarea = atoi(subElement->GetText());
                std::cout << "minarea: " << minarea << endl;
                pstInfo->minarea[label] = minarea;
            } else {
                std::cout << "Warning! No minarea " << endl;
                pstInfo->minarea[label] = 340;
            }

            subElement = tmpElement->FirstChildElement( "sizelimit" );
            vector<SIZE_LIMIT_S> sizelimits;
            while (subElement) {
                SIZE_LIMIT_S sizelimit;
                tinyxml2::XMLElement* min = subElement->FirstChildElement( "min" );
                if (min) {
                    sizelimit.min = atoi(min->GetText());
                } else {
                    std::cout << "Error! No min " << endl;
                    return FAIL;
                }
                tinyxml2::XMLElement* max = subElement->FirstChildElement( "max" );
                if (max) {
                    sizelimit.max = atoi(max->GetText());
                } else {
                    std::cout << "Error! No min " << endl;
                    return FAIL;
                }
				sizelimits.push_back(sizelimit);
                subElement = subElement->NextSiblingElement( "sizelimit" );
            }
			pstInfo->sizelimits[label] = sizelimits;

            tmpElement = tmpElement->NextSiblingElement("labelinfo");
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

    return SUCCESS;
}

int LoadXmlFile2ModelInfo(string configfile, MODEL_INFO_S *pstInfo) {
	string strpath = "";
	size_t found = configfile.find_last_of("/\\");
	if (-1 == found) {
		strpath = GetDLLPath();
	} else {
		strpath = configfile.substr(0, found + 1);
	}

	tinyxml2::XMLDocument doc;
	tinyxml2::XMLError err = doc.LoadFile(configfile.c_str());
    tinyxml2::XMLElement* cfg = doc.FirstChildElement( "cfg" );
    tinyxml2::XMLElement* tmpElement;
    
    //Get gpus
    tmpElement = cfg->FirstChildElement( "gpus" );
    if (tmpElement) {
        pstInfo->gpus = tmpElement->GetText();
        std::cout << "gpus:" << pstInfo->gpus <<endl;
    } else {
		std::cout << "Warning! No gpus in " << configfile << endl;
        std::cout << "use gpus = all" << endl;
        pstInfo->gpus = "all";
    }
    
    //Get enEncryType
    tmpElement = cfg->FirstChildElement( "encry" );
    if (tmpElement) {
        pstInfo->encry = tmpElement->GetText();
        std::cout << "encry:" << pstInfo->encry <<endl;
    } else {
		std::cout << "Warning! No encry in " << configfile << endl;
        std::cout << "use encry = rc4" << endl;
        pstInfo->encry = "rc4";
    }

    //Get weight
    tmpElement = cfg->FirstChildElement( "weight" );
    if (tmpElement) {
		pstInfo->weight = strpath + tmpElement->GetText();
        std::cout << "weight:" << pstInfo->weight <<endl;
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
        tmpElement = cfg->FirstChildElement( "model" );
        if (tmpElement) {
			pstInfo->model = strpath + tmpElement->GetText();
            std::cout << "model:" << pstInfo->model <<endl;
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
        tmpElement = cfg->FirstChildElement( "key" );
        if (tmpElement) {
            pstInfo->key = tmpElement->GetText();
        } else {
            pstInfo->key = "12345678901234567890";
        }
    }
    
    //Get postmode
    tmpElement = cfg->FirstChildElement( "postmode" );
    if (tmpElement) {
        pstInfo->postmode = tmpElement->GetText();
        std::cout << "postmode:" << pstInfo->postmode <<endl;
    } else {
        std::cout << "Warning! No postmode in " << configfile << endl;
        std::cout << "use postmode = none" << endl;
        pstInfo->postmode = "none";
    }

    //Get runmode
    tmpElement = cfg->FirstChildElement( "runmode" );
    if (tmpElement) {
        pstInfo->runmode = tmpElement->GetText();
        std::cout << "runmode:" << pstInfo->runmode <<endl;
    } else {
        std::cout << "Warning! No runmode in " << configfile << endl;
        std::cout << "use runmode = normal" << endl;
        pstInfo->runmode = "normal";
    }

    //Get outmode
    tmpElement = cfg->FirstChildElement( "outmode" );
    if (tmpElement) {
        pstInfo->outmode = tmpElement->GetText();
        std::cout << "outmode:" << pstInfo->outmode <<endl;
    } else {
        std::cout << "Warning! No outmode in " << configfile << endl;
        std::cout << "use outmode = json" << endl;
        pstInfo->outmode = "json";
    }

    //Get cropscale
    tmpElement = cfg->FirstChildElement( "cropscale" );
    if (tmpElement) {
        pstInfo->cropscale = atof(tmpElement->GetText());
        std::cout << "cropscale:" << pstInfo->cropscale <<endl;
    } else {
        std::cout << "Warning! No cropscale in " << configfile << endl;
        std::cout << "use cropscale = 1.0" << endl;
        pstInfo->cropscale = 1.0;
    }

    
    //Get outputexpand
    tmpElement = cfg->FirstChildElement( "outputexpandratio" );
    if (tmpElement) {
        pstInfo->outputexpandratio = atof(tmpElement->GetText());
        std::cout << "outputexpandratio:" << pstInfo->outputexpandratio <<endl;
    } else {
        std::cout << "Warning! No outputexpandratio in " << configfile << endl;
        std::cout << "use outputexpandratio = 0" << endl;
        pstInfo->outputexpandratio = 0;
    }

    //Get mean
    tmpElement = cfg->FirstChildElement( "mean" );
    while(tmpElement) {
        float mean = atof(tmpElement->GetText());
        pstInfo->means.push_back(mean);
        tmpElement = tmpElement->NextSiblingElement("mean");
    }
    // set default mean
    if (pstInfo->means.size() == 0) {
        pstInfo->means.push_back(104.0);
        pstInfo->means.push_back(117.0);
        pstInfo->means.push_back(123.0);
    }

    //Get LabelMap
    tmpElement = cfg->FirstChildElement( "labelmapfile" );
    if (tmpElement) {
		pstInfo->labelmapfile = strpath + tmpElement->GetText();
        std::cout << "labelmapfile:" << pstInfo->labelmapfile <<endl;
        if (!IsFileExist(pstInfo->labelmapfile)) {
            std::cout << "Error! file not exist " << pstInfo->labelmapfile << endl;
            return FAIL;
        }
        std::cout << "labelmapfile:" << pstInfo->labelmapfile << endl;
        
        GetLabelMap(pstInfo);

        //init
        pstInfo->scores.resize(pstInfo->labelmap->item_size());
        pstInfo->scoreslow.resize(pstInfo->labelmap->item_size());
        pstInfo->scoreshigh.resize(pstInfo->labelmap->item_size());
        pstInfo->maxcnts.resize(pstInfo->labelmap->item_size());
        pstInfo->colors.resize(pstInfo->labelmap->item_size());
        pstInfo->labels.resize(pstInfo->labelmap->item_size());
        pstInfo->sizelimits.resize(pstInfo->labelmap->item_size());
        pstInfo->minarea.resize(pstInfo->labelmap->item_size());
        
        for (int i = 0; i < pstInfo->labelmap->item_size(); ++i) {
            int idx = pstInfo->labelmap->item(i).label();
            if (idx >= pstInfo->labelmap->item_size()) {
                std::cout << "Error label in labelmapfile" << endl;
                return FAIL;
            }
            pstInfo->scores[idx] = 1.0;
            pstInfo->scoreslow[idx] = 0.0;
            pstInfo->scoreshigh[idx] = 1.0;
            pstInfo->maxcnts[idx] = 0.0;
            pstInfo->colors[idx] = 0xFF0000;
            pstInfo->labels[idx] = pstInfo->labelmap->item(i).display_name();
            pstInfo->minarea[idx] = 360;
        }
        
        tmpElement = cfg->FirstChildElement( "labelinfo" );
        while(tmpElement) {
            tinyxml2::XMLElement* subElement;
            int label;
            
            subElement = tmpElement->FirstChildElement( "name" );
            if (subElement) {
				string name = subElement->GetText();
				label = GetIdxByName(pstInfo, name);
				pstInfo->labels[label] = name;
                std::cout << "labels:" << pstInfo->labels[label] << endl;
                if (label < 0) {
                    std::cout << "Error! name not in labelmapfile" << endl;
                    return FAIL;
                }
            } else {
                std::cout << "Error! No name " << endl;
                return FAIL;
            }

            subElement = tmpElement->FirstChildElement( "scores" );
            if (subElement) {
                float score = atof(subElement->GetText());
                std::cout << "score: " << score << endl;
                pstInfo->scores[label] = score;
            } else {
                std::cout << "Warning! No scores " << endl;
                pstInfo->scores[label] = 1.0;
            }
            
            subElement = tmpElement->FirstChildElement( "scorelow" );
            if (subElement) {
                float scorelow = atof(subElement->GetText());
                std::cout << "scorelow: " << scorelow << endl;
                pstInfo->scoreslow[label] = scorelow;
            } else {
                std::cout << "Warning! No scorelow " << endl;
                pstInfo->scoreslow[label] = 0.0;
            }
            
            subElement = tmpElement->FirstChildElement( "scorehigh" );
            if (subElement) {
                float scorehigh = atof(subElement->GetText());
                std::cout << "scorehigh: " << scorehigh << endl;
                pstInfo->scoreshigh[label] = scorehigh;
            } else {
                std::cout << "Warning! No scorehigh " << endl;
                pstInfo->scoreshigh[label] = 1.0;
            }
            
            subElement = tmpElement->FirstChildElement( "maxcnt" );
            if (subElement) {
                int maxcnt = atoi(subElement->GetText());
                std::cout << "maxcnt: " << maxcnt << endl;
                pstInfo->maxcnts[label] = maxcnt;
            } else {
                std::cout << "Warning! No maxcnt "  << endl;
                pstInfo->maxcnts[label] = 0;
            }
            
            subElement = tmpElement->FirstChildElement( "color" );
            if (subElement) {
                int color = strtol(subElement->GetText(), NULL, 16);
                std::cout << "color: " << color << endl;
                pstInfo->colors[label] = color;
            } else {
                std::cout << "Warning! No color " << endl;
                pstInfo->colors[label] = 0xFF0000;
            }

            subElement = tmpElement->FirstChildElement( "minarea" );
            if (subElement) {
                int minarea = atoi(subElement->GetText());
                std::cout << "minarea: " << minarea << endl;
                pstInfo->minarea[label] = minarea;
            } else {
                std::cout << "Warning! No minarea " << endl;
                pstInfo->minarea[label] = 340;
            }

            subElement = tmpElement->FirstChildElement( "sizelimit" );
            vector<SIZE_LIMIT_S> sizelimits;
            while (subElement) {
                SIZE_LIMIT_S sizelimit;
                tinyxml2::XMLElement* min = subElement->FirstChildElement( "min" );
                if (min) {
                    sizelimit.min = atoi(min->GetText());
                } else {
                    std::cout << "Error! No min " << endl;
                    return FAIL;
                }
                tinyxml2::XMLElement* max = subElement->FirstChildElement( "max" );
                if (max) {
                    sizelimit.max = atoi(max->GetText());
                } else {
                    std::cout << "Error! No min " << endl;
                    return FAIL;
                }
				sizelimits.push_back(sizelimit);
                subElement = subElement->NextSiblingElement( "sizelimit" );
            }
			pstInfo->sizelimits[label] = sizelimits;

            tmpElement = tmpElement->NextSiblingElement("labelinfo");
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

    return SUCCESS;
}



