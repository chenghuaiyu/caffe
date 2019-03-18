/// \file SCWObjectDetectionV2.h
/// \brief sino cloud wisdom Object Detection header file.
/// 
/// sino cloud wisdom Object Detection header file, for Object Detection purpose.
///
/// \author chenghuaiyu@yunkouan.com
/// \version 2.0
/// \date 2019-2-20

#ifndef __SCW_OBJECT_DETECTION_H__
#define __SCW_OBJECT_DETECTION_H__

/// \brief image file extension.
/// 
/// image file extension.
enum SCWErroEnum {
	SCWERR_NOERROR = 0, ///< no error
	SCWERR_PARAMETER = 1, ///< function parameter error 
	SCWERR_NOINIT = 2, ///< algorithm initialization function not invoke yet
	SCWERR_INITIALED = 3, ///< algorithm initialization function invoked already
	SCWERR_CONFIG = 4, ///< configure error
	SCWERR_FILENOTFOUND = 5, ///< file not found
	SCWERR_IMGFILEBUF = 6, ///< SCWImgBufStruct * format error, cannot be saved to file
	SCWERR_FILE = 7, ///< other file error.
	SCWERR_MODEL = 8, ///< algorithm model error
	SCWERR_ALLOBJECTSUNUSED = 9, ///< all objects unused.
	SCWERR_JSON = 10, ///< JSON error
	SCWERR_NOMEMORY = 11, ///< no sufficient memory
	SCWERR_DONGLE = 12, ///< no dongle or dongle has problem
	SCWERR_UNSUPPORT = 13, ///< unsupported yet
	SCWERR_FUNCTION = 14, ///< function invocation error
	SCWERR_FORMAT = 15, ///< format error

	SCWERR_NOTIMPLEMENT = 16, ///< not implement yet

	SCWERR_NOGPU = 64,	///< GPU not found
	SCWERR_NOGPUMEMORY = 65,	///< GPU memory not enough
	//SCWERR_,
	//SCWERR_,
	};

typedef void * HScwAlg;

/// \brief algorithm initialization function.
/// 
/// It should be invoked at the first step and invoked only once.
/// 
/// \param[out] hAlg 
/// \param[out] ppszObjectTypeRes the object types result of 3 kinds: used, 
///    unused, invalid, which split by double symbol ";". for example, if 
///    the algorithm support four objects: knife, scissors, knife_straight, 
///    knife_blade, and string "knife:刀;scissors=scissor:剪刀;gun:枪;spray:喷雾;" 
///    is given to the first parameter, then ppszObjectTypeRes should point 
///    to the following string if it's not NULL:<em>
///    "knife:刀;scissors=scissor:剪刀;;knife_straight;knife_blade;;gun:枪;spray:喷雾;"
///    </em>
/// \param[in] pszObjectTypes specify object types to detect, which specified 
///    in the UTF-8 string directly or by the file path. objects are split 
///    by symbol ";", symbol ":" means using the string replacement in the 
///    JSON detection result of image detection function, symbol "=" means 
///    a synonym. for example: in the string 
///			"knife:刀;scissors=scissor:剪刀;gun:枪;spray:喷雾;"
///    , "刀" is the replacement of "knife" in the JSON result, and "scissor" 
///    is a synonym for "scissors".
/// \param[in] pszConfig UTF-8 configuration string or file used for object 
///    detection, set NULL if no need any configuration.
/// \param[in] nKind switch to determine the first two parameters' format.
/// \return 0: success, otherwise failure, recommend to use value defined in SCWErroEnum.
///    
/// \note \a ppszObjectTypeRes can be null pointer. if non null pointer 
///    specified and function return successfully, SCWRelease should 
///    be invoked to avoid the memory leakage.
///    <table>
///      <tr><th>nKind value of bits <th> Comments
///      <tr><td>|*|*|*|*|*|*|*|*|  <td> 0 means UTF-8 content; 1 means UTF-8 file name string, the file content encoding is not specified, although UTF-8 is recommended.
///      <tr><td> * * * * * * * ^--><td> the lowest bit determine the first paramter's format.
///      <tr><td> * * * ^----------><td> the forth bit determine the second paramter's format.
///    </table>so that the following value of nKind means:
///    +  0: the first and second paramter: use content directly;
///    +  1: the first paramter: file, the second paramter: content;
///    + 16: the first paramter: content, the second paramter: file;
///    + 17: the first paramter: file, the second paramter: file;
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
/// \return 0: success, otherwise failure, recommend to use value defined in SCWErroEnum.
///    
int SCWUninitObjectDetection(
	HScwAlg* phAlg
	);

