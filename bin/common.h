#ifndef COMMON_H
#define COMMON_H

/*Log file*/
extern char *LOG_FILE;
extern int LOG_LEVEL;

extern int MAX_EVENTS;//epoll max events
extern int MSG_BUF_LENGTH;//socket buffer length
extern int SOCKET_PORT;  //receiver socket port number

/*Common DB Specification*/
extern char * DB_IP;
extern int DB_PORT;
extern char *DB_USER_NAME;
extern char *DB_PASSWD;
extern char *DB_NAME;
extern int DB_MAX_ERROR_COUNT;

/*Backup DB Specification*/
extern char * BACKUP_DB_IP;
extern int  BACKUP_DB_PORT;
extern char * BACKUP_DB_NAME;
extern char * BACKUP_DB_USER;
extern char * BACKUP_DB_PASSWD;
extern int BACKUP_MAX_ERROR_COUNT;

/*Specification for alert module*/
extern char * ALERT_IP;
extern int  ALERT_PORT;

/*Specification for master module*/
extern char* MASTER_IP;
extern int MASTER_PORT;

/*The number of threads for dp*/
extern int THREAD_NUM;

/*Message id in headers for communication*/
extern int MSG_ID_IPMI_MAPPING;
extern int MSG_ID_SNMP_DATA;
extern int MSG_ID_IPMI_DATA;

extern int IR_CONTROLLER_THRESH;
extern int IR_PD_THRESH;
extern int IR_VD_THRESH;
extern int N_CONTROLLER_THRESH;
extern int N_PD_THRESH;
extern int N_VD_THRESH;
extern int HP_CONTROLLER_THRESH;
extern int HP_PD_THRESH;
extern int HP_VD_THRESH;
extern int SERVER_PERFORM_THRESH;
extern int DISK_IO_THRESH;
extern int NETWORK_IO_THRESH;
extern int SSD_THRESH;
extern int IPMI_RAW_THRESH;
extern int LOG_THRESH;

#endif

