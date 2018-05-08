
#ifndef _JSON_API_H_
#define _JSON_API_H_

#include "../common.h"
#include <vector>
#include <string>

char* JsonOutputSpec(vector<DETECT_FILE_S> files, vector<string> labels);
char* JsonOutput(vector<DETECT_FILE_S> files, vector<string> labels);
vector<DETECT_FILE_S> JsonParse(char* info, vector<string> labels);
void JsonSave(string src, string dst, string fn, char* info, int savenobox);

#endif //_JSON_API_H_

