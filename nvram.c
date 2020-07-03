#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <sys/file.h>
#include <stdarg.h>
#include "nvram.h"
#include <linux/compiler.h>
#ifdef WAG54GX
#include <linux/mtd/mtd.h>
#else
#include <mtd/mtd-user.h>
#endif

int readFileBin(char *path, char **data) {
	int total;
	int fd=0;
	if((fd=open(path, O_RDONLY)) < 0)
	       	return -1;
	
	lockf(fd,F_LOCK,0);
	
	total=lseek(fd,0,SEEK_END);
	lseek(fd,0,0);
	
	if((*data=malloc(total))==NULL){
		lockf(fd,F_ULOCK,0);
        close(fd);
        return -1;
	}
	if(read(fd,*data,total)<0){ 
		free(*data);
		lockf(fd,F_ULOCK,0);
        close(fd);
		return -1;
	}
	
	lockf(fd,F_ULOCK,0);
	close(fd);
   	return total;
}

void writeFileBin(char *path, char *data, int len) {
   int fd;

	if((fd=open(path, O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR)) < 0)
   		return;
	lockf(fd,F_LOCK,0);
	write(fd, data, len);
	lockf(fd,F_ULOCK,0);
	close(fd);
}
      
static unsigned long crc32(char *data, int length)
{
	unsigned long crc, poly;
	long crcTable[256];
	int i, j;
	poly = 0xEDB88320L;
	for (i=0; i<256; i++) {
		crc = i;
		for (j=8; j>0; j--) {
			if (crc&1) {
				crc = (crc >> 1) ^ poly;
			} else {
				crc >>= 1;
			}
		}
		crcTable[i] = crc;
	}
	crc = 0xFFFFFFFF;

	while( length-- > 0) {
		crc = ((crc>>8) & 0x00FFFFFF) ^ crcTable[ (crc^((char)*(data++))) & 0xFF ];
	}
	
	return crc^0xFFFFFFFF;
}

extern int nvram_load(){
	unsigned long crc;
	int fd1;
	char *data;
	
	nvram_header_t header;
		
	fd1=open(NVRAM_PATH,O_RDONLY);
	if(fd1<0)
		return NVRAM_FLASH_ERR;
		
	read(fd1, &header,sizeof(nvram_header_t));

	
	/*seek to header end*/
	lseek(fd1,NVRAM_HEADER_SIZE,0);

	if(header.magic!=NVRAM_MAGIC)
	{

		close(fd1);
		return NVRAM_MAGIC_ERR;
	}

	
	data=malloc(header.len+1);	
	read(fd1, data, header.len+1);		
	close(fd1);	
	
	crc=crc32(data, header.len);
	if(crc!=header.crc)
	{
#ifdef TEST 
		printf("CRC Error!!\n");
		printf("header.crc=%lx\tcrc=%lx\n",header.crc,crc);
#endif
		
		free(data);
		return NVRAM_CRC_ERR;
	}
		
	writeFileBin(NVRAM_TMP_PATH, data, header.len);
	writeFileBin("/tmp/nvram.mirror", data, header.len);
	
	free(data);

	return NVRAM_SUCCESS;
}

int mtd_erase(const char *mtd)
{
	int mtd_fd;
	mtd_info_t mtd_info;
	erase_info_t erase_info;
	
	/* Open MTD device */
	if ((mtd_fd = open(mtd, O_RDWR)) < 0) {
		return 1;
	}

	/* Get sector size */
	if (ioctl(mtd_fd, MEMGETINFO, &mtd_info) != 0) {
		close(mtd_fd);
		return 1;
	}

	erase_info.length = mtd_info.erasesize;

	for (erase_info.start = 0;
	     erase_info.start < mtd_info.size;
	     erase_info.start += mtd_info.erasesize) {
		if (ioctl(mtd_fd, MEMERASE, &erase_info) != 0) {
			close(mtd_fd);
			return 1;
		}
	}

	close(mtd_fd);
	return 0;
}

