
#include "../server/memcache.h"
#include "../server/filterManager.h"
#include <unistd.h>

size_t elemInOnedim = 100;

void testput_double_1d(std::string varName, size_t i, MemCache *mcache)
{
    //generate the data and put it in memcache
    std::vector<double> array;

    for (int i = 0; i < elemInOnedim; i++)
    {
        array.push_back(i * 1.1);
    }


    int ts = 0;
    //TODO distinguish local info and the global info
    //Block id could be calculated by the simulation
    //the minidataspace only know the data block id but it don't know how user partition the data domain
    //this two parameters is for the data variable in specific time step
    //std::array<size_t, 3> baseoffset = {0, 0, 0};
    std::array<size_t, 3> shape = {elemInOnedim, 0, 0};
    size_t blockID = i;

    //TODO, update the method for type string here
    DataMeta datameta = DataMeta(varName, ts, typeid(double).name(), sizeof(double), shape);
    mcache->putIntoCache<double>(datameta, blockID, array);
}



int main(int ac, char *av[])
{

    std::string varName = "testName1d";

    FilterProfile fp("testProfile","default","default","default","testsubaddr");

    FilterManager * fmanager = new FilterManager();
    fmanager->profileSubscribe(varName,fp);

    //init memory cache
    MemCache *mcache = new MemCache(10);

    mcache->loadFilterManager(fmanager);

    for (size_t i = 0; i < 100; i++)
    {
        testput_double_1d(varName, i, mcache);
        std::cout << "ok to put the block " << i << std::endl;
    }

    sleep(20);

    free(mcache);

    return 0;
}