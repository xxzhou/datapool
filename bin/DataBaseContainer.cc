#include"DataBaseContainer.h"
int* DataBaseContainer::cur_count = new int[METRIC_COUNT];
int* DataBaseContainer::thresholds = new int[METRIC_COUNT];
pthread_rwlock_t* DataBaseContainer::rw_lock = new pthread_rwlock_t[METRIC_COUNT];
vector<string>** DataBaseContainer::values_buffer = new vector<string>*[METRIC_COUNT];
map<string, int>* DataBaseContainer::table_id = new map<string,int>();
//allocate rwlock and buffer
void DataBaseContainer::allocResource()
{
    for(int i=0; i<METRIC_COUNT; i++)
    {
        pthread_rwlock_init(&rw_lock[i], NULL);
        //rw_lock[i] = PTHREAD_RWLOCK_INITIALIZER;
        values_buffer[i] = new vector<string>();
        cur_count[i] = 0;
    }
    (*table_id)["ir_controller"] = 0;
    (*table_id)["ir_pd"] = 1;
    (*table_id)["ir_vd"] = 2;
    (*table_id)["no_ir_controller"] = 3;
    (*table_id)["no_ir_pd"] = 4;
    (*table_id)["no_ir_vd"] = 5;
    (*table_id)["hp_controller"] = 6;
    (*table_id)["hp_pd"] = 7;
    (*table_id)["hp_vd"] = 8;
    (*table_id)["server_perform"] = 9;
    (*table_id)["disk_io"] = 10;
    (*table_id)["network_io"] = 11;
    (*table_id)["ssd"] = 12;
    (*table_id)["ipmi_raw"] = 13;
    (*table_id)["log"] = 14;
    (*table_id)["fru"] = 15;
    (*table_id)["hrStorage"] = 16;
}
//release resources 
void DataBaseContainer::releaseResource()
{
    delete []cur_count;
    delete []thresholds;
    for(int i=0; i<METRIC_COUNT; i++)
    {
        delete values_buffer[i];
        pthread_rwlock_destroy(&(rw_lock[i]));
    }
    delete []rw_lock;
    delete []values_buffer;
    delete table_id;
}
DataBaseContainer::DataBaseContainer(string ip, int port, string name, string passwd ,string db_name, int max_error_count, string backup_ip, int backup_port, string backup_name, string backup_passwd, string backup_db_name, int backup_max_error_count)
{
    common = new DataBaseProxy(ip, port, name, passwd, db_name, max_error_count);
    backup = new DataBaseProxy(backup_ip, backup_port, backup_name, backup_passwd, backup_db_name, backup_max_error_count);
}
DataBaseContainer::DataBaseContainer(const DataBaseContainer& other)
{
    common = new DataBaseProxy(*other.common);
    backup = new DataBaseProxy(*other.backup);
}
DataBaseContainer::~DataBaseContainer()
{
    delete common;
    delete backup;
}
void DataBaseContainer::setBufferThreshold(int ir_controller, int ir_pd, int ir_vd, int no_ir_controller, int no_ir_pd, int no_ir_vd, int hp_controller, int hp_pd, int hp_vd, int server_perform,int disk_io, int network_io, int ssd , int ipmi_raw, int log, int fru, int hrStorage)
{
    thresholds[0] = ir_controller;
    thresholds[1] = ir_pd;
    thresholds[2] = ir_vd;
    thresholds[3] = no_ir_controller;
    thresholds[4] = no_ir_pd;
    thresholds[5] = no_ir_vd;
    thresholds[6] = hp_controller;
    thresholds[7] = hp_pd;
    thresholds[8] = hp_vd;
    thresholds[9] = server_perform;
    thresholds[10] = disk_io;
    thresholds[11] = network_io;
    thresholds[12] = ssd;
    thresholds[13] = ipmi_raw;
    thresholds[14] = log;
    thresholds[15] = fru;
    thresholds[16] = hrStorage;
}
bool DataBaseContainer::isThreadSafe()
{
    if (DataBaseProxy::isThreadSafe())    
        return true;
    else
        return false;
}
bool DataBaseContainer::connect()
{
    common->connect();
    backup->connect();
    return true;
}
void DataBaseContainer::disconnect()
{
    common->disconnect();
    backup->disconnect();
}
MYSQL_RES* DataBaseContainer::db_query(const char* query)
{
    int errorno = 0;
    MYSQL_RES* result = common->db_query(query, errorno);
    if(result == NULL)
    {
        if(errorno == 1205 || errorno == 2003 || errorno == 2006)//if lock timeout or can't connect to mysql
            return backup->db_query(query, errorno);
        else
            return NULL;
    }
    else
        return result;
}
bool DataBaseContainer::db_insert(const char* query)
{
    int errorno = 0;
    bool  result = common->db_insert(query, errorno);
    if(!result)
    {
        if(errorno == 1205 || errorno == 2003 || errorno == 2006)//if lock timeout
            return backup->db_insert(query, errorno);
        else
            return false;
    }
    else
        return result;
}
bool DataBaseContainer::db_insert(const char* query, const char * table)
{
    string table_name = string(table);//convert const char* to string
    bool action = false;
    string sql;
    action = bufferValues(query, sql, table_name);
    if(action == false)
    {
        logging("DEBUG", "Batch insert thresholds not matched. %s", table_name.c_str());
        return true;
    }
    else
    {
        logging("DEBUG", "meet thresholds, table %s ", table_name.c_str());
        //logging("DEBUG", "sql: %s", sql.c_str());
        int errorno = 0;
        if(!common->db_insert(sql.c_str(), errorno))
        {
            logging("INFO", "insert error, errorno %d", errorno);
            if(errorno == 1205 || errorno == 2003 || errorno == 2006)
                return backup->db_insert(sql.c_str(), errorno);
            else
                return false;
        }
        else
        {
            logging("DEBUG","insert finished");
            return true;
        }
    }
}
bool DataBaseContainer::bufferValues(const char* values, string& sql, string& table)
{
    int table_index;
    map<string,int>::iterator it  = table_id->find(table);
    if( it == table_id->end() )
    {
        logging("ERROR","Table %s buffer not exists", table.c_str());
        return false;
    }
    else
        table_index = it->second;
    pthread_rwlock_wrlock(&rw_lock[table_index]);
    cur_count[table_index]++;
    values_buffer[table_index]->push_back(string(values));
    pthread_rwlock_unlock(&rw_lock[table_index]);
    if( cur_count[table_index] >=  thresholds[table_index])
    {
        logging("DEBUG", "cur %d ,thresh %d, table %s", cur_count[table_index], thresholds[table_index], table.c_str());
        //change the cur_count to 0, prevent another thread step in
        pthread_rwlock_wrlock(&rw_lock[table_index]);
        cur_count[table_index] = 0;
        pthread_rwlock_unlock(&rw_lock[table_index]);
        //
        pthread_rwlock_rdlock(&rw_lock[table_index]);
        string values = parseVectorToSQL(values_buffer[table_index]);
        pthread_rwlock_unlock(&rw_lock[table_index]);
        sql = "insert into " + getTableSchema(table_index) + " values " + values;
        //
        pthread_rwlock_wrlock(&rw_lock[table_index]);
        values_buffer[table_index]->clear();
        pthread_rwlock_unlock(&rw_lock[table_index]);
        return true;
    }
    else
    {
        logging("DEBUG", "Table %s count %d ", table.c_str(), cur_count[table_index]);
        return false;
    }
}
//parse data in vector into values string
string DataBaseContainer::parseVectorToSQL(vector<string>*  buffer)
{
    string values="";
    for(int i = 0; i<buffer->size(); i++)
    {
        values += (*buffer)[i];
        if(i != buffer->size() -1)
            values += ",";
    }
    return values;
}
string DataBaseContainer::getTableSchema(int table_id)
{
    string schema="NULL";
    switch(table_id)
    {
        case 0:
            schema = "ir_controller(server_id,collect_time,`controller_index`,`adapterID`,`alarmState`,`bgiRate`,`ccRate`,`dedicatedHotSpares`,`devInterfacePortCnt`,`flashSize`,`hostInterfacePortCnt`,`memorySize`,`nvramSize`,`patrolReadRate`,`pdCount`,`pdDiskFailedCount`,`pdDiskPredFailureCount`,`pdDiskPresentCount`,`pdPresentCount`,`rebuildRate`,`reconstructionRate`,`revertibleHotSpares`,`slotCount`,`vdDegradedCount`,`vdNumber`,`vdOfflineCount`,`vdPresentCount`,`devInterfacePortAddr`,`driverVersion`,`firmwareVersion`,`productName`,`sasAddr`)";
            break;
        case 1:
            schema="ir_pd(server_id,collect_time,`controller_index`,`index`,`diskType`,`linkSpeed`,`mediatype`,`mediaErrCount`,`otherErrCount`,`pdAdpID`,`pdState`,`phyPos`,`predFailCount`,`rawSize`,`slotNumber`,`connectedAdapterPort`,`pdFwversion`,`pdOperationProgress`,`pdSasAddr`,`pdSerialNumber`,`productID`,`vendorID`)";
            break;
        case 2:
            schema="ir_vd(server_id,collect_time,`controller_index`,`index`,`currentCachePolicy`,`numDrives`,`prl`,`sRL`,`size`,`state`,`stripeSize`,`vdAdpID`,`vdDiskCachePolicy`,`operationProgress`,`pdList`)";
            break;
        case 3:
            schema="no_ir_controller(server_id,collect_time,`controller_index`,`absStateOfCharge`,`adapterID-APT`,`alarmState`,`batteryReplacement`,`bbuNumber`,`bgiRate`,`ccRate`,`copyBackState`,`devInterfacePortCnt`,`hostInterface`,`memorySize`,`nextLearnTime`,`patrolReadRate`,`pdDiskFailedCount`,`pdDiskPredFailureCount`,`pdDiskPresentCount`,`pdNumber`,`pdPresentCount`,`rebuildRate`,`reconstructionRate`,`vdDegradedCount`,`vdNumbers`,`vdOfflineCount`,`vdPresentCount`,`adapterVendorID`,`bbuState`,`devID`,`devInterface`,`driverVersion`,`firmwareVersion`,`hardwarePresent`,`productName`,`serialNo`,`subDevID`,`subVendorID`)";
            break;
        case 4:
            schema="no_ir_pd(server_id,collect_time,`controller_index`,`index`,`adpID-PDT`,`disabledForRemoval`,`diskType`,`linkSpeed`,`mediaErrCount`,`mediatype`,`otherErrCount`,`pdIndex`,`pdState`,`physDevID`,`predFailCount`,`rawSize`,`slotNumber`,`connectedAdapterPort`,`operationProgress`,`pdFwversion`,`pdProductID`,`pdSerialNumber`,`pdVendorID`,`sasAddr`)";
            break;
        case 5:
            schema="no_ir_vd(server_id,collect_time,`controller_index`,`index`,`adapterID-VDT`,`bgiState`,`diskCachePolicy`,`numDrives`,`prl`,`sRL`,`size`,`state`,`stripeSize`,`virtualDevID`,`currentCachePolicy`,`pdList`)";
            break;
        case 6:
            schema="";
            break;
        case 7:
            break;
        case 8:
            break;
        case 9:
            schema="server_perform(server_id,collect_time,mem_usage,cpu_usage,`cpu_load`,`dskPercent`, `hrMemorySize`, `memAvailReal`,`memAvailSwap`,`memBuffer`,`memCached`,`memTotalFree`,`memTotalReal`,`memTotalSwap`,`uptime`)";
            break;
        case 10:
            schema="disk_io(server_id,collect_time,`index`,`disk_io_read`,`disk_io_rtimes`,`disk_io_write`,`disk_io_wtimes`,read_speed, write_speed)";
            break;
        case 11:
            schema="network_io(server_id,collect_time,`ifInDiscards`,`ifInErrors`,`ifInNUcastPkts`,`ifInOctets`,`ifInUcastPkts`,`ifInUnknownProtos`,`ifIndex`,`ifOutDiscards`,`ifOutErrors`,`ifOutNUcastPkts`,`ifOutOctets`,`ifOutQLen`,`ifOutUcastPkts`,`ifSpeed`, `in_speed`, `out_speed`)";
            break;
        case 12:
            schema="ssd(server_id, ssd, id, attribute_name, flag, value, worst, thresh, type, updated, when_failed, raw_value, collect_time)";
            break;
        case 13:
            schema="";
            break;
        case 14:
            schema="`log`(server_id,log_id,log_value,collect_time)";
            break;
        case 15:
            schema="fru(server_id, collect_time, board_serial, manufacturer, product_name, product_serial)";
            break;
        case 16:
            schema="hrStorage(server_id, collect_time, hrStorageAllocationFailures, hrStorageAllocationUnits,  hrStorageIndex, hrStorageSize, hrStorageUsed,use_percent)";
            break;
        default:
            logging("ERROR", "table %d dismatched", table_id);
            break;
    }
    return schema;
}
