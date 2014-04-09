#include"DataBaseProxy.h"
#include<string>
#include<time.h>
#include<sstream>
#include<stdio.h>
#include<pthread.h>
#include<iostream>
using namespace std;

DataBaseProxy::DataBaseProxy(string hostname, int port, string username, string passwd, string default_db, int error_max_count)
{
	this->hostname = hostname;
	this->port = port;
	this->username = username;
	this->passwd = passwd;
	this->default_db = default_db;
	this->error_max_count = error_max_count;
}
DataBaseProxy::DataBaseProxy(const DataBaseProxy &other) : hostname(other.hostname), 
	port(other.port), username(other.username), passwd(other.passwd), 
	default_db(other.default_db), error_max_count(other.error_max_count){
	}
DataBaseProxy::~DataBaseProxy()
{

}
bool DataBaseProxy::isThreadSafe()
{
	if(mysql_thread_safe())
		return true;
	else 
		return false;
}
bool DataBaseProxy::connect()
{
	int tries;
	int timeout;
	int success;

	tries = 5;
	success = 0;
	timeout = 5;

	this->mysql = mysql_init(NULL);

	if( this->mysql == NULL )
	{
		logging("ERROR","init MYSQL error");
		return false;
	}

#ifdef MYSQL_OPT_RECONNECT
	bool reconnect =1;
	int options_error;
	options_error = mysql_options(this->mysql, MYSQL_OPT_RECONNECT, &reconnect);
	if(options_error < 0)
	{
		logging("ERROR","MySQL options unable to set timeout value");
		return false;
	}
#endif
	while(tries > 0)
	{
		tries--;
		if(!mysql_real_connect(this->mysql, this->hostname.c_str(), this->username.c_str(), this->passwd.c_str(), this->default_db.c_str(), this->port, NULL, 0))
		{
			if((mysql_errno(this->mysql) != 1049)&&(mysql_errno(this->mysql) != 2005)&&( mysql_errno(this->mysql) != 1045))
			{
				logging("Error","MYSQL::Connection Failed: Error: '%u', Message:'%s', DB host %s\n", mysql_errno(this->mysql), mysql_error(this->mysql), hostname.c_str());
				success = 0;
				usleep(2000);//sleep 2 seconds before another try
			}
			else //if username and passwd is wrong
			{
				tries = 0;
				success = 0;
			}
		}
		else
		{
			tries = 0;
			success = 1;
		}
	}
	if(!success)
	{
		logging("FATAL"," Connection Failed, Error:'%i', Message:'%s', DB host %s ", mysql_errno(this->mysql), mysql_error(this->mysql), hostname.c_str());
		return false;
	}
	else
		return true;
}
void DataBaseProxy::disconnect()
{
	mysql_close(this->mysql);
	this->mysql = NULL;
}
bool DataBaseProxy::db_insert(const char *query, int &error ) {
	error = 0;
	int    error_count = 0;
	char   query_frag[BUFSIZE];
	/* save a fragment just in case */
	snprintf(query_frag, BUFSIZE, "%s", query);
	while(1) {
		if (mysql_query(this->mysql, query)) {
			error = mysql_errno(this->mysql);
			if ((error == 1213) || (error == 1205)) {//1205 Lock time out; 1213 Dead lock
				usleep(50000);
				error_count++;
				if (error_count > this->error_max_count) {
					logging("ERROR","Too many Lock/Deadlock errors occurred!, SQL Fragment:'%s'", query_frag);
					return false;
				}

			}else if (error == 2006) {//mysql server has gone away
				logging("WARN","SQL Failed! Error:'%i', Message:'%s', Attempting to Reconnect", error, mysql_error(this->mysql));
				this->disconnect();
				usleep(50000);
				this->connect(); 
				error_count++;
				if (error_count > this->error_max_count) {
					logging("FATAL","Too many Reconnect Attempts!\n");
					return false;
				}
			}else{
				logging("ERROR","insert failed, errno %d, sql: %s", error, query);
				return false;
			}
		}else{
			return true;
		}
	}
}

MYSQL_RES *DataBaseProxy::db_query(const char *query, int & error) {
	MYSQL_RES  *mysql_res = 0;
	error       = 0;
	int    error_count = 0;
	char   query_frag[BUFSIZE];
	/* save a fragment just in case */
	snprintf(query_frag, BUFSIZE, "%s", query);
	while (1) {
		if (mysql_query(this->mysql, query)) {
			error = mysql_errno(this->mysql);
			if ((error == 1213) || (error == 1205)) { //1213 Dead Lock; 1205 Lock timeout
				usleep(50000);
				error_count++;
				if (error_count > this->error_max_count) {
					logging("FATAL","Too many Lock/Deadlock errors occurred!, SQL Fragment:'%s'\n", query_frag);
					break;
				}
				//continue;
			}else if (error == 2006) {
				logging("WARN","SQL Failed! Error:'%i', Message:'%s', Attempting to Reconnect", error, mysql_error(this->mysql));
				this->disconnect();
				usleep(50000);
				this->connect();
				error_count++;
				if (error_count > this->error_max_count) {
					logging("FATAL","Too many Reconnect Attempts!\n");
					break;
				}
				//continue;
			}else{
				logging("FATAL","MySQL Error:'%i', Message:'%s' Sql: %s", error, mysql_error(this->mysql), query);
				break;
			}
		}else{
			mysql_res = mysql_store_result(this->mysql);
			break;
		}
	}
	return mysql_res;
}
/*
   int main()
   {
   DataBaseProxy dbp;
   bool r = dbp.connect();
   cout<<r<<endl;
   dbp.db_insert("insert into server_type(type_name, type_des, type_notes) values('test','test','test')");
   MYSQL_RES * result = dbp.db_query("select * from server_type");
   MYSQL_ROW row;
   unsigned int num_fields = mysql_num_fields(result);
   while( (row = mysql_fetch_row(result)))
   {
   unsigned long* lengths = mysql_fetch_lengths(result);
   for( unsigned int  i=0; i<num_fields; i++)
   {
   printf("[%.*s]",(int)lengths[i],row[i]?row[i]:"NULL");
   }
   printf("\n");
   }
   dbp.disconnect();


   }*/
