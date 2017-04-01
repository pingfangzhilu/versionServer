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

//���͸��°汾״̬�������Ự����
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
//�������ؽ�����
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

//����json ���ݣ���ȡ�������汾����Ϣ����url��ַ
static int getNewVersionUrl(Version *v,char *recvdata,char *newUrl){
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
		sprintf(newUrl,"%s",url);
		v->newSize = cJSON_GetObjectItem(pItem, "size")->valueint;
		printf("v->newSize =%d\n",v->newSize);
		pSub = cJSON_GetObjectItem(pItem, "md5");
		if(pSub){
			snprintf(v->newImageMd5,33,"%s",pSub->valuestring);
			printf("md5 value =%s\n",v->newImageMd5);
		}
    }  
exit:
	cJSON_Delete(pJson);
	return 0;
}
//��ʼ�����ļ�
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
#ifdef SEND_DOWN_STATE		//�������ع̼��Ľ���״̬
	DFile->progress=0;
#endif
}
//��ȡ��������
static void versionGetStreamData(const char *data,int size){
	int ret =0,wSize=size;
	if(DFile->urlFp!=NULL){
		do{
			ret = fwrite( data, 1,wSize,DFile->urlFp);
			wSize -=ret;
		}while(wSize);	
	}	
	DFile->downSize +=size;
#ifdef SEND_DOWN_STATE		//�������ع̼��Ľ���״̬
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
#else
	DOWN_DBG("DFile->downSize =%d\n",DFile->downSize);
#endif
}
//��������
static void versionEndDownFile(int endsize){
	if(DFile->urlFp!=NULL){
		fclose(DFile->urlFp);
		DFile->urlFp=NULL;
	}
	//DOWN_DBG("versionEndDownFile =%d\n",DFile->downSize);
}

//��ȡ�������汾��Ϣ
static void getServerVersion(Version *v,const char *vserionUrl){
#if 0
	DOWN_FILE(vserionUrl,versionStartDownFile,versionGetStreamData,versionEndDownFile);
	if(DFile->urlFp!=NULL){
		fclose(DFile->urlFp);
		DFile->urlFp=NULL;
	}	
	//printf("v->versionName = %s\n",v->versionName);
	char *data =readFileBuf(DFile->filename);
#else
	char *data =readFileBuf("version.json");//����txt�汾����
#endif
	if(data==NULL){
		return ;
	}
	getNewVersionUrl(v,data,v->newUrl);
	free(data);	
	DOWN_DBG("newUrl = %s newSize = %d\n",v->newUrl,v->newSize);
	memset(DFile->filename,0,64);
	return ;
}

//�����ں˾���д��flash
static int updateKernelImage(char *ImageFile,int ImageSize,int newVersion){
	char cmd[256]={0};
	SendVersionState(IMAGE_JSON,START_UPIMAGE);
	snprintf(cmd,256,"%s%d%s%s%s","/bin/mtd_write -o 0 -l ",ImageSize," write ",ImageFile," Kernel");
	DOWN_DBG("cmd : %s\n",cmd);
	int status ;
	status= system(cmd);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0){
		printf("exit status = %d\n",status);
		return -1;
	}
	return 0;
}

