#include<iostream>
#include <string>
#include"AsyncDataListener.h"
#include"log.h"
#include"Util.h"
using namespace std;
#include <unistd.h>
#define my_delete(x) {delete (x); (x) = NULL;}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
struct task{
    int type_id;
    void* data;
    struct task* next;
};
task *readhead=NULL, *readtail=NULL, *writehead=NULL;
static volatile long long cnt = 0;
static volatile long long cur_bytes = 0;
SensorMap* AsyncDataListener::sensor_map = new SensorMap();
/**
 * 构造函数
 * 参数：
 * hostname: DB IP
 * port: DB port
 * username: DB user name
 * passwd: DB password
 * db_name: Database name
 * error_count: max error count in database
 * msg_buf_len: message buffer length
 * max_events: max events in epoll
 * socket_port: socket port
 * alert_ip: ip of alert module
 * alert_port: port of alert module
 * thread_num: number of threads in datapool module
 * master_ip: ip of master module
 * master_port: port of master module
 **/
AsyncDataListener::AsyncDataListener( )
{
    db = new DataBaseContainer(string(DB_IP), DB_PORT, string(DB_USER_NAME), string(DB_PASSWD), string(DB_NAME), DB_MAX_ERROR_COUNT, string(BACKUP_DB_IP), BACKUP_DB_PORT, string(BACKUP_DB_USER), string(BACKUP_DB_PASSWD), string(BACKUP_DB_NAME), BACKUP_MAX_ERROR_COUNT);
    db->connect();
    ipmi_raw = new IPMIRaw(db);
    master_sender = new AsyncDataSender(string(MASTER_IP),MASTER_PORT);
    ipmi_raw->setSender(master_sender);
    ipmi_raw->setSensorMap(AsyncDataListener::sensor_map);
    sel_table = new Sel(db);
    snmp_db = new SNMPDataDB(db);
    msg_sender = new AsyncDataSender(string(ALERT_IP),ALERT_PORT);
    ssd_data = new SSDData();
    ssd_data->setDB(this->db);
}
AsyncDataListener::AsyncDataListener(const AsyncDataListener &other)
{
    this->db = new DataBaseContainer(*(other.db));
    this->db->connect();
    ipmi_raw = new IPMIRaw(this->db);
    master_sender = new AsyncDataSender(*(other.master_sender));
    ipmi_raw->setSender(master_sender);
    sel_table = new Sel(this->db);
    snmp_db = new SNMPDataDB(this->db);
    msg_sender = new AsyncDataSender(*(other.msg_sender));
    ssd_data = new SSDData(*(other.ssd_data));
    ssd_data->setDB(this->db);
}
AsyncDataListener::~AsyncDataListener()
{
    db->disconnect();
    my_delete(db);
    my_delete(ipmi_raw);
    my_delete(master_sender);
    my_delete(sel_table);
    my_delete(snmp_db);
    my_delete(msg_sender);
    my_delete(ssd_data);
}
google::protobuf::uint32 readHdr(char * buf)
{
    google::protobuf::uint32 size;
    google::protobuf::io::ArrayInputStream ais(buf,4);
    CodedInputStream coded_input(&ais);
    coded_input.ReadVarint32(&size);
    //cout<<"content size "<<size<<endl;
    cur_bytes += size;
    return size;
}
int  readBody(int csock, google::protobuf::uint32 siz, void** return_data, AsyncDataSender * msg_sender)
{
    int bytecount;
    char * buffer = new char[siz+4*3];
    if (NULL == buffer)
        return 0;
    int cur_length = 0;
    int total_length = siz+4*3;
    while(cur_length < siz)
    {
        if((bytecount = read(csock, (void *)(buffer+cur_length), total_length))== -1){
            if( errno == EAGAIN)
            {
                bytecount = read(csock, (void *)(buffer+cur_length), total_length);
                logging("INFO","Read Body EAGAIN");
                if(bytecount == -1)
                    //return -1;
                    continue;
            }
            else
            {
                logging("ERROR","Error receiving data: %s; errno:%d", strerror(errno),errno);
                return -1;
            }

        }else if(bytecount==0)
        {
            if(cur_length < total_length)
            {
                logging("ERROR","Error data insufficient. total_length:%d, cur_length:%d ", total_length,cur_length);
                return -1;
            }
        }
        else
        {
            cur_length += bytecount;
        }
    }
    msg_sender->send(buffer,cur_length,0);
    /**Assign ArrayInputStream with enough memory **/
    google::protobuf::io::ArrayInputStream ais(buffer,siz+4*3);
    CodedInputStream coded_input(&ais);
    /**Read an unsigned integer with Varint encoding, truncating to 32 bits.**/
    coded_input.ReadVarint32(&siz);
    google::protobuf::uint32 type, magic_id;
    coded_input.ReadVarint32(&type);
    coded_input.ReadVarint32(&magic_id);
    /**After the message's length is read, PushLimit() is used to prevent the CodedInputStream 
      from reading beyond that length.Limits are used when parsing length-delimited 
      embedded messages**/
    google::protobuf::io::CodedInputStream::Limit msgLimit = coded_input.PushLimit(siz);
    /**Calculate the latency for each message**/
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint32_t cur_ms = tv.tv_sec * 1000 + tv.tv_usec/1000;
    cur_ms -= magic_id;
    logging("INFO","magic id % u latency %u ms", magic_id, cur_ms);
    //De-Serialize
    logging("DEBUG","message type_id %u", type);
    if(type == MSG_ID_IPMI_DATA)
    {
        (*return_data) = new CollectData();
        CollectData* temp = (CollectData*)(*return_data);
        temp->ParseFromCodedStream(&coded_input);
        logging("DEBUG", "%s", (temp->DebugString()).c_str());
    }
    else if(type == MSG_ID_SNMP_DATA)
    {
        (*return_data) = new SNMPData();
        SNMPData* temp= (SNMPData*)(*return_data);
        temp->ParseFromCodedStream(&coded_input); 
        logging("DEBUG", "%s", (temp->DebugString()).c_str());
    }
    else if(type == MSG_ID_IPMI_MAPPING)
    {
        (*return_data) = new Action();
        Action* temp = (Action*)(*return_data);
        temp->ParseFromCodedStream(&coded_input);
        logging("DEBUG", "%s", (temp->DebugString()).c_str());
    }
    else//to be implemented
    {
        logging("WARN","message type %u uncovered",type);
        (*return_data) = 0;
    }
    //Once the embedded message has been parsed, PopLimit() is called to undo the limit
    coded_input.PopLimit(msgLimit);
    delete []buffer;
    //Print the message
    return type;
}
int AsyncDataListener::setnonblocking(int sockfd)
{
    if(fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK)== -1)
        return -1;
    return 0;    
}
void *AsyncDataListener::run(void* ctx) {
    AsyncDataListener* params = static_cast<AsyncDataListener*>(ctx);
    AsyncDataListener* cptr = new AsyncDataListener(*(params));
    cptr->handle_message(NULL);
    delete cptr;
    return NULL;
}

