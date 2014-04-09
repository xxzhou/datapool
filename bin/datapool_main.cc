#include <sys/types.h>
#include<mysql/mysql.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include "config.h"
#include "log.h"
#include "AsyncDataListener.h"
#include"common.h"

char *LOG_FILE;
int LOG_LEVEL;

int MAX_EVENTS;
int MSG_BUF_LENGTH;
int SOCKET_PORT;  //receiver socket port number

/*Common DB Specification*/
char * DB_IP;
int DB_PORT;
char *DB_USER_NAME;
char *DB_PASSWD;
char *DB_NAME;
int DB_MAX_ERROR_COUNT;

/*Backup DB Specification*/
char * BACKUP_DB_IP;
int  BACKUP_DB_PORT;
char * BACKUP_DB_NAME;
char * BACKUP_DB_USER;
char * BACKUP_DB_PASSWD;
int BACKUP_MAX_ERROR_COUNT;

char * ALERT_IP;
int  ALERT_PORT;

char* MASTER_IP;
int MASTER_PORT;

int THREAD_NUM;

int MSG_ID_IPMI_MAPPING;
int MSG_ID_SNMP_DATA;
int MSG_ID_IPMI_DATA;

int IR_CONTROLLER_THRESH;
int IR_PD_THRESH;
int IR_VD_THRESH;
int N_CONTROLLER_THRESH;
int N_PD_THRESH;
int N_VD_THRESH;
int HP_CONTROLLER_THRESH;
int HP_PD_THRESH;
int HP_VD_THRESH;
int SERVER_PERFORM_THRESH;
int DISK_IO_THRESH;
int NETWORK_IO_THRESH;
int SSD_THRESH;
int IPMI_RAW_THRESH;
int LOG_THRESH;
int FRU_THRESH;
int STORAGE_THRESH;

