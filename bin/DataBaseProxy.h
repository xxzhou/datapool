#ifndef DATABASE_PROXY_H
#define DATABASE_PROXY_H
#include<mysql/mysql.h>
#include<stdio.h>
#include<string>
#define BUFSIZE 1024
#include"log.h"
using namespace std;
class DataBaseProxy
{
    public:
        DataBaseProxy(string, int, string, string, string, int);
        DataBaseProxy(const DataBaseProxy &other); 
        ~DataBaseProxy();
        static bool isThreadSafe();
        bool connect();
        void disconnect();
        bool db_insert(const char*, int& );
        MYSQL_RES* db_query(const char*, int& );
    protected:    
        string hostname;
        int port;
        string username;
        string passwd;
        string default_db;
        int error_max_count;
        MYSQL *mysql;
};
#endif
