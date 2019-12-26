
#include "../server/memcache.h"



void testput_double_1d(MemCache *mcache)
{
    //generate the data and put it in memcache
    std::vector<double> array;

    for (int i = 0; i < 10; i++)
    {
        array.push_back(i * 1.1);
    }

    std::string varName = "testName";
    int ts = 0;
    //TODO distinguish local info and the global info
    //Block id could be calculated by the simulation
    //the minidataspace only know the data block id but it don't know how user partition the data domain
    //this two parameters is for the data variable in specific time step
    std::array<size_t, 3> lowbound = {0, 0, 0};
    std::array<size_t, 3> shape = {10, 0, 0};
    size_t blockID = 0;

    //TODO, update the method for type string here
    DataMeta datameta = DataMeta(varName, ts, 1, typeid(double).name(), sizeof(double), lowbound, shape);
    mcache->putIntoCache<double>(datameta, blockID, array);

    blockID = 1;
    mcache->putIntoCache(datameta, blockID, (void *)(array.data()));
}

void runputget_double(MemCache *mcache)
{

    //put double
    testput_double_1d(mcache);

    //get double

    //check
}

int main(int ac, char *av[])
{

    //init memory cache
    MemCache *mcache = new MemCache();

    runputget_double(mcache);

    free(mcache);


    return 0;
}