/*function to load the config file, and store the values in global variable*/
static int load_config(const char * CONFIG_FILE)
{
    static struct cfg_line	cfg[] =
    {
        /* PARAMETER,			VAR,					TYPE,			MANDATORY,	MIN,			MAX */
        {"LOG_FILE", &LOG_FILE, TYPE_STRING, PARM_MAND, 0, 0},
        {"MAX_EVENTS", &MAX_EVENTS, TYPE_INT, PARM_MAND, 1, 10000},
        {"MSG_BUF_LENGTH", &MSG_BUF_LENGTH, TYPE_INT, PARM_MAND, 1, 655350},
        {"LOG_LEVEL", &LOG_LEVEL, TYPE_INT, PARM_MAND, 0, 20},
        {"SOCKET_PORT", &SOCKET_PORT, TYPE_INT, PARM_MAND, 0, 65535},
        {"DB_IP", &DB_IP, TYPE_STRING, PARM_MAND, 0, 0},
        {"DB_PORT", &DB_PORT, TYPE_INT, PARM_MAND, 1, 65535},
        {"DB_NAME", &DB_NAME, TYPE_STRING, PARM_MAND, 0, 0},
        {"DB_USER_NAME", &DB_USER_NAME, TYPE_STRING, PARM_MAND, 0, 0},
        {"DB_PASSWORD",	&DB_PASSWD, TYPE_STRING, PARM_MAND, 0, 0},
        {"DB_MAX_ERROR_COUNT", &DB_MAX_ERROR_COUNT, TYPE_INT, PARM_MAND, 0, 65535},
        {"BACKUP_DB_IP", &BACKUP_DB_IP, TYPE_STRING, PARM_MAND, 0, 0},
        {"BACKUP_DB_PORT", &BACKUP_DB_PORT, TYPE_INT, PARM_MAND, 0, 65535},
        {"BACKUP_DB_NAME", &BACKUP_DB_NAME, TYPE_STRING, PARM_MAND, 0, 0},
        {"BACKUP_DB_USER", &BACKUP_DB_USER, TYPE_STRING, PARM_MAND, 0, 0},
        {"BACKUP_DB_PASSWD", &BACKUP_DB_PASSWD, TYPE_STRING, PARM_MAND, 0, 0},
        {"BACKUP_DB_MAX_ERROR_COUNT", &BACKUP_MAX_ERROR_COUNT, TYPE_INT, PARM_MAND, 0, 100000},
        {"ALERT_IP", &ALERT_IP, TYPE_STRING, PARM_MAND, 0, 0},
        {"ALERT_PORT", &ALERT_PORT, TYPE_INT, PARM_MAND, 0, 65535},
        {"MASTER_IP", &MASTER_IP, TYPE_STRING, PARM_MAND, 0, 0},
        {"MASTER_PORT", &MASTER_PORT, TYPE_INT, PARM_MAND, 0, 65535},
        {"THREAD_NUM", &THREAD_NUM, TYPE_INT, PARM_MAND, 0, 50},
        {"MSG_ID_IPMI_MAPPING", &MSG_ID_IPMI_MAPPING, TYPE_INT, PARM_MAND, 0, 10000},
        {"MSG_ID_IPMI_DATA", &MSG_ID_IPMI_DATA, TYPE_INT, PARM_MAND, 0 ,10000},
        {"MSG_ID_SNMP_DATA", &MSG_ID_SNMP_DATA, TYPE_INT, PARM_MAND, 0, 10000},
        {"IR_CONTROLLER_THRESH", &IR_CONTROLLER_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"IR_PD_THRESH", &IR_PD_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"IR_VD_THRESH", &IR_VD_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"N_CONTROLLER_THRESH", &N_CONTROLLER_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"N_PD_THRESH", &N_PD_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"N_VD_THRESH", &N_VD_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"HP_CONTROLLER_THRESH", &HP_CONTROLLER_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"HP_PD_THRESH", &HP_PD_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"HP_VD_THRESH", &HP_VD_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"SERVER_PERFORM_THRESH", &SERVER_PERFORM_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"DISK_IO_THRESH", &DISK_IO_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"NETWORK_IO_THRESH", &NETWORK_IO_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"SSD_THRESH", &SSD_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"IPMI_RAW_THRESH", &IPMI_RAW_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"LOG_THRESH", &LOG_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"FRU_THRESH", &FRU_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
        {"STORAGE_THRESH", &STORAGE_THRESH, TYPE_INT, PARM_MAND, 0, 10000},
    };	
    return parse_cfg_file(CONFIG_FILE,cfg);
}

int main(int argc, char *argv[]) {
    load_config("../conf/datapool.conf");
    logOpen(LOG_FILE);
    logging("DEBUG","LOG_FILE %s MAX_EVENTS %d MSG_BUF_LENGTH %d LOG_LEVEL %d SOCKET_PORT %d DB_IP %s DB_PORT %d DB_NAME %s DB_USER_NAME %s DB_PASSWORD %s DB_MAX_ERROR_COUNT %d ALERT_IP %s ALERT_PORT %d THREAD_NUM %d MASTER_IP %s MASTER_PORT %d, BACKUP_DB_IP %s, BACKUP_DB_PORT %d, BACKUP_DB_NAME %s, BACKUP_DB_USER %s, BACKUP_DB_PASSWD %s, BACKUP_DB_MAX_ERROR_COUNT %d MSG_ID_IPMI_MAPPING %d MSG_ID_SNMP_DATA %d, MSG_ID_IPMI_DATA %d",LOG_FILE,MAX_EVENTS,MSG_BUF_LENGTH,LOG_LEVEL,SOCKET_PORT,DB_IP,DB_PORT,DB_NAME,DB_USER_NAME,DB_PASSWD,DB_MAX_ERROR_COUNT, ALERT_IP, ALERT_PORT, THREAD_NUM, MASTER_IP, MASTER_PORT, BACKUP_DB_IP, BACKUP_DB_PORT, BACKUP_DB_NAME, BACKUP_DB_USER, BACKUP_DB_PASSWD, BACKUP_MAX_ERROR_COUNT, MSG_ID_IPMI_MAPPING, MSG_ID_SNMP_DATA, MSG_ID_IPMI_DATA);
    setLogLevel(LOG_LEVEL);
    if( !DataBaseContainer::isThreadSafe())
    {
        logging("ERROR","MYSQL Library is NOT thread safe");
        return 0;
    }
    if (mysql_library_init(0, NULL, NULL)) {  
        logging("ERROR", "Could not initialize mysql library."); 
        return 0; 
    }
    DataBaseContainer::allocResource();
    DataBaseContainer::setBufferThreshold(IR_CONTROLLER_THRESH, IR_PD_THRESH, IR_VD_THRESH, N_CONTROLLER_THRESH, N_PD_THRESH, N_VD_THRESH, HP_CONTROLLER_THRESH, HP_PD_THRESH, HP_VD_THRESH, SERVER_PERFORM_THRESH, DISK_IO_THRESH, NETWORK_IO_THRESH, SSD_THRESH, IPMI_RAW_THRESH, LOG_THRESH, FRU_THRESH, STORAGE_THRESH);
    AsyncDataListener adl;   
    adl.start();
    mysql_library_end();
    return 1;
}
