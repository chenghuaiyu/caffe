
#include <vector>
#include <string>
#include <stdio.h> 
#include "../common.h"
#include "../util/util.h"
#include "tinyxml2.h"

#define MAX_LENG (1024*1024)

char g_OutXml[MAX_LENG + 1];

char* XmlOutput(vector<DETECT_FILE_S> files, vector<string> labels)
{
	vector<DETECT_BOX_S> boxes;
    
    tinyxml2::XMLDocument doc;

    for (int i = 0; i < files.size(); i += 1)
    {
        tinyxml2::XMLElement* annotation = doc.NewElement( "annotation" );
        tinyxml2::XMLElement* filename = doc.NewElement( "filename" );
        string filenameonly = GetFileNameByFilePathName(files[i].filename);
        filename->SetText(filenameonly.c_str());
        annotation->InsertEndChild( filename );
        tinyxml2::XMLElement* size = doc.NewElement( "size" );
        tinyxml2::XMLElement* width = doc.NewElement( "width" );
        width->SetText(files[i].width);
        size->InsertEndChild( width );
        tinyxml2::XMLElement* height = doc.NewElement( "height" );
		height->SetText(files[i].height);
        size->InsertEndChild( height );
        tinyxml2::XMLElement* depth = doc.NewElement( "depth" );
		depth->SetText(3);
        size->InsertEndChild( depth );
        annotation->InsertEndChild( size );
            
        boxes = files[i].boxes;
        for (int j = 0; j < boxes.size(); j++)
        {
            int label = static_cast<int>(boxes[j].label);
			int score = static_cast<int>(boxes[j].score * 100);
            int x = static_cast<int>(boxes[j].x);
            int y = static_cast<int>(boxes[j].y);
            int width = static_cast<int>(boxes[j].w);
            int height = static_cast<int>(boxes[j].h);
            
            tinyxml2::XMLElement* object = doc.NewElement( "object" );
            tinyxml2::XMLElement* name = doc.NewElement( "name" );
			name->SetText(labels[label].c_str());
			object->InsertEndChild(name);
			tinyxml2::XMLElement* scorex = doc.NewElement("score");
			scorex->SetText(score);
			object->InsertEndChild(scorex);
            tinyxml2::XMLElement* bndbox = doc.NewElement( "bndbox" );
            tinyxml2::XMLElement* xmin = doc.NewElement( "xmin" );
            xmin->SetText(x);
            bndbox->InsertEndChild( xmin );
            tinyxml2::XMLElement* ymin = doc.NewElement( "ymin" );
            ymin->SetText(y);
            bndbox->InsertEndChild( ymin );
            tinyxml2::XMLElement* xmax = doc.NewElement( "xmax" );
            xmax->SetText(x+width);
            bndbox->InsertEndChild( xmax );
            tinyxml2::XMLElement* ymax = doc.NewElement( "ymax" );
            ymax->SetText(y+height);
            bndbox->InsertEndChild( ymax );
            object->InsertEndChild( bndbox );
            annotation->InsertEndChild( object );
        }
        
        doc.InsertEndChild( annotation );
    }
    
    tinyxml2::XMLPrinter printer;
	doc.Print( &printer );
    
    const char *out = printer.CStr();
    int outlength = strlen(out) >= MAX_LENG ? MAX_LENG : strlen(out);

	memcpy(g_OutXml, out, outlength);
	g_OutXml[outlength] = '\0';

    return g_OutXml;
}

vector<DETECT_FILE_S> XmlParse(char* info, vector<string> labels)
{
	vector<DETECT_FILE_S> files;
    tinyxml2::XMLDocument doc;
	tinyxml2::XMLError err = doc.Parse(info);
    
    tinyxml2::XMLElement* annotation = doc.FirstChildElement( "annotation" );
    while(annotation)
    {
		DETECT_FILE_S dfile;
		tinyxml2::XMLElement* filename = annotation->FirstChildElement("filename");
		tinyxml2::XMLElement* size = annotation->FirstChildElement("size");
		tinyxml2::XMLElement* width = size->FirstChildElement("width");
		tinyxml2::XMLElement* height = size->FirstChildElement("height");
		tinyxml2::XMLElement* depth = size->FirstChildElement("depth");
		dfile.filename = filename->GetText();
		dfile.width = atoi(width->GetText());
		dfile.height = atoi(height->GetText());
		dfile.boxes.clear();
            
		tinyxml2::XMLElement* object = annotation->FirstChildElement("object");
		while (object)
        {
			tinyxml2::XMLElement* name = object->FirstChildElement("name");
			tinyxml2::XMLElement* score = object->FirstChildElement("score");
			tinyxml2::XMLElement* bndbox = object->FirstChildElement("bndbox");
			tinyxml2::XMLElement* xmin = bndbox->FirstChildElement("xmin");
			tinyxml2::XMLElement* ymin = bndbox->FirstChildElement("ymin");
			tinyxml2::XMLElement* xmax = bndbox->FirstChildElement("xmax");
			tinyxml2::XMLElement* ymax = bndbox->FirstChildElement("ymax");
			vector <string>::iterator iElement = find(labels.begin(), labels.end(), name->GetText());
			int label = 0;
			if (iElement != labels.end())
			{
				label = distance(labels.begin(), iElement);
			}
			DETECT_BOX_S outbox;
			outbox.label = label;
			outbox.score = atof(score->GetText());
			outbox.x = atof(xmin->GetText());
			outbox.y = atof(ymin->GetText());
			outbox.w = atof(xmax->GetText()) - atof(xmin->GetText());
			outbox.h = atof(ymax->GetText()) - atof(ymin->GetText());
			dfile.boxes.insert(dfile.boxes.end(), outbox);

			object = object->NextSiblingElement("object");
        }
		files.push_back(dfile);
        annotation = annotation->NextSiblingElement( "annotation" );
    }

	return files;
}

