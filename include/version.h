#ifndef _VERSION_H
#define _VERSION_H

#include <stdio.h>

#define VERSION_JSON	"updateHost"
#define IMAGE_JSON		"updateImage"


#define NEW_VERSION		"newversion" 	//���°汾����Ҫ����
#define START_DOWNIMAGE	"start"			//�������ع̼�
#define ERROR_DOWNIMAGE	"error"			//���ع̼�����
#define END_DOWNIMAGE 	"end"			//���ؽ���
#define PROGRESS	 	"progress"		//���ع̼�����

#define START_UPIMAGE 	"start"			//��ʼ���¹̼�
#define ERROR_UPIMAGE 	"error"			//���¹̼�����
#define END_UPIMAGE 	"end"			//���¹̼�����


#define START_GET_HTTP_FILENAME_UNLOCK    0
#define START_GET_HTTP_FILENAME_LOCK	  1

//�汾�������ӵ�ַ
#define VERSION_HTTP_URL	"https://raw.githubusercontent.com/daylightnework/MGW/master/version.txt"
//�豸����Ȩ�޹������ӵ�ַ
#define HOST_PERMISSION_URL	"https://raw.githubusercontent.com/daylightnework/MGW/master/permission.txt"


#define DEVICES_CURRENT_VERSION_JSON_FILE	"/home/devicesVersion.json"		//���ӵĵ�ǰ�汾��Ϣ	(sdkĿ¼��romfs/)


typedef struct{
	unsigned char getHttpLock; 
#ifdef SEND_DOWN_STATE
	unsigned char enprogress;
	float  progress;
#endif
	unsigned int downSize;	
	char filename[64];
	FILE *urlFp;
}DownFile_t;

typedef struct{
	unsigned int newSize;
	int curVersion;
	int newVersion;
	char newImageMd5[33];
	char newUrl[256];	
	int vsock;
	struct sockaddr_in addr;
}Version;

#define DOWN_DBG(fmt, args...)	printf("%s: "fmt,__func__, ## args)

#endif