void init_thread_pool(void (*alg)(), void *(*handler)(void *), void *arg, int thread_num)
{
    if (NULL == alg) {
        pthread_t tid[thread_num];
        for (int i = 0; i < thread_num; i++) {
            if (0 != pthread_create(&tid[i], NULL, handler, (void*)arg))
                exit(1);
            else
                pthread_detach(tid[i]);
            //			pthread_join(tid[i], NULL);
        }
        pthread_t temp;
        if (0 != pthread_create(&temp, NULL, AsyncDataListener::calcSpeed, NULL))
            exit(1);
        else
            pthread_detach(temp);
    }
    else
        (*alg)();
    return;
}

static void sig_handler(int i)
{
    //release the task_queue 
    while (NULL != readhead) {
        task *tmp = readhead;
        int  msg_type  = readhead->type_id;
        void* msg_data = readhead->data;
        if(msg_type == MSG_ID_IPMI_DATA)
        {
            CollectData* cd = (CollectData*)msg_data;
            msg_data = 0;
            delete cd;
        }
        else if(msg_type == MSG_ID_SNMP_DATA )
        {
            SNMPData* snmp_data = (SNMPData*)msg_data;
            msg_data=0;
            delete snmp_data;
        }
        else if(msg_type == MSG_ID_IPMI_MAPPING)
        {
            Action* map_data = (Action*)msg_data;
            msg_data=0;
            delete map_data;
        }
        else
        {
            //
        }
        readhead = readhead->next;
        my_delete(tmp);
    }
    DataBaseContainer::releaseResource();
    exit(0);
}

