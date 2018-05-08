#include <stdio.h> 
#include <stdlib.h>
#include <string>
#include "rc4.hpp"
#ifdef _LINUX_
#include <string.h>
#endif

/*****************************************************************************/
/*                                    rc4                                    */
/*****************************************************************************/
//Downlaod by http://www.codefans.net
rc4::rc4(unsigned char *key_data_ptr,int nLen)
{
    prepare_key(key_data_ptr,nLen);
}

void rc4::prepare_key(unsigned char *key_data_ptr, int key_data_len)
{
    unsigned char index1;
    unsigned char index2;
    unsigned char* state;
    short counter;
    
    state = &key.state[0];
    
    for(counter = 0; counter < 256; counter++)
        state[counter] = (unsigned char)counter;
    
    key.x = 0;
    key.y = 0;
    index1 = 0;
    index2 = 0;
    
    for(counter = 0; counter < 256; counter++)
    {
        index2 = (key_data_ptr[index1] + state[counter] + index2) % 256;
        swap_byte(&state[counter], &state[index2]);
        index1 = (index1 + 1) % key_data_len;
    }
}

void rc4::swap_byte(unsigned char *a, unsigned char *b)
{
    unsigned char x;
    x=*a;*a=*b;*b=x;
}

void rc4::rc4_encode(unsigned char *buffer_ptr, int buffer_len)
{
    unsigned char x;
    unsigned char y;
    unsigned char* state;
    unsigned char xorIndex;
    short counter;
    
    x = key.x;
    y = key.y;
    state = &key.state[0];
    
    for(counter = 0; counter < buffer_len; counter++)
    {
        x = (x + 1) % 256;
        y = (state[x] + y) % 256;
        swap_byte(&state[x], &state[y]);
        xorIndex = (state[x] + state[y]) % 256;
        buffer_ptr[counter] ^= state[xorIndex];
    }
    key.x = x;
    key.y = y;
}

/*****************************************************************************/
/*                                    app                                    */
/*****************************************************************************/
#define BUF_SIZE 1024
#define MAX_MODEL_LENGTH (1024*1024)
#define MAX_WEIGHT_LENGTH (1024*1024*500)

bool VimDecrypt2BufRc4(const char* infile, const char *pwd, char** out1, 
                            char** out2, int *len1, int *len2)
{
    rc4 rc4_cona((unsigned char *)pwd, strlen(pwd));
    char buf[BUF_SIZE];
    int uLen;
    int modelLength = 0;
    int weightLength = 0;
    char* modelBuf;
    char* weightBuf;
    FILE * fsrc = fopen(infile, "rb");
    fseek(fsrc, 0, SEEK_SET);
    uLen = fread(&modelLength, 1, sizeof(int), fsrc);
    uLen = fread(&weightLength, 1, sizeof(int), fsrc);

    if (modelLength > MAX_MODEL_LENGTH || weightLength > MAX_WEIGHT_LENGTH)
    {
        fclose(fsrc);
        return false;
    }
    modelBuf = (char *)malloc(modelLength);
    weightBuf = (char *)malloc(weightLength);
    *out1 = modelBuf;
    *out2 = weightBuf;
    *len1 = modelLength;
    *len2 = weightLength;
    printf("modelLength:%d, weightLength:%d\n", modelLength, weightLength);

    int tmp = modelLength;
    while (tmp  > 0)
    {
        if (tmp  > BUF_SIZE)
        {
            uLen = fread(buf, 1, BUF_SIZE, fsrc);
        }
        else
        {
            uLen = fread(buf, 1, tmp, fsrc);
        }
        if (uLen <= 0)
        {
            break;
        }
        rc4_cona.rc4_encode((unsigned char*)buf, uLen);
        memcpy(modelBuf, buf, uLen);
        modelBuf += uLen;
        tmp -= uLen;
    }

    tmp = weightLength;
    while (tmp > 0)
    {
        if (tmp > BUF_SIZE)
        {
            uLen = fread(buf, 1, BUF_SIZE, fsrc);
        }
        else
        {
            uLen = fread(buf, 1, tmp, fsrc);
        }
        if (uLen <= 0)
        {
            break;
        }
        rc4_cona.rc4_encode((unsigned char*)buf, uLen);
        memcpy(weightBuf, buf, uLen);
        weightBuf += uLen;
        tmp -= uLen;
    }
    fclose(fsrc); 
    return true;
}