/// \brief image detection function.
/// 
/// It should be invoked after successful invocation of initialization
/// function, SCWInitObjectDetection.
///
/// \param[in] pszImagePath specify the UTF-8 string denoting the image file 
///    path to detect, if more than one file needed, semicolon symbol ";" 
///    is used to split the path. such as: "d:/image1_H.jpg;d:/image1_V.jpg".
/// \param[in] nConfidentialThreshold confident level used as a threshold, 
///    lower score's object should be discarded, value range: [0, 100]. 0
///    means it's the algorithm its own responsibility to choose the threshold.
/// \param[out] ppszDetectionResult detection result, UTF-8 JSON string.
/// \return 0: success, otherwise failure.
/// 
/// \note ppszDetectionResult cannot be null pointer, and need to invoke 
///    SCWRelease to avoid the memory leakage.
///    \n following is an example of \a ppszDetectionResult string: \n<em>
///        {"ver":"2.0","images":[{"name":"201502022312_1.jpg","w":600,"h":
///        300,"conf":99,"objs":[{"obj":"apple","conf":90,"locations":[{"rect"
///        :{"left":50,"top":50,"right":150,"bottom":300},"conf":90},{"points"
///        :"channels:2;250,50;500,50;500,320;250,320","conf":80}]},{"obj":
///        "gun","locations":[{"rect":{"left":450,"top":20,"right":500,
///        "bottom":300},"conf":95}]}]},{"name":"201502022313.jpg","w":600
///        ,"h":300,"objs":[{"obj":"apple","conf":85,"locations":[{"rect":{
///        "left":50,"top":50,"right":150,"bottom":300},"conf":90},{"rect":
///        {"left":250,"top":50,"right":500,"bottom":320},"conf":80}]},{
///        "obj":"gun","conf":95,"locations":[{"rect":{"left":450,"top":20,
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
/// The function should be invoked to avoid the memory leakage.
///
/// \param[in] ppszBuf buffer to release.
void SCWRelease(
	char** const ppszBuf
	);

/// \brief image file extension.
/// 
/// image file extension.
enum SCWImgTypeEnum {
	SCWImgType_NONE = 0, ///< not specified
	SCWImgType_JPG, ///< jpg type 
	SCWImgType_PNG, ///< png type 
	SCWImgType_BMP, ///< bmp type 
	SCWImgType_TIF, ///< tif type 
	SCWImgType_GIF, ///< gif type 
	SCWImgType_EXT1, ///< reserved
	SCWImgType_EXT2, ///< reserved
	SCWImgType_EXT3, ///< reserved
	};

/// \brief image file buffer struct.
/// 
/// image file extension.
struct SCWImgBufStruct {
	const unsigned char* pImageBuf; ///< image file buffer 
	int nImageBufLen; ///< image file buffer size in bytes
	int nImgType; ///< image file extension defined in SCWImgTypeEnum
	int	nConfidentialThreshold; ///<  confident level used as a threshold, 
	///< lower score's object should be discarded, value range: [0, 100]. 
	///< 0 means it's the algorithm its own responsibility to choose the threshold.
	};

/// \brief image detection function.
/// 
/// It should be invoked after successfully invocation of initialization
/// function, SCWInitObjectDetection.
///
/// \param[in] pScwImgStruct specify the UTF-8 string denoting the image file 
///    path to detect, if more than one file needed, semicolon symbol ";" 
///    is used to split the path. such as: "d:/image1_H.jpg;d:/image1_V.jpg".
/// \param[in] nScwImgStructCount how many ScwImgStruct passed in.
/// \param[out] ppszDetectionResult detection result, UTF-8 JSON string.
/// \return 0: success, otherwise failure.
/// 
/// \note ppszDetectionResult cannot be null pointer, and need to invoke 
///    SCWRelease to avoid the memory leakage.
///
int SCWDetectObjectByFileBuf(
	char** const ppszDetectionResult,
	HScwAlg hAlg,
	const struct SCWImgBufStruct * pScwImgStruct,
	const int		nScwImgStructCount
	);

#endif // !__SCW_OBJECT_DETECTION_H__