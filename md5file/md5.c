#include <stdio.h>
#include <openssl/md5.h>

/*
生成文件的md5校验值 
filename :待校验的文件
md5Val:生成出的校验值
*/
int CreateFile_Md5Sum(const char *filename,char *md5Val){  
        MD5_CTX ctx;  
        int len = 0;  
        unsigned char buffer[1024] = {0};  
        unsigned char digest[16] = {0};  
          
        FILE *pFile = fopen (filename, "rb"); 
			if(pFile==NULL){
			return -1;
		}
        MD5_Init (&ctx);  
     	
        while ((len = fread (buffer, 1, 1024, pFile)) > 0)  
        {  
	    	
            MD5_Update (&ctx, buffer, len);  
        }  
      
        MD5_Final (digest, &ctx);  
          
        fclose(pFile);  
          
      
        int i = 0,pos=0;  
        char buf[33] = {0};  
        char tmp[3] = {0};  
		len =0;
        for(i = 0; i < 16; i++ ){  
            snprintf(tmp,3,"%02x", digest[i]); 
            len = snprintf(buf+pos,33-pos,"%s",tmp);
	    pos +=len;
	    //printf("buf = %s len =%d \n",buf,len);
        }  
        snprintf(md5Val,33,"%s",buf);
     	//printf("%s : md5 is %s\n",__func__,buf); 
        return 0;  
}  

#ifdef MAIN_TEST
int main(int argc,char **argv){
	if(argc<2){
		printf("input filename \n");	
		return -1;	
	}
	char md5Val[33]={0};
	File_Md5Sum((const char *)argv[1],md5Val); 
	printf("md5Val = %s \n",md5Val);
	return 0;
}
#endif
