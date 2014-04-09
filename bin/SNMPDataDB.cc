/*本文件用于处理 SNMP 协议采集数据的解析与存储
 *
 * */
#include"SNMPDataDB.h"
#include<iostream>
#include<stdio.h>
#include<mysql.h>
#include<stdint.h>
#include<pthread.h>
#include"Util.h"
const string PERFORM_STATUS[] = {
    "sysdescr",
    "cpuDescr",
    "cpuNum"
};
const string IF_STATUS[] = {
    "ifDescr",
    "ifType",
    "ifMtu",//the max transport bytes
    "ifPhysAddress",
    "ifAdminStatus",
    "ifOperStatus",
    "ifLastChange"
};
const string DISK_STATUS[] = {
    "diskIODevice", 
};
const string STORAGE_STATUS[] = {
    "hrStorageDescr",
};
using namespace std;

SNMPDataDB::SNMPDataDB(DataBaseContainer* db)
{
    this->db = db;
}
SNMPDataDB::~SNMPDataDB()
{
}
bool SNMPDataDB::insert(SNMPData * data)
{
    if(data == NULL)
    {
        logging("WARN","SNMP Data is null \n");
        return false;
    }
    //获取RAID MIB类型(1,2,3)
    int mib_type = data->mib_type();
    //获取服务器ID
    string server_id = data->server_id();
    //获取采集时间
    string collect_time = convert<string>(data->collect_time());
    //Raid_token用于存储Raid类型（ir, no_ir, hp）
    string raid_token;
    if( data->has_proc_data())//处理 SNMP采集的 proc 数据
    {
        logging("INFO", "proc data consumed");
        ProcData proc_data = data->proc_data();
        this->magic_map.buildMap(&proc_data);
        unsigned long long int cpu_load = this->magic_map.findIntKey("cpu_load");
        float cpu_usage = -255;
        int cpuNum = this->magic_map.findIntKey("cpuNum");
        if( cpuNum != INT_DEFAULT && cpu_load != INT_DEFAULT)//将cpu_load除以微内核个数,得到平均cpu负载
        {
            cpu_load = (int)(cpu_load/(float)cpuNum);
            this->magic_map.setIntKey("cpu_load", cpu_load);
        }
        if(cpu_load == INT_DEFAULT)//如果cpu_load缺省，cpu_usage也缺省
            cpu_usage = -255;
        else if (cpu_load > 100)//如果cpu_load大于100，cpu_usage=100
            cpu_usage = 100;
        else
            cpu_usage = cpu_load;
        unsigned long long int memTotalReal = this->magic_map.findIntKey("memTotalReal");//unit: kB
        unsigned long long int memAvailReal = this->magic_map.findIntKey("memAvailReal");//unit  kB
        float mem_usage= -255;
        if( memTotalReal != INT_DEFAULT  && memAvailReal != INT_DEFAULT  &&  memTotalReal != 0) // ensure memTotalReal, memAvailReal exists and valid
            mem_usage = ((float)(memTotalReal - memAvailReal))/((float)memTotalReal);//计算内存使用率
        updatePerformStatus(server_id, collect_time, &proc_data, mem_usage, cpu_usage);//更新 server_perform 最新值
        this->magic_map.setFilterKey(PERFORM_STATUS, 3); //将采集数据中的状态型数据过滤，不存入server_perform表中
        insertServerPerform(server_id, collect_time, &proc_data, mem_usage, cpu_usage);
        this->magic_map.clearMap();//清空工具类 magic_map

        int if_size = proc_data.if_info_size();
        unsigned long long int ifInOctets = 0;
        unsigned long long int ifOutOctets = 0;
        unsigned long long int total_in = 0;
        unsigned long long int total_out = 0;
        int if_index;
        map<int,unsigned long long int> ifIn;
        map<int,unsigned long long int> ifOut;
        map<int,unsigned long long int> if_time;
        float ifIn_speed = 0;
        float ifOut_speed = 0;
        bool result = findPreviousIFIO(server_id, ifIn, ifOut, if_time );
        while(if_size)//处理网卡IO信息
        {
            if_size--;
            IfData ifd = proc_data.if_info(if_size);
            this->magic_map.buildMap(&ifd);
            //计算网卡速度-开始
            if_index = this->magic_map.findIntKey("ifIndex");  
            if(if_index == INT_DEFAULT)//如果ifIndex为 NULL（该维度采集超时，无意义，跳过后续过程）
            {
                this->magic_map.clearMap();
                continue;
            }
            ifInOctets = this->magic_map.findIntKey("ifInOctets");
            ifOutOctets = this->magic_map.findIntKey("ifOutOctets");
            ifIn_speed = 0;
            ifOut_speed = 0;
            if(ifInOctets != INT_DEFAULT)//计算读速度, Mbps
            {
                if(result)
                {
                    if(ifIn.find(if_index) != ifIn.end())
                    {
                        if( (data->collect_time() - if_time[if_index] > 0) && (ifInOctets - ifIn[if_index] > 0))
                        {
                            ifIn_speed = (ifInOctets - ifIn[if_index])*8.0/(data->collect_time() - if_time[if_index])/1000000;
                            logging("DEBUG","server_id %s ifIndex %d if in_speed %f Mbps", server_id.c_str(), if_index, ifIn_speed);
                        }
                    }
                }
                total_in += ifInOctets;
            }
            if(ifOutOctets != INT_DEFAULT)//计算写速度, Mbps
            {
                if(result)
                {
                    if(ifOut.find(if_index) != ifOut.end())
                    {
                        if( (data->collect_time() > if_time[if_index] ) && (ifOutOctets > ifOut[if_index]))
                        {
                            ifOut_speed = (ifOutOctets - ifOut[if_index])*8.0/(data->collect_time() - if_time[if_index])/1000000;
                            logging("DEBUG","server_id %s ifIndex %d if out_speed %f Mbps", server_id.c_str(), if_index, ifOut_speed);
                        }
                    }
                }
                total_out += ifOutOctets;

            }
            //计算网卡速度-结束
            updateIfStatus(server_id, collect_time, this->magic_map.findIntKey("ifIndex"), &ifd, ifIn_speed, ifOut_speed);
            this->magic_map.setFilterKey(IF_STATUS, 7);//将状态性信息过滤
            insertIfData(server_id, collect_time, &ifd, ifIn_speed, ifOut_speed);
            this->magic_map.clearMap();
        }
        ifIn_speed = 0;
        ifOut_speed = 0;
        //计算全局读速度
        if(total_in > 0)
        {
            if(ifIn.find(TOTAL_IF_INDEX) != ifIn.end())
            {
                if( (data->collect_time() > if_time[TOTAL_IF_INDEX]) && (total_in > ifIn[TOTAL_IF_INDEX]))
                {
                    ifIn_speed = (total_in - ifIn[TOTAL_IF_INDEX])*8.0/(data->collect_time() - if_time[TOTAL_IF_INDEX])/1000000;
                    logging("DEBUG","server_id %s total if in_speed %f Mbps", server_id.c_str(), ifIn_speed);
                }
            }
        }
        //计算全局写速度
        if(total_out > 0)
        {
            if(ifOut.find(TOTAL_IF_INDEX) != ifOut.end())
            {
                if( (data->collect_time() > if_time[TOTAL_IF_INDEX] > 0) && (total_out > ifOut[TOTAL_IF_INDEX]))
                {
                    ifOut_speed = (total_out - ifOut[TOTAL_IF_INDEX])*8.0/(data->collect_time() - if_time[TOTAL_IF_INDEX])/1000000;
                    logging("DEBUG","server_id %s total if out_speed %f Mbps", server_id.c_str(), ifOut_speed);
                }
            }

        }
        int disk_size = proc_data.disk_info_size();
        unsigned long long int  disk_io_read;
        unsigned long long int  disk_io_write;
        unsigned long long int total_read;
        unsigned long long int total_write;
        map<int, unsigned long long int> diskIn;
        map<int, unsigned long long int> diskOut;
        map<int ,unsigned long long int> disk_time;
        result = findPreviousDISKIO(server_id, diskIn, diskOut, disk_time);
        int disk_index;
        float read_speed;
        float write_speed;
        while(disk_size)//处理磁盘IO信息
        {
            disk_size--;
            read_speed = 0;
            write_speed = 0;
            DiskData disk = proc_data.disk_info(disk_size);
            this->magic_map.buildMap(&disk);
            //judge if disk_io data is null， 过滤 空数据
            if(   ( this->magic_map.findIntKey("disk_io_read") == 0 || this->magic_map.findIntKey("disk_io_read") == INT_DEFAULT     )
                    &&( this->magic_map.findIntKey("disk_io_write")== 0 || this->magic_map.findIntKey("disk_io_write") == INT_DEFAULT    )
                    &&( this->magic_map.findIntKey("disk_io_rtimes") == 0 || this->magic_map.findIntKey("disk_io_rtimes") == INT_DEFAULT )
                    &&( this->magic_map.findIntKey("disk_io_wtimes") == 0 || this->magic_map.findIntKey("disk_io_wtimes") == INT_DEFAULT )
              )
            {
                //do nothing
            }
            else
            {
                disk_index = disk.index();
                disk_io_read = this->magic_map.findIntKey("disk_io_read");
                disk_io_write = this->magic_map.findIntKey("disk_io_write");
                if(disk_io_read != INT_DEFAULT) //计算读速度
                {
                    if(result)
                    {
                        if( (data->collect_time() > disk_time[disk_index]) && (disk_io_read > diskIn[disk_index]))
                        {
                            read_speed = (disk_io_read - diskIn[disk_index])*1.0/(data->collect_time()-disk_time[disk_index])/1024;      //read_speed with unit of KB/s
                            logging("DEBUG","server_id %s disk read_speed %f KB/s", server_id.c_str(), read_speed);
                        }
                    }
                    total_read += disk_io_read;
                }
                if(disk_io_write != INT_DEFAULT)//计算写速度
                {
                    if(result)
                    {
                        if((data->collect_time() > disk_time[disk_index])&&(disk_io_write > diskOut[disk_index]))
                        {
                            write_speed = (disk_io_write - diskOut[disk_index])*1.0/(data->collect_time() - disk_time[disk_index])/1024; //write speed with unit of KB/s
                            logging("DEBUG","server_id %s disk write_speed %f KB/s", server_id.c_str(), write_speed);
                        }

                    }
                    total_write += disk_io_write;
                }
                updateDiskStatus(server_id, collect_time, disk_index, &disk, read_speed, write_speed);
                this->magic_map.setFilterKey(DISK_STATUS, 1); //将采集数据中的状态型数据过滤        
                insertDiskData(server_id, collect_time, disk_index, &disk, read_speed, write_speed);
            }
            this->magic_map.clearMap();
        }
        /**
          read_speed = 0;
          write_speed = 0;
          if(diskIn.find(TOTAL_DISK_INDEX) != diskIn.end() && total_read > 0)
          {
          if((data->collect_time() > disk_time[TOTAL_DISK_INDEX]) && (total_read > diskIn[TOTAL_DISK_INDEX]))
          {
          read_speed = (total_read - diskIn[TOTAL_DISK_INDEX])*1.0/(data->collect_time() - disk_time[TOTAL_DISK_INDEX])/1024; // KB/s
          logging("DEBUG","server_id %s total disk read_speed %f KB/s", server_id.c_str(), read_speed);
          }
          }
          if(diskOut.find(TOTAL_DISK_INDEX) != diskOut.end() && total_write > 0)
          {
          if((data->collect_time() > disk_time[TOTAL_DISK_INDEX])&&(total_write > diskOut[TOTAL_DISK_INDEX]))
          {
          write_speed = (total_write - diskOut[TOTAL_DISK_INDEX])*1.0/(data->collect_time() - disk_time[TOTAL_DISK_INDEX])/1024;// KB/s
          logging("DEBUG","server_id %s total disk write_speed %f KB/s", server_id.c_str(), write_speed);
          }
          }                
        //存储Disk整体IO
        DiskData t_disk;
        KV_UINT64* kv1 = t_disk.add_uint64_kv();
        kv1->set_name("disk_io_read");
        kv1->set_value(total_read);
        KV_UINT64* kv2 = t_disk.add_uint64_kv();
        kv2->set_name("disk_io_write");
        kv2->set_value(total_write);
        updateDiskStatus(server_id, collect_time, TOTAL_DISK_INDEX, &t_disk, read_speed, write_speed);
        insertDiskData(server_id, collect_time, TOTAL_DISK_INDEX, &t_disk, read_speed, write_speed);
         **/

        //处理 hrStorage
        int storage_size = proc_data.storage_info_size();
        StorageData storage;
        float use_percent;
        unsigned long long int hrStorageSize;
        unsigned long long int hrStorageUsed;
        string hrStorageDescr;
        string storage_content = "(";
        while(storage_size--)
        {
            storage = proc_data.storage_info(storage_size);
            this->magic_map.buildMap(&storage);
            hrStorageSize = this->magic_map.findIntKey("hrStorageSize");
            hrStorageUsed = this->magic_map.findIntKey("hrStorageUsed");
            hrStorageDescr = this->magic_map.findStringKey("hrStorageDescr");
            if(hrStorageDescr != "NULL")
            {
                if(hrStorageDescr.find("\\") != string::npos)
                {
                    hrStorageDescr.replace(hrStorageDescr.find("\\"), 1, "/");
                    this->magic_map.setStringKey("hrStorageDescr", hrStorageDescr);
                }
                storage_content += ("'" + hrStorageDescr + "'");
                if(storage_size != 0)
                {
                    storage_content += ",";
                }
                else
                    storage_content += ")";
            }
            else
            {
                //如果为分区名为NULL, 不入库
                this->magic_map.clearMap();
                continue;
            }
            if(hrStorageSize > 0)
            {
                use_percent = (float)(hrStorageUsed)*100/hrStorageSize;
                if( use_percent > 100 )
                {
                    use_percent = 100;
                }
            }
            else
                use_percent = 0;
            updateStorageStatus(server_id, collect_time, hrStorageDescr, use_percent);
            this->magic_map.setFilterKey(STORAGE_STATUS, 1); //将采集数据中的状态型数据过滤        
            insertStorageData(server_id, collect_time,  use_percent);
            this->magic_map.clearMap();
        }
        if( proc_data.storage_info_size() > 0)
        {
            string storage_offline = "delete from hrStorage_status where server_id = " + server_id + " and hrStorageDescr not in " + storage_content;
            logging("INFO", storage_offline.c_str());
            this->db->db_insert(storage_offline.c_str());
        }
    }

    switch(mib_type)//处理磁盘Raid信息
    {
        case 1:
            raid_token= "ir";
            break;
        case 2:
            raid_token= "no_ir";
            break;
        case 3:
            raid_token= "hp";
            break;
            //no raid data
        case 255:
            return true;
    }
    int raid_size = data->raid_info_size();
    if(raid_size > 0)
        logging("INFO", "Raid data consumed");
    else//没有抓取到raid数据时，自动修改raid类型
    {
        if(data->has_proc_data())//能取到proc数据，说明目标服务器采集环境正常，而此时无法获得raid数据，说明raid_type不对
        {
            //reviseRaidType(server_id, mib_type);
        }
    }
    unsigned long long int controller_index;
    int pd_index;
    int vd_index;
    while(raid_size)
    {
        raid_size--;
        Raid raid = data->raid_info(raid_size);
        this->magic_map.buildMap(&raid, mib_type);
        switch(mib_type)
        {
            case 1://ir
                controller_index = this->magic_map.findIntKey("adapterID");
                break;
            case 2://no_ir
                controller_index = this->magic_map.findIntKey("adapterID-APT");
                logging("DEBUG","no_ir_controller index %d",controller_index);
                break;
            case 3://for hp, only one raid controller
                controller_index = 0;
                break;
            default:
                logging("ERROR", "Invalid mib_type");
        }
        updateControllerStatus(server_id, collect_time, controller_index, &raid, raid_token);
        insertControllerData(server_id, collect_time, controller_index, &raid, raid_token);
        this->magic_map.clearMap();
    }

    int pd_size = data->pd_info_size();
    int pd_count_db;
    bool ssd_exists ;
    bool need_update = true;
    if( data->has_ssd_data())//如果当前已经采集到ssd数据，可以断定SSD存在
    {
        ssd_exists = true;
        need_update = false;
    }
    else
        ssd_exists = false;
    int media_type = 0; //用来判断硬盘媒介类型： 0: HDD; 1: SSD;
    unsigned long long int slotNumber = 0;
    string pdSerialNumber;
    //
    if( pd_size > 0 )
    {
        string table_name;
        if( mib_type == 1)
            table_name = "ir_pd_status";
        else if( mib_type == 2)
            table_name = "no_ir_pd_status";
        else
            table_name = "hp_pd_status";
        string query_db = "select count(*) from " + table_name + " where server_id =" + server_id;
        MYSQL_RES* result = this->db->db_query(query_db.c_str());
        if(result == NULL)
            return false;
        int num = mysql_num_rows(result);
        if(num > 0)
        {
            MYSQL_ROW row = mysql_fetch_row(result);
            pd_count_db = atoi(row[0]);
            if( pd_count_db > pd_size)//如果数据库中pd记录数比此次采集更多，说明有pd下线
            {
                string set_pd = "update " + table_name + " set online_status=0 where server_id=" + server_id;
                this->db->db_insert(set_pd.c_str());
            }
        }
        mysql_free_result(result);
    }
    while(pd_size)
    {
        pd_size--;
        PD pd = data->pd_info(pd_size);
        this->magic_map.buildMap(&pd, mib_type);
        switch(mib_type)
        {
            case 1://ir
                controller_index = this->magic_map.findIntKey("pdAdpID");
                media_type= this->magic_map.findIntKey("mediatype");
                slotNumber = this->magic_map.findIntKey("slotNumber");
                pdSerialNumber = this->magic_map.findStringKey("pdSerialNumber");
                break;
            case 2://no_ir
                controller_index = this->magic_map.findIntKey("adpID-PDT");
                media_type= this->magic_map.findIntKey("mediatype");
                slotNumber = this->magic_map.findIntKey("slotNumber");
                pdSerialNumber = this->magic_map.findStringKey("pdSerialNumber");
                break;
            case 3://for hp, only one raid controller
                controller_index = 0;
                break;
            default:
                logging("ERROR", "Invalid mib_type");
        }
        pd_index = pd.index();
        if(slotNumber != INT_DEFAULT && pdSerialNumber != "NULL")//如果slotNumber 或 pdSerialNumber缺省，丢弃当前数据
        {
            updatePDStatus(server_id, collect_time, controller_index, pd_index, slotNumber, pdSerialNumber, &pd, raid_token);
            insertPDData(server_id, collect_time, controller_index, pd_index, &pd, raid_token);
        }
        //if not sure ssd exists, check it
        if( !ssd_exists)
        {
            if(media_type == 1)
                ssd_exists = true;
        }
        this->magic_map.clearMap();
    }
    //if server has ssd
    if(ssd_exists &&  need_update)
    {
        updateServerSSD(server_id);  
    }
    int vd_size = data->vd_info_size();
    int vd_count_db;
    string pdList;//根据 controller_index, pdList 判断 VD 是否唯一
    if( vd_size > 0)
    {
        string table_name;
        if( mib_type == 1)
            table_name = "ir_vd_status";
        else if( mib_type == 2)
            table_name = "no_ir_vd_status";
        else
            table_name = "hp_vd_status";
        string query_vd = "select count(*) from " + table_name + " where server_id=" + server_id;
        MYSQL_RES* result = this->db->db_query(query_vd.c_str());
        if(result == NULL)
            return false;
        int num = mysql_num_rows(result);
        if(num > 0)
        {
            MYSQL_ROW row = mysql_fetch_row(result);
            vd_count_db = atoi(row[0]);
            if( vd_count_db > vd_size)//如果数据库中pd记录数比此次采集更多，说明有pd下线
            {
                string set_vd = "update " + table_name + " set online_status=0 where server_id=" + server_id;
                this->db->db_insert(set_vd.c_str());
            }
        }
        mysql_free_result(result);
    }
    while(vd_size)
    {
        vd_size--;
        VD vd = data->vd_info(vd_size);
        this->magic_map.buildMap(&vd, mib_type);
        switch(mib_type)
        {
            case 1://ir
                controller_index = this->magic_map.findIntKey("vdAdpID");
                pdList = this->magic_map.findStringKey("pdList");
                break;
            case 2://no_ir
                controller_index = this->magic_map.findIntKey("adapterID-VDT");
                pdList = this->magic_map.findStringKey("pdList");
                break;
            case 3://for hp, only one raid controller
                controller_index = 0;
                break;
            default:
                logging("ERROR", "Invalid mib_type");
        }
        vd_index = vd.index();
        if(controller_index != INT_DEFAULT && pdList != "NULL")//如果 controller_index为空或者pdList为空，将数据丢弃.
        {
            updateVDStatus(server_id, collect_time, controller_index, vd_index, pdList, &vd, raid_token);
            insertVDData(server_id, collect_time, controller_index, vd_index, &vd, raid_token);
        }
        this->magic_map.clearMap();
    }
    return true;
}
string SNMPDataDB::getSQL(string table_controller, string table_pd, string table_vd, SNMPData* snmp_data)
{
    return NULL;
}