//�����µ��ں˾���
static int DownNewVersion(Version *v){
	SendVersionState(VERSION_JSON,START_DOWNIMAGE);
#ifdef SEND_DOWN_STATE		//�������ع̼��Ľ���״̬
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
	if(DFile->downSize==v->newSize){
		DOWN_DBG("^_^ ^_^ ^_^ ^_^ ^_^ https down image ok ^_^ ^_^ ^_^\n");
		if(File_Md5Sum((const char *)DFile->filename,md5Val)){
			DOWN_DBG("get md5 val failed \n");
			goto exit0;
		}	
		if(!strcmp(md5Val,v->newImageMd5)){
			if(updateKernelImage(DFile->filename,v->newSize,v->newVersion)==0){
				DOWN_DBG("^_^ ^_^ ^_^ ^_^ ^_^ update image ok ^_^ ^_^ ^_^ \n");
				SendVersionState(IMAGE_JSON,END_UPIMAGE);
			}

		}
		else{
			SendVersionState(IMAGE_JSON,ERROR_UPIMAGE);
		}
	}else{
		DOWN_DBG("error :https down image less [%d] data  \n",v->newSize-DFile->downSize);
		SendVersionState(VERSION_JSON,ERROR_DOWNIMAGE);
	}
	return 0;
exit0:
	return -1;
}
//��ȡ·�ɱ����а汾��  (ÿ�θ��º��ϴ��汾������Ҫ�޸� )\
//vim vendors/Ralink/MT7628/RT2860_default_vlan --->Version=xxx
static int getCurrentVersion(int *Version,char *currentMd5,char *versionurl){
	char *data =readFileBuf(DEVICES_CURRENT_VERSION_JSON_FILE);//����txt�汾����
	if(data==NULL){
		return -1;
	}
	cJSON * pJson = cJSON_Parse(data);
	if(NULL == pJson){
		printf("cJSON_Parse failed \n");
		return -1;
	}
	cJSON * pSub = cJSON_GetObjectItem(pJson, "version");
	if(NULL == pSub){
		printf("get json data  failed\n");
		goto exit;
	}
	*Version = pSub->valueint;
	DOWN_DBG("version = %d\n", pSub->valueint);
	pSub = cJSON_GetObjectItem(pJson, "versionurl");
	if(pSub==NULL){
		goto exit;
	}
	snprintf(versionurl,128,"%s",pSub->valuestring);	
	pSub = cJSON_GetObjectItem(pJson, "md5");
	if(pSub==NULL){
		goto exit;
	}
	snprintf(currentMd5,33,"%s",pSub->valuestring);
	DOWN_DBG(" currentMd5 value =%s\n",currentMd5);
    	  
exit:
	free(data);
	cJSON_Delete(pJson);
	return 0;
}

static int startCheckVerionAndUpdate(void){
	char versionurl[128]={0}; 
	if(getCurrentVersion(&v->curVersion,v->newImageMd5,versionurl)){
		return -1;
	}
	printf("v->curVersion =%d v->newVersion=%d versionurl=%s\n",v->curVersion,v->newVersion,versionurl);
	return -1;
	getServerVersion(v,versionurl);
	if(v->curVersion<v->newVersion){	//��Ҫ����
		SendVersionState(VERSION_JSON,NEW_VERSION);
		DownNewVersion(v);
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
		if(!strcmp(host->request,STOP_REQ))	//����Ҫ��ȥ�����������
		{
			printf("system work ok	......\n");
			goto exit0; 
		}
	}else if(!strcmp(host->permission,DISABLE)){	//��������������ȥ���������
		printf(" disable: request server state   ......\n");
		system("reboot");
	}else if(!strcmp(host->permission,REBOOT_REQ)){	//��������������ֱ������
		printf(" reboot system  ......\n");
		system("reboot");
	}else{
		printf("unkown: request server state   ......\n");
	}

	while(1){ 		//�������
		if(checkNetwork(5) == 0){
			printf("network is ok \n");
			break;
		}
		sleep(1);
	}
#if 1
	DOWN_FILE(HOST_PERMISSION_URL,versionStartDownFile,versionGetStreamData,versionEndDownFile);
	//CheckPermission(DFile->filename,host);	//��Ȩ�޲���ʹ��
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

#if 0
static int savefd;
static void StartDownFile(const char *filename,int streamLen)
{
	savefd= open(filename, O_CREAT | O_WRONLY, S_IRWXG | S_IRWXO | S_IRWXU);    
	if (savefd < 0)    
	{        
		printf("Create file failed\n");   
	}  

}
static void GetStreamData(const char *data,int size)
{
	//printf("size = %d \n",size);
	write(savefd, data, size);
}
static void EndDownFile(int endSize)
{
	printf("endSize = %d \n",endSize);
	close(savefd);
}
#endif
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
		//char *url_4 = "https://raw.githubusercontent.com/daylightnework/MGW/master/root_uImage_new4300_v2";
		//char *url_4 = "http://fdfs.xmcdn.com/group7/M01/A3/8D/wKgDX1d2Rr6w3CegABHDHZzUiUs448.mp3";
		//DOWN_FILE(url_4 ,versionStartDownFile,versionGetStreamData,versionEndDownFile);
#ifdef HOST_PERMISSION
		HostPermission();
#endif
		startCheckVerionAndUpdate();
		cleanCurl();
	}
	else if(!strcmp(argv[1],"web")){	//web���淢������֪ͨ���¹̼�
		SendVersionState(IMAGE_JSON,END_DOWNIMAGE);
	}
	close(v->vsock);
	free(v);
	free(DFile);
	return 0;
}