void XmlSave(string src, string dst, string fn, char* info, int savenobox)
{
	vector<DETECT_FILE_S> files;
	tinyxml2::XMLDocument doc;
	doc.Parse(info);

	tinyxml2::XMLElement* annotation = doc.FirstChildElement("annotation");
	while (annotation)
	{
		DETECT_FILE_S dfile;
		tinyxml2::XMLElement* filename = annotation->FirstChildElement("filename");
		tinyxml2::XMLElement* size = annotation->FirstChildElement("size");
		tinyxml2::XMLElement* width = size->FirstChildElement("width");
		tinyxml2::XMLElement* height = size->FirstChildElement("height");
		tinyxml2::XMLElement* depth = size->FirstChildElement("depth");
		dfile.filename = filename->GetText();
		dfile.width = atoi(width->GetText());
		dfile.height = atoi(height->GetText());
		dfile.boxes.clear();

		tinyxml2::XMLElement* object = annotation->FirstChildElement("object");
		if ((object == 0) && (!savenobox))
		{
			annotation = annotation->NextSiblingElement("annotation");
			continue;
		}

		std::string src_fn = "";
		src_fn += src;
		src_fn += filename->GetText();
		cv::Mat org_img = imread(src_fn.c_str());

		//save jpeg
		string dst_fn = "";
		dst_fn += dst;
		dst_fn += "/JPEGImages/";
		dst_fn += filename->GetText();
		imwrite(dst_fn.c_str(), org_img);

		//save xml
		string fileNameOnly = GetFileNameWithNoSuffix(src_fn);
		string filePathName = dst;
		filePathName += "/Annotations/";
		filePathName += fileNameOnly;
		filePathName += ".xml";
		FILE * fileXml = fopen(filePathName.c_str(), "wb");
		fwrite(info, 1, strlen(info), fileXml);
		fclose(fileXml);

		while (object)
		{
			//save jpeg with box
			tinyxml2::XMLElement* name = object->FirstChildElement("name");
			tinyxml2::XMLElement* score = object->FirstChildElement("score");
			tinyxml2::XMLElement* bndbox = object->FirstChildElement("bndbox");
			tinyxml2::XMLElement* xmin = bndbox->FirstChildElement("xmin");
			tinyxml2::XMLElement* ymin = bndbox->FirstChildElement("ymin");
			tinyxml2::XMLElement* xmax = bndbox->FirstChildElement("xmax");
			tinyxml2::XMLElement* ymax = bndbox->FirstChildElement("ymax");
			object = object->NextSiblingElement("object");
			std::cout << "name:" << name->GetText() << "\n" << endl;
			cv::Rect cvrect;
			cvrect.x = atoi(xmin->GetText());
			cvrect.y = atoi(ymin->GetText());
			cvrect.width = atoi(xmax->GetText()) - atoi(xmin->GetText());
			cvrect.height = atoi(ymax->GetText()) - atoi(ymin->GetText());
			rectangle(org_img, cvrect, cv::Scalar(0, 0, 255), 2);
			int x = (std::min)(cvrect.x, org_img.cols - 20);
			int y = (std::max)(cvrect.y, 10);
			string showinfo = "";
			showinfo += name->GetText();
			showinfo += ' ';
			showinfo += std::to_string((int)(atoi(score->GetText())));
			cv::putText(org_img, showinfo,
				cvPoint(x, y), cv::FONT_HERSHEY_PLAIN, 1, cvScalar(255, 0, 0), 1, CV_AA);
		}

		cout << "save jpgxml" << endl;
		string AnnJpg_dir = dst;
		AnnJpg_dir += "/AnnotationJpegs/";
		std::string jpgxml_fn = AnnJpg_dir;
		jpgxml_fn += filename->GetText();
		std::cout << "dst_fn:" << jpgxml_fn << "\n" << endl;
		imwrite(jpgxml_fn.c_str(), org_img);

		annotation = annotation->NextSiblingElement("annotation");
	}
}