int SNMPDataDB::findPerformStatus(string server_id)
{
    int index_id;
    string find_sql = "select id from server_perform_status where server_id=" + server_id;
    MYSQL_RES* result = this->db->db_query(find_sql.c_str());
    MYSQL_ROW row = mysql_fetch_row(result);
    if(row)//return the idc_id of destination rec
    {
        index_id = atoi(row[0]);
        mysql_free_result(result);
        return index_id;
    }
    else//rec not exist
    {
        mysql_free_result(result);
        return -1;
    }
}
int SNMPDataDB::findIfStatus(string server_id, int if_index)
{
    int index_id;
    string find_sql = "select id from network_io_status where server_id=" + server_id + " and ifIndex=" + convert<string>(if_index);
    MYSQL_RES* result = this->db->db_query(find_sql.c_str());
    MYSQL_ROW row = mysql_fetch_row(result);
    if(row)//return the idc_id of destination idc
    {
        index_id = atoi(row[0]);
        mysql_free_result(result);
        return index_id;
    }
    else//idc not exists
    {
        mysql_free_result(result);
        return -1;
    }
}
int SNMPDataDB::findDiskStatus(string server_id, int disk_index)
{
    int index_id;
    string find_sql = "select id from disk_io_status where server_id=" + server_id + " and `index`=" + convert<string>(disk_index);
    MYSQL_RES* result = this->db->db_query(find_sql.c_str());
    MYSQL_ROW row = mysql_fetch_row(result);
    if(row)//return the idc_id of destination idc
    {
        index_id = atoi(row[0]);
        mysql_free_result(result);
        return index_id;
    }
    else//idc not exists
    {
        mysql_free_result(result);
        return -1;
    }
}
int SNMPDataDB::findStorageStatus(string server_id, string storage_desc)
{
    int index_id;
    string find_sql = "select id from hrStorage_status where server_id="
        + server_id 
        + " and hrStorageDescr='" + storage_desc + "'";
    MYSQL_RES* result = this->db->db_query(find_sql.c_str());
    MYSQL_ROW row = mysql_fetch_row(result);
    if(row)//return the id of destination storage 
    {
        index_id = atoi(row[0]);
        mysql_free_result(result);
        return index_id;
    }
    else//idc not exists
    {
        mysql_free_result(result);
        return -1;
    }

}
int SNMPDataDB::findControllerStatus(string server_id, unsigned long long int controller_index, string raid_token)
{
    if(controller_index == INT_DEFAULT)
    {
        logging("INFO","controller_index is NULL %s", __func__);
        return -1;
    }
    int index_id;
    string find_sql = "select id from " + raid_token + "_controller_status where server_id=" +  server_id + " and `controller_index`=" + convert<string>(controller_index);
    MYSQL_RES* result = this->db->db_query(find_sql.c_str());
    MYSQL_ROW row = mysql_fetch_row(result);
    if(row)//return the idc_id of destination idc
    {
        index_id = atoi(row[0]);
        mysql_free_result(result);
        return index_id;
    }
    else//idc not exists
    {
        mysql_free_result(result);
        return -1;
    }
}
int SNMPDataDB::findPDStatus(string server_id, unsigned long long int slotNumber, string pdSerialNumber, string raid_token)
{
    int index_id;
    string find_sql = "select id from " + raid_token + "_pd_status where server_id=" + server_id + " and `slotNumber`=" + convert<string>(slotNumber)
        + " and `pdSerialNumber`='" + pdSerialNumber + "'";
    //logging("INFO",find_sql.c_str());
    MYSQL_RES* result = this->db->db_query(find_sql.c_str());
    MYSQL_ROW row = mysql_fetch_row(result);
    if(row)//return the id 
    {
        index_id = atoi(row[0]);
        mysql_free_result(result);
        return index_id;
    }
    else//not exists
    {
        mysql_free_result(result);
        return -1;
    }
}
int SNMPDataDB::findVDStatus(string server_id, unsigned long long int controller_index, string pdList, string raid_token)
{
    int index_id;
    string find_sql = "select id from " + raid_token + "_vd_status where server_id=" + server_id 
        + " and `controller_index`=" + convert<string>(controller_index)
        + " and `pdList`='" + pdList + "'"; 
    MYSQL_RES* result = this->db->db_query(find_sql.c_str());
    MYSQL_ROW row = mysql_fetch_row(result);
    if(row)//return the id 
    {
        index_id = atoi(row[0]);
        mysql_free_result(result);
        return index_id;
    }
    else//not exist
    {
        mysql_free_result(result);
        return -1;
    }
}
int SNMPDataDB::updatePerformStatus(string server_id, string collect_time, const ProcData * proc_data, float mem_usage, float cpu_usage)
{
    int index_id = findPerformStatus(server_id);
    string sys_descr = magic_map.findStringKey("sysdescr");
    string os_type;
    string host_name;
    string arch;
    string kernal_version;
    parseSysDescr(sys_descr,os_type,host_name,arch,kernal_version);
    if(index_id == -1)
    {
        string s1,s2;
        if(magic_map.parseInsertString(&s1, &s2))
        {
            string insert_sql = "insert into server_perform_status(server_id,mem_usage,cpu_usage,os_type,host_name,arch,kernal_version," + s1 + ",collect_time) values(" + server_id + ","+(mem_usage == -255 ? "NULL":convert<string>(mem_usage)) + "," +(cpu_usage==-255? "NULL":convert<string>(cpu_usage))+",'"+os_type+"','"+host_name+"','"+arch+"','"+kernal_version+"',"+s2 + ",FROM_UNIXTIME(" + collect_time + "))";
            if(! (this->db->db_insert(insert_sql.c_str())))
            {
                logging("ERROR", "server_perform_status insert error"); 
                index_id = -1;
            }
            else
            {
                logging("DEBUG","server_perform_status insert finished");
                MYSQL_RES* res =  this->db->db_query("select LAST_INSERT_ID()");
                index_id  = atoi(mysql_fetch_row(res)[0]);
                mysql_free_result(res);
            }
        }
    }
    else
    {
        string temp;
        if(magic_map.parseUpdateString(temp))
        {
            string update_sql = "update server_perform_status set mem_usage="+(mem_usage == -255? "NULL":convert<string>(mem_usage))+",collect_time=FROM_UNIXTIME("+collect_time+"),os_type='"+os_type+"',host_name='"+host_name+"',arch='"+arch+"',kernal_version='"+kernal_version+"'," + temp + " where id=" + convert<string>(index_id);
            if(this->db->db_insert(update_sql.c_str()))
            {
                logging("DEBUG","update server_perform_status finished");
            }
            else
            {
                logging("ERROR", "update server_perform_status failed");
                index_id = -1;
            }
        }
    }
    return index_id;
}
int SNMPDataDB::updateIfStatus(string server_id, string collect_time, int if_index, const IfData* if_data, float ifIn_speed, float ifOut_speed)
{
    int index_id = findIfStatus(server_id, if_index);
    if(index_id == -1)
    {
        string s1,s2;
        if( magic_map.parseInsertString(&s1, &s2))
        {
            string insert_sql = "insert into network_io_status(server_id," + s1 + ",collect_time, in_speed, out_speed) values(" + server_id + "," + s2 + ",FROM_UNIXTIME(" + collect_time + "),"+ convert<string>(ifIn_speed)+ "," +convert<string>(ifOut_speed) + ")";
            //logging("DEBUG", insert_sql.c_str());
            if(! (this->db->db_insert(insert_sql.c_str())))
            {
                logging("ERROR", "network_io_status insert error"); 
                index_id = -1;
            }
            else
            {
                logging("DEBUG","network_io_status insert finished");
                MYSQL_RES* res =  this->db->db_query("select LAST_INSERT_ID()");
                index_id  = atoi(mysql_fetch_row(res)[0]);
                mysql_free_result(res);
            }
        }
    }
    else
    {
        string temp;
        if(magic_map.parseUpdateString(temp))
        {
            string update_sql = "update network_io_status set collect_time=FROM_UNIXTIME("+collect_time +"),in_speed=" + convert<string>(ifIn_speed) + ",out_speed=" + convert<string>(ifOut_speed) + "," + temp + " where id=" + convert<string>(index_id);
            //logging("DEBUG", update_sql.c_str());
            if(this->db->db_insert(update_sql.c_str()))
            {
                logging("DEBUG","update network_io_status finished");
            }
            else
            {
                logging("ERROR", "update network_io_status failed");
                index_id = -1;
            }
        }
    }
    return index_id;
}
int SNMPDataDB::updateDiskStatus(string server_id, string collect_time, int disk_index, const DiskData* disk_data, float read_speed, float write_speed)
{
    int index_id = findDiskStatus(server_id, disk_index);
    if(index_id == -1)
    {
        string s1,s2;
        if( magic_map.parseInsertString(&s1, &s2))
        {
            string insert_sql = "insert into disk_io_status(server_id,`index`," + s1 + ",collect_time, read_speed, write_speed) values(" + server_id + "," + convert<string>(disk_index) + ","+ s2 + ",FROM_UNIXTIME(" + collect_time + "),"+ convert<string>(read_speed) + "," + convert<string>(write_speed) +")";
            if(! (this->db->db_insert(insert_sql.c_str())))
            {
                logging("ERROR", "disk_io_status insert error"); 
                index_id = -1;
            }
            else
            {
                logging("DEBUG","disk_io_status insert finished");
                MYSQL_RES* res =  this->db->db_query("select LAST_INSERT_ID()");
                index_id  = atoi(mysql_fetch_row(res)[0]);
                mysql_free_result(res);
            }
        }
    }
    else
    {
        string temp;
        if(magic_map.parseUpdateString(temp))
        {
            string update_sql = "update disk_io_status set collect_time=FROM_UNIXTIME("+collect_time+")," + temp + ",read_speed=" + convert<string>(read_speed) + ",write_speed=" + convert<string>(write_speed) + " where id=" + convert<string>(index_id);
            if(this->db->db_insert(update_sql.c_str()))
            {
                logging("DEBUG","update disk_io_status finished");
            }
            else
            {
                logging("ERROR", "update disk_io_status failed");
                index_id = -1;
            }
        }
    }
    return index_id;
}
int SNMPDataDB::updateStorageStatus(string server_id, string collect_time, string storage_desc,  float use_percent)
{
    int index_id = findStorageStatus(server_id, storage_desc);
    if(index_id == -1)
    {
        string s1,s2;
        if( magic_map.parseInsertString(&s1, &s2))
        {
            string insert_sql = "insert into hrStorage_status(server_id, " + s1 + ",collect_time, use_percent) values(" + server_id + "," + s2 + ",FROM_UNIXTIME(" + collect_time + "),"+ convert<string>(use_percent) + ")";
            if(! (this->db->db_insert(insert_sql.c_str())))
            {
                logging("ERROR", "hrStorage_status insert error"); 
                index_id = -1;
            }
            else
            {
                logging("DEBUG","hrStorage_status insert finished");
                MYSQL_RES* res =  this->db->db_query("select LAST_INSERT_ID()");
                index_id  = atoi(mysql_fetch_row(res)[0]);
                mysql_free_result(res);
            }
        }
    }
    else
    {
        string temp;
        if(magic_map.parseUpdateString(temp))
        {
            string update_sql = "update hrStorage_status set collect_time=FROM_UNIXTIME("+collect_time+")," + temp + ", use_percent=" + convert<string>(use_percent) + " where id=" + convert<string>(index_id);
            if(this->db->db_insert(update_sql.c_str()))
            {
                logging("DEBUG","update hrStorage_status finished");
            }
            else
            {
                logging("ERROR", "update hrStorage_status failed");
                index_id = -1;
            }
        }
    }
    return index_id;

}
int SNMPDataDB::updateControllerStatus(string server_id, string collect_time, unsigned long long int controller_index, const Raid* raid, string raid_token)
{
    if(controller_index == INT_DEFAULT)
    {
        logging("INFO","controller_index is NULL %s", __func__);
        return -1;
    }
    int index_id = findControllerStatus(server_id, controller_index, raid_token);
    if(index_id == -1)
    {
        string s1,s2;
        if( magic_map.parseInsertString(&s1, &s2))
        {
            string insert_sql = "insert into " + raid_token +"_controller_status(server_id,`controller_index`," + s1 + ",collect_time) values(" + server_id + "," + convert<string>(controller_index) + ","+ s2 + ",FROM_UNIXTIME(" + collect_time + "))";
            if(! (this->db->db_insert(insert_sql.c_str())))
            {
                logging("ERROR", "controller_status insert error"); 
                index_id = -1;
            }
            else
            {
                logging("DEBUG","controller_status insert finished");
                MYSQL_RES* res =  this->db->db_query("select LAST_INSERT_ID()");
                index_id  = atoi(mysql_fetch_row(res)[0]);
                mysql_free_result(res);
            }
        }
        else
        {
            logging("ERROR","UpdateControllerStatus:magic map parsing insert data error");
        }
    }
    else
    {
        string temp;
        if(magic_map.parseUpdateString(temp))
        {
            string update_sql = "update " + raid_token + "_controller_status set collect_time=FROM_UNIXTIME("+collect_time+")," + temp + " where id=" + convert<string>(index_id);
            if(this->db->db_insert(update_sql.c_str()))
            {
                logging("DEBUG","update controller_status finished");
            }
            else
            {
                logging("ERROR", "update controller_status failed");
                index_id = -1;
            }
        }
        else
        {
            logging("ERROR","UpdateControllerStatus:magic map parsing update data error");
        }
    }
    return index_id;
}
int SNMPDataDB::updatePDStatus(string server_id, string collect_time,unsigned long long  int controller_index, int pd_index, unsigned long long int slotNumber, string pdSerialNumber, const PD* pd, string raid_token)
{
    if(controller_index == INT_DEFAULT)
    {
        logging("INFO","controller_index is NULL %s", __func__);
        return -1;
    }
    int index_id = findPDStatus(server_id, slotNumber, pdSerialNumber, raid_token);
    if(index_id == -1)
    {
        string s1,s2;
        if( magic_map.parseInsertString(&s1, &s2))
        {
            string insert_sql = "insert into " + raid_token +"_pd_status(server_id,`controller_index`,`index`," + s1 + ",collect_time) values(" + server_id + "," + (controller_index == INT_DEFAULT? "NULL":convert<string>(controller_index)) + "," + (pd_index == INT_DEFAULT? "NULL":convert<string>(pd_index)) + "," + s2 + ",FROM_UNIXTIME(" + collect_time + "))";
            if(! (this->db->db_insert(insert_sql.c_str())))
            {
                logging("ERROR", "pd_status insert error"); 
                index_id = -1;
            }
            else
            {
                logging("DEBUG","pd_status insert finished");
                MYSQL_RES* res =  this->db->db_query("select LAST_INSERT_ID()");
                index_id  = atoi(mysql_fetch_row(res)[0]);
                mysql_free_result(res);
            }
        }
    }
    else
    {
        string temp;
        if(magic_map.parseUpdateString(temp))
        {
            string update_sql = "update " + raid_token + "_pd_status set collect_time=FROM_UNIXTIME("+collect_time+")," + temp + ",online_status=1  where id=" + convert<string>(index_id);
            if(this->db->db_insert(update_sql.c_str()))
            {
                logging("DEBUG","update pd_status finished");
            }
            else
            {
                logging("ERROR", "update pd_status failed");
                index_id = -1;
            }
        }
    }
    return index_id;
}
int SNMPDataDB::updateVDStatus(string server_id, string collect_time,unsigned long long  int controller_index, int vd_index, string pdList, const VD* vd, string raid_token)
{
    int index_id = findVDStatus(server_id, controller_index, pdList, raid_token);
    if(index_id == -1)
    {
        string s1,s2;
        if( magic_map.parseInsertString(&s1, &s2))
        {
            string insert_sql = "insert into " + raid_token +"_vd_status(server_id,`controller_index`,`index`," + s1 + ",collect_time) values(" + server_id + "," + convert<string>(controller_index) + "," + convert<string>(vd_index) + "," + s2 + ",FROM_UNIXTIME(" + collect_time + "))";
            if(! (this->db->db_insert(insert_sql.c_str())))
            {
                logging("ERROR", "vd_status insert error"); 
                index_id = -1;
            }
            else
            {
                logging("DEBUG","vd_status insert finished");
                MYSQL_RES* res =  this->db->db_query("select LAST_INSERT_ID()");
                index_id  = atoi(mysql_fetch_row(res)[0]);
                mysql_free_result(res);
            }
        }
    }
    else
    {
        string temp;
        if(magic_map.parseUpdateString(temp))
        {
            string update_sql = "update " + raid_token + "_vd_status set collect_time=FROM_UNIXTIME("+collect_time+")," + temp + ",online_status=1 where id=" + convert<string>(index_id);
            if(this->db->db_insert(update_sql.c_str()))
            {
                logging("DEBUG","update vd_status finished");
            }
            else
            {
                logging("ERROR", "update vd_status failed");
                index_id = -1;
            }
        }
    }
    return index_id;
}
bool SNMPDataDB::insertServerPerform(string server_id, string collect_time, const ProcData * proc_data, float mem_usage, float cpu_usage)
{
    string s1, s2;
    if(magic_map.parseInsertString(&s1, &s2))
    {
        string insert_sql = "(" + server_id + ", FROM_UNIXTIME(" + collect_time + "),"+ (mem_usage == -255? "NULL":convert<string>(mem_usage)) +","+(cpu_usage == -255? "NULL":convert<string>(cpu_usage))+ "," + s2 + ")";
        if(this->db->db_insert(insert_sql.c_str(), "server_perform"))
        {
            logging("DEBUG","insert server_perform finished");
            return true;
        }
        else
        {
            logging("ERROR", "insert server_perform failed");
            return false;
        } 
    }
    return true;
}
bool SNMPDataDB::insertIfData(string server_id, string collect_time,  const IfData * if_data, float in_speed, float out_speed)
{
    string s1, s2;
    if(magic_map.parseInsertString(&s1, &s2))
    {
        string insert_sql = "(" + server_id + ", FROM_UNIXTIME(" + collect_time + ")," + s2 + ","+ convert<string>(in_speed) + "," + convert<string>(out_speed) + ")";
        if(this->db->db_insert(insert_sql.c_str(), "network_io"))
        {
            logging("DEBUG","insert network_io finished");
            return true;
        }
        else
        {
            logging("ERROR", "insert network_io failed");
            return false;
        } 
    }
    return true;
}
bool SNMPDataDB::insertDiskData(string server_id, string collect_time, int disk_index,  const DiskData * disk_data, float read_speed, float write_speed)
{
    string s1, s2;
    if(magic_map.parseInsertString(&s1, &s2))
    {
        string insert_sql = "(" + server_id + ", FROM_UNIXTIME(" + collect_time + ")," +convert<string>(disk_index) +","+ s2 + "," + convert<string>(read_speed) + "," + convert<string>(write_speed) +")";
        if(this->db->db_insert(insert_sql.c_str(), "disk_io"))
        {
            logging("DEBUG","insert disk_io finished");
            return true;
        }
        else
        {
            logging("ERROR", "insert disk_io failed");
            return false;
        } 
    }
    return true;
}
bool SNMPDataDB::insertStorageData(string server_id, string collect_time,  float use_percent)
{
    string s1, s2;
    if(magic_map.parseInsertString(&s1, &s2))
    {
        string insert_sql = "(" + server_id + ", FROM_UNIXTIME(" + collect_time + ")," + s2 + "," + convert<string>(use_percent) + ")";
        if(this->db->db_insert(insert_sql.c_str(), "hrStorage"))
        {
            logging("DEBUG","insert hrStorage finished");
            return true;
        }
        else
        {
            logging("ERROR", "insert hrStorage failed");
            return false;
        } 
    }
    return true;

}
bool SNMPDataDB::insertControllerData(string server_id, string collect_time,unsigned long long  int controller_index,  const Raid * raid, string raid_token)
{
    if(controller_index == INT_DEFAULT)
    {
        logging("INFO","controller_index is NULL %s", __func__);
        return false;
    }
    string s1, s2;
    if(magic_map.parseInsertString(&s1, &s2))
    {
        logging("INFO", s1.c_str());
        string insert_sql = "(" + server_id + ", FROM_UNIXTIME(" + collect_time + ")," +convert<string>(controller_index) +","+ s2 + ")";
        if(this->db->db_insert(insert_sql.c_str(), (raid_token + "_controller").c_str()))
        {
            logging("DEBUG","insert controller finished");
            return true;
        }
        else
        {
            logging("ERROR", "insert controller failed");
            return false;
        } 
    }
    return true;
}
bool SNMPDataDB::insertPDData(string server_id, string collect_time, unsigned long long  int controller_index, int pd_index,  const PD * pd, string raid_token)
{
    if(controller_index == INT_DEFAULT)
    {
        logging("INFO","controller_index is NULL %s", __func__);
        return false;
    }
    string s1, s2;
    if(magic_map.parseInsertString(&s1, &s2))
    {
        string insert_sql = "(" + server_id + ", FROM_UNIXTIME(" + collect_time + ")," +convert<string>(controller_index) +","+ convert<string>(pd_index) + "," + s2 + ")";
        if(this->db->db_insert(insert_sql.c_str(),(raid_token+"_pd").c_str()))
        {
            logging("DEBUG","insert pd finished");
            return true;
        }
        else
        {
            logging("ERROR", "insert pd failed");
            return false;
        } 
    }
    return true;
}
bool SNMPDataDB::insertVDData(string server_id, string collect_time, unsigned long long int controller_index, int vd_index,  const VD * vd, string raid_token)
{
    if(controller_index == INT_DEFAULT)
    {
        logging("INFO","controller_index is NULL %s", __func__);
        return false;
    }
    string s1, s2;
    if(magic_map.parseInsertString(&s1, &s2))
    {
        string insert_sql = "(" + server_id + ", FROM_UNIXTIME(" + collect_time + ")," +convert<string>(controller_index) +","+ convert<string>(vd_index) + "," + s2 + ")";
        if(this->db->db_insert(insert_sql.c_str(), (raid_token + "_vd").c_str()))
        {
            logging("DEBUG","insert vd finished");
            return true;
        }
        else
        {
            logging("ERROR", "insert vd failed");
            return false;
        } 
    }
    return true;
}
/*To parse sysDescr*/
//Linux zw_77_222 2.6.32-131.0.15.el6.x86_64 #1 SMP Tue May 10 15:42:40 EDT 2011 x86_64
//Hardware: x86 Family 6 Model 15 Stepping 6 AT/AT COMPATIBLE - Software: Windows Version 5.2 (Build 3790 Multiprocessor Free)
bool SNMPDataDB::parseSysDescr(string& sys_descr, string& os_type, string& host_name, string& arch, string& kernal)
{
    if (sys_descr.size() == 0 || sys_descr == "NONE")
    {
        logging("ERROR","sysdescr is empty");
        return false;
    }
    const int n = sys_descr.size();
    int start = 0;
    int count = 0;
    if(sys_descr[0] == 'L')//Linux
    {
        os_type = "Linux";
        for(int i=0;i<n;)
        {
            while((i<n) && (sys_descr[i]==' '))//split the blank prefix
                i++;
            start=i;
            while((i<n) && ( sys_descr[i]!=' '))
                i++;
            if(start<i)
            {
                count++;
                if(count == 2)
                    host_name = sys_descr.substr(start, i-start);
                if(count == 3)
                    kernal = sys_descr.substr(start, i-start);
                //arch is the last word
                arch = sys_descr.substr(start, i-start);
            }
        }
    }
    else if(sys_descr[0] == 'H')//Windows
    {
        os_type = "Windows";
        host_name = "NULL";
        bool flag = false;
        for(int i=0;i<n;)
        {
            while((i<n) && (sys_descr[i]==' '))//split the blank prefix
                i++;
            start=i;
            while((i<n) && ( sys_descr[i]!=' '))
                i++;
            if(start<i)
            {
                count++;
                if(count == 2)
                    arch = sys_descr.substr(start, i-start);
                if( sys_descr.substr(start, i-start) == "Version" )
                {
                    flag = true;
                    continue;
                }
                if(flag)
                {
                    kernal = "Windows Version " + sys_descr.substr(start,i-start);
                    flag = false;
                }

            }
        }
    }
    else//type not supported
    {
        logging("ERROR","sysdescr type not supported %s", sys_descr.c_str());
        return false;
    }
    return true;
}
bool SNMPDataDB::updateServerSSD(string server_id)
{
    string sql = "update server set ssd_avai=1, ssd_freq=600, auto_update=1 where server_id=" + server_id ; 
    if(this->db->db_insert(sql.c_str()))
    {
        logging("INFO","context awared, server %s has ssd", server_id.c_str());        
        return true;
    }
    else
    {
        logging("ERROR","server %s has ssd ,updated failed", server_id.c_str());
        return false;
    }
}
bool SNMPDataDB::reviseRaidType(string server_id, int raid_type)
{
    string sql;
    if(raid_type == 1)//从IR类型转换为NO_IR类型
    {
        sql = "select id from ir_constroller_status where server_id=" + server_id;
        raid_type = 2;
    }
    else if(raid_type ==2)//将NO_IR类型转换为IR类型
    {
        sql = "select id from no_ir_controller_status where server_id=" + server_id;
        raid_type = 1;
    }
    else//raid_type=3 not supported (HP Raid) 
        return true;
    MYSQL_RES * res = this->db->db_query(sql.c_str());//查看历史记录，是否采集到数据
    if(res == NULL || mysql_num_rows(res) > 0)//如果有历史数据，不执行raid_type适配
    {
        mysql_free_result(res);
        return false;
    }
    sql = "update server set raid_type="+ convert<string>(raid_type) + ", raid_avai=1, raid_freq=600, auto_update=1 where server_id="+server_id;
    if(this->db->db_insert(sql.c_str()))
    {
        logging("INFO","raid_type awared, server %s change mib_type to %d", server_id.c_str(), raid_type);        
        return true;
    }
    else
    {
        logging("ERROR","raid_type awared, server %s mib_type updated failed", server_id.c_str());
        return false;
    }
}
bool SNMPDataDB::findPreviousIFIO(string& server_id, map<int ,unsigned long long int>& ifIn, map<int, unsigned long long int>& ifOut, map<int, unsigned long long int>& if_time )
{
    string sql = "select ifIndex, ifInOctets, ifOutOctets, UNIX_TIMESTAMP(collect_time) from network_io_status where server_id=" + server_id ;
    MYSQL_RES* result = this->db->db_query(sql.c_str());
    if(result != NULL)
    {
        MYSQL_ROW row;
        int cur_index;
        while( (row = mysql_fetch_row(result)) != NULL)
        {
            if(row[0] != NULL)
            {
                cur_index = atoi(row[0]);
                if(cur_index == TOTAL_IF_INDEX) // 下表255代表服务器网卡总IO
                {
                    if(row[1] != NULL)
                        ifIn[TOTAL_IF_INDEX] = strtoull(row[1], NULL ,10);
                    if(row[2] != NULL)
                        ifOut[TOTAL_IF_INDEX] = strtoull(row[2], NULL, 10);
                }
                else
                {
                    if(row[1] != NULL)
                        ifIn[cur_index] = strtoull(row[1], NULL, 10);
                    if(row[2] != NULL)
                        ifOut[cur_index] = strtoull(row[2], NULL, 10);
                }
                if( row[3] != NULL)
                    if_time[cur_index] = strtoull(row[3], NULL, 10);
            }
        }
        mysql_free_result(result);
        return true;
    }
    else
        return false;
}
bool SNMPDataDB::findPreviousDISKIO(string& server_id, map<int ,unsigned long long int>& diskIn, map<int, unsigned long long int>& diskOut, map<int, unsigned long long int>& disk_time )
{
    string sql = "select `index`, disk_io_read, disk_io_write, UNIX_TIMESTAMP(collect_time) from disk_io_status where server_id=" + server_id ;
    MYSQL_RES* result = this->db->db_query(sql.c_str());
    if(result != NULL)
    {
        MYSQL_ROW row;
        int cur_index;
        while( (row = mysql_fetch_row(result)) != NULL )
        {
            if(row[0] != NULL)
            {
                cur_index = atoi(row[0]);
                if(cur_index == TOTAL_DISK_INDEX) // 总IO
                {
                    if(row[1] != NULL)
                        diskIn[TOTAL_DISK_INDEX] = strtoull(row[1], NULL ,10);
                    if(row[2] != NULL)
                        diskOut[TOTAL_DISK_INDEX] = strtoull(row[2], NULL, 10);
                }
                else
                {
                    if(row[1] != NULL)
                        diskIn[cur_index] = strtoull(row[1], NULL, 10);
                    if(row[2] != NULL)
                        diskOut[cur_index] = strtoull(row[2], NULL, 10);
                }
                if( row[3] != NULL)
                    disk_time[cur_index] = strtoull(row[3], NULL, 10);
            }
        }
        if(diskIn.find(TOTAL_DISK_INDEX) == diskIn.end())//如果没有，初始化
        {
            diskIn[TOTAL_DISK_INDEX] = 0;
        }
        mysql_free_result(result);
        return true;
    }
    else
        return false;

}
