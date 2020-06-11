/// \file SCWObjectDetectionV2.h
/// \brief SINO CLOUD WISDOM Object Detection header file.
/// 
/// Sinocloud Wisdom Object Detection header file, for Object Detection purpose.
///
/// \author chenghuaiyu@yunkouan.com
/// \version 2.1
/// \date 2020-06-02

#ifndef __SCW_OBJECT_DETECTION_H__
#define __SCW_OBJECT_DETECTION_H__

const char cObjectSep = ';';
const char cAliasSep = ':';
const char cSynonymSep = '=';
const char szObjKindSep[] = { cObjectSep, cObjectSep, '\0' };

/// \brief SCW error code enumerator.
/// 
/// SCW error code enumerator.
enum SCWErroEnum {
	SCWERR_NOERROR = 0, ///< no error
	SCWERR_PARAMETER = 1, ///< function parameter error 
	SCWERR_NOINIT = 2, ///< algorithm initialization function not invoked yet
	SCWERR_INITIALIZED = 3, ///< algorithm initialization function invoked already
	SCWERR_CONFIG = 4, ///< configure error
	SCWERR_FILENOTFOUND = 5, ///< file not found
	SCWERR_IMGFILEBUF = 6, ///< SCWImgBufStruct format error, cannot be saved to file
	SCWERR_FILE = 7, ///< other file error
	SCWERR_MODEL = 8, ///< algorithm model error
	SCWERR_ALLOBJECTSUNUSED = 9, ///< all objects unused
	SCWERR_JSON = 10, ///< JSON error
	SCWERR_NOMEMORY = 11, ///< insufficient memory
	SCWERR_DONGLE = 12, ///< no dongle or dongle has problem
	SCWERR_UNSUPPORT = 13, ///< unsupported yet
	SCWERR_FUNCTION = 14, ///< function invocation error
	SCWERR_FORMAT = 15, ///< format error
	SCWERR_NOTIMPLEMENT = 16, ///< not implemented yet

	SCWERR_NOGPU = 64,	///< GPU not found
	SCWERR_NOGPUMEMORY = 65,	///< insufficient GPU memory
};

typedef void * HScwAlg; ///< algorithm handle

/// \brief algorithm initialization function.
/// 
/// It should be invoked at the first step and invoked only once.
/// 
/// \param[out] phAlg algorithm handle pointer
/// \param[out] ppszObjectTypeRes the object types result in 3 kinds: used, 
///    unused, invalid, which split by double symbol ";". for example, if 
///    the algorithm support four objects: knife, scissors, knife_straight, 
///    knife_blade, and string "knife:刀;scissors=scissor:剪刀;gun:枪;spray:喷雾;" 
///    is given to the \a pszObjectTypes parameter, then \a ppszObjectTypeRes 
///    should point to the following string if it's not NULL:<em>
///    "knife:刀;scissors:剪刀;;knife_straight;knife_blade;;gun:枪;spray:喷雾;scissor:剪刀"
///    or
///    "knife:刀;scissors=scissor:剪刀;;knife_straight;knife_blade;;gun:枪;spray:喷雾;"
///    </em>
/// \param[in] pszObjectTypes object types to detect, which is specified 
///    in the UTF-8 string directly or by the file path. objects are split 
///    by symbol ";", symbol ":" means using the string replacement in the 
///    JSON detection result of image detection function, symbol "=" means 
///    a synonym. for example: in the string 
///			"knife:刀;scissors=scissor:剪刀;gun:枪;spray:喷雾;"
///    , "刀" is the replacement of "knife" in the JSON result, and "scissor" 
///    is a synonym for "scissors".
/// \param[in] pszConfig UTF-8 configuration string or file used for object 
///    detection, set NULL if no any configuration need.
/// \param[in] nKind switch to determine the format of two parameters: \a 
///    pszObjectTypes and \a pszConfig.
/// \return 0: success, otherwise failure, recommend to use value defined in #SCWErroEnum.
///    
/// \note \a ppszObjectTypeRes can be null pointer. if non null pointer 
///    specified and function return successfully, #SCWRelease(char **const) 
///    should be invoked to avoid the memory leakage.
///    \a pszObjectTypes can be null pointer. if null pointer or empty string 
///    specified, the algorithm supported objects are all used. so in the above 
///    example, the algorithm support four objects: knife, scissors, knife_straight,
///    knife_blade, then \a ppszObjectTypeRes should point to the following string 
///    if it's not NULL: "knife;scissors;knife_straight;knife_blade;;;;" or 
///    simply "knife;scissors;knife_straight;knife_blade"
///    <table>
///      <tr><th>\a nKind value of bits <th> Comments
///      <tr><td>|*|*|*|*|*|*|*|*|  <td> 0 means UTF-8 content; 1 means UTF-8 file name string, the file content encoding is not specified, although UTF-8 is recommended.
///      <tr><td> * * * * * * * ^--><td> the lowest bit determines the first parameter's format.
///      <tr><td> * * * ^----------><td> the fourth bit determines the second parameter's format.
///    </table>so that the following value of \a nKind means:
///    +  0: the first and second parameter: use content directly;
///    +  1: the first parameter: file, the second parameter: content;
///    + 16: the first parameter: content, the second parameter: file;
///    + 17: the first parameter: file, the second parameter: file;
int SCWInitObjectDetection(
	HScwAlg* phAlg,
	char** const ppszObjectTypeRes,
	const char* pszObjectTypes,
	const char* pszConfig,
	const unsigned char nKind
);

