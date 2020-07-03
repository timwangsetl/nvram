#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "nvram.h"

extern int readFileBin(char *path, char **data);
extern void writeFileBin(char *path, char *data, int len);

int nvram_unset(const char* name)
{
	char *bufspace, *targetspace;
	int size;
	char *sp, *s;
	int found=0;

	if((size=readFileBin(NVRAM_TMP_PATH, &bufspace))>0)
	    targetspace=malloc(size);
	else
		return NVRAM_SUCCESS;
		
	sp=targetspace;
	if(size > 0) {
	   for (s = bufspace; *s; s++) {
		if (!strncmp(s, name, strlen(name)) && *(s+strlen(name))=='=') {
			found=1;
			while (*(++s));
		}
		else{
			while(*s) *(sp++)=*(s++);
	        	*(sp++)=END_SYMBOL;
		}
	    }
	
		free(bufspace);
	}
	if(!found){
		free(targetspace);
		return NVRAM_SUCCESS;
	}
	
	*(sp) = END_SYMBOL;
	
	writeFileBin(NVRAM_TMP_PATH, targetspace, (sp-targetspace)+1);
	
	free(targetspace);
	return NVRAM_SUCCESS;
	
}

void nvram_show()
{
	char *bufspace;
	int size;
	char *s,*pt;
	
	if((size=readFileBin(NVRAM_TMP_PATH, &bufspace))<0) {
	    printf("can't find nvram!!\n");
	    exit(1);
	}
	
	if(size > 0) {
	   for (s = bufspace; *s; s++) {
	   	for(pt=strchr(s,0x01);pt;pt=strchr(pt+1,0x01)) *pt=0x20;
		printf("%s\n",s);
		while(*(++s));
	   }
	
		free(bufspace);
	}
	
}
void usage()
{
	puts("\nUsage: nvram [show|get|set|unset|add|init|commit] [name=value]\n");
	exit(0);
}

int main(int argc,char *argv[]){

	char *name;
	char *value;
	
	//printf("[%s:%d] %s %s!!\n", __FUNCTION__ ,__LINE__, argv[0], argv[1]);

	if (argv[1]==NULL)
		usage();
	else if(!strcmp(argv[1],"show"))
		nvram_show();
	
	else if(!strcmp(argv[1],"set")){
		name=strtok(argv[2],"=");
		value=(strtok(NULL,"")?:"");
		if(!name) usage();
		nvram_set(name,value);
		nvram_commit();
		
	}else if(!strcmp(argv[1],"add")){
		char tmp[4096];
		name=strtok(argv[2],"=");
		value=strtok(NULL,"");
			
		if(!name || !value) usage();
		sprintf(tmp,"%s%c%s",nvram_get(name),0x01,value);
		nvram_set(name,tmp);
		nvram_commit();
		
	}else if(!strcmp(argv[1],"unset")){
		name=argv[2];
		if(!name) usage();
		nvram_unset(name);
		nvram_commit();
			
	}else if(!strcmp(argv[1],"get")){
		name=argv[2];
		if(!name) usage();
		printf("name=%s\n",nvram_get(name)?:"");
	}else if(!strcmp(argv[1],"init"))
	{
			printf("[%s:%d] %s %s!!\n", __FUNCTION__ ,__LINE__, argv[0], argv[1]);	    
			nvram_load();
    }
	else if(!strcmp(argv[1],"commit"))
			nvram_commit();
	return 0;
}
