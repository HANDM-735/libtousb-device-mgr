#ifndef  MGR_SHM__H_H
#define  MGR_SHM__H_H

#include "linux_shm.h"

#define SHM_KEY 	"shm.key"


#define ASIC_READ		0x01		//asic AD9528读操作命令字
#define ASIC_WRITE		0x02		//asic AD9528写操作命令字

#define DATA_INVALID	0x00		//数据无效
#define DATA_VALID		0x01		//数据有效

//读ASIC AD9528寄存器操作请求消息结构体
typedef struct {
	unsigned char   valid;			 //数据是否可读标识，0:不可读，1:可读 
	unsigned char 	asic_csid;	     //asic芯片片选id
	unsigned char  	cmd;		 	 //读写操作命令字
	unsigned short 	regaddr;		 //asic AD9528寄存器地址	
} asic_rd_req;

//读ASIC AD9528寄存器操作应答消息结构体
typedef struct {
	unsigned char   valid;			 //数据是否可读标识，0:不可读，1:可读 
	unsigned char 	asic_csid;		//asic芯片片选id
	unsigned short 	regaddr;		//asic AD9528寄存器地址
	unsigned char  	value;			//asic ad9528寄存器地址中读取的值
} asic_rd_resp;


//写ASIC AD9528寄存器操作请求消息结构体
typedef struct {
	unsigned char   valid;			 //数据是否可读标识，0:不可读，1:可读 
	unsigned char 	asic_csid;   	//asic芯片片选id
	unsigned char  	cmd;      		//读写操作命令字
	unsigned short 	regaddr;  		//asic AD9528寄存器地址	
	unsigned char  	value;			//写入asic ad9528寄存器地址中的值
} asic_wr_req;

//写ASIC AD9528寄存器操作应答消息结构体
typedef struct {
	unsigned char   valid;			 //数据是否可读标识，0:不可读，1:可读 
	unsigned int 	asic_csid;	   	//asic芯片片选id
	unsigned short 	regaddr;   		//asic AD9528寄存器地址
	unsigned int 	status;    		//写入asic AD9528寄存器地址中的值成功与否，0:成功 -1:失败
} asic_wr_resp;




typedef struct {
	asic_wr_req    	wr_req;
	asic_wr_resp 	wr_resp;
	asic_rd_req    	rd_req;
	asic_rd_resp 	rd_resp;	
} asic_msg;


int attach_shm(const char* keypath);
void init_asic_msg(asic_msg* ptr);
void detach_shm(int shmid);
asic_msg* get_asic_msg();
void print_asic_msg(asic_msg* ptr);


#endif
