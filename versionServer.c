#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#include "cJSON.h"
#include "version.h"

#ifdef MIPS
#include "nvram.h"
#endif
#ifdef HOST_PERMISSION
#include "permission.h"
#endif

static DownFile_t *DFile=NULL;
static Version *v;
//#define WGET_DOWN
static void DownhttpFile(char *url,void StartDownFile(const char *filename,int streamLen),void GetStreamData(const char *data,int size),void EndDownFile(int endSize));
#define DOWN_FILE		DownhttpFile

char *readFileBuf(char *filename);

//发送更新版本状态给语音会话进程
static int SendVersionState(char *handler,char *state){
	int ssize =-1;

	char* szJSON = NULL;
	cJSON* pItem = NULL;
	pItem = cJSON_CreateObject();
	cJSON_AddStringToObject(pItem, "handler", handler);
	cJSON_AddStringToObject(pItem, "status",state);
	szJSON = cJSON_Print(pItem);
	ssize = sendto(v->vsock,szJSON,strlen(szJSON),0,(const struct sockaddr *)&v->addr,sizeof(struct sockaddr_in));
	return ssize;
}
//发送下载进度条
static int SendVersionProguess(char *handler,int value){
	int ssize =-1;
	char* szJSON = NULL;
	cJSON* pItem = NULL;
	pItem = cJSON_CreateObject();
	cJSON_AddStringToObject(pItem, "handler", handler);
	cJSON_AddStringToObject(pItem, "status",PROGRESS);
	cJSON_AddNumberToObject(pItem, "value",value);
	szJSON = cJSON_Print(pItem);
	ssize = sendto(v->vsock,szJSON,strlen(szJSON),0,(const struct sockaddr *)&v->addr,sizeof(struct sockaddr_in));
	return ssize;
}

//解析服务器上版本json 数据，提取服务器版本号信息、和固件url地址
static int getNewVersionUrl(Version *v,char *recvdata){
	cJSON * pJson = cJSON_Parse(recvdata);
	if(NULL == pJson){
		printf("cJSON_Parse failed \n");
		return -1;
	}
	//printf("handler_CtrlMsg : handler_CtrlMsg = %s\n",recvdata);
	cJSON * pSub = cJSON_GetObjectItem(pJson, "version");
	if(NULL == pSub){
		printf("get json data  failed\n");
		goto exit;
	}
	v->newVersion =pSub->valueint;
	char item[16]={0};
	printf("getNewVersionUrl : version = %d\n", pSub->valueint);
	snprintf(item,16,"%s%d","v",v->newVersion);
	cJSON * pArray =cJSON_GetObjectItem(pJson, item);
	if(NULL == pArray){
		printf("get json data  failed\n");
		goto exit;
	}
	int iCount = cJSON_GetArraySize(pArray); 
	int i = 0;	
    for (i=0; i < iCount; ++i) {  
       	cJSON* pItem = cJSON_GetArrayItem(pArray, i);  
       	if (NULL == pItem){  
       		continue;  
       	}  
    	char *url = cJSON_GetObjectItem(pItem, "url")->valuestring;  
		snprintf(v->newUrl,256,"%s",url);
		v->newSize = cJSON_GetObjectItem(pItem, "size")->valueint;
		printf("v->newSize =%d\n",v->newSize);
		pSub = cJSON_GetObjectItem(pItem, "md5");
		if(pSub){
			snprintf(v->newImageMd5,33,"%s",pSub->valuestring);	//提取最新版本信息的md5值
			printf("newversion md5 value =%s\n",v->newImageMd5);
		}
    }  
exit:
	cJSON_Delete(pJson);
	return 0;
}
//开始下载文件
static void versionStartDownFile(const char *filename,int streamLen){
	printf("versionStartDownFile %s len [%d]\n",filename,streamLen);
	if(DFile->getHttpLock ==START_GET_HTTP_FILENAME_LOCK){
		return ;
	}
	DFile->getHttpLock = START_GET_HTTP_FILENAME_LOCK;
	if(access(filename,F_OK)==0){
		remove(filename);
	}
	memset(DFile->filename,0,sizeof(DFile->filename));
	snprintf(DFile->filename,64,"%s",filename);
	if ((DFile->urlFp=fopen(filename, "w+"))==NULL){        
		perror("Create file failed");   
	}  
	DFile->downSize =0;
#ifdef SEND_DOWN_STATE		//发送下载固件的进度状态
	DFile->progress=0;
#endif
}
//获取到流数据
static void versionGetStreamData(const char *data,int size){
	int ret =0,wSize=size;
	if(DFile->urlFp!=NULL){
		do{
			ret = fwrite( data, 1,wSize,DFile->urlFp);
			wSize -=ret;
		}while(wSize);	
	}	
	DFile->downSize +=size;
#ifdef SEND_DOWN_STATE		//发送下载固件的进度状态
	if(DFile->enprogress==1){
		float progess=0.0;
		progressBar(DFile->downSize, v->newSize,&progess);
		if(progess>25.0&&progess<30.0&&DFile->progress==0){
			DFile->progress=25.0;
			DOWN_DBG("progress =%.2f\n",DFile->progress);
			SendVersionProguess(VERSION_JSON,25);
		}else if(progess>45.0&&progess<55.0&&DFile->progress==25.0){
			DFile->progress=50.0;
			DOWN_DBG("progress =%.2f\n",DFile->progress);
			SendVersionProguess(VERSION_JSON,50);
		}else if(progess>70.0&&progess<80.0&&DFile->progress==50.0){
			DFile->progress=75.0;
			DOWN_DBG("progress =%.2f\n",DFile->progress);
			SendVersionProguess(VERSION_JSON,75);
		}
	}
#endif
	DOWN_DBG("DFile->downSize =%d\n",DFile->downSize);

}
//结束下载
static void versionEndDownFile(int endsize){
	if(DFile->urlFp!=NULL){
		fclose(DFile->urlFp);
		DFile->urlFp=NULL;
	}
	//DOWN_DBG("versionEndDownFile =%d\n",DFile->downSize);
}

