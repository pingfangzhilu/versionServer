#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include<sys/types.h> 
#include<sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "nvram.h"
#include "cJSON.h"
#include "permission.h"
#include "systools.h"

static void ReadCacheList(HostList *host){
	char *list=nvram_bufget(RT2860_NVRAM, "smartlist");
	snprintf(host->smartlist,24,"%s",list);

	char *permission=nvram_bufget(RT2860_NVRAM, "permission");
	snprintf(host->permission,24,"%s",permission);

	char *request=nvram_bufget(RT2860_NVRAM, "request");
	snprintf(host->request,24,"%s",request);
}

static void WriteCacheList(char *permission,char *request){
	if(request==NULL){
		nvram_bufset(RT2860_NVRAM, "request","yes");
	}
	if(!strcmp(request,STOP_REQ)){
		nvram_bufset(RT2860_NVRAM, "request",STOP_REQ);
	}else if(!strcmp(request,Repeat)){
		nvram_bufset(RT2860_NVRAM, "request",Repeat);
	}else{
		nvram_bufset(RT2860_NVRAM, "request",Repeat);
	}
	nvram_bufset(RT2860_NVRAM, "permission",permission);
}

int CheckPermission(char *filename,HostList *host){
	char *data = readFileBuf(filename);
	if(data==NULL){
		return -1;
	}
	printf("data =%s\n",data);
	cJSON * pJson = cJSON_Parse(data);
	if(NULL == pJson)
	{
		printf("cJSON_Parse failed \n");
		return -1;
	}
	//printf("host->smartlist =%s\n",host->smartlist);
	cJSON * pArray =cJSON_GetObjectItem(pJson, host->smartlist);
	if(NULL == pArray)
	{
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
        char *permission = cJSON_GetObjectItem(pItem, "permission")->valuestring;  
        char *request = cJSON_GetObjectItem(pItem, "request")->valuestring;   
		//printf("request = %s Repeat =%s\n",request,Repeat);

		WriteCacheList(permission,request);
 	 	if(!strcmp(permission,OK_WORK)){			//板子正常工作
			printf("allow \n");
			goto exit;
		}else if(!strcmp(permission,DISABLE)){		//重启板子，每次开机都需要请求服务器，进行验证
			printf("disable system ... \n");
			system("reboot");
		}
		else if(!strcmp(permission,REBOOT_REQ)){	//重启板子，以后都不需要请求服务器验证
			printf("reboot system ... \n");
			system("reboot");
		}else{
			printf("unkown disable \n");
			//system("reboot");
		}
    }  

exit:
	cJSON_Delete(pJson);
	free(data);
	return 0;
}

HostList *readPermission(void){
	HostList *host = (HostList *)calloc(1,sizeof(HostList));
	if(host==NULL){
		return NULL;
	}
	ReadCacheList(host);
	return host;
}

#if 0
int main(int argc,char **argv)
{
	HostList host;
	memset(&host,0,sizeof(host));
	memcpy(host.smartlist,argv[2],strlen(argv[2]));
		
	CheckPermission(argv[1],&host);
	return 0;
}

//#else
#include <stdio.h>  
#include <stdlib.h>  
  
int main (int argc, const char * argv[])  
{  
    cJSON* pRoot = cJSON_CreateObject();  
    cJSON* pArray = cJSON_CreateArray();  
    cJSON_AddItemToObject(pRoot, "students_info", pArray);  
    char* szOut = cJSON_Print(pRoot);  
  
    cJSON* pItem = cJSON_CreateObject();  
    cJSON_AddStringToObject(pItem, "name", "chenzhongjing");  
    cJSON_AddStringToObject(pItem, "sex", "male");  
    cJSON_AddNumberToObject(pItem, "age", 28);  
    cJSON_AddItemToArray(pArray, pItem);  
  
    pItem = cJSON_CreateObject();  
    cJSON_AddStringToObject(pItem, "name", "fengxuan");  
    cJSON_AddStringToObject(pItem, "sex", "male");  
    cJSON_AddNumberToObject(pItem, "age", 24);  
    cJSON_AddItemToArray(pArray, pItem);  
  
    pItem = cJSON_CreateObject();  
    cJSON_AddStringToObject(pItem, "name", "tuhui");  
    cJSON_AddStringToObject(pItem, "sex", "male");  
    cJSON_AddNumberToObject(pItem, "age", 22);  
    cJSON_AddItemToArray(pArray, pItem);  
  
    char* szJSON = cJSON_Print(pRoot);  
    printf("%s\n",szJSON);  
    cJSON_Delete(pRoot);  
  
    pRoot = cJSON_Parse(szJSON);  
    pArray = cJSON_GetObjectItem(pRoot, "students_info");  
    if (NULL == pArray) {  
        return -1;  
    }  
  
    int iCount = cJSON_GetArraySize(pArray);  
    int i = 0;  
    for (; i < iCount; ++i) {  
        cJSON* pItem = cJSON_GetArrayItem(pArray, i);  
        if (NULL == pItem){  
            continue;  
        }  
        char *strName = cJSON_GetObjectItem(pItem, "name")->valuestring;  
        char *trSex = cJSON_GetObjectItem(pItem, "sex")->valuestring;  
        int iAge = cJSON_GetObjectItem(pItem, "age")->valueint;  
        printf("---name=%s\n", strName);  
        printf("---sex=%s\n", trSex);  
        printf("---age=%d\n", iAge);  
    }  
  
    cJSON_Delete(pRoot);  
    free(szJSON);  
    return 0;  
}  
#endif

