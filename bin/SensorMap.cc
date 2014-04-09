#include"DataBaseProxy.h"
#include"SensorMap.h"
#include"log.h"
SensorMap::SensorMap()
{
    data = NULL;
    pthread_mutex_init(&d_mutex, NULL);
    d_write_request = false;
    pthread_cond_init(&d_write_request_false, NULL);
    d_nreading = 0;
    pthread_cond_init(&d_nreading_zero, NULL);
}
SensorMap::SensorMap(DataBaseContainer* db)
{}
void SensorMap::initialize(Action* action)
{
    data = new map<uint32_t, map<string,string>*>();
    for(int i = 0 ; i < action->data_size(); i++)
    {
        Map map_data = action->data(i);
        map<string,string>* temp;
        if(data->find(map_data.type_id())==data->end())
        {
            temp = new map<string,string>();
            (*data)[map_data.type_id()] = temp;
        }
        else
        {
            temp = (*data)[map_data.type_id()];
        }
        (*temp)[map_data.sensor()] = map_data.column();         
    }
    logging("INFO","SensorMap initialized");
}
SensorMap::~SensorMap()
{
    for(map<uint32_t, map<string,string>*>::iterator it = data->begin(); it!= data->end(); it++) 
    {
        delete(it->second);
    }
    delete data;
    pthread_mutex_destroy(&d_mutex);
    pthread_cond_destroy(&d_write_request_false);
    pthread_cond_destroy(&d_nreading_zero);
    logging("INFO","SensorMap destructed");
}
bool SensorMap::getMap(uint32_t type_id, map<string,string>** src)
{
    //wait and increment the read number
    pthread_mutex_lock(&d_mutex);
    while(d_write_request)
        pthread_cond_wait(&d_write_request_false, &d_mutex);
    d_nreading++;
    pthread_mutex_unlock(&d_mutex);
    //begin reading
    bool result = false;
    if(this->data == NULL)
    {
        logging("ERROR","ipmi map hasn't been initialized");
        result = false;
    }
    else if(this->data->find(type_id) == this->data->end())
    {
        logging("WARN","ipmi map not ready for type_id %d", type_id);
        result = true;
    }
    else
    {
        logging("DEBUG","read map, type_id %d ", type_id);
        map<string,string>* map_temp = (*data)[type_id];
        for(map<string,string>::iterator it = map_temp->begin(); it!=map_temp->end(); it++)
        {
            (**src)[it->first] = it->second;
        }
        result = true;
    }
    //decrement the reader number
    pthread_mutex_lock(&d_mutex);
    d_nreading--;
    if(d_nreading == 0)
        pthread_cond_signal(&d_nreading_zero);
    pthread_mutex_unlock(&d_mutex);
    return result;
}
bool SensorMap::expandMap(Action* action)
{
    //wait
    pthread_mutex_lock(&d_mutex);
    d_write_request = true;
    while(d_nreading != 0)
        pthread_cond_wait(&d_nreading_zero, &d_mutex);
    pthread_mutex_unlock(&d_mutex);
    //write
    logging("DEBUG","expandMap begin...");
    for(int i=0;i<action->data_size(); i++)
    {
        Map map_d = action->data(i);
        uint32_t type_id = map_d.type_id();
        map<string,string>* temp;
        if( data->find(type_id) == data->end())
        {
            temp = new map<string,string>();
            (*data)[type_id] = temp;
        }
        else
        {
            temp = (*data)[type_id];
        }
        (*temp)[map_d.sensor()] = map_d.column(); 
    }
    //send signal to other threads
    pthread_mutex_lock(&d_mutex);
    d_write_request = false;
    pthread_cond_broadcast(&d_write_request_false);
    pthread_mutex_unlock(&d_mutex);
    return true;
}
