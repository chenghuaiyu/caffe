#ifndef __RC4_HPP__
#define __RC4_HPP__

typedef struct rc4_key
{      
    unsigned char state[256];       
    unsigned char x;        
    unsigned char y;
}rc4_key;

class rc4
{
public:
    rc4(unsigned char *key_data_ptr,int nLen);//类初始化时接受字串，初始化key
    void rc4_encode(unsigned char *buffer_ptr, int buffer_len);//明文与暗文使用同一个函数转换

private:
    rc4_key key;//加密与解密用到的key，初始化时就需要赋值
    void prepare_key(unsigned char *key_data_ptr, int key_data_len);//初始化key
    void swap_byte(unsigned char *a, unsigned char *b);//交换

};

bool VimDecrypt2BufRc4(const char* infile, const char *pwd, char** out1, 
                            char** out2, int *len1, int *len2);

#endif  // __RC4_HPP__

