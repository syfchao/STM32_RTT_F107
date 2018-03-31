#ifndef __VERSION_H__
#define	__VERSION_H__

#define ECU_VERSION "ECU_R"		//ECU_R版本号前缀
#define ECU_EMA_VERSION		"R"	//上报EMA的版本标识符
#define MAJORVERSION  "1"			//主版本号
#define MINORVERSION	"2.1"		//次版本号.小版本号

#define INTERNAL_TEST_VERSION		0	//内部版本号，在内部测试过程中使用

#define ECU_VERSION_LENGTH (strlen(ECU_VERSION)+strlen(MAJORVERSION)+strlen(MINORVERSION) + 2)
#endif /*__VERSION_H__*/
