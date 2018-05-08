#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "cjson.h"
#include "../common.h"
#include "../util/util.h"
#include "opencv2/opencv.hpp"

using namespace cv;

typedef struct cImageCoordsJSON {
    string objecttype;
    int score;
    int x1, y1;
    int x2, y2;
    int x3, y3;
    int x4, y4;
}cImageCoordsJSON;

typedef struct cImageJSON {
    string imgname;
    int width;
    int height;
    std::vector<cImageCoordsJSON> coords;
}cImageJSON;

#define MAX_LENG (1024*1024)
char g_OutSring[MAX_LENG+1];

char* JsonCreateSpec(std::vector<cImageJSON> &imgArrayJSON, vector<string> labels)
{
    int i, j, k;
    char *out;
    int outlength = 0;
    //string out;
    cJSON *root, *result, *coords;
    
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "result", result = cJSON_CreateArray());

    for (i = 0; i < imgArrayJSON.size(); i++)
    {
        cJSON *resultImage, *coordsImage;
        cImageJSON imageJSON = imgArrayJSON[i];
		if (imageJSON.coords.size() == 0)
		{
			cJSON_AddItemToObject(result, "", resultImage = cJSON_CreateObject());
			cJSON_AddStringToObject(resultImage, "imgname", imageJSON.imgname.c_str());
			cJSON_AddStringToObject(resultImage, "objecttype", labels[0].c_str());
			cJSON_AddNumberToObject(resultImage, "width", imageJSON.width);
			cJSON_AddNumberToObject(resultImage, "height", imageJSON.height);
			cJSON_AddItemToObject(resultImage, "coords", coords = cJSON_CreateArray());
			continue;
		}
        for (k = 0; k < labels.size(); k++)
        {
            string label = labels[k];
            int found = 0;
            for (j = 0; j < imageJSON.coords.size(); j++)
            {
                if ((found == 0) && (imageJSON.coords[j].objecttype == label))
                {
                    found = 1;
                    cJSON_AddItemToObject(result, "", resultImage = cJSON_CreateObject());
                    cJSON_AddStringToObject(resultImage, "imgname", imageJSON.imgname.c_str());
					cJSON_AddStringToObject(resultImage, "objecttype", imageJSON.coords[j].objecttype.c_str());
                    cJSON_AddNumberToObject(resultImage, "width", imageJSON.width);
					cJSON_AddNumberToObject(resultImage, "height", imageJSON.height);
					cJSON_AddItemToObject(resultImage, "coords", coords = cJSON_CreateArray());
                }
                if (found)
                {
                    cJSON_AddItemToObject(coords, "", coordsImage = cJSON_CreateObject());
                    cJSON_AddNumberToObject(coordsImage, "x1", imageJSON.coords[j].x1);
                    cJSON_AddNumberToObject(coordsImage, "y1", imageJSON.coords[j].y1);
                    cJSON_AddNumberToObject(coordsImage, "x2", imageJSON.coords[j].x2);
                    cJSON_AddNumberToObject(coordsImage, "y2", imageJSON.coords[j].y2);
                    cJSON_AddNumberToObject(coordsImage, "x3", imageJSON.coords[j].x3);
                    cJSON_AddNumberToObject(coordsImage, "y3", imageJSON.coords[j].y3);
                    cJSON_AddNumberToObject(coordsImage, "x4", imageJSON.coords[j].x4);
                    cJSON_AddNumberToObject(coordsImage, "y4", imageJSON.coords[j].y4);
                }
            }
        }
    }

    out = cJSON_Print(root);
	printf("%s\n",out);
    cJSON_Delete(root);

    outlength = strlen(out) >= MAX_LENG ? MAX_LENG : strlen(out);

    memcpy(g_OutSring, out, outlength);
    g_OutSring[outlength] = '\0';
    free(out);

    return g_OutSring;
}

