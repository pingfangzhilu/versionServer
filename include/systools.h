#ifndef _SYSTOOLS_H
#define _SYSTOOLS_H

// ͷ�ļ��ӿ����ڿ⺯��Ϊ:��libsystools463.so

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>

enum{ALL,USR,FREE};

extern int __safe_write(FILE *fp,const void *data,int input_size);
extern int GetStorageInfo(char * MountPoint,  //SD�����һ������
                                 int *Capacity,  //  ��Ҫ��ȡ�Ŀռ��С
                                 int type); //��ȡʲô���͵Ŀռ�

//���ϵͳ�����Ƿ����ĳ���������У� ���ڷ���-1�������ڷ���0
extern int judge_pid_exist(int get_pid_name(char *pid_name));

//���ַ���str����base64���� ,���ش�����
extern unsigned char *base64_encode(unsigned char *str,int inSize);
//���ַ���code����base64���룬���ش�����
extern unsigned char *base64_decode(unsigned char *code,int inSize);

/****************************************************************
����˵��	:д��Log  ��־
��ڲ���	:bLog  �����Ƿ�Ϊ��־�ļ�,place ����λ��
��������	:
�޸ļ�¼	:
��;����	:�������־�ļ���������strǰ���ϵ�ǰʱ��(��ʽ�磺2011-04-12 12:10:20)
****************************************************************/
extern int writeLog(const char *filename,const char *str);


/********************************************
���ܣ������ַ����ұ���ĵ�һ��ƥ���ַ�
********************************************/
extern char* Rstrchr(char* s, char x);
/********************************************
���ܣ����ַ���ת��ΪȫСд
********************************************/
extern void   ToLowerCase(char* s);

#ifdef __cplusplus
};
#endif
#endif

