#ifndef _LINUX_
#include "windows.h"
#include <tchar.h>
#include <io.h>
#endif

#include <string>
#include <vector>
#include <iostream>  
#include <stdio.h>  
#include <cstring>
#include <fstream>
#include "../output/tinyxml2.h"
#include "../util/util.h"
#include "opencv2/opencv.hpp"
#include <ctime>

using namespace std;
using namespace cv;

void CheckFilename(string dir, ofstream& Save)
{
	int i;
	vector<string> files;
	GetFiles(dir, files);

	for (i = 0; i < files.size(); i++)
	{
		string::size_type ix = files[i].find(' ');
		if (ix != string::npos)
		{
			Save << files[i] << " has space!" << endl;
		}
	}
}

void CheckConsistency(string xmldir, string jpgdir, ofstream& Save)
{
	int i,j;
	vector<string> xmlfiles;
	vector<string> jpgfiles;
	GetFiles(xmldir, xmlfiles);
	GetFiles(jpgdir, jpgfiles);

	for (i = 0; i < xmlfiles.size(); i++)
	{
		string XmlfileNameOnly = GetFileNameWithNoSuffix(xmlfiles[i].c_str());
		for (j = 0; j < jpgfiles.size(); j++)
		{
			string JpgfileNameOnly = GetFileNameWithNoSuffix(jpgfiles[j].c_str());
			if (XmlfileNameOnly == JpgfileNameOnly)
			{
				break;
			}
		}
		if (j == jpgfiles.size())
		{
			Save << xmlfiles[i] << " has no jpg in dir " << jpgdir << endl;
		}
	}

	for (i = 0; i < jpgfiles.size(); i++)
	{
		string JpgfileNameOnly = GetFileNameWithNoSuffix(jpgfiles[i].c_str());
		for (j = 0; j < xmlfiles.size(); j++)
		{
			string XmlfileNameOnly = GetFileNameWithNoSuffix(xmlfiles[j].c_str());
			if (XmlfileNameOnly == JpgfileNameOnly)
			{
				break;
			}
		}
		if (j == xmlfiles.size())
		{
			Save << jpgfiles[i] << " has no xml in dir " << xmldir << endl;
		}
	}
}

