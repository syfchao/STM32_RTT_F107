#ifndef __VERSION_H__
#define	__VERSION_H__

#define ECU_VERSION "ECU_R"		//ECU_R�汾��ǰ׺
#define ECU_EMA_VERSION		"R"	//�ϱ�EMA�İ汾��ʶ��
#define MAJORVERSION  "1"			//���汾��
#define MINORVERSION	"2.3"		//�ΰ汾��.С�汾��

#define INTERNAL_TEST_VERSION		0	//�ڲ��汾�ţ����ڲ����Թ�����ʹ��

#define ECU_VERSION_LENGTH (strlen(ECU_VERSION)+strlen(MAJORVERSION)+strlen(MINORVERSION) + 2)
#endif /*__VERSION_H__*/