char* JsonOutputSpec(vector<DETECT_FILE_S> files, vector<string> labels)
{
    std::vector<cImageJSON> imgArrayJSON;
    cImageJSON imgJSON;
    cImageCoordsJSON imageCoords;
	vector<DETECT_BOX_S> boxes;

    for (int i = 0; i < files.size(); i += 1)
    {
        imgJSON.coords.clear();
        imgJSON.imgname = files[i].filename;
        imgJSON.width = files[i].width;
        imgJSON.height = files[i].height;
            
        boxes = files[i].boxes;
        for (int j = 0; j < boxes.size(); j++)
        {
            int label = static_cast<int>(boxes[j].label);
            int score = static_cast<int>(boxes[j].score*100);
            imageCoords.objecttype = labels[label];
            imageCoords.score = score;
            imageCoords.x1 = boxes[j].x;
            imageCoords.y1 = boxes[j].y;
            imageCoords.x2 = boxes[j].x + boxes[j].w;
            imageCoords.y2 = boxes[j].y;
            //3和4的位置互换
            imageCoords.x4 = boxes[j].x;
            imageCoords.y4 = boxes[j].y + boxes[j].h;
            imageCoords.x3 = boxes[j].x + boxes[j].w;
            imageCoords.y3 = boxes[j].y + boxes[j].h;
            imgJSON.coords.push_back(imageCoords);
        }
        imgArrayJSON.push_back(imgJSON);
    }

    return JsonCreateSpec(imgArrayJSON, labels);
}

char* JsonCreate(std::vector<cImageJSON> &imgArrayJSON)
{
    int i, j;
    char *out;
    int outlength = 0;
    //string out;
    cJSON *root, *result, *coords;
    
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "result", result = cJSON_CreateArray());

    for (i = 0; i < imgArrayJSON.size(); i++)
    {
        cJSON *resultImage, *coordsImage;
        cImageJSON imageJSON = imgArrayJSON[i];
        cJSON_AddItemToObject(result, "", resultImage = cJSON_CreateObject());
        cJSON_AddStringToObject(resultImage, "imgname", imageJSON.imgname.c_str());
        cJSON_AddNumberToObject(resultImage, "width", imageJSON.width);
        cJSON_AddNumberToObject(resultImage, "height", imageJSON.height);
        cJSON_AddItemToObject(resultImage, "coords", coords = cJSON_CreateArray());

        for (j = 0; j < imageJSON.coords.size(); j++)
        {
            cJSON_AddItemToObject(coords, "", coordsImage = cJSON_CreateObject());
            cJSON_AddStringToObject(coordsImage, "objecttype", imageJSON.coords[j].objecttype.c_str());
            cJSON_AddNumberToObject(coordsImage, "score", imageJSON.coords[j].score);
            cJSON_AddNumberToObject(coordsImage, "x1", imageJSON.coords[j].x1);
            cJSON_AddNumberToObject(coordsImage, "y1", imageJSON.coords[j].y1);
            cJSON_AddNumberToObject(coordsImage, "x2", imageJSON.coords[j].x2);
            cJSON_AddNumberToObject(coordsImage, "y2", imageJSON.coords[j].y2);
            cJSON_AddNumberToObject(coordsImage, "x3", imageJSON.coords[j].x3);
            cJSON_AddNumberToObject(coordsImage, "y3", imageJSON.coords[j].y3);
            cJSON_AddNumberToObject(coordsImage, "x4", imageJSON.coords[j].x4);
            cJSON_AddNumberToObject(coordsImage, "y4", imageJSON.coords[j].y4);
        }
    }

    out = cJSON_Print(root);
    cJSON_Delete(root);

    outlength = strlen(out) >= MAX_LENG ? MAX_LENG : strlen(out);

    memcpy(g_OutSring, out, outlength);
    g_OutSring[outlength] = '\0';
    free(out);

    return g_OutSring;
}

