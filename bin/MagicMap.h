#ifndef MAGIC_MAP_H
#define MAGIC_MAP_H
#include<map>
#include<string>
#include"SNMPData.pb.h"
#include"IPMIData.pb.h"
#define INT_DEFAULT 18446744073709551615
#include<cstdio>
using namespace std;
const string SF_KEY_INT[]={
    "uptime",
    "memTotalSwap",
    "memAvailSwap",
    "memTotalReal",
    "memAvailReal",
    "memTotalFree",
    "memBuffer" ,
    "memCached" ,
    "dskPercent",
    "cpu_load",
    "hrMemorySize",
};
const string SF_KEY_STRING[]={
    "cpuDescr",
    "sysdescr",
};
const string DISK_KEY_INT[]={
    "disk_io_read",
    "disk_io_write",
    "disk_io_rtimes",
    "disk_io_wtimes",
};
const string DISK_KEY_STRING[]={
    "diskIODevice",
};
const string STORAGE_KEY_INT[]={
    "hrStorageIndex",
    "hrStorageAllocationUnits",
    "hrStorageSize",
    "hrStorageUsed",
    "hrStorageAllocationFailures",
};
const string STORAGE_KEY_STRING[]={
    "hrStorageDescr",
};
const string NETWORK_KEY_INT[]={
    "ifIndex" ,
    "ifSpeed" ,
    "ifAdminStatus" ,
    "ifOperStatus" ,
    "ifLastChange",
    "ifInOctets" ,
    "ifInUcastPkts" ,
    "ifInNUcastPkts",
    "ifInDiscards",
    "ifInErrors" ,
    "ifInUnknownProtos",
    "ifOutOctets" ,
    "ifOutUcastPkts" ,
    "ifOutNUcastPkts",
    "ifOutDiscards",
    "ifOutErrors",
    "ifOutQLen",

};
const string NETWORK_KEY_STRING[]={
    "ifDescr" ,
    "ifPhysAddress" ,
};
const string IR_RAID_INT[]={
    //"adpNumber",
    "adapterID",
    "rebuildRate",
    "reconstructionRate",
    "alarmState",
    "patrolReadRate",
    "bgiRate",
    "ccRate",
    "hostInterfacePortCnt",
    "devInterfacePortCnt",
    "nvramSize",
    "memorySize",
    "flashSize",
    "vdPresentCount",
    "slotCount",
    "pdCount",
    "pdPresentCount",
    "pdDiskPresentCount",
    "pdDiskPredFailureCount",
    "pdDiskFailedCount",
    "vdDegradedCount",
    "vdOfflineCount",
    "dedicatedHotSpares",
    "revertibleHotSpares" ,
    "vdNumber",
};
const string IR_RAID_STRING[]={
    "sasAddr",
    "productName",
    "firmwareVersion",
    "driverVersion",
    "devInterfacePortAddr",
};
const string IR_PD_INT[]={
    "mediaErrCount",
    "otherErrCount",
    "predFailCount",
    "pdState",
    "linkSpeed",
    "rawSize",
    "slotNumber",
    "pdAdpID",
    "diskType",
    "phyPos",
    "mediatype",
};
const string IR_PD_STRING[]={
    "pdSasAddr",
    "pdOperationProgress",
    "vendorID",
    "productID",
    "pdFwversion",
    "pdSerialNumber",
    "connectedAdapterPort",//change this attribute to string type
};
const string IR_VD_INT[]={
    "size",
    "state",
    "vdDiskCachePolicy",
    "prl",
    "sRL",
    "stripeSize",
    "numDrives",
    "currentCachePolicy",
    "vdAdpID",
};
const string IR_VD_STRING[]={
    "operationProgress",
    "pdList",
};
const string N_RAID_INT[]={
    //"adpNumber",
    "adapterID-APT",
    "rebuildRate",
    "reconstructionRate",
    "alarmState",
    "patrolReadRate",
    "bgiRate",
    "vdPresentCount",
    "vdDegradedCount",
    "vdOfflineCount",
    "pdPresentCount",
    "pdDiskPresentCount",
    "pdDiskPredFailureCount",
    "pdDiskFailedCount",
    "ccRate",
    "copyBackState",
    "hostInterface",
    "devInterfacePortCnt",
    "bbuNumber",
    "nextLearnTime",
    "absStateOfCharge",
    "batteryReplacement",
    "pdNumber",
    "memorySize",
    "vdNumbers",
};
const string N_RAID_STRING[]={ 
    "adapterVendorID",
    "devID",
    "subVendorID",
    "subDevID",
    "devInterface",
    "serialNo",
    "firmwareVersion",
    "driverVersion",
    "hardwarePresent",
    "bbuState",
    "productName",
};
const string N_PD_STRING[]={
    "connectedAdapterPort",
    "sasAddr",
    "operationProgress",
    "pdVendorID",
    "pdProductID",
    "pdFwversion",
    "pdSerialNumber",
};
const string N_PD_INT[]={
    "pdIndex",
    "physDevID",
    "mediaErrCount",
    "otherErrCount",
    "predFailCount",
    "pdState",
    "disabledForRemoval",
    "linkSpeed",
    "rawSize",
    "slotNumber",
    "adpID-PDT",
    "diskType",
    "mediatype",
};
const string N_VD_INT[]={
    "virtualDevID",
    "size",
    "state",
    "diskCachePolicy",
    "bgiState",
    "prl",
    "sRL",
    "stripeSize",
    "numDrives",
    "adapterID-VDT",
};
const string N_VD_STRING[]={
    "currentCachePolicy",
    "pdList",
};
const string HP_RAID_INT[]={
    "cpqDaMibCondition",
    "cpqDaCntlrModel",
    "cpqDaCntlrSlot",
    "cpqDaCntlrCondition",
    "cpqDaCntlrBoardStatus",
    "cpqDaCntlrRebuildPriority",
    "cpqDaCntlrExpandPriority",
    "cpqDaCntlrNumberOfInternalPorts",
    "cpqDaCntlrNumberOfExternalPorts",
    "cpqDaCntlrDriveWriteCacheState",
    "cpqDaAccelStatus",
    "cpqDaAccelBadData",
    "cpqDaAccelErrCode",
    "cpqDaAccelBattery",
    "cpqDaAccelCondition",
    "cpqDaAccelSerialNumber",
    "cpqDaAccelTotalMemory",
    "cpqDaAccelReadCachePercent",
    "cpqDaAccelWriteCachePercent",
};
const string HP_RAID_STRING[]={
    "cpqDaCntlrFWRev",
    "cpqDaCntlrSerialNumber",
    "cpqDaAccelFailedBatteries",
};
const string HP_PD_INT[]={
    "cpqDaPhyDrvIndex",
    "cpqDaPhyDrvBay",
    "cpqDaPhyDrvStatus",
    "cpqDaPhyDrvHardReadErrs",
    "cpqDaPhyDrvRecvReadErrs",
    "cpqDaPhyDrvHardWriteErrs",
    "cpqDaPhyDrvRecvWriteErrs",
    "cpqDaPhyDrvDrqTimeouts",
    "cpqDaPhyDrvOtherTimeouts",
    "cpqDaPhyDrvSize",
    "cpqDaPhyDrvBusFaults",
    "cpqDaPhyDrvSmartStatus",
    "cpqDaPhyDrvRotationalSpeed",
    "cpqDaPhyDrvType",
    "cpqDaPhyDrvNegotiatedLinkRate",
};
const string HP_PD_STRING[]={
    "cpqDaPhyDrvModel",
    "cpqDaPhyDrvFWRev",
    "cpqDaPhyDrvSerialNum",
    "cpqDaPhyDrvLocationString",
};
const string HP_VD_INT[]={
    "cpqDaLogDrvIndex",
    "cpqDaLogDrvFaultTol",
    "cpqDaLogDrvStatus",
    "cpqDaLogDrvHasAccel",
    "cpqDaLogDrvSize",
    "cpqDaLogDrvCondition",
    "cpqDaLogDrvPercentRebuild",
    "cpqDaLogDrvRebuildingPhyDrv",
};
const string HP_VD_STRING[]={
    "cpqDaLogDrvPhyDrvIDs",
    "cpqDaLogDrvStripeSize",
    "cpqDaLogDrvOsName",
};
class MagicMap
{
    public:
        MagicMap();
        ~MagicMap();
        bool buildMap(const ProcData * );
        bool buildMap(const IfData * );
        bool buildMap(const DiskData* );
        bool buildMap(const StorageData*);
        bool buildMap(const Raid *, int);
        bool buildMap(const PD *, int);
        bool buildMap(const VD *, int);
        bool buildMap(const CollectData*);
        bool clearMap();
        unsigned long long int findIntKey(const string &);
        string findStringKey(const string &);
        bool parseInsertString(string*, string*);
        bool parseUpdateString(string&);
        void setFilterKey(const string[], int);
        void setIntKey(string , unsigned long long int);
        void setStringKey(string, string);
    private:
        map<string, unsigned long long int> int_map;
        map<string, string> string_map;
        //fucntions to initialize default keys for collect metric
        void initServerPerform();
        void initDiskIO();
        void initStorage();
        void initNetworkIO();
        void initRaid(int);
        void initPD(int);
        void initVD(int);

};
#endif
