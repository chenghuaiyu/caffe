#ifndef BASE_H_
#define BASE_H_

#include <stdio.h>
#include <string.h>
#include <WINDOWS.H>
#include <WINDEF.H>



/**
 * @brief	ʮ����ת������
 *
 * @param data	��� ������������
 * @param dec	���� ʮ������
 */
void dec_to_byte(char *data, int dec);


/**
 * @brief	ʮ����תʮ������
 *
 * @param data	��� ʮ������������
 * @param dec	���� ʮ������
 */
void dec_to_hex(char *data, int dec);


/**
 * @brief	ʮ������תʮ����
 *
 * @param s	ʮ������������
 *
 * @return 	ʮ������
 */
int hex_to_dec(char *s);



//ʮ����תΪ������
char* __bit(int n,char*array,int size);

//������תΪʮ����(Ӧ���˲��,�������еı���mask����)
int __bit(char *array,int strlen);

//typedef unsigned long DWORD;
//typedef unsigned char BYTE;
DWORD HexToStr(const BYTE *pbHex, DWORD dwHexLen, BYTE *pbStr);

DWORD StrToHex(const BYTE *pbStr, DWORD dwStrLen, BYTE *pbHex); //����ת��
int checkHEX(unsigned char str[],int iLen);

#endif 
