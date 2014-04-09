#Makefile for the socket programming example
CC = g++
CFLAGS = -o3 -Wall -g -lpthread -lprotobuf -Wno-sign-compare
LDFLAGS = -lm
MYSQL_CONFIG = `mysql_config --cflags --libs_r`
LIBPROTOBUF_FLAG = -lprotobuf

log_objects = common.h config.h config.c log.h log.c
db_conn_objects = DataBaseProxy.h DataBaseProxy.cc 
sender_objects = AsyncDataSender.h AsyncDataSender.cc  
collect_metric_objects = IPMIData.pb.h IPMIData.pb.cc
return_code_objects = ReturnCode.pb.h ReturnCode.pb.cc
snmp_data_objects = SNMPData.pb.h SNMPData.pb.cc
snmp_db_objects = SNMPDataDB.h SNMPDataDB.cc
sensor_map_objects = SensorMap.h SensorMap.cc IPMIMapping.pb.cc IPMIMapping.pb.h
data_con_objects = DataBaseContainer.h DataBaseContainer.cc
#sample_client_objects = ClientSocket.o Socket.o SampleClient.o CollectMetric.o
async_data_listener_objects = $(log_objects) datapool_main.cc  AsyncDataListener.h AsyncDataListener.cc  $(collect_metric_objects) $(return_code_objects) $(db_conn_objects) $(sender_objects) IPMIRaw.h IPMIRaw.cc Sel.h Sel.cc $(snmp_data_objects) $(snmp_db_objects) MagicMap.h MagicMap.cc $(sensor_map_objects) SSDData.h SSDData.cc $(data_con_objects)

ipmi_raw_objects = $(collect_metric_objects) $(db_conn_objects) IPMIRaw.h IPMIRaw.cc
sel_objects = $(collect_metric_objects) $(db_conn_objects) Sel.h Sel.cc
#async_data_sender_objects = Socket.h Socket.cpp ClientSocket.h ClientSocket.cpp AsyncDataSender.h AsyncDataSender.cpp

all : dp
simple_server: $(simple_server_objects)
	$(CC) $(CFLAGS) $(simple_server_objects) -o simple_server $(LDFLAGS)
simple_client: $(simple_client_objects)
	$(CC) $(CFLAGS) $(simple_client_objects) -o simple_client $(LDFLAGS)
data_listener: $(data_listener_objects)
	$(CC) $(CFLAGS) $(data_listener_objects) -o data_listener $(LDFLAGS)
dp: $(async_data_listener_objects)
	$(CC) $(CFLAGS) $(MYSQL_CONFIG) $(async_data_listener_objects) $(LIBPROTOBUF_FLAG) -o dp $(LDFLAGS)
sample_client: $(sample_client_objects)
	$(CC) $(CFLAGS) $(sample_client_objects) -o sample_client $(LDFLAGS)
async_data_sender: $(async_data_sender_objects)
	$(CC) $(CFLAGS) $(async_data_sender_objects) -o aysnc_data_sender $(LDFLAGS)
db_conn: $(db_conn_objects)
	$(CC) $(CFLAGS) $(MYSQL_CONFIG) $(db_conn_objects) -o db_conn $(LDFLAGS)
ipmi_raw: $(ipmi_raw_objects)
	$(CC) $(CFLAGS) $(MYSQL_CONFIG) $(ipmi_raw_objects) -o ipmi_raw $(LDFLAGS)
sel: $(sel_objects)
	$(CC) $(CFLAGS) $(MYSQL_CONFIG) $(sel_objects) -o sel $(LDFLAGS)
#sample_client: $(sample_client_objects)   
#	$(CC) $(CFLAGS) $(sample_client_objects) -o sample_client $(LDFLAGS) 


.PHONY : clean rebuild
clean:
	rm -f *.o simple_server simple_client data_listener sample_client async_data_listener async_data_sender dp

rebuild:
	make clean; make
