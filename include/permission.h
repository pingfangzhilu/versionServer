#ifndef _PERMISSION_H
#define _PERMISSION_H

typedef struct{
	char request[24];			//����״̬	1����Ҫ���������������֤�� 0 ��Ҫ�����������֤
	char smartlist[24];			//�������к�
	char permission[24];		//���ӹ���Ȩ��
}HostList;

#define Repeat  			"yes"	//�ظ����������
#define STOP_REQ  		"no"		//����Ҫ���������


#define OK_WORK  		"okay"		//��������
#define DISABLE  			"disable"	//���ܹ�������Ҫ��������֤
#define REBOOT_REQ  	"reboot"	//ϵͳ����,�Ժ󶼲��������������֤

extern int CheckPermission(char *filename,HostList *host);
extern HostList *readPermission(void);

#endif
