#ifndef _VERSION_H
#define _VERSION_H

#include <stdio.h>

#define VERSION_JSON	"updateHost"
#define IMAGE_JSON		"updateImage"


#define NEW_VERSION		"newversion" 	//有新版本，需要更新
#define START_DOWNIMAGE	"start"			//正在下载固件
#define ERROR_DOWNIMAGE	"error"			//下载固件错误
#define END_DOWNIMAGE 	"end"			//下载结束
#define PROGRESS	 	"progress"		//下载固件进度

#define START_UPIMAGE 	"start"			//开始更新固件
#define ERROR_UPIMAGE 	"error"			//更新固件错误
#define END_UPIMAGE 	"end"			//更新固件结束


#define START_GET_HTTP_FILENAME_UNLOCK    0
#define START_GET_HTTP_FILENAME_LOCK	  1

//版本管理链接地址
#define VERSION_HTTP_URL	"https://raw.githubusercontent.com/daylightnework/MGW/master/version.txt"
//设备开机权限管理链接地址
#define HOST_PERMISSION_URL	"https://raw.githubusercontent.com/daylightnework/MGW/master/permission.txt"


#define DEVICES_CURRENT_VERSION_JSON_FILE	"/home/devicesVersion.json"		//板子的当前版本信息	(sdk目录下romfs/)


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
