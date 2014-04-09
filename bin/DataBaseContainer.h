#ifndef DATABASE_CONTAINER_H
#define DATABASE_CONTAINER_H
#include"DataBaseProxy.h"
#include<vector>
#include<pthread.h>
#include<unistd.h>
#include<map>
#include"common.h"
#define METRIC_COUNT 17

using namespace std;
class DataBaseContainer
{
    public:
        DataBaseContainer(string, int, string, string, string, int, string, int, string, string, string, int);
        DataBaseContainer(const DataBaseContainer &);
        ~DataBaseContainer();
        bool connect();
        void disconnect();
        bool db_insert(const char*, const char*);
        bool db_insert(const char*);
        MYSQL_RES* db_query(const char*);
        static void setBufferThreshold(int, int, int, int, int, int, int, int, int, int ,int, int, int, int, int, int , int);
        static void releaseResource();
        static void allocResource();
        static bool isThreadSafe();
    private:
        DataBaseProxy* common;
        DataBaseProxy* backup;
        static int *cur_count;
        static int *thresholds;
        static vector<string>** values_buffer;
        static pthread_rwlock_t* rw_lock;
        string parseVectorToSQL(vector<string>*);
        static map<string,int>* table_id;
        bool bufferValues(const char*, string &, string&);
        string getTableSchema(int);
};
#endif