//获取服务器固件版本信息澹(包含最新版本号、固件下载地址、固件大小、固件md5校验值)
static int getServerVersionMessage(Version *v,const char *vserionUrl){
#if 0
	//下载配置文件
	DOWN_FILE(vserionUrl,versionStartDownFile,versionGetStreamData,versionEndDownFile);
	if(DFile->urlFp!=NULL){
		fclose(DFile->urlFp);
		DFile->urlFp=NULL;
	}	
	//printf("v->versionName = %s\n",v->versionName);
	char *data =readFileBuf(DFile->filename);	//下载配置文件
#else
	char *data =readFileBuf("version.json");//本地txt版本测试
#endif
	if(data==NULL){
		return -1;
	}
	getNewVersionUrl(v,data);
	free(data);	
	DOWN_DBG("newUrl = %s newSize = %d\n",v->newUrl,v->newSize);
	memset(DFile->filename,0,64);
	return 0;
}

//更新内核镜像，写入flash
static int updateKernelImage(char *ImageFile,int ImageSize,int newVersion){
	char cmd[256]={0};
	SendVersionState(IMAGE_JSON,START_UPIMAGE);
	snprintf(cmd,256,"%s%d%s%s%s","/bin/mtd_write -o 0 -l ",ImageSize," write ",ImageFile," Kernel");
	DOWN_DBG("cmd : %s\n",cmd);
	int status ;
	status= system(cmd);
	//升级成功
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0){
		printf("exit status = %d\n",status);
		return -1;
	}
	return 0;
}

//下载新的内核镜像
static int DownNewImageVersion(Version *v){
	SendVersionState(VERSION_JSON,START_DOWNIMAGE);
#ifdef SEND_DOWN_STATE		//发送下载固件的进度状态
	DFile->enprogress=1;
#endif
	DOWN_DBG("start down image url = %s\n",v->newUrl);
#if 1
	DOWN_FILE(v->newUrl,versionStartDownFile,versionGetStreamData,versionEndDownFile);
#else
	char *url_4 = "https://raw.githubusercontent.com/daylightnework/MGW/master/root_uImage_new4300_v2";
	DOWN_FILE(url_4,versionStartDownFile,versionGetStreamData,versionEndDownFile);
#endif	
	char md5Val[33]={0};
	if(DFile->downSize==v->newSize){	//下载文件大小一样
		DOWN_DBG("\n^_^ ^_^ ^_^ ^_^ ^_^ ^_^ ^_^ ^_^ ^_^ ^_^\nhttps down image ok ^_^ ^_^ ^_^ ^_^ ^_^^_^ ^_^ ^_^\n");
		//检查md5值，确定文件唯一性
		if(CreateFile_Md5Sum((const char *)DFile->filename,md5Val)){
			DOWN_DBG("get md5 val failed \n");
			goto exit0;
		}	
		if(!strcmp(md5Val,v->newImageMd5)){		//判断下载文件的md5值和服务器上的md5值是不是一样
			DOWN_DBG("..............check md5 ok ............\n");
			if(updateKernelImage(DFile->filename,v->newSize,v->newVersion)==0){
				DOWN_DBG("^_^ ^_^ ^_^ ^_^ ^_^ ^_^ ^_^ ^_^ ^_^ ^_^\n update image ok \n ^_^ ^_^ ^_^ ^_^ ^_^^_^ ^_^ ^_^ \n");
				SendVersionState(IMAGE_JSON,END_UPIMAGE);
			}else{
				DOWN_DBG("error :check md5  failed .........\n");
				SendVersionState(IMAGE_JSON,ERROR_UPIMAGE);
			}
		}
	
	}else{	//下载错误
		DOWN_DBG("error :https down image less [%d] data  \n",v->newSize-DFile->downSize);
		SendVersionState(VERSION_JSON,ERROR_DOWNIMAGE);
	}
	return 0;
exit0:
	return -1;
}
//获取当前板子正在运行的版本信息(每次更新和上传版本，都需要修改 )\
//Version 当前板子版本信息  versionurl:服务器版本下载地址信息(包含最新版本号、固件下载地址、固件大小、固件md5校验值)
static int getCurrentVersion(int *Version,char *versionurl){
	char *data =readFileBuf(DEVICES_CURRENT_VERSION_JSON_FILE);//板子的当前版本信息
	if(data==NULL){
		return -1;
	}
	cJSON * pJson = cJSON_Parse(data);
	if(NULL == pJson){
		printf("cJSON_Parse failed \n");
		return -1;
	}
	cJSON * pSub = cJSON_GetObjectItem(pJson, "version");	//当前板子的版本号
	if(NULL == pSub){
		printf("get json data  failed\n");
		goto exit;
	}
	*Version = pSub->valueint;
	DOWN_DBG("version = %d\n", pSub->valueint);
	pSub = cJSON_GetObjectItem(pJson, "versionurl");		//待升级版本信息配置文件下载地址 (服务器上的地址)
	if(pSub==NULL){
		goto exit;
	}
	snprintf(versionurl,128,"%s",pSub->valuestring);
exit:
	free(data);
	cJSON_Delete(pJson);
	return 0;
}
//开始检查版本信息，并升级
static int startCheckVerionAndUpdate(void){
	char versionurl[128]={0}; 
	if(getCurrentVersion(&v->curVersion,versionurl)){
		printf("get current version message failed \n");
		return -1;
	}
	printf("v->curVersion =%d versionurl=%s\n",v->curVersion,versionurl);
	if(getServerVersionMessage(v,versionurl)){
		printf("get server version message failed \n");
		return -1;
	}
	if(v->curVersion<v->newVersion){	//检查版本是否需要更新
		SendVersionState(VERSION_JSON,NEW_VERSION);	//发送更新指令到localserver 进程，语音播放开始下载
		DownNewImageVersion(v);
	}else{
		printf("^_^ ^_^ ^_^do not update image ^_^ ^_^ ^_^\n");
		remove(DFile->filename);
		//SendVersionState(VERSION_JSON,NEW_VERSION);
		//DownNewVersion(v);

	}
	return 0;
}

