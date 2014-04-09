#include"IPMIRaw.h"
#include<iostream>
#include<stdio.h>
#include<mysql.h>
#include<stdint.h>
#include"Util.h"
using namespace std;
IPMIRaw::IPMIRaw(DataBaseContainer * db)
{
    this->db = db;
}
IPMIRaw::~IPMIRaw()
{
}
SensorMap* IPMIRaw::sensor_map = NULL;
void IPMIRaw::setSender(AsyncDataSender* src)
{
    this->sender = src;
}
void IPMIRaw::setSensorMap(SensorMap* arg)
{
    IPMIRaw::sensor_map = arg;
}
bool IPMIRaw::insert(CollectData * data)
{
    if(data == NULL)
        return false;
    //如果有错误码，返回
    if(data->has_error_code())
    {
        logging("INFO","server %s collect ipmi error, errno: %d", data->server_id().c_str(), data->error_code());
        return true;
    }
    //处理FRU 数据
    insertFRU(data);
    //处理传感器相关数据
    map<string, float>* raw_map = new map<string, float>();
    for(int i=0; i<data->value_data_size(); i++)
    {
        ValueData vd = data->value_data(i);
        (*raw_map)[vd.sdr_name()+","+convert<string>(vd.sdr_num())] = vd.value();
    }
    if( raw_map->size() == 0)
    {
        logging("INFO","ipmi raw  data is empty");
        delete raw_map;
        return true;
    }
    map<string, string>* sensor_map = new map<string,string>();
    bool is_map_initialized = false;
    if(data->server_type() != 0)
    {
        is_map_initialized = IPMIRaw::sensor_map->getMap(data->server_type(),&sensor_map);
        //if map has not been initialized
        if( !is_map_initialized)
        {
            this->sendGet();
            delete raw_map;
            delete sensor_map;
            return false;
        }
    }
    else
    {
        logging("Error","server_type missing");
        delete raw_map;
        delete sensor_map;
        return false;
    }
    //Initialize mapping for new server_type
    if( sensor_map->size() == 0 )
    {
        logging("INFO","Initialize mapping for server_type %u \n", data->server_type());
        this->addMapping(raw_map, data->server_type());
        delete raw_map;
        delete sensor_map;
        return true;
    }
    string server_id = convert<string>(data->server_id());
    string update_sql = "update ipmi_raw_status set ";
    string prefix = "(server_id,";
    string values = " values(";
    values += server_id;
    values += ",";
    map<string, float>::const_iterator it = raw_map->begin();
    set<string> uncovered;
    logging("DEBUG","IPMI data for server_type %u", data->server_type());
    while( it != raw_map->end() )
    {
        logging("DEBUG","%s", it->first.c_str());
        //deal with the existing columns only
        if(sensor_map->count(it->first))
        {
            prefix += (*sensor_map)[it->first];
            prefix += ",";
            values += convert<string>(it->second);
            values += ",";
            update_sql += (*sensor_map)[it->first];
            update_sql += "=";
            update_sql += convert<string>(it->second);
            update_sql += ",";
        }
        else
        {
            uncovered.insert(it->first);
            logging("ERROR","%s| Not exist in mapping, value %f. type_id:%s,server_id:%s",it->first.c_str(), it->second,convert<string>(data->server_type()).c_str(),server_id.c_str());
        }
        it++;
    }
    prefix += "collect_time)";
    values += "FROM_UNIXTIME(";
    values += convert<string>(data->collect_time());
    values += "))";
    update_sql += "collect_time=FROM_UNIXTIME(";
    update_sql += convert<string>(data->collect_time());
    update_sql += ") where collect_id=";
    string sql = "insert into ipmi_raw" + prefix + values;
    logging("DEBUG",sql.c_str());
    if(uncovered.size() != 0)
    {
        //传感器信息缺省时，发送扩展请求
        this->appendMapping(sensor_map, uncovered, data->server_type());
        return false;
    }
    if( !this->db->db_insert(sql.c_str()))
    {
        logging("ERROR","ipmi raw insert failed");
    }
    else
    {
        logging("DEBUG","ipmi raw insert finished");
    }
    // update ipmi_raw_status - start
    int index_id;
    string find_sql = "select collect_id from ipmi_raw_status where server_id=" +  server_id;
    MYSQL_RES* result = this->db->db_query(find_sql.c_str());
    if( result == NULL)
    {
        logging("ERROR", "Read NULL when reading ipmi_raw_status");
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if(row)
    {
        index_id = atoi(row[0]);
    }
    else
    {
        index_id =  -1;
    }
    mysql_free_result(result);
    if(index_id == -1)
    {
        sql = "insert into ipmi_raw_status" + prefix + values;
        if(this->db->db_insert(sql.c_str()))
        {
            logging("DEBUG","ipmi raw status insert finished");
        }
        else
            logging("ERROR","ipmi raw status insert failed");
    }
    else
    {
        sql = update_sql + convert<string>(index_id);
        if(this->db->db_insert(sql.c_str()))
        {
            logging("DEBUG","ipmi raw status update finished");
        }
        else
            logging("ERROR","ipmi raw status update failed");

    }
    // udate ipmi_raw_status - end
    delete raw_map;
    delete sensor_map;
    return true;
}
bool IPMIRaw::addMapping(map<string,float>* data_in, uint32_t server_type)
{
    set<string> uncovered;
    for(map<string,float>::iterator it=data_in->begin();it!=data_in->end();it++)
    {
        uncovered.insert(it->first);
    }
    return extendMapping(uncovered, server_type);
}
bool IPMIRaw::judgeMapping(map<string,float>* raw_data, map<string,string>* exist_map)
{
    map<string, float>::const_iterator it = raw_data->begin(); 
    while(it != raw_data->end())
    {
        if(exist_map->count(it->first) == 0 )
        {
            return false;
        }
    }
    return true;
}
//this function is closed because it has to deal with the concurrent problem.
bool IPMIRaw::appendMapping(map<string,string>* existing_map, set<string> & uncovered_sensors, uint32_t type_id)
{
    return extendMapping(uncovered_sensors, type_id);
}
bool IPMIRaw::extendMapping(set<string> & uncovered_sensors, uint32_t type_id)
{
    Action action;
    action.set_act("update");
    set<string>::iterator it = uncovered_sensors.begin();
    while(it != uncovered_sensors.end())
    {
        Map* data = action.add_data();
        data->set_type_id(type_id);
        data->set_sensor(*it);
        it++;
    }
    time_t current;
    time(&current);
    action.set_collect_time(current);
    int pack_size = action.ByteSize() + 4*3;
    char * pack = new char[pack_size];
    google::protobuf::io::ArrayOutputStream aos(pack,pack_size);
    CodedOutputStream * coded_output = new CodedOutputStream(&aos);
    coded_output->WriteVarint32(action.ByteSize());
    coded_output->WriteVarint32(MSG_ID_IPMI_MAPPING);
    coded_output->WriteVarint32(1000000);
    action.SerializeToCodedStream(coded_output);
    sender->send(pack, pack_size, 1);
    delete []pack;
    delete coded_output;
    logging("INFO","Send ipmi mapp add request");
    return true;
}
void IPMIRaw::sendGet()
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
    sender->send(pack, pack_size, 1);
    delete []pack;
    delete coded_output;
    logging("INFO","send get command of ipmi mapping to master");
    logging("DEBUG", action.DebugString().c_str());
}
//插入FRU信息
bool IPMIRaw::insertFRU(CollectData* cd)
{
    if(cd == NULL)
        return false;
    int fru_size = cd->fru_data_size();
    if(fru_size == 0)
    {
        logging("INFO", "FRU data is empty");
        return false;
    }
    this->magic_map.buildMap(cd);
    bool is_new = false;
    int index_id = findFRUStatus(cd->server_id(), is_new);
    if(index_id == -1 && is_new)//fru_status中无对应的记录，插入fru_status中，并将该数据插入fru中
    {
        string s1,s2;
        if(magic_map.parseInsertString(&s1, &s2))
        {
            string insert_sql = "insert into fru_status(server_id, collect_time, " + s1 + ") values(" + cd->server_id()+ ", FROM_UNIXTIME(" + convert<string>(cd->collect_time()) + ")," + s2 + ")";
            if(this->db->db_insert(insert_sql.c_str()))
            {
                logging("DEBUG", "fru_status insert finished");
            }
            else
            {
                logging("ERROR", "fru_status insert failed");
                index_id = -1;
            }
        }
        insertFRUData(cd);
    }
    else if(index_id != -1 && is_new)//fru_status中有对应的记录，并且与当前采集数据有出入，更新fru_status表，并将数据放入fru中
    {
        string temp;
        if(magic_map.parseUpdateString(temp))
        {
            string update_sql = "update fru_status set collect_time=FROM_UNIXTIME(" + convert<string>(cd->collect_time()) + ")," + temp + " where id=" + convert<string>(index_id);
            if(this->db->db_insert(update_sql.c_str()))
            {
                logging("DEBUG", "update fru_status finished");
            }
            else
            {
                logging("ERROR", "update fru_status failed");
                index_id = -1;
            }
        }
        insertFRUData(cd);
    }
    this->magic_map.clearMap();
    return true;
}
int IPMIRaw::findFRUStatus(string server_id, bool & is_new)
{
    string sql = "SELECT id,board_serial,manufacturer,product_name,product_serial from fru_status where server_id=" + server_id;
    MYSQL_RES* result = this->db->db_query(sql.c_str()); 
    if( result == NULL)
        return false;
    int index;
    MYSQL_ROW row = mysql_fetch_row(result);
    string board_serial;
    string manufacturer;
    string product_name;
    string product_serial;
    if(row)//record exist
    {
        index = atoi(row[0]);
        board_serial = (row[1] == NULL? "NULL":string(row[1]));
        manufacturer = (row[2] == NULL? "NULL":string(row[2]));
        product_name = (row[3] == NULL? "NULL":string(row[3]));
        product_serial = (row[4]==NULL? "NULL":string(row[4]));
        //如果当前数据中各项数据与之前数据无差异，不存储
        if(this->magic_map.findStringKey("board_serial") == board_serial && this->magic_map.findStringKey("manufacturer")==manufacturer && this->magic_map.findStringKey("product_name")==product_name && this->magic_map.findStringKey("product_serial")==product_serial)
        {
            is_new = false;
        }
        else//否则，存储
            is_new = true;
    }
    else//record not exists
    {
        index = -1;
        is_new = true;
    }
    mysql_free_result(result);
    return index;
}
bool IPMIRaw::insertFRUData(CollectData* cd)
{
    string s1,s2;
    if(magic_map.parseInsertString(&s1, &s2))
    {
        string insert_sql = "insert into fru(server_id, collect_time, " + s1 + ") values(" + cd->server_id()+ ", FROM_UNIXTIME(" + convert<string>(cd->collect_time()) + ")," + s2 + ")";
        if(this->db->db_insert(insert_sql.c_str()))
        {
            logging("DEBUG", "fru insert finished");
        }
        else
        {
            logging("ERROR", "fru insert failed");
            return false;
        }
    }
    return true;
}
