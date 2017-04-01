#ifndef _PERMISSION_H
#define _PERMISSION_H

typedef struct{
	char request[24];			//请求状态	1不需要请求服务器进行验证， 0 需要请求服务器验证
	char smartlist[24];			//板子序列号
	char permission[24];		//板子工作权限
}HostList;

#define Repeat  			"yes"	//重复请求服务器
#define STOP_REQ  		"no"		//不需要请求服务器


#define OK_WORK  		"okay"		//正常工作
#define DISABLE  			"disable"	//不能工作，需要服务器验证
#define REBOOT_REQ  	"reboot"	//系统重启,以后都不会请求服务器验证

extern int CheckPermission(char *filename,HostList *host);
extern HostList *readPermission(void);

#endif