extern int nvram_commit(){
	int fd1;
	char *data;
	int len;

	nvram_header_t header;
#ifdef TEST	
	if(mtd_erase(NVRAM_PATH))
		return NVRAM_FLASH_ERR;
#endif
        //printf("open %s for read/write\n",NVRAM_PATH);
        
	if((fd1=open(NVRAM_PATH,O_WRONLY))<0)
		return NVRAM_FLASH_ERR;
	if((len=readFileBin(NVRAM_TMP_PATH, &data))<=0){
		close(fd1);
		return NVRAM_SHADOW_ERR;
	}
	
	header.magic=NVRAM_MAGIC;
	header.crc=crc32(data, len);
	header.len=len; 
	write(fd1, &header,sizeof(nvram_header_t));
	lseek(fd1,NVRAM_HEADER_SIZE,0);
	write(fd1, data, len);
	
// check data 
	lseek(fd1,NVRAM_HEADER_SIZE,0);
	read(fd1, data,len);
        //printf("check crc\n",NVRAM_PATH);
	if(header.crc!=crc32(data, len)){
            //printf("crc incorrect\n",NVRAM_PATH);
		close(fd1);
		free(data);
		return NVRAM_FLASH_ERR;
	}			
        //printf("crc correct! done\n",NVRAM_PATH);
	close(fd1);
	free(data);
	
	return NVRAM_SUCCESS;
}

extern int nvram_commit2(char *path){
	int fd1;
	char *data;
	int len;

	nvram_header_t header;
#ifdef TEST	
	if(mtd_erase(NVRAM_PATH))
		return NVRAM_FLASH_ERR;
#endif
    //printf("open %s for read/write\n",path);
        
	if((fd1=open(NVRAM_PATH,O_WRONLY))<0)
		return NVRAM_FLASH_ERR;
	if((len=readFileBin(path, &data))<=0){
		close(fd1);
		return NVRAM_SHADOW_ERR;
	}
	
	header.magic=NVRAM_MAGIC;
	header.crc=crc32(data, len);
	header.len=len; 
	write(fd1, &header,sizeof(nvram_header_t));
	lseek(fd1,NVRAM_HEADER_SIZE,0);
	write(fd1, data, len);
	
	// check data 
	lseek(fd1,NVRAM_HEADER_SIZE,0);
	read(fd1, data,len);

	if(header.crc!=crc32(data, len)){

		close(fd1);
		free(data);
		return NVRAM_FLASH_ERR;
	}			

	close(fd1);
	free(data);
	
	return NVRAM_SUCCESS;
}

/* restore show running-config to nvram */
/* added by xichen 110120 */
extern int nvram_running_commit(void){
	
	return NVRAM_SUCCESS;
}
/* end */

extern char* nvram_get_fun(const char *name,char *path)
{
	char *bufspace;
	int size;
	char *s,*sp;
	
	if((size=readFileBin(path, &bufspace))<0) 
		return NULL;

	for (s = bufspace; *s; s++) {
		if (!strncmp(s, name, strlen(name)) && *(s+strlen(name))=='=') {
			sp=malloc(strlen(s)-strlen(name));
			memcpy(sp,(s+strlen(name)+1),(strlen(s)-strlen(name)));
			free(bufspace);
			return sp;
		}
		while(*(++s));
	}
	free(bufspace);
	return NULL;
}
/*
 *   write for called by scfmgr_getall
 * */
extern char*  nvram_getall(char *data,int bytes)
{
	char *bufspace;
	int size;

	if((size=readFileBin(NVRAM_TMP_PATH, &bufspace))<0)
	    return (char *)NULL;
	if (size<bytes)
		bytes=size;
	memcpy(data,bufspace,bytes);

	free(bufspace);
	return data;
}

