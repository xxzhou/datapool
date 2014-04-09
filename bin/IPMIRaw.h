#ifndef IPMI_RAW_H
#define IPMI_RAW_H
#include"DataBaseContainer.h"
#include"IPMIData.pb.h"
#include<sstream>
#include"log.h"
#include<set>
#include"IPMIMapping.pb.h"
#include"AsyncDataSender.h"
#include"SensorMap.h"
#include<vector>
#include<mysql.h>
#include"common.h"
#include"MagicMap.h"
#include<time.h>
extern int MSG_ID_IPMI_MAPPING;
using namespace std;
class IPMIRaw
{
    public:
        IPMIRaw(DataBaseContainer *);
        ~IPMIRaw();
        static void setSensorMap(SensorMap*);
        bool insert(CollectData *);
        bool remove();
        IPMIRaw* find();
        bool update();
        void setSender(AsyncDataSender*);
        void sendGet();
    private:
        DataBaseContainer * db;
        AsyncDataSender* sender;
        static SensorMap* sensor_map;
        MagicMap magic_map;
        map<string, string>* getIPMIMapping(uint32_t);
        bool addMapping(map<string,float>*, uint32_t);
        bool judgeMapping(map<string,float>*, map<string,string>*);
        bool appendMapping(map<string,string>*, set<string> &, uint32_t);
        bool extendMapping(set<string> & , uint32_t );
        bool insertFRU(CollectData*);
        int findFRUStatus(string, bool &);
        bool insertFRUData(CollectData*);
};
#endif
