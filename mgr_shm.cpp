#include "mgr_shm.h"
#include "xbasic.hpp"
#include <string.h>



static asic_msg* pASICMsg = NULL;

static void set_asic_msg(asic_msg* ptr)
{
	pASICMsg = ptr;
}


int attach_shm(const char* keypath)
{
	int chshm;
	char    cFileName [256];
	sprintf(cFileName,"%s/%s",keypath,SHM_KEY);
	 
	chshm = create_shm((char *)cFileName,sizeof(asic_msg));
	asic_msg* tmp = (asic_msg*)map_shm(chshm);
	if (tmp == NULL )
	{		
		return -1;
	}
	
	set_asic_msg(tmp);
	
	return chshm ;
}


void init_asic_msg(asic_msg* ptr)
{
	if(ptr == NULL) 
		return;
	
	ptr->rd_req.valid = 0;
	ptr->rd_req.asic_csid = 0;
	ptr->rd_req.cmd = 0;
	ptr->rd_req.regaddr = 0;

	ptr->wr_req.valid = 0;
	ptr->wr_req.asic_csid = 0;
	ptr->wr_req.cmd = 0;
	ptr->wr_req.regaddr = 0;
	ptr->wr_req.value = 0;

	ptr->rd_resp.valid = 0;
	ptr->rd_resp.asic_csid = 0;
	ptr->rd_resp.regaddr = 0;
	ptr->rd_resp.value = 0;

	ptr->wr_resp.valid = 0;
	ptr->wr_resp.asic_csid = 0;
	ptr->wr_resp.regaddr = 0;
	ptr->wr_resp.status = -1;
}


void detach_shm(int shmid)
{
	unmap_shm((char*)pASICMsg);
	close_shm(shmid);
}


asic_msg* get_asic_msg()
{
	return pASICMsg;
}

void print_asic_msg(asic_msg* ptr)
{
	printf("ptr->rd_req.valid=%d\n",ptr->rd_req.valid);
	printf("ptr->rd_req.asic_csid=%d\n",ptr->rd_req.asic_csid);
	printf("ptr->rd_req.cmd=%d\n",ptr->rd_req.cmd);
	printf("ptr->rd_req.regaddr=%d\n",ptr->rd_req.regaddr);

	printf("ptr->rd_resp.valid=%d\n",ptr->rd_resp.valid);
	printf("ptr->rd_resp.asic_csid=%d\n",ptr->rd_resp.asic_csid);
	printf("ptr->rd_resp.regaddr=%d\n",ptr->rd_resp.regaddr);
	printf("ptr->rd_resp.value=%d\n",ptr->rd_resp.value);

  printf("ptr->wr_req.valid=%d\n",ptr->wr_req.valid);
	printf("ptr->wr_req.asic_csid=%d\n",ptr->wr_req.asic_csid);
	printf("ptr->wr_req.cmd=%d\n",ptr->wr_req.cmd);
	printf("ptr->wr_req.regaddr=%d\n",ptr->wr_req.regaddr);
	printf("ptr->wr_req.value=%d\n",ptr->wr_req.value);

	printf("ptr->wr_resp.valid=%d\n",ptr->wr_resp.valid);
	printf("ptr->wr_resp.asic_csid=%d\n",ptr->wr_resp.asic_csid);
	printf("ptr->wr_resp.regaddr=%d\n",ptr->wr_resp.regaddr);
	printf("ptr->wr_resp.status=%d\n",ptr->wr_resp.status);
}
