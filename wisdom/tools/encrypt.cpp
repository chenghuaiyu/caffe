#include <stdio.h> 
#include <stdlib.h>
#include <string>
#include "../decryption/rc4.hpp"
#ifdef _LINUX_
#include <string.h>
#endif

#define BUF_SIZE 1024
#define MAX_MODEL_LENGTH (1024*1024)
#define MAX_WEIGHT_LENGTH (1024*1024*500)

void VimEncrypt2FileRc4(char* infile1, char* infile2, char *outfile, char*pwd) {
    rc4 rc4_cona((unsigned char *)pwd, strlen(pwd));
    char buf[BUF_SIZE];
    int uLen;
    int flen1 = 0;
    int flen2 = 0;
    FILE * fsrc1 = fopen(infile1, "rb");
    FILE * fsrc2 = fopen(infile2, "rb");
    FILE * fdst = fopen(outfile, "wb");
    fseek(fdst, 0, SEEK_SET);
    fseek(fsrc1, 0, SEEK_END);
    flen1 = ftell(fsrc1);
    fseek(fsrc1, 0, SEEK_SET);
    fwrite(&flen1, 1, sizeof(int), fdst);
    fseek(fsrc2, 0, SEEK_END);
    flen2 = ftell(fsrc2);
    fseek(fsrc2, 0, SEEK_SET);
    fwrite(&flen2, 1, sizeof(int), fdst);
    
    uLen = fread(buf, 1, BUF_SIZE, fsrc1);
    while (uLen>0) {
        rc4_cona.rc4_encode((unsigned char*)buf,uLen);
        fwrite(buf, 1, uLen, fdst);
        uLen = fread(buf, 1, BUF_SIZE, fsrc1);
    }
    
    uLen = fread(buf, 1, BUF_SIZE, fsrc2);
    while (uLen>0) {
        rc4_cona.rc4_encode((unsigned char*)buf,BUF_SIZE);
        fwrite(buf, 1, uLen, fdst);
        uLen = fread(buf, 1, BUF_SIZE, fsrc2);
    }

    fclose(fsrc1);
    fclose(fsrc2);
    fclose(fdst);
}


void VimDecrypt2FileRc4(char* infile, char *outfile1, char* outfile2 ,char*pwd) {
    rc4 rc4_cona((unsigned char *)pwd, strlen(pwd));
    char buf[BUF_SIZE];
    int uLen;
    int flen1 = 0;
    int flen2 = 0;
    FILE * fsrc = fopen(infile, "rb");
    FILE * fdst1 = fopen(outfile1, "wb");
    FILE * fdst2 = fopen(outfile2, "wb");
    fseek(fsrc, 0, SEEK_SET);
    uLen = fread(&flen1, 1, sizeof(int), fsrc);
    uLen = fread(&flen2, 1, sizeof(int), fsrc);
    fseek(fdst1, 0, SEEK_SET);
    fseek(fdst2, 0, SEEK_SET);
    
    while (flen1 > 0) {
        if (flen1 > BUF_SIZE) {
            uLen = fread(buf, 1, BUF_SIZE, fsrc);
        } else {
            uLen = fread(buf, 1, flen1, fsrc);
        }
        if (uLen <= 0) {
            break;
        }
        rc4_cona.rc4_encode((unsigned char*)buf,uLen);
        fwrite(buf, 1, uLen, fdst1);
        flen1 -= uLen;
    }
    
    while (flen2 > 0) {
        if (flen2 > BUF_SIZE) {
            uLen = fread(buf, 1, BUF_SIZE, fsrc);
        } else {
            uLen = fread(buf, 1, flen2, fsrc);
        }
        if (uLen <= 0) {
            break;
        }
        rc4_cona.rc4_encode((unsigned char*)buf,uLen);
        fwrite(buf, 1, uLen, fdst2);
        flen2 -= uLen;
    }
    
    fclose(fsrc);
    fclose(fdst1);
    fclose(fdst2);
}

