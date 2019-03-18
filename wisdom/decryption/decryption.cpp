#include <stdio.h>
#include <string.h>
#include "rc4.hpp"
#include "../common.h"

int VimDecrypt(const char* infile, char** out1, char** out2, int *len1, 
                int *len2, const char* encrypt, const char *key) {
    if (!strcmp(encrypt,"none")) {
        return SUCCESS;
    } else if (!strcmp(encrypt,"rc4")) {
        if (!VimDecrypt2BufRc4(infile, key, out1, out2, len1, len2)) {
            return FAIL;
        }
    } else {
        printf("Not Support Encry Type:%s", encrypt);
		return FAIL;
    }

	return SUCCESS;
}

