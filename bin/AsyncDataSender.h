#ifndef ASYNC_DATA_SENDER_H
#define ASYNC_DATA_SENDER_H
#include<sys/types.h>
#include<sys/socket.h>
#include<string>
#include<netinet/in.h>
#include"MessageType.h"
#include<time.h>
#include"ReturnCode.pb.h"
#include<google/protobuf/io/coded_stream.h>
#include<google/protobuf/io/zero_copy_stream_impl.h>
#include"ReturnCode.pb.h"
#include<errno.h>
#include <arpa/inet.h>
#include<netdb.h>
using namespace google::protobuf::io;
using namespace std;

class AsyncDataSender
{
    public:
        AsyncDataSender(string, int);
		AsyncDataSender(const AsyncDataSender &);
        ~AsyncDataSender();
        bool send(char *,int,int);
        int getUniqueID();
    private:
        int hsock;
        struct sockaddr_in my_addr;
        struct hostent * he;
        string host;
        int port;
};
#endif