#ifdef HOST_PERMISSION
static void HostPermission(void){
	HostList *host = readPermission();
	if(host==NULL){
		return -1;
	}
	if(!strcmp(host->permission,OK_WORK)){
		if(!strcmp(host->request,STOP_REQ))	//不需要再去请求服务器了
		{
			printf("system work ok	......\n");
			goto exit0; 
		}
	}else if(!strcmp(host->permission,DISABLE)){	//不能正常工作，去请求服务器
		printf(" disable: request server state   ......\n");
		system("reboot");
	}else if(!strcmp(host->permission,REBOOT_REQ)){	//不能正常工作，直接重启
		printf(" reboot system  ......\n");
		system("reboot");
	}else{
		printf("unkown: request server state   ......\n");
	}

	while(1){ 		//检查网络
		if(checkNetwork(5) == 0){
			printf("network is ok \n");
			break;
		}
		sleep(1);
	}
#if 1
	DOWN_FILE(HOST_PERMISSION_URL,versionStartDownFile,versionGetStreamData,versionEndDownFile);
	//CheckPermission(DFile->filename,host);	//有权限才能使用
#else
	CheckPermission("permission.txt",host);
#endif
exit0:
	free(host);
}
#endif
static void DownhttpFile(char *url,void StartDownFile(const char *filename,int streamLen),void GetStreamData(const char *data,int size),void EndDownFile(int endSize)){
	setDowning();
	demoDownFile(url ,60,versionStartDownFile,versionGetStreamData,versionEndDownFile);
}
int main(int argc,char **argv){
	if(access("/var/versionServer.lock",0) < 0){
		fopen("/var/versionServer.lock","w+");
	}else{
		printf("please delete /var/versionServer.lock \n");
		exit(1);
	}
	if(argc<2){
		exit(1);
	}
	
	v= (Version *)calloc(1,sizeof(Version));
	if(v==NULL){
		return -1;
	}	
	DFile= (DownFile_t *)calloc(1,sizeof(DownFile_t));
	if(DFile==NULL){
		return -1;
	}	
	if(!strcmp(argv[1],"http")){
		v->vsock= create_listen_udp(NULL,20002);
		DFile->getHttpLock = START_GET_HTTP_FILENAME_UNLOCK;
	}
	else if(!strcmp(argv[1],"web"))
		v->vsock= create_listen_udp(NULL,20005);
	
	if(v->vsock<=0){
		perror("create udp socket failed ");
		return -1;
	}

	char IP[20]={0};
	if(GetNetworkcardIp("br0",IP)) {
		perror("get br0 ip failed");
		//return -1;
	}
	printf("IP = %s\n",IP);
	init_addr(&v->addr, IP,  20001);
	if(!strcmp(argv[1],"http")){
		initCurl();
#ifdef HOST_PERMISSION
		HostPermission();
#endif
		startCheckVerionAndUpdate();
		cleanCurl();
	}
	else if(!strcmp(argv[1],"web")){	//web界面发送语音通知更新固件
		SendVersionState(IMAGE_JSON,END_DOWNIMAGE);
	}
	close(v->vsock);
	free(v);
	free(DFile);
	return 0;
}
