
#ifndef _VIM_OUTPUTXML_H_
#define _VIM_OUTPUTXML_H_

#include "../common.h"
#include <vector>
#include <string>

char* XmlOutput(vector<DETECT_FILE_S> files, vector<string> labels);
vector<DETECT_FILE_S> XmlParse(char* info, vector<string> labels);
void XmlSave(string src, string dst, string fn, char* info, int savenobox);

#endif //_VIM_OUTPUTXML_H_