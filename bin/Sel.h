#ifndef SEL_H 
#define SEL_H
#include"DataBaseContainer.h"
#include"IPMIData.pb.h"
#include<sstream>
#include"Sel.h"
#include"log.h"
class Sel
{
    public:
        Sel(DataBaseContainer *);
        ~Sel();
        bool insert(CollectData *);
        bool remove();
        Sel* find();
        bool update();
    private:
        DataBaseContainer* db;
        bool updateStatus(string&, int, string&, string&);
};
#endif