extern char* nvram_get(const char *name)
{	
	char *pt;
	if((pt=nvram_get_fun(name,NVRAM_TMP_PATH))==NULL){
			if((pt=nvram_get_fun(name,NVRAM_DEFAULT))!=NULL)
				nvram_set(name,pt);
			else
				return NULL;
	}
	return pt;
}

static char emp_str[]="";
extern char* nvram_safe_get(const char *name)
{
	char *pt;
	if((pt=nvram_get_fun(name,NVRAM_TMP_PATH)) == NULL){
			if((pt=nvram_get_fun(name,NVRAM_DEFAULT))!=NULL)
				nvram_set(name,pt);
			else
				pt=strdup(emp_str);
	}
	return pt;
}

extern char* nvram_safe_get_def(const char *name)
{
	char *pt;
	if((pt=nvram_get_fun(name,NVRAM_DEFAULT))==NULL)
		pt=strdup(emp_str);
	return pt;
}

char* nvram_safe_get_mirror(const char *name)
{
	char *pt;
	if((pt=nvram_get_fun(name, "/tmp/nvram.mirror"))==NULL)
		pt=strdup(emp_str);
		
	return pt;
}

extern int nvram_set(const char* name,const char* value)
{
	char *bufspace, *targetspace;
	int size;
	char *sp, *s;
	int found=0;
		
	if((size=readFileBin(NVRAM_TMP_PATH, &bufspace))>0) {
	    targetspace=malloc(size+strlen(name)+strlen(value)+2);
	}
	else {
	    targetspace=malloc(strlen(name)+strlen(value)+2);
	}

	sp=targetspace;
	if(size > 0) {
	   for (s = bufspace; *s; s++) {
		if (!strncmp(s, name, strlen(name)) && *(s+strlen(name))=='=') {
			found=1;
  			strcpy(sp, name);
			sp+=strlen(name);
        		*(sp++) = '=';
       			strcpy(sp, value);
			sp+=strlen(value);		
			while (*(++s));
		}
		while(*s) *(sp++)=*(s++);
	        *(sp++)=END_SYMBOL;
	    }
	
		free(bufspace);
	}
	if(!found){
		strcpy(sp, name);
		sp+=strlen(name);
        	*(sp++) = '=';
	        strcpy(sp, value);
		sp+=strlen(value);
	        *(sp++) = END_SYMBOL;
	}
        
	*(sp) = END_SYMBOL;

	writeFileBin(NVRAM_TMP_PATH, targetspace, (sp-targetspace)+1);
	writeFileBin("/tmp/nvram.mirror", targetspace, (sp-targetspace)+1);
	free(targetspace);
	
	return NVRAM_SUCCESS;
}


/*
 *  Function:  nvram_string_insert
 *  Purpose:  用于替换一段字符串中指定位置的数据
 *  Parameters:
 *     char *string_des 			目的字符串，即最终输出的字符串
 *	   const char *string_src		源字符串（字符串格式为XXX;XXX;XX;XXXXX;)后续如果需要可以修改该API,即传入用于切割的字符号st即可灵活的替换
 *	   char *data					插入的数据
 *	   int position					插入的位置
 *  Returns:
 *     0
 *  Author:   lxk
 *  Date:     2020/03/17
 */
extern int nvram_string_insert(char *string_des,const char *string_src, char *data, int position)
{	
	int count = 1;													//字符串定位
	char *p = NULL;	
	char st[2] = ";";												//用于切割的符号
	char src_tmp[128];		
	char des_tmp[128];	

	memset(src_tmp, '\0', sizeof(src_tmp));
	memset(des_tmp, '\0', sizeof(des_tmp));		

	sprintf(src_tmp, "%s", string_src);	
	
	p = strtok(src_tmp, st);										//进行切割
	while(NULL != p)	
	{			
		if(count == position)			
			strcat(des_tmp, data);		
		else 			
			strcat(des_tmp, p);		

		strcat(des_tmp, st);			

		p = strtok(NULL, ";");		
		count++;	
	}			
	
	strcpy(string_des, des_tmp);	
	
	return 0;
}