char* JsonOutput(vector<DETECT_FILE_S> files, vector<string> labels)
{
    std::vector<cImageJSON> imgArrayJSON;
    cImageJSON imgJSON;
    cImageCoordsJSON imageCoords;
	vector<DETECT_BOX_S> boxes;

    for (int i = 0; i < files.size(); i += 1)
    {
        imgJSON.coords.clear();
        imgJSON.imgname = GetFileNameByFilePathName(files[i].filename);
        imgJSON.width = files[i].width;
        imgJSON.height = files[i].height;
            
        boxes = files[i].boxes;
        for (int j = 0; j < boxes.size(); j++)
        {
            int label = static_cast<int>(boxes[j].label);
            int score = static_cast<int>(boxes[j].score*100);
            imageCoords.objecttype = labels[label];
            imageCoords.score = score;
            imageCoords.x1 = boxes[j].x;
            imageCoords.y1 = boxes[j].y;
            imageCoords.x2 = boxes[j].x + boxes[j].w;
            imageCoords.y2 = boxes[j].y;
            //3和4的位置互换
            imageCoords.x4 = boxes[j].x;
            imageCoords.y4 = boxes[j].y + boxes[j].h;
            imageCoords.x3 = boxes[j].x + boxes[j].w;
            imageCoords.y3 = boxes[j].y + boxes[j].h;
            imgJSON.coords.push_back(imageCoords);
        }
        imgArrayJSON.push_back(imgJSON);
    }

    return JsonCreate(imgArrayJSON);
}

vector<DETECT_FILE_S> JsonParse(char* info, vector<string> labels)
{
    vector<DETECT_FILE_S> files;
    cJSON * json = cJSON_Parse(info);
    cJSON * result = cJSON_GetObjectItem(json, "result");
    int filenum = cJSON_GetArraySize(result);
    printf("filenum:%d\n", filenum);
    for (int i = 0; i < filenum; i++)
    {
        DETECT_FILE_S dfile;
        cJSON * tmpfile = cJSON_GetArrayItem(result,i);
        cJSON * kongge = cJSON_GetObjectItem(tmpfile, "");
        cJSON * imgname = cJSON_GetObjectItem(tmpfile, "imgname");
        cJSON * width = cJSON_GetObjectItem(tmpfile, "width");
        cJSON * height = cJSON_GetObjectItem(tmpfile, "height");
        cJSON * coords = cJSON_GetObjectItem(tmpfile, "coords");
        
        dfile.filename = imgname->valuestring;
        dfile.width = width->valueint;
        dfile.height = height->valueint;
        dfile.boxes.clear();
        int boxnum = cJSON_GetArraySize(coords);
        printf("boxnum:%d\n", boxnum);
        for (int j = 0; j < boxnum; j++)
        {
            cJSON * tmpbox = cJSON_GetArrayItem(coords,j);
            cJSON * objecttype = cJSON_GetObjectItem(tmpbox, "objecttype");
            vector <string>::iterator iElement = find(labels.begin(), labels.end(),objecttype->valuestring);  
            int label = 0;
            if ( iElement != labels.end() )  
            {  
                label = distance(labels.begin(),iElement);  
            }  
            cJSON * score = cJSON_GetObjectItem(tmpbox, "score");
            cJSON * x1 = cJSON_GetObjectItem(tmpbox, "x1");
            cJSON * y1 = cJSON_GetObjectItem(tmpbox, "y1");
            cJSON * x2 = cJSON_GetObjectItem(tmpbox, "x2");
            cJSON * y2 = cJSON_GetObjectItem(tmpbox, "y2");
            cJSON * x3 = cJSON_GetObjectItem(tmpbox, "x3");
            cJSON * y3 = cJSON_GetObjectItem(tmpbox, "y3");
            cJSON * x4 = cJSON_GetObjectItem(tmpbox, "x4");
            cJSON * y4 = cJSON_GetObjectItem(tmpbox, "y4");
            DETECT_BOX_S outbox;
    		outbox.label = label;
    		outbox.score = score->valueint;
    		outbox.x = x1->valueint;
    		outbox.y = y1->valueint;
    		outbox.w = x3->valueint - x1->valueint;
    		outbox.h = y3->valueint - y1->valueint;
            dfile.boxes.insert(dfile.boxes.end(), outbox);
        }
        files.push_back(dfile);
    }
    
    char *out = cJSON_Print(json);
    cJSON_Delete(json);
    printf("out:%s\n", out);
    free(out);

    return files;
}

