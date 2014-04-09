#include"SSDData.h"
#include"log.h"
#include"Util.h"
SSDData::SSDData()
{}
SSDData::~SSDData()
{}
SSDData::SSDData(const SSDData & src)
{
}
void SSDData::setDB(DataBaseContainer* src_db)
{
    this->db = src_db;
}
bool SSDData::insert(string& src_data, string server_id, string collect_time)
{
    const int n = src_data.size();
    int start = 0;
    logging("DEBUG","SSD begin");
    string cur;
    string prev;
    string ssd;
    bool insert_flag = false;
    int insert_index = 0;
    int index_id = -1;
    string row_data[20];
    string insert_sql;
    string update_sql;
    bool end_of_line = false;
    for(int i=0;i<n;)
    {
        while((i<n) && (src_data[i]==' ' || src_data[i]==',' || src_data[i]=='\n' || src_data[i]=='#' || src_data[i]=='='))//split the blank prefix
        {
            if( src_data[i] == '\n')
            {
                end_of_line = true;
            }
            i++;
        }
        start=i;
        while((i<n) && (src_data[i]!=' '&& src_data[i]!=',' && src_data[i]!='\n' && src_data[i]!='#' && src_data[i]!='='))
            i++;
        if(start<i)
        {
            //logging("INFO",src_data.substr(start, i-start).c_str());
            prev = cur;
            cur = src_data.substr(start, i-start);
            if( cur == "smartctl" )
            {
                ssd = prev;
                insert_flag = false;
                insert_index = 0;
            }
            if( insert_flag == true )
            {
                logging("DEBUG","insert_idex:%d cur:%s",insert_index,cur.c_str());
                row_data[insert_index] = cur;
                //End of the line
                if( ++insert_index > 9)
                {
                    if(row_data[0] == "ID")
                    {
                        //Not matched row names
                        if(row_data[1]!="ATTRIBUTE_NAME" || row_data[2]!="FLAG" || row_data[3]!="VALUE" || row_data[4]!="WORST" || row_data[5]!="THRESH" || row_data[6] != "TYPE"  || row_data[7]!="UPDATED" || row_data[8]!="WHEN_FAILED" || row_data[9]!="RAW_VALUE")
                        {
                            logging("ERROR","SSD result Not supported");
                            return false;                        
                        }
                    }
                    else
                    {
                        //filter the Unknown_Attribute
                        if(row_data[1] == "Unknown_Attribute")
                        {
                            insert_index = 0;
                            continue;
                        }
                        //to make the last element precise
                        while((i<n) && (src_data[i] !='\n'))
                            i++;
                        row_data[9] = src_data.substr(start,i-start);
                        //insert data into db
                        insert_sql = "(" + server_id + ",'"+ssd+"',"+row_data[0]+",'"+row_data[1]+"','"+row_data[2]+"',"+row_data[3]+","+row_data[4]+",'"+row_data[5]+"','"+row_data[6]+"','"+row_data[7]+"','"+row_data[8]+"','"+row_data[9]+"',FROM_UNIXTIME(" + collect_time + "))";
                        if(this->db->db_insert(insert_sql.c_str(), "ssd") == false)
                        {
                            logging("ERROR","ssd insert failed"); 
                        }
                        index_id = findSSDStatus(server_id, ssd, row_data[1]);
                        if(index_id == -1) // insert ssd data in status
                        {
                            insert_sql = "insert into ssd_status(server_id,ssd,id,attribute_name,flag,value,worst,thresh,type,updated,when_failed,raw_value,collect_time) values (" + server_id + ",'"+ssd+"',"+row_data[0]+",'"+row_data[1]+"','"+row_data[2]+"',"+row_data[3]+","+row_data[4]+",'"+row_data[5]+"','"+row_data[6]+"','"+row_data[7]+"','"+row_data[8]+"','"+row_data[9]+"',FROM_UNIXTIME(" + collect_time + "))";
                            if(this->db->db_insert(insert_sql.c_str()) == false)
                            {
                                logging("ERROR","ssd_status insert failed"); 
                            }

                        }
                        else// update 
                        {
                            update_sql = "update ssd_status set id="+row_data[0]+",attribute_name='"+row_data[1]+"',flag='"+row_data[2]+"',value="+row_data[3]+",worst="+row_data[4]+",thresh='"+row_data[5]+"',type='"+row_data[6]+"',updated='"+row_data[7]+"',when_failed='"+row_data[8]+"',raw_value='"+row_data[9]+"',collect_time=FROM_UNIXTIME("+collect_time+") where rec_id=" + convert<string>(index_id);
                            if(this->db->db_insert(update_sql.c_str()) == false)
                            {
                                logging("ERROR","ssd_status update failed"); 
                            }

                        }

                    }
                    insert_index = 0;
                    end_of_line = false;
                }
            }
            if( cur == "Thresholds:")
            {
                insert_flag = true;
                insert_index = 0;
            }

        }
    }
    logging("DEBUG","SSD end");
    return true;
}
int SSDData::findSSDStatus(string server_id, string ssd, string attribute)
{
    int index_id;
    string find_sql = "select rec_id from ssd_status where server_id=" + server_id + " and `ssd`='" + ssd + "' and attribute_name='"+attribute+"'";
    MYSQL_RES* result = this->db->db_query(find_sql.c_str());
    MYSQL_ROW row = mysql_fetch_row(result);
    if(row)//return rec_id of destination ssd
    {
        index_id = atoi(row[0]);
        mysql_free_result(result);
        return index_id;
    }
    else//ssd not exists
    {
        mysql_free_result(result);
        return -1;
    }

}

