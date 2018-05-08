
#ifndef _VIM_OUTPUT_H_
#define _VIM_OUTPUT_H_

#include <vector>
#include <string>
#include "common.h"

char* Output(vector<DETECT_FILE_S> files, string outmode, vector<string> labels);
vector<DETECT_FILE_S> OutputParse(char* info, string outmode, vector<string> labels);
void OutputSave(string src, string dst, string fn, char *info, int savenobox, string outmode);

#endif