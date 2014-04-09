#ifndef ASYNCDATALISTENER_H
#define ASYNCDATALISTENER_H
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<openssl/err.h>
#include<openssl/ssl.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/time.h>
#include<vector>
#include<sstream>
#include<google/protobuf/io/coded_stream.h>
#include<google/protobuf/io/zero_copy_stream_impl.h>
#include"IPMIData.pb.h"
#include"ReturnCode.pb.h"
#include"SNMPData.pb.h"
#include"IPMIMapping.pb.h"
#include"DataBaseProxy.h"
#include"IPMIRaw.h"
#include"Sel.h"
#include"SNMPDataDB.h"
#include"AsyncDataSender.h"
#include"SensorMap.h"
#include"SSDData.h"
#include<sys/time.h>
#include"common.h"
extern int MSG_ID_IPMI_MAPPING;
extern int MSG_ID_IPMI_DATA;
extern int MSG_ID_SNMP_DATA;
extern int MSG_BUF_LENGTH;
using namespace google::protobuf::io;
using namespace std;

class AsyncDataListener
{
    public:
        AsyncDataListener();
        AsyncDataListener(const AsyncDataListener &other);
        ~AsyncDataListener();
        void start();
        static void *run(void* ctx);
        static void *calcSpeed(void* ctx);
        void *handle_message(void *);
        int read_message(int);
        int setnonblocking(int);
    private: 
        bool getIPandPort(int , char * , int *);
        int getCurUsec();
        bool sendResponse(int , int , string);
        vector<string> & split(const string &,char,vector<string> &);
        void initializeRequest();

        DataBaseContainer *db;
        IPMIRaw* ipmi_raw;
        Sel* sel_table;
        SNMPDataDB * snmp_db;
        AsyncDataSender* msg_sender;
        AsyncDataSender* master_sender;
        SSDData* ssd_data;
        static SensorMap* sensor_map;
};
google::protobuf::uint32 readHdr(char *);
int readBody(int, google::protobuf::uint32, void**, AsyncDataSender *);
int SocketHandler(void *, void**, AsyncDataSender *);
#endif
