#ifndef __ENCRYPT_HPP__
#define __ENCRYPT_HPP__

void VimEncrypt2FileRc4(char* infile1, char* infile2, char *outfile, char*pwd);
void VimDecrypt2FileRc4(char* infile, char *outfile1, char* outfile2 ,char*pwd);

#endif  // __RC4_HPP__

