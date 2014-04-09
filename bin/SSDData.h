#ifndef SSD_DATA_H_
#define SSD_DATA_H_
#include"DataBaseContainer.h"
#include<string>
#include<vector>
#include<map>
using namespace std;
class SSDData{
    public:
        SSDData();
        ~SSDData();
        SSDData(const SSDData &);
        void setDB(DataBaseContainer*);
        bool insert(string&, string, string);
    private:
        DataBaseContainer* db;
        int  findSSDStatus(string,string,string);

};
#endif
