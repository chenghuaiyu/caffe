
#include <vector>
#include <string>

#include "../common.h"
#include "outputcjson.h"
#include "outputxml.h"

char* Output(vector<DETECT_FILE_S> files, string outmode, vector<string> labels)
{
    if (outmode == "json")
    {
        return JsonOutput(files, labels);
    }
    else if (outmode == "jsonspec")
    {
        return JsonOutputSpec(files, labels);
    }
    else if (outmode == "xml")
    {
        return XmlOutput(files, labels);
    }
    else
    {
        return JsonOutput(files, labels);
    }
}

vector<DETECT_FILE_S> OutputParse(char* info, string outmode, vector<string> labels)
{
	if (outmode == "json")
	{
		return JsonParse(info, labels);
	}
    else if (outmode == "jsonspec")
    {
        vector<DETECT_FILE_S> tmp;
        tmp.clear();
        return tmp;
    }
	else if (outmode == "xml")
	{
		return XmlParse(info, labels);
	}
	else
	{
		return JsonParse(info, labels);
	}
}

void OutputSave(string src, string dst, string fn, char *info, int savenobox, string outmode)
{
	if (outmode == "json")
	{
		return JsonSave(src, dst, fn, info, savenobox);
	}
    else if (outmode == "jsonspec")
    {
    }
	else if (outmode == "xml")
	{
		return XmlSave(src, dst, fn, info, savenobox);
	}
	else
	{
		return JsonSave(src, dst, fn, info, savenobox);
	}
}