/// \brief algorithm uninitialization function.
/// 
/// It should be invoked at the last step and invoked only once.
///    
/// \return 0: success, otherwise failure, recommend to use value defined in #SCWErroEnum.
///    
int SCWUninitObjectDetection(
	HScwAlg* phAlg
);

/// \brief image detection function.
/// 
/// It should be invoked after successful invocation of initialization
/// function: #SCWInitObjectDetection.
///
/// \param[out] ppszDetectionResult detection result, UTF-8 JSON string.
/// \param[in] hAlg algorithm handle
/// \param[in] pszImagePath specify the UTF-8 string denoting the image file 
///    path to detect, if more than one file needed, semicolon symbol ";" 
///    is used to split the path. such as: "d:/image1_H.jpg;d:/image1_V.jpg".
/// \param[in] nConfidentialThreshold confident level used as a threshold, 
///    lower score's object should be discarded, value range: [0, 100]. 0
///    means it's the algorithm its own responsibility to choose the threshold.
/// \return 0: success, otherwise failure, recommend to use value defined in #SCWErroEnum.
/// 
/// \note \a ppszDetectionResult cannot be null pointer, and need to invoke 
///    #SCWRelease(char **const) to avoid the memory leakage.
///    \n following is an example of \a ppszDetectionResult string: \n<em>
///        {"ver":"2.0","images":[{"name":"201502022312_1.jpg","w":600,"h":
///        300,"conf":99,"objs":[{"obj":"knife: 刀 ","conf":90,"locations":[{"rect"
///        :{"left":50,"top":50,"right":150,"bottom":300},"conf":90},{"points"
///        :"channels:2;250,50;500,50;500,320;250,320","conf":80}]},{"obj":
///        "gun:枪","locations":[{"rect":{"left":450,"top":20,"right":500,
///        "bottom":300},"conf":95}]}]},{"name":"201502022313.jpg","w":600
///        ,"h":300,"objs":[{"obj":"liquid:液体","conf":85,"locations":[{"rect":{
///        "left":50,"top":50,"right":150,"bottom":300},"conf":90},{"rect":
///        {"left":250,"top":50,"right":500,"bottom":320},"conf":80}]},{
///        "obj":"gun:枪","conf":95,"locations":[{"rect":{"left":450,"top":20,
///        "right":500,"bottom":300},"conf":95}]}]},{"name":"1.jpg","w":20,
///        "h":30,"objs":[]}]}
///    </em>
int SCWDetectObjectByFile(
	char** const ppszDetectionResult,
	HScwAlg hAlg,
	const char*	pszImagePath,
	const int	nConfidentialThreshold
);


/// \brief release the memory buffer.
/// 
/// release the memory buffer.
///
/// \param[in] ppszBuf buffer to release.
void SCWRelease(
	char** const ppszBuf
);

/// \brief image file extension enumerator.
/// 
/// image file extension enumerator.
enum SCWImgTypeEnum {
	SCWImgType_NONE = 0, ///< not specified
	SCWImgType_JPG = 1, ///< jpg type 
	SCWImgType_PNG = 2, ///< png type 
	SCWImgType_BMP = 3, ///< bmp type 
	SCWImgType_TIF = 4, ///< tif type 
	SCWImgType_GIF = 5, ///< gif type 
	SCWIMGTYPE_WEBP = 6, ///< webp type
	SCWImgType_EXT1 = 7, ///< reserved
	SCWImgType_EXT2 = 8, ///< reserved
	SCWImgType_EXT3 = 9, ///< reserved
};

/// \brief image file buffer struct.
/// 
/// image file buffer struct.
struct SCWImgBufStruct {
	unsigned char* pImageBuf; ///< image file buffer
	unsigned long int nImageBufLen; ///< image file buffer size in bytes
	int nImgType; ///< image file buffer's type, specified in SCWImgTypeEnum
	char*	pszImagePath; ///< image filename, UTF-8 encoding
	int	nConfidentialThreshold; ///< confidential level used as a threshold, value range: [0, 100].
};

/// \brief image detection function.
/// 
/// It should be invoked after successfully invocation of initialization
/// function, #SCWInitObjectDetection.
///
/// \param[out] ppszDetectionResult detection result, UTF-8 JSON string.
/// \param[in] hAlg algorithm handle
/// \param[in] pScwImgStruct SCWImgBufStruct array, specify the image file 
///    buffer, the detection object should be discarded if its score lower 
///    than nConfidentialThreshold, one exception is if nConfidentialThreshold 
///    equal 0 means it's the algorithm's own responsibility to choose the 
///    threshold to discard or not the detection object.
/// \param[in] nScwImgStructCount how many ScwImgStruct passed in.
/// \return 0: success, otherwise failure, recommend to use value defined in #SCWErroEnum.
/// 
/// \note \a ppszDetectionResult cannot be null pointer, and need to invoke 
///    #SCWRelease(char **const) to avoid the memory leakage.
///
int SCWDetectObjectByFileBuf(
	char** const ppszDetectionResult,
	HScwAlg hAlg,
	const struct SCWImgBufStruct * pScwImgStruct,
	const int nScwImgStructCount
);

#endif // !__SCW_OBJECT_DETECTION_H__