void CheckLegality(string xmldir, int minw, int minh, ofstream& Save)
{
	int i;
	vector<string> xmlfiles;
	GetFiles(xmldir, xmlfiles);

	for (i = 0; i < xmlfiles.size(); i++)
	{
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLError err = doc.LoadFile(xmlfiles[i].c_str());
		tinyxml2::XMLElement* annotation = doc.FirstChildElement("annotation");
		while (annotation)
		{
			tinyxml2::XMLElement* filename = annotation->FirstChildElement("filename");
			tinyxml2::XMLElement* size = annotation->FirstChildElement("size");
			tinyxml2::XMLElement* width = size->FirstChildElement("width");
			tinyxml2::XMLElement* height = size->FirstChildElement("height");
			tinyxml2::XMLElement* depth = size->FirstChildElement("depth");
			if (!size)
			{
				Save << xmlfiles[i] << ": size not exist" << endl;
				break;
			}
			if (!width)
			{
				Save << xmlfiles[i] << ": width not exist" << endl;
				break;
			}
			if (!height)
			{
				Save << xmlfiles[i] << ": height not exist" << endl;
				break;
			}
			int tmpwidth = atoi(width->GetText());
			if (tmpwidth <= 0)
			{
				Save << xmlfiles[i] << ": width(" << tmpwidth << ") <= 0" << endl;
			}
			int tmpheight = atoi(height->GetText());
			if (tmpheight <= 0)
			{
				Save << xmlfiles[i] << ": height(" << tmpheight << ") <= 0" << endl;
			}

			tinyxml2::XMLElement* object = annotation->FirstChildElement("object");
			while (object)
			{
				tinyxml2::XMLElement* name = object->FirstChildElement("name");
				tinyxml2::XMLElement* score = object->FirstChildElement("score");
				tinyxml2::XMLElement* bndbox = object->FirstChildElement("bndbox");
				if (!bndbox)
				{
					Save << xmlfiles[i] << ": bndbox not exist" << endl;
					object = object->NextSiblingElement("object");
					continue;
				}
				tinyxml2::XMLElement* xmin = bndbox->FirstChildElement("xmin");
				tinyxml2::XMLElement* ymin = bndbox->FirstChildElement("ymin");
				tinyxml2::XMLElement* xmax = bndbox->FirstChildElement("xmax");
				tinyxml2::XMLElement* ymax = bndbox->FirstChildElement("ymax");
				if (!xmin)
				{
					Save << xmlfiles[i] << ": xmin not exist" << endl;
					object = object->NextSiblingElement("object");
					continue;
				}
				if (!ymin)
				{
					Save << xmlfiles[i] << ": ymin not exist" << endl;
					object = object->NextSiblingElement("object");
					continue;
				}
				if (!xmax)
				{
					Save << xmlfiles[i] << ": xmax not exist" << endl;
					object = object->NextSiblingElement("object");
					continue;
				}
				if (!ymax)
				{
					Save << xmlfiles[i] << ": ymax not exist" << endl;
					object = object->NextSiblingElement("object");
					continue;
				}
				float tmpxmin = atof(xmin->GetText());
				float tmpymin = atof(ymin->GetText());
				float tmpxmax = atof(xmax->GetText());
				float tmpymax = atof(ymax->GetText());
				// < 0
				if (tmpxmin < 0)
				{
					Save << xmlfiles[i] << ": xmin(" << tmpxmin << ") < 0" << endl;
				}
				if (tmpymin < 0)
				{
					Save << xmlfiles[i] << ": ymin(" << tmpymin << ") < 0" << endl;
				}
				if (tmpxmax < 0)
				{
					Save << xmlfiles[i] << ": xmax(" << tmpxmax << ") < 0" << endl;
				}
				if (tmpymax < 0)
				{
					Save << xmlfiles[i] << ": ymax(" << tmpymax << ") < 0" << endl;
				}
				// > width or height
				if (tmpxmin >= tmpwidth)
				{
					Save << xmlfiles[i] << ": xmin(" << tmpxmin << ") > width(" << tmpwidth << ")" << endl;
				}
				if (tmpymin >= tmpheight)
				{
					Save << xmlfiles[i] << ": ymin(" << tmpymin << ") > height(" << tmpheight << ")" << endl;
				}
				if (tmpxmax > tmpwidth)
				{
					Save << xmlfiles[i] << ": xmax(" << tmpxmax << ") > width(" << tmpwidth << ")" << endl;
				}
				if (tmpymax > tmpheight)
				{
					Save << xmlfiles[i] << ": ymax(" << tmpymax << ") > height(" << tmpheight << ")" << endl;
				}

				//min >= max
				if (tmpxmin >= tmpxmax)
				{
					Save << xmlfiles[i] << ": xmin(" << tmpxmin << ") > xmax(" << tmpxmax << ") " << endl;
				}
				if (tmpymin >= tmpymax)
				{
					Save << xmlfiles[i] << ": ymin(" << tmpymin << ") > ymax(" << tmpymax << ") " << endl;
				}
				
				// < minw or minh
				if ((tmpxmax - tmpxmin) < minw)
				{
					Save << xmlfiles[i] << ": (xmax(" << tmpxmax << ") - xmin(" << tmpxmin << ")) <" << minw << endl;
				}
				if ((tmpymax - tmpymin) < minh)
				{
					Save << xmlfiles[i] << ": (ymax(" << tmpymax << ") - ymin(" << tmpymin << "))<" << minh << endl;
				}

				object = object->NextSiblingElement("object");
			}
			annotation = annotation->NextSiblingElement("annotation");
		}
	}
}

int main(int argc, char** argv)
{
	string outfile;
	string xmldir, jpgdir;
	int minw, minh;
	if (argc < 6)
	{
		std::cout << argv[0] << " xmldir jpgdir minw minh" << endl;
	}
	else
	{
		xmldir = argv[1];
		jpgdir = argv[2];
		minw = atoi(argv[3]);
		minh = atoi(argv[4]);
		outfile = argv[5];

		std::cout << "xmldir:" << xmldir << endl;
		std::cout << "jpgdir:" << jpgdir << endl;
		std::cout << "minw:" << minw << endl;
		std::cout << "minh:" << minh << endl;
		std::cout << "outfile:" << outfile << endl;

		ofstream Save(outfile);
		if (!Save.is_open())
		{
			std::cout << "open error:" << outfile << endl;
			return -1;
		}

		CheckFilename(xmldir, Save);
		CheckFilename(jpgdir, Save);
		CheckConsistency(xmldir, jpgdir, Save);
		CheckLegality(xmldir, minw, minh, Save);
	}

	return 0;
}
