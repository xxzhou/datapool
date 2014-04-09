#include"Sel.h"
#include<iostream>
#include<stdio.h>
#include<mysql.h>
#include<stdint.h>
#include"Util.h"
using namespace std;
Sel::Sel(DataBaseContainer* db)
{
    this->db = db;
}
Sel::~Sel()
{
}
bool Sel::insert(CollectData * data)
{
    if(data == NULL or data->log_data_size()==0)
    {
        logging("DEBUG","ipmi log data is null \n");
        return false;
    }
    string prefix="";
    string server_id = data->server_id();
    int log_id=0;
    string log_value;
    string collect_time;
    for(int i=0; i<data->log_data_size(); i++)
    {
        LogData ld = data->log_data(i);
        prefix += "(";
        prefix += (convert<string>(data->server_id())+",'");
        prefix += (convert<string>(ld.id())+"','");
        prefix += (ld.content()+"',");
        prefix += "FROM_UNIXTIME(" + convert<string>(ld.time());
        prefix += "))";
        if(i != (data->log_data_size() -1) )
        {
            prefix += ",";
        }
        if(ld.id() > log_id)
        {
            log_id = ld.id();
            log_value = ld.content();
            collect_time = "FROM_UNIXTIME(" + convert<string>(ld.time()) + ")";
        }
    }
    if(!this->db->db_insert(prefix.c_str(), "log"))
    {
        logging("ERROR","insert sel log failed");
        return false;
    }
    else
    {
        logging("DEBUG","insert sel log finished");
        return updateStatus(server_id, log_id, log_value, collect_time);
    }
    return true;
}
bool Sel::updateStatus(string& server_id, int log_id, string& log_value, string& collect_time)
{
    string find_sql = "SELECT rec_id,log_id from log_status where server_id=" + server_id ;   
    string m_sql;
    MYSQL_RES* result = this->db->db_query(find_sql.c_str());
    int rec_id;
    MYSQL_ROW row = NULL;
    if(result != NULL)
        row = mysql_fetch_row(result);
    if( row != NULL)
    {
        rec_id = atoi(row[0]);
        if( log_id > atoi(row[1])) 
            m_sql = "UPDATE log_status set log_id='" + convert<string>(log_id) + "', log_value='" + log_value + "', collect_time=" + collect_time + " where rec_id=" + convert<string>(rec_id);
    }
    else
    {
        m_sql = "INSERT INTO log_status(server_id, log_id, log_value, collect_time) values(" + server_id + ", '" + convert<string>(log_id) + "','"  + log_value + "'," + collect_time + ")";
    }
    mysql_free_result(result);
    if(!this->db->db_insert(m_sql.c_str()))
    {
        logging("ERROR", "Insert sel_status failed");
        return false;
    }
    return true;
}
