
#include "../server/memcache.h"
#include <string>
#include <omp.h>

size_t elemInOnedim = 100;

//this is supposed to be called by the rpc
int testget_double_1d(MemCache *mcache, std::string varName, int ts, size_t blockID)
{
    //get variable type from the data
    void *data = nullptr;

    BlockMeta blockMeta = mcache->getFromCache(varName, ts, blockID, data);

    //check the datameta
    if (blockMeta.m_dimension!= 0)
    {
        blockMeta.printMeta();
    }
    else
    {
        std::cout << "get data with blockID " << blockID << " is empty" << std::endl;
        return 1;
    }

    std::string typeString = typeid(double).name();

    if (blockMeta.m_typeName.compare(typeString) == 0)
    {
        //There is bug if use static_cast<int*> here, not sure why
        if (data != nullptr)
        {
            double *dataValue = (double *)(data);
            std::cout << "check output..." << std::endl;
            //assume there is only one dimention
            //TODO add function to get index from shape
            for (int i = 0; i < blockMeta.m_shape[0]; i++)
            {
                //std::cout << "index " << i << " value " << *dataValue << std::endl;
                double temp = *dataValue;
                if (temp != (i * 1.1))
                {
                    throw std::runtime_error("return value is incorrect for index  " + std::to_string(i));
                }
                dataValue++;
            }
        }
        else
        {

            std::cout << "data pointer is nullptr for block " << blockID << std::endl;
            return 1;
        }
    }
    return 0;
}

void testput_double_1d(MemCache *mcache, std::string varName, int ts, size_t blockID)
{
    //generate the data and put it in memcache
    std::vector<double> array;

    for (int i = 0; i < elemInOnedim; i++)
    {
        array.push_back(i * 1.1);
    }
    //TODO distinguish local info and the global info
    //Block id could be calculated by the simulation
    //the minidataspace only know the data block id but it don't know how user partition the data domain
    //this two parameters is for the data variable in specific time step
    //std::array<size_t, 3> baseoffset = {0, 0, 0};
    std::array<size_t, 3> shape = {elemInOnedim, 0, 0};
    size_t dimention = 1;
    //TODO, update the method for type string here
    DataMeta datameta = DataMeta(varName, ts, dimention, typeid(double).name(), sizeof(double), shape);
    mcache->putIntoCache<double>(datameta, blockID, array);
}

void runput_blockinparallel(MemCache *mcache)
{

    std::string varName = "testVar";
    int ts = 0;

    omp_set_num_threads(64);
#pragma omp parallel for
    for (int varid = 0; varid < 100; varid++)
    {
        std::string varNameFull = varName + std::to_string(varid);
        for (int ts = 0; ts < 100; ts++)
        {
            for (int i = 0; i < 100; i++)
            {
                testput_double_1d(mcache, varNameFull, ts, i);
            }
        }
    }

    return;
}

void runget_blockinparallel(MemCache *mcache)
{

    std::string varName = "testVar";
    int ts = 0;

    omp_set_num_threads(64);
#pragma omp parallel for
    for (int varid = 0; varid < 100; varid++)
    {
        std::string varNameFull = varName + std::to_string(varid);
        for (int ts = 0; ts < 100; ts++)
        {
            for (int i = 0; i < 100; i++)
            {
                testget_double_1d(mcache, varNameFull, ts, i);
            }
        }
    }
    return;
}

void runput_multiplets()
{
    return;
}

void runput_multipleVar()
{
    return;
}

int main(int ac, char *av[])
{

    //init memory cache
    MemCache *mcache = new MemCache();

    runput_blockinparallel(mcache);

    std::cout << "ok to run put block id in paralle" << std::endl;

    runget_blockinparallel(mcache);

    std::cout << "ok to run get block id in paralle" << std::endl;

    runput_multiplets();

    runput_multipleVar();

    free(mcache);

    return 0;
}