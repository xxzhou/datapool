#include"AsyncDataSender.h"
#include <stdio.h>
#include"log.h"
AsyncDataSender::AsyncDataSender(string  host, int  port)
{
    this->host = host;
    this->port = port;
}
AsyncDataSender::AsyncDataSender(const AsyncDataSender& other):host(other.host),port(other.port){}
AsyncDataSender::~AsyncDataSender()
{
}
bool AsyncDataSender::send(char * pack, int pack_size, int FLAG)
{
    hsock = socket(AF_INET, SOCK_STREAM, 0);
    if(hsock == -1)
    {
        logging("ERROR","Error initializing socket, errno %d", errno);
        //printf("Error initializing socket %d \n", errno);
        close(hsock);
        return false;
    }
    int pint =1;
    if(setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, &pint, sizeof(pint)) == -1 || setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, &pint, sizeof(pint)) == -1      )
    {
        logging("ERROR","Error setting options, errno %d", errno);
        return false;
    }
    he = gethostbyname(host.c_str());
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    //my_addr.sin_addr.s_addr  = inet_addr(host.c_str());
    if( inet_aton(host.c_str(), &my_addr.sin_addr) == 0)
    {
        logging("ERROR","converting ip address");
        return false;
    } 
    bzero(&(my_addr.sin_zero), 8);
    int ret;
    int bytecount;
    /**
    if(fcntl(hsock, F_SETFL, fcntl(hsock, F_GETFD, 0) | O_NONBLOCK)== -1)
    {
        logging("ERROR","set socket nonblock");
        close(hsock);
        return false;
    }**/
    if( (ret = connect(hsock, (struct sockaddr*)&my_addr, sizeof(struct sockaddr))) == -1 )
    {
        if( errno != EINPROGRESS)
        {
            logging("ERROR","error connecting socket, errno %d", errno); 
            close(hsock);
            return false;
        }
    }
    //logging("DEBUG","Connected");
    if((bytecount = ::send(hsock, (void *)pack, pack_size, 0)) == -1)
    {
        logging("ERROR","error sending data %d", errno);
        close(hsock);
        return false;
    }
    close(hsock);
    return true;
}
int AsyncDataSender::getUniqueID()
{
    time_t cur;
    time(&cur);
    return cur;
}