void JsonSaveInfo(string dst, string fn, char* info)
{
    string fileNameOnly = GetFileNameWithNoSuffix(fn.c_str());
    string filePathName = dst;
    filePathName += "/Annotations/";
    filePathName += fileNameOnly;
    filePathName += ".json";
    FILE * fileJson = fopen(filePathName.c_str(), "wb");
    fwrite(info, 1, strlen(info), fileJson);
    fclose(fileJson);
}

void JsonSaveJpeg(string src, string dst, string fn, char* info)
{
    int ret;
    
    fn = GetFileNameByFilePathName(fn.c_str());
    
    std::string src_fn = "";
    src_fn += src;
    src_fn += fn;
    cv::Mat org_img = imread(src_fn.c_str());

    string dst_fn = "";
    dst_fn += dst;
    dst_fn += "/JPEGImages/";
    dst_fn += fn;

    imwrite(dst_fn.c_str(), org_img);
}

void JsonSaveJpegWithBox(string src, string dst, string fn, char* info,int savenobox)
{
    int ret;

    cJSON * json = cJSON_Parse(info);
    cJSON * result = cJSON_GetObjectItem(json, "result");
    int filenum = cJSON_GetArraySize(result);
    for (int i = 0; i < filenum; i++)
    {
        cJSON * tmpfile = cJSON_GetArrayItem(result,i);
        cJSON * imgname = cJSON_GetObjectItem(tmpfile, "imgname");
        std::string src_fn = "";
        src_fn += src;
        src_fn += imgname->valuestring;
        cv::Mat org_img = imread(src_fn.c_str());
        
        cJSON * coords = cJSON_GetObjectItem(tmpfile, "coords");
        int boxnum = cJSON_GetArraySize(coords);
        if ((boxnum == 0) && (!savenobox))
        {
            continue;
        }
        for (int j = 0; j < boxnum; j++)
        {
            cJSON * tmpbox = cJSON_GetArrayItem(coords,j);
            cJSON * objecttype = cJSON_GetObjectItem(tmpbox, "objecttype");
            string label = objecttype->valuestring;
            cJSON *score = cJSON_GetObjectItem(tmpbox, "score");
            cJSON * x1 = cJSON_GetObjectItem(tmpbox, "x1");
            cJSON * y1 = cJSON_GetObjectItem(tmpbox, "y1");
            cJSON * x3 = cJSON_GetObjectItem(tmpbox, "x3");
            cJSON * y3 = cJSON_GetObjectItem(tmpbox, "y3");
           
            cv::Rect cvrect;
            cvrect.x = x1->valueint;
            cvrect.y = y1->valueint;
            cvrect.width = x3->valueint - x1->valueint;
            cvrect.height = y3->valueint - y1->valueint;
            rectangle(org_img, cvrect, cv::Scalar(0, 0, 255), 2);
            int x = (std::min)(cvrect.x, org_img.cols - 20);
            int y = (std::max)(cvrect.y, 10);
            cv::putText(org_img, label + ' ' + std::to_string((int)(score->valueint)),
                cvPoint(x, y), cv::FONT_HERSHEY_PLAIN, 1, cvScalar(255, 0, 0), 1, CV_AA);
        }
        
        cout << "save jpgxml" << endl;
        string AnnJpg_dir = dst;
        AnnJpg_dir += "/AnnotationJpegs/";
        std::string jpgxml_fn = AnnJpg_dir;
        jpgxml_fn += imgname->valuestring;
        std::cout << "dst_fn:" << jpgxml_fn << "\n" << endl;
        imwrite(jpgxml_fn.c_str(), org_img);
    }
    
    char *out = cJSON_Print(json);
    cJSON_Delete(json);
    free(out);
}

