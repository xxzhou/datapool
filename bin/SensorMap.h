#ifndef SENSOR_MAP
#define SENSOR_MAP
#include"DataBaseContainer.h"
#include"IPMIMapping.pb.h"
#include<map>
#include<pthread.h>
using namespace std;
class SensorMap
{
    public:
        SensorMap();
        SensorMap(DataBaseContainer*);
        void initialize(Action*);
        ~SensorMap();
        bool  getMap(uint32_t, map<string,string>**);
        bool expandMap(Action*);
    private:
        map<uint32_t,map<string,string>*>* data;
        pthread_mutex_t d_mutex;
        int d_nreading;
        pthread_cond_t d_nreading_zero;
        bool d_write_request;
        pthread_cond_t d_write_request_false;
};
#endif
