#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../tools/encrypt.hpp"


int main(int argc,char *argv[])/*����main()�����������в���*/
{
    char fname1[256];/*�û������Ҫ���ܵ��ļ���*/
    char fname2[256];/*�û������Ҫ���ܵ��ļ���*/
    char fname3[256];/*�û������Ҫ���ܵ��ļ���*/
    char pwd[256];/*������������*/
    int mode;
    if (argc != 6)
    {
        printf("usage: rc 0 infile1 infile2 outfile pwd\n");
        printf("usage: rc 1 infile outfile1 outfile2 pwd \n");
    }
    else
    {/*��������в�����ȷ,��ֱ�����г���*/
        mode = atoi(argv[1]);
        strcpy(fname1,argv[2]);
        strcpy(fname2,argv[3]);
        strcpy(fname3,argv[4]);
        strcpy(pwd,argv[5]);
        if (mode)
        {
            VimDecrypt2FileRc4(fname1,fname2,fname3,pwd);
        }
        else
        {
            VimEncrypt2FileRc4(fname1,fname2,fname3,pwd);
        }
    }
    return 0;
}