void JsonSaveAll(string src, string dst, char* info,int savenobox)
{
    int ret;

    cJSON * json = cJSON_Parse(info);
    cJSON * result = cJSON_GetObjectItem(json, "result");
    int filenum = cJSON_GetArraySize(result);
    for (int i = 0; i < filenum; i++)
    {
        cJSON * tmpfile = cJSON_GetArrayItem(result,i);
        cJSON * coords = cJSON_GetObjectItem(tmpfile, "coords");
        int boxnum = cJSON_GetArraySize(coords);
        if ((boxnum == 0) && (!savenobox))
        {
            continue;
        }

        cJSON * imgname = cJSON_GetObjectItem(tmpfile, "imgname");
        std::string src_fn = "";
        src_fn += src;
        src_fn += imgname->valuestring;
        cv::Mat org_img = imread(src_fn.c_str());
        
        //save jpeg
        string dst_fn = "";
        dst_fn += dst;
        dst_fn += "/JPEGImages/";
        dst_fn += imgname->valuestring;
        imwrite(dst_fn.c_str(), org_img);

        //save json
        string fileNameOnly = GetFileNameWithNoSuffix(src_fn);
        string filePathName = dst;
        filePathName += "/Annotations/";
        filePathName += fileNameOnly;
        filePathName += ".json";
        FILE * fileJson = fopen(filePathName.c_str(), "wb");
        fwrite(info, 1, strlen(info), fileJson);
        fclose(fileJson);

        //save jpeg with box
        for (int j = 0; j < boxnum; j++)
        {
            cJSON * tmpbox = cJSON_GetArrayItem(coords,j);
            cJSON * objecttype = cJSON_GetObjectItem(tmpbox, "objecttype");
            string label = objecttype->valuestring;
            cJSON *score = cJSON_GetObjectItem(tmpbox, "score");
            cJSON * x1 = cJSON_GetObjectItem(tmpbox, "x1");
            cJSON * y1 = cJSON_GetObjectItem(tmpbox, "y1");
            cJSON * x3 = cJSON_GetObjectItem(tmpbox, "x3");
            cJSON * y3 = cJSON_GetObjectItem(tmpbox, "y3");
           
            cv::Rect cvrect;
            cvrect.x = x1->valueint;
            cvrect.y = y1->valueint;
            cvrect.width = x3->valueint - x1->valueint;
            cvrect.height = y3->valueint - y1->valueint;
            rectangle(org_img, cvrect, cv::Scalar(0, 0, 255), 2);
            int x = (std::min)(cvrect.x, org_img.cols - 20);
            int y = (std::max)(cvrect.y, 10);
            cv::putText(org_img, label + ' ' + std::to_string((int)(score->valueint)),
                cvPoint(x, y), cv::FONT_HERSHEY_PLAIN, 1, cvScalar(255, 0, 0), 1, CV_AA);
        }
        
        cout << "save jpgxml" << endl;
        string AnnJpg_dir = dst;
        AnnJpg_dir += "/AnnotationJpegs/";
        std::string jpgxml_fn = AnnJpg_dir;
        jpgxml_fn += imgname->valuestring;
        std::cout << "dst_fn:" << jpgxml_fn << "\n" << endl;
        imwrite(jpgxml_fn.c_str(), org_img);

    }
    
    char *out = cJSON_Print(json);
    cJSON_Delete(json);
    free(out);
}

void JsonSave(string src, string dst, string fn, char* info, int savenobox)
{
#if 0
    /* save cjson */
    cout << "save cjson" << endl;
    JsonSaveInfo(dst, fn, info);
    
    /*save jpg */
    cout << "save jpg" << endl;
    JsonSaveJpeg(src, dst, fn, info);
    
    /*save jpg + box */
    cout << "save jpg + box" << endl;
    JsonSaveJpegWithBox(src, dst, fn, info, savenobox);
#else
    JsonSaveAll(src, dst, info, savenobox);
#endif
}

