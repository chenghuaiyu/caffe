#ifndef _VIM_DECRYPT_H_
#define _VIM_DECRYPT_H_

int VimDecrypt(const char* infile, char** out1, char** out2, int *len1, 
	int *len2, const char* encrypt, const char *key);
int VimDecrypt(const wchar_t* infile, char** out1, char** out2, int *len1,
	int *len2, const char* encrypt, const char *key);

#endif //_VIM_DECRYPT_H_