void AsyncDataListener::start() 
{
    struct epoll_event ev, events[MAX_EVENTS];
    struct sockaddr_in their_addr;
    socklen_t their_len = sizeof(struct sockaddr_in);
    struct sockaddr_in my_addr;
    int listen_sock, on, conn_sock, nfds, epollfd;

    init_thread_pool(NULL, run, this, THREAD_NUM);
    signal(SIGINT, sig_handler);

    if( (listen_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        logging("ERROR","socket created failed");
        exit(1);
    }
    else
        logging("INFO","socket created");
    /**set the socket non-block**/
    setnonblocking(listen_sock);
    /**set the socket port reuseable**/
    on = 1;
    if(setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
    {
        logging("ERROR","setsockopt reuseaddr error");
        exit(1);
    }
    //set receive buffer size
    if(setsockopt(listen_sock, SOL_SOCKET, SO_RCVBUF, &MSG_BUF_LENGTH, sizeof(MSG_BUF_LENGTH)))
    {
        logging("ERROR","setsockopt rcvbuf size error");
    }
    //set address
    bzero(&my_addr,sizeof(my_addr));
    my_addr.sin_family = PF_INET;
    my_addr.sin_port = htons(SOCKET_PORT);
    my_addr.sin_addr.s_addr=INADDR_ANY;
    //socket bind the address
    if( bind(listen_sock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 )
    {   
        logging("ERROR","bind error");
        exit(1);
    }
    else
        logging("INFO","IP and port binded");
    //listen
    if(listen(listen_sock, 2)== -1)
    {
        logging("ERROR","Listen error");
        exit(1);
    }
    else
        logging("INFO","start service");
    //create epoll, set the max events
    epollfd = epoll_create(MAX_EVENTS);
    if( epollfd == -1)
    {
        logging("ERROR","epoll created error");
        exit(EXIT_FAILURE);
    }
    //set epoll in edge trigered, and just read data
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listen_sock;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1 )
    {
        logging("ERROR","epoll_ctl:listen_sock error");
        exit(EXIT_FAILURE);
    }
    //send get ipmi-mapping command to master
    //ipmi_raw->sendGet();
    //listen 
    for(;;)
    {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        //		usleep(100);
        if(nfds == -1)
        {
            if (EINTR == errno) continue;
            //perror("epoll_wait");
            logging("ERROR","epoll_wait error, errno = %d, %d", errno, __LINE__);
            exit(EXIT_FAILURE);
        }
        for(int n=0; n< nfds; n++)
        {
            if(events[n].data.fd == listen_sock)
            {
                conn_sock = accept(listen_sock, (struct sockaddr*)&their_addr, &their_len);
                if (conn_sock == -1)
                {
                    logging("ERROR", "accept error, errno = %d", errno);
                    exit(EXIT_FAILURE);
                }
                else
                {
                    int port=ntohs(their_addr.sin_port);
                    char ipstr[INET_ADDRSTRLEN];
                    if( inet_ntoa(their_addr.sin_addr))
                    {
                        strcpy(ipstr,inet_ntoa(their_addr.sin_addr));
                        logging("INFO","\nconnection from %s:%d, socket %d created",ipstr,port,conn_sock);
                    }else
                    {
                        logging("INFO","\nconnection from unknown host, socket %d created",conn_sock);
                    }
                }
                setnonblocking(conn_sock);
                ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT ;
                ev.data.fd = conn_sock;
                if( epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1)
                {
                    logging("ERROR","epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
            }
            else {
                read_message(events[n].data.fd);
            }
        }
    }
}
void *AsyncDataListener::handle_message(void *arg)
{
    while (1) {
        //sleep(100);
        pthread_mutex_lock(&mutex);
        while (readhead == NULL)
            pthread_cond_wait(&cond1, &mutex);
        int msg_type = readhead->type_id;
        void* msg_data = readhead->data;
        task *tmp = readhead;
        readhead = readhead->next;
        my_delete(tmp);
        cnt--;
        pthread_mutex_unlock(&mutex);
        if ( msg_data != 0)
        {
            if(msg_type == MSG_ID_IPMI_DATA)
            {
                logging("INFO","IPMI Data consume");
                CollectData* cd = (CollectData*)msg_data;
                this->ipmi_raw->insert(cd);
                this->sel_table->insert(cd);    
                msg_data = 0;
                delete cd;
            }
            else if(msg_type == MSG_ID_SNMP_DATA)
            {
                logging("INFO", "SNMP data consume");
                SNMPData* snmp_data = (SNMPData*)msg_data;
                //如果有SSD数据
                if(snmp_data->has_ssd_data())
                {
                    logging("INFO","SSD Data consume");
                    string ssd_raw = snmp_data->ssd_data();
                    ssd_data->insert(ssd_raw, snmp_data->server_id(),convert<string>(snmp_data->collect_time()));
                }
                this->snmp_db->insert(snmp_data);
                msg_data=0;
                delete snmp_data;
            }
            else if(msg_type == MSG_ID_IPMI_MAPPING)
            {
                Action* map_data = (Action*)msg_data;
                if( map_data->act()=="initial")
                {
                    logging("INFO","Initialize ipmi-map data");
                    AsyncDataListener::sensor_map->initialize(map_data);    
                }
                else if(map_data->act()== "update")
                {
                    logging("INFO","Update ipmi-map data");
                    AsyncDataListener::sensor_map->expandMap(map_data); 
                }
                else
                {
                    logging("ERROR","Invalid act option in action data");
                }
                msg_data=0;
                delete map_data;
            }
        }
        else
        {
            logging("WARN","Message is NULL");
        }
    }
    return NULL;
}
int AsyncDataListener::read_message(int new_fd)
{
    int *fd = new int(new_fd);
    void* msg_data = 0;
    //read data from fd	
    int msg_type  = SocketHandler(fd, &msg_data, this->msg_sender);
    //further sends and receives are disallowed
    shutdown(*fd, 2); 
    //close the fd
    close(*fd);
    if ( msg_data != 0)
    {
        //start - put the task in queue
        struct task* new_task = new task();
        new_task->type_id = msg_type;
        new_task->data = msg_data;
        pthread_mutex_lock(&mutex);
        if(readhead == NULL) {
            readhead = new_task;
            readtail = new_task;
        }
        else {
            readtail->next = new_task;
            readtail = new_task;
        }
        cnt++;
        logging("INFO","Read data from fd %d in queue, current queue size: %d", new_fd, cnt);
        pthread_cond_broadcast(&cond1);
        pthread_mutex_unlock(&mutex);
        //end - put the task in queue
    }
    else
    {
        if(msg_type == -1)
            logging("ERROR","Message can't be read");
        else
            logging("WARN","Message is NULL");
    }
    //delete the fd
    delete(fd);
    return 0;
}
int AsyncDataListener::getCurUsec()
{
    struct timeval start;
    gettimeofday(&start,NULL);
    long usec = start.tv_sec * 1000 * 1000 + start.tv_usec;
    return usec;
}
vector<string> & AsyncDataListener::split(const string &s, char delim, vector<string> & elems)
{
    stringstream ss(s);
    string item;
    while(getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}
int SocketHandler(void * lp, void ** return_data, AsyncDataSender* msg_sender)
{
    try{
        int *csock = (int*)lp;
        char buffer[4];
        int bytecount=0;
        memset(buffer, '\0', 4);
        if((bytecount = recv(*csock,buffer,4, MSG_PEEK))== -1){
            if(errno == EAGAIN)
            {
                usleep(5000);
                logging("INFO","MSG_PEEK EAGAIN");
                bytecount = recv(*csock,buffer,4, MSG_PEEK);
                if( bytecount == -1)
                    return -1;
            }
            else
            {
                logging("ERROR","Error peaking data: %s; errno:%d, fd: %d", strerror(errno),errno, *csock);
                return -1;
            }
        }else if (bytecount == 0)
        {
            logging("WARN","MSG_PEEK bytcount = 0");
            return -1;
        }
        else
        {
            int size = readHdr(buffer);
            logging("DEBUG","Message len %d", size );
            return readBody(*csock, size, return_data, msg_sender);
        }
    }catch(exception& e)
    {
        logging("ERROR","Exception caught: %s",e.what());
        return -1; 
    }
}
bool AsyncDataListener::sendResponse(int sock_fd, int code, string msg)
{
    ReturnCode rc;
    rc.set_return_code(code);
    rc.set_message(msg);
    int pack_size = rc.ByteSize() + 4*3;
    char * pack = new char[pack_size];
    google::protobuf::io::ArrayOutputStream aos(pack,pack_size);
    CodedOutputStream * coded_output = new CodedOutputStream(&aos);
    coded_output->WriteVarint32(rc.ByteSize());
    coded_output->WriteVarint32(1025);
    coded_output->WriteVarint32(1000000);
    rc.SerializeToCodedStream(coded_output);
    //client_socket << pack;
    send(sock_fd, pack, pack_size, 0);
    delete []pack;
    delete coded_output;
    return true;
}
bool AsyncDataListener::getIPandPort(int sock_fd, char * ip, int * port)
{
    struct sockaddr_storage addr;
    socklen_t len = sizeof addr;
    if(getpeername(sock_fd,(sockaddr*)&addr,&len) != -1)
    {
        if(addr.ss_family == AF_INET)
        {
            struct sockaddr_in *s = (struct sockaddr_in*)&addr;
            *port = ntohs(s->sin_port);
            inet_ntop(AF_INET,&s->sin_addr,ip,sizeof ip);
        }
        else// AF_INET6
        {
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
            *port = ntohs(s->sin6_port);
            inet_ntop(AF_INET6,&s->sin6_addr,ip,sizeof ip);
        }
        return true;
    }
    else
        return false;
}
//send "get" to master
void AsyncDataListener::initializeRequest()
{
    Action action;
    action.set_act("get");
    int pack_size = action.ByteSize() + 4*3;
    char * pack = new char[pack_size];
    google::protobuf::io::ArrayOutputStream aos(pack,pack_size);
    CodedOutputStream * coded_output = new CodedOutputStream(&aos);
    coded_output->WriteVarint32(action.ByteSize());
    coded_output->WriteVarint32(MSG_ID_IPMI_MAPPING);
    coded_output->WriteVarint32(1000000);
    action.SerializeToCodedStream(coded_output);
    this->master_sender->send(pack, pack_size, 1);
    delete []pack;
    delete coded_output;
    logging("INFO","initialize request for ipmi mapping");
    logging("DEBUG", action.DebugString().c_str());
}
//thread run fuction to calculate speed
void * AsyncDataListener::calcSpeed(void * arg)
{
    while(1)
    {
        sleep(1);
        logging("INFO", "Speed: %f  KB/s", (float)cur_bytes/1000);
        cur_bytes=0;
    }
}
