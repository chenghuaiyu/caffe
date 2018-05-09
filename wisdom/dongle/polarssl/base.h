#ifndef BASE_H_
#define BASE_H_

#include <stdio.h>
#include <string.h>
#include <WINDOWS.H>
#include <WINDEF.H>



/**
 * @brief	十进制转二进制
 *
 * @param data	输出 二进制数数组
 * @param dec	输入 十进制数
 */
void dec_to_byte(char *data, int dec);


/**
 * @brief	十进制转十六进制
 *
 * @param data	输出 十六进制数数组
 * @param dec	输入 十进制数
 */
void dec_to_hex(char *data, int dec);


/**
 * @brief	十六进制转十进制
 *
 * @param s	十六进制数数组
 *
 * @return 	十进制数
 */
int hex_to_dec(char *s);



//十进制转为二进制
char* __bit(int n,char*array,int size);

//二进制转为十进制(应用了查表法,本程序中的表是mask数组)
int __bit(char *array,int strlen);

//typedef unsigned long DWORD;
//typedef unsigned char BYTE;
DWORD HexToStr(const BYTE *pbHex, DWORD dwHexLen, BYTE *pbStr);

DWORD StrToHex(const BYTE *pbStr, DWORD dwStrLen, BYTE *pbHex); //类型转换
int checkHEX(unsigned char str[],int iLen);

#endif 
