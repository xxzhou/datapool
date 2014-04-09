#ifndef SNMPDATA_DB_H
#define SNMPDATA_DB_H
#include"DataBaseContainer.h"
#include"SNMPData.pb.h"
#include<sstream>
#include"log.h"
#include"MagicMap.h"
#include<string>
#include<stdlib.h>
#define TOTAL_DISK_INDEX 255
#define TOTAL_IF_INDEX 255
extern const string PERFORM_STATUS[];
extern const string IF_STATUS[];
extern const string DISK_STATUS[];
extern const string STORAGE_STATUS[];
class SNMPDataDB 
{
    public:
        SNMPDataDB(DataBaseContainer *);
        ~SNMPDataDB();
        bool insert(SNMPData *);
        bool remove();
        SNMPData * find();
        bool update();
    private:
        DataBaseContainer * db;
        string getSQL(string,string,string,SNMPData*);
        MagicMap magic_map;

        int findPerformStatus(string );
        int findIfStatus(string , int );
        int findDiskStatus(string , int );
        int findStorageStatus(string , string);
        int findControllerStatus(string , unsigned long long int , string );
        int findPDStatus(string, unsigned long long int, string, string);
        int findVDStatus(string, unsigned long long int , string, string);
        
        int updatePerformStatus(string, string , const ProcData*, float, float);
        int updateIfStatus(string , string , int , const IfData*, float ,float);
        int updateDiskStatus(string, string ,int , const DiskData*, float, float );
        int updateStorageStatus(string, string, string, float);
        int updateControllerStatus(string, string, unsigned long long int, const Raid*, string);
        int updatePDStatus(string, string, unsigned long long int, int ,unsigned long long int, string , const PD*, string);
        int updateVDStatus(string, string, unsigned long long int, int , string , const VD*, string);
        bool updateServerSSD(string);
        bool reviseRaidType(string,int);
        
        bool insertServerPerform(string, string, const ProcData*, float, float);
        bool insertIfData(string , string ,  const IfData *, float, float );
        bool insertDiskData(string , string , int,  const DiskData *, float ,float);
        bool insertStorageData(string , string, float);
        bool insertControllerData(string, string, unsigned long long int, const Raid*, string);
        bool insertPDData(string, string ,unsigned long long int, int, const PD*, string);
        bool insertVDData(string, string ,unsigned long long int, int, const VD*, string);
        bool parseSysDescr(string&, string&, string&, string&, string &);
        
        //获取network io 速度
        bool findPreviousIFIO(string& , map<int ,unsigned long long int>& , map<int, unsigned long long int>& , map<int, unsigned long long int>& );
        bool findPreviousDISKIO(string& , map<int ,unsigned long long int>& , map<int, unsigned long long int>& , map<int, unsigned long long int>& );
};
#endif
