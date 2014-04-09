#include"MagicMap.h"
#include"Util.h"
MagicMap::MagicMap()
{

}
MagicMap::~MagicMap()
{}
bool MagicMap::buildMap(const ProcData * proc_data)
{
    clearMap();
    initServerPerform();
    KV_UINT64 int_kv;
    KV_STRING string_kv;
    int int_kv_size = proc_data->uint64_kv_size();
    int string_kv_size = proc_data->string_kv_size();
    while(int_kv_size)
    {
        int_kv = proc_data->uint64_kv(--int_kv_size);
        int_map[int_kv.name()] = int_kv.value();
    }
    while(string_kv_size)
    {
        string_kv = proc_data->string_kv(--string_kv_size);
        string_map[string_kv.name()] = string_kv.value();
    }
    return true;
}
bool MagicMap::buildMap(const IfData * if_data)
{
    clearMap();
    initNetworkIO();
    KV_UINT64 int_kv;
    KV_STRING string_kv;
    int int_kv_size = if_data->uint64_kv_size();
    int string_kv_size = if_data->string_kv_size();
    while(int_kv_size)
    {
        int_kv = if_data->uint64_kv(--int_kv_size);
        int_map[int_kv.name()] = int_kv.value();
    }
    while(string_kv_size)
    {
        string_kv = if_data->string_kv(--string_kv_size);
        string_map[string_kv.name()] = string_kv.value();
    }
    return true;
}
bool MagicMap::buildMap(const DiskData * disk_data)
{
    clearMap();
    initDiskIO();
    KV_UINT64 int_kv;
    KV_STRING string_kv;
    int int_kv_size = disk_data->uint64_kv_size();
    int string_kv_size = disk_data->string_kv_size();
    while(int_kv_size)
    {
        int_kv = disk_data->uint64_kv(--int_kv_size);
        int_map[int_kv.name()] = int_kv.value();
    }
    while(string_kv_size)
    {
        string_kv = disk_data->string_kv(--string_kv_size);
        string_map[string_kv.name()] = string_kv.value();
    }
    return true;
}
bool MagicMap::buildMap(const StorageData* storage_data)
{
    clearMap();
    initStorage();
    KV_UINT64 int_kv;
    KV_STRING string_kv;
    int int_kv_size = storage_data->uint64_kv_size();
    int string_kv_size = storage_data->string_kv_size();
    while(int_kv_size)
    {
        int_kv = storage_data->uint64_kv(--int_kv_size);
        int_map[int_kv.name()] = int_kv.value();
    }
    while(string_kv_size)
    {
        string_kv = storage_data->string_kv(--string_kv_size);
        string_map[string_kv.name()] = string_kv.value();
    }
    return true;
}
bool MagicMap::buildMap(const Raid * raid, int mib_type)
{
    clearMap();
    initRaid(mib_type);
    KV_UINT64 int_kv;
    KV_STRING string_kv;
    int int_kv_size = raid->uint64_kv_size();
    int string_kv_size = raid->string_kv_size();
    while(int_kv_size)
    {
        int_kv = raid->uint64_kv(--int_kv_size);
        int_map[int_kv.name()] = int_kv.value();
    }
    while(string_kv_size)
    {
        string_kv = raid->string_kv(--string_kv_size);
        string_map[string_kv.name()] = string_kv.value();
    }
    return true;
}
bool MagicMap::buildMap(const PD * pd, int mib_type)
{
    clearMap();
    initPD(mib_type);
    KV_UINT64 int_kv;
    KV_STRING string_kv;
    int int_kv_size = pd->uint64_kv_size();
    int string_kv_size = pd->string_kv_size();
    while(int_kv_size)
    {
        int_kv = pd->uint64_kv(--int_kv_size);
        int_map[int_kv.name()] = int_kv.value();
    }
    while(string_kv_size)
    {
        string_kv = pd->string_kv(--string_kv_size);
        string_map[string_kv.name()] = string_kv.value();
    }
    return true;
}
bool MagicMap::buildMap(const VD * vd, int mib_type)
{
    clearMap();
    initVD(mib_type);
    KV_UINT64 int_kv;
    KV_STRING string_kv;
    int int_kv_size = vd->uint64_kv_size();
    int string_kv_size = vd->string_kv_size();
    while(int_kv_size)
    {
        int_kv = vd->uint64_kv(--int_kv_size);
        int_map[int_kv.name()] = int_kv.value();
    }
    while(string_kv_size)
    {
        string_kv = vd->string_kv(--string_kv_size);
        string_map[string_kv.name()] = string_kv.value();
    }
    return true;
}
bool MagicMap::buildMap(const CollectData* data)
{
    clearMap();
    //将IPMI的FRU数据放入string_map中
    int fru_size = data->fru_data_size();
    string key;
    while(fru_size--)
    {
        //将key转化
        if(data->fru_data(fru_size).fru_name() == "Product Name")
        {
            key = "product_name";
        }
        else if(data->fru_data(fru_size).fru_name() == "Product Manufacturer")
        {
            key = "manufacturer";
        }
        else if(data->fru_data(fru_size).fru_name() == "Product Serial")
        {
            key = "product_serial";
        }
        else if(data->fru_data(fru_size).fru_name() == "Board Serial")
        {
            key = "board_serial";
        }
        string_map[key] = data->fru_data(fru_size).value();
    }
}
bool MagicMap::clearMap()
{
    string_map.clear();
    int_map.clear();
    return true;
}
unsigned long long int MagicMap::findIntKey(const string & des)
{
    if(int_map.find(des) != int_map.end())
        return int_map.find(des)->second;
    else
        return INT_DEFAULT;

}
string MagicMap::findStringKey(const string & des)
{
    if(string_map.find(des) != string_map.end())
        return string_map.find(des)->second;
    else
        return "NULL";
}
bool  MagicMap::parseInsertString(string * s1, string * s2)
{
    int int_kv_size = int_map.size(); 
    int string_kv_size = string_map.size();
    map<string, unsigned long long int>::iterator  int_it=int_map.begin();
    map<string, string>::iterator string_it=string_map.begin();
    if( int_kv_size == 0 && string_kv_size == 0)
        return false;
    int insert_count = 0;
    while(int_kv_size)
    {
        *s1 += "`";
        *s1 += int_it->first;
        *s1 += "`";
        if(int_it->second == INT_DEFAULT)//如果为缺省值，转化为NULL
            *s2 += "NULL";
        else
        {
            *s2 += convert<string>(int_it->second);
            insert_count++;
        }
        if(int_kv_size ==1 && string_kv_size==0)
        {
            //do nothing
        }
        else
        {
            *s1 += ",";
            *s2 += ",";
        }
        int_it++;
        int_kv_size--;         
    }
    while(string_kv_size)
    {
        *s1 += "`";
        *s1 += string_it->first;
        *s1 += "`";
 	    if(string_it->second != "NULL")
	    {
            *s2 += ("'"+string_it->second+"'");
            insert_count++;
	    }
        else
            *s2 += "NULL";
        if(string_kv_size==1)
        {
            //do nothing
        }
        else
        {
            *s1 += ",";
            *s2 += ",";
        }
        string_it++;
        string_kv_size--;
    }
    if(insert_count > 0)
        return true;
    else 
        return false;
}
bool MagicMap::parseUpdateString(string & s1)
{
    int int_kv_size = int_map.size(); 
    int string_kv_size = string_map.size();
    map<string, unsigned long long int>::iterator  int_it=int_map.begin();
    map<string, string>::iterator string_it=string_map.begin();
    int update_count = 0;
    if( int_kv_size == 0 && string_kv_size == 0)
        return false; 
    while(int_kv_size)
    {
        if(int_it->second != -1)
        {
            s1 += "`";
            s1 += int_it->first;
            s1 += "`=";
            if(int_it->second == INT_DEFAULT)//如果为缺省值，用NULL代替
                s1 += "NULL";
            else
            {
                update_count ++;
                s1 += convert<string>(int_it->second);
            }
            if(int_kv_size ==1 && string_kv_size==0)
            {
                //do nothing
            }
            else
            {
                s1 += ",";
            }
        }
        int_it++;
        int_kv_size--;         
    }
    while(string_kv_size)
    {
        if(string_it->second != "")
        {
            s1 += "`";
            s1 += string_it->first;
            s1 += "`=";
            if(string_it->second != "NULL") 
            {
                s1 += ("'"+string_it->second+"'");
                update_count ++;
            }
            else
                s1 += "NULL";
            if(string_kv_size==1)
            {
                //do nothing
            }
            else
            {
                s1 += ",";
            }
        }
        string_it++;
        string_kv_size--;
    }
    if(s1.size() > 1)
    {
        if( s1.substr(s1.size()-1, 1) == ",")
            s1 = s1.substr(0, s1.size()-1);
    }    
    if(update_count != 0)
        return true;
    else
        return false;
}
void MagicMap::setFilterKey(const string strs[], int size)
{
    for(int i=0; i<size; i++)
    {
       if(string_map.erase(strs[i]) == 0) 
            int_map.erase(strs[i]);
    }
}
void MagicMap::initServerPerform()
{
    int INT_COUNT = sizeof(SF_KEY_INT)/sizeof(SF_KEY_INT[0]);
    int STRING_COUNT = sizeof(SF_KEY_STRING)/sizeof(SF_KEY_STRING[0]);
    for(int i=0; i<INT_COUNT; i++)
    {
        int_map[SF_KEY_INT[i]] = INT_DEFAULT;
    }
    for(int j=0; j<STRING_COUNT; j++)
    {
        string_map[SF_KEY_STRING[j]] = "NULL";
    }
}
void MagicMap::initDiskIO()
{
    int INT_COUNT = sizeof(DISK_KEY_INT)/sizeof(DISK_KEY_INT[0]);
    int STRING_COUNT = sizeof(DISK_KEY_STRING)/sizeof(DISK_KEY_STRING[0]);
    for(int i=0; i<INT_COUNT; i++)
        int_map[DISK_KEY_INT[i]] = INT_DEFAULT;
    for(int j=0; j<STRING_COUNT; j++)
        string_map[DISK_KEY_STRING[j]] = "NULL";
}
void MagicMap::initStorage()
{
    int INT_COUNT = sizeof(STORAGE_KEY_INT)/sizeof(STORAGE_KEY_INT[0]);
    int STRING_COUNT = sizeof(STORAGE_KEY_STRING)/sizeof(STORAGE_KEY_STRING[0]);
    for(int i=0; i<INT_COUNT; i++)
        int_map[STORAGE_KEY_INT[i]] = INT_DEFAULT;
    for(int j=0; j<STRING_COUNT; j++)
        string_map[STORAGE_KEY_STRING[j]] = "NULL";
}
void MagicMap::initNetworkIO()
{
    int INT_COUNT = sizeof(NETWORK_KEY_INT)/sizeof(NETWORK_KEY_INT[0]);
    int STRING_COUNT = sizeof(NETWORK_KEY_STRING)/sizeof(NETWORK_KEY_STRING[0]);
    for(int i=0; i<INT_COUNT; i++)
        int_map[NETWORK_KEY_INT[i]] = INT_DEFAULT;
    for(int j=0; j<STRING_COUNT; j++)
        string_map[NETWORK_KEY_STRING[j]] = "NULL";
}
void MagicMap::initRaid(int mib_type)
{
    int INT_COUNT;
    int STRING_COUNT;
    switch(mib_type)
    {
        case 1:
            INT_COUNT = sizeof(IR_RAID_INT)/sizeof(IR_RAID_INT[0]);
            STRING_COUNT = sizeof(IR_RAID_STRING)/sizeof(IR_RAID_STRING[0]);
            for(int i=0; i<INT_COUNT; i++)
                int_map[IR_RAID_INT[i]] = INT_DEFAULT;
            for(int j=0; j<STRING_COUNT; j++)
                string_map[IR_RAID_STRING[j]] = "NULL";
            break;
        case 2:
            INT_COUNT = sizeof(N_RAID_INT)/sizeof(N_RAID_INT[0]);
            STRING_COUNT = sizeof(N_RAID_STRING)/sizeof(N_RAID_STRING[0]);
            for(int i=0; i<INT_COUNT; i++)
                int_map[N_RAID_INT[i]] = INT_DEFAULT;
            for(int j=0; j<STRING_COUNT; j++)
                string_map[N_RAID_STRING[j]] = "NULL";
            break;
        case 3:
            INT_COUNT = sizeof(HP_RAID_INT)/sizeof(HP_RAID_INT[0]);
            STRING_COUNT = sizeof(HP_RAID_STRING)/sizeof(HP_RAID_STRING[0]);
            for(int i=0; i<INT_COUNT; i++)
                int_map[HP_RAID_INT[i]] = INT_DEFAULT;
            for(int j=0; j<STRING_COUNT; j++)
                string_map[HP_RAID_STRING[j]] = "NULL";
            break;
        default:
            break;
    }
}
void MagicMap::initPD(int mib_type)
{
    int INT_COUNT;
    int STRING_COUNT;
    switch(mib_type)
    {
        case 1:
            INT_COUNT = sizeof(IR_PD_INT)/sizeof(IR_PD_INT[0]);
            STRING_COUNT = sizeof(IR_PD_STRING)/sizeof(IR_PD_STRING[0]);
            for(int i=0; i<INT_COUNT; i++)
                int_map[IR_PD_INT[i]] = INT_DEFAULT;
            for(int j=0; j<STRING_COUNT; j++)
                string_map[IR_PD_STRING[j]] = "NULL";
            break;
        case 2:
            INT_COUNT = sizeof(N_PD_INT)/sizeof(N_PD_INT[0]);
            STRING_COUNT = sizeof(N_PD_STRING)/sizeof(N_PD_STRING[0]);
            for(int i=0; i<INT_COUNT; i++)
                int_map[N_PD_INT[i]] = INT_DEFAULT;
            for(int j=0; j<STRING_COUNT; j++)
                string_map[N_PD_STRING[j]] = "NULL";
            break;
        case 3:
            INT_COUNT = sizeof(HP_PD_INT)/sizeof(HP_PD_INT[0]);
            STRING_COUNT = sizeof(HP_PD_STRING)/sizeof(HP_PD_STRING[0]);
            for(int i=0; i<INT_COUNT; i++)
                int_map[HP_PD_INT[i]] = INT_DEFAULT;
            for(int j=0; j<STRING_COUNT; j++)
                string_map[HP_PD_STRING[j]] = "NULL";
            break;
        default:
            break;
    }
}
void MagicMap::initVD(int mib_type)
{
    int INT_COUNT;
    int STRING_COUNT;
    switch(mib_type)
    {
        case 1:
            INT_COUNT = sizeof(IR_VD_INT)/sizeof(IR_VD_INT[0]);
            STRING_COUNT = sizeof(IR_VD_STRING)/sizeof(IR_VD_STRING[0]);
            for(int i=0; i<INT_COUNT; i++)
                int_map[IR_VD_INT[i]] = INT_DEFAULT;
            for(int j=0; j<STRING_COUNT; j++)
                string_map[IR_VD_STRING[j]] = "NULL";
            break;
        case 2:
            INT_COUNT = sizeof(N_VD_INT)/sizeof(N_VD_INT[0]);
            STRING_COUNT = sizeof(N_VD_STRING)/sizeof(N_VD_STRING[0]);
            for(int i=0; i<INT_COUNT; i++)
                int_map[N_VD_INT[i]] = INT_DEFAULT;
            for(int j=0; j<STRING_COUNT; j++)
                string_map[N_VD_STRING[j]] = "NULL";
            break;
        case 3:
            INT_COUNT = sizeof(HP_VD_INT)/sizeof(HP_VD_INT[0]);
            STRING_COUNT = sizeof(HP_VD_STRING)/sizeof(HP_VD_STRING[0]);
            for(int i=0; i<INT_COUNT; i++)
                int_map[HP_VD_INT[i]] = INT_DEFAULT;
            for(int j=0; j<STRING_COUNT; j++)
                string_map[HP_VD_STRING[j]] = "NULL";
            break;
        default:
            break;
    }
}
void MagicMap::setIntKey(string key, unsigned long long int value)
{
    int_map[key] = value;
}
void MagicMap::setStringKey(string key, string value)
{
    string_map[key] = value;
}
