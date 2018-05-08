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
    rc4(unsigned char *key_data_ptr,int nLen);//���ʼ��ʱ�����ִ�����ʼ��key
    void rc4_encode(unsigned char *buffer_ptr, int buffer_len);//�����밵��ʹ��ͬһ������ת��

private:
    rc4_key key;//����������õ���key����ʼ��ʱ����Ҫ��ֵ
    void prepare_key(unsigned char *key_data_ptr, int key_data_len);//��ʼ��key
    void swap_byte(unsigned char *a, unsigned char *b);//����

};

bool VimDecrypt2BufRc4(const char* infile, const char *pwd, char** out1, 
                            char** out2, int *len1, int *len2);

#endif  // __RC4_HPP__

