#include "log.h"
FILE * logfile=NULL;
int logLevel = 16;
//16 = debug, 8 = info, 4 = warn, 2 = error, 1 = fatal
void logOpen(char *file){
	if(file==NULL)return;
	//.if(access(file,0)==1)logfile = fopen(file,"w+");
	//else logfile = fopen(file,"w");
	logfile = fopen(file,"ab");
	if(logfile == NULL)return ;
	setvbuf(logfile,NULL,_IONBF,0);
	
}
void logClose(){
	if(logfile!=NULL){
		fclose(logfile);
	}
}
void setLogLevel(int level){
	logLevel = level;
}

void logging(const char* tag, const char* message, ...) {
	//FILE *logfile = file ; 
	int level = 16;
    time_t timep; 
    struct tm *p; 
	va_list arg_ptr;
	char format[MAX_STRING_LEN] = "%d/%0.2d/%0.2d %0.2d:%0.2d:%0.2d [%s]\t";
	if(tag == NULL || message == NULL)return;
	if( strlen(message)> 2000)return;
	if(strcmp(tag,"DEBUG") ==0){
		level = 16;
	}
	else if(strcmp(tag,"INFO") ==0){
		level = 8;
	}
	else if(strcmp(tag,"WARN") ==0){
		level = 4;
	}
	else if(strcmp(tag,"ERROR") ==0){
		level = 2;
	}
	else if(strcmp(tag,"FATAL") ==0){
		level = 1;
	}
	if(logfile==NULL){
		return;
	}
    time(&timep); 
    p=localtime(&timep);
	sprintf(format,"%d/%.2d/%.2d %.2d:%.2d:%.2d [%s] ",(1900+p->tm_year),( 1+p->tm_mon), p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec,tag);
	va_start(arg_ptr, message);  
	strcpy(format + strlen(format), message);
	strcpy(format + strlen(format), "\n");
	//printf("%s", format);
    if(level<=logLevel){
		//fprintf(logfile,format,(1900+p->tm_year),( 1+p->tm_mon), p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec,tag, arg_ptr);
		vfprintf(logfile, format, arg_ptr);
	}
}
