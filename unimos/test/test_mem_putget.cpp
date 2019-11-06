
#include "../server/memcache.h"
#include <unistd.h>

size_t elemInOnedim = 100;

void testput_double_1d(MemCache *mcache)
{
    //generate the data and put it in memcache
    std::vector<double> array;

    for (int i = 0; i < elemInOnedim; i++)
    {
        array.push_back(i * 1.1);
    }

    std::string varName = "testName1d";
    int ts = 0;
    //TODO distinguish local info and the global info
    //Block id could be calculated by the simulation
    //the minidataspace only know the data block id but it don't know how user partition the data domain
    //this two parameters is for the data variable in specific time step
    //std::array<size_t, 3> baseoffset = {0, 0, 0};
    std::array<size_t, 3> shape = {elemInOnedim, 0, 0};
    size_t blockID = 0;

    //TODO, update the method for type string here
    DataMeta datameta = DataMeta(varName, ts, typeid(double).name(), sizeof(double), shape);
    mcache->putIntoCache<double>(datameta, blockID, array);
}

void testput_double_3d(MemCache *mcache)
{
    //generate the data and put it in memcache
    //default is the xyz
    std::vector<double> array;
    for (int i = 0; i < elemInOnedim; i++)
    {
        for (int j = 0; j < elemInOnedim; j++)
        {
            for (int k = 0; k < elemInOnedim; k++)
            {
                array.push_back(i * 0.1 + j * 0.01 + k * 0.001);
            }
        }
    }

    std::string varName = "testName3d";
    int ts = 0;
    //TODO distinguish local info and the global info
    //Block id could be calculated by the simulation
    //the minidataspace only know the data block id but it don't know how user partition the data domain
    //this two parameters is for the data variable in specific time step
    //std::array<size_t, 3> baseoffset = {0, 0, 0};
    std::array<size_t, 3> shape = {elemInOnedim, elemInOnedim, elemInOnedim};
    size_t blockID = 0;

    //TODO, update the method for type string here
    DataMeta datameta = DataMeta(varName, ts, typeid(double).name(), sizeof(double), shape);
    mcache->putIntoCache<double>(datameta, blockID, array);
}

//this is supposed to be called by the rpc
int testget_double_1d(MemCache *mcache, size_t blockID)
{
    //get variable type from the data
    void *data = nullptr;

    std::string varName = "testName1d";
    int ts = 0;

    BlockMeta blockMeta = mcache->getFromCache(varName, ts, blockID, data);

    //check the datameta
    if (blockMeta.getValidDimention()!= 0)
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

//this is supposed to be called by the rpc
int testget_double_3d(MemCache *mcache, size_t blockID)
{
    //get variable type from the data
    void *data = nullptr;

    std::string varName = "testName3d";
    int ts = 0;

    BlockMeta blockMeta = mcache->getFromCache(varName, ts, blockID, data);

    //check the datameta
    if (blockMeta.getValidDimention()!= 0)
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
                for (int j = 0; j < blockMeta.m_shape[1]; j++)
                {
                    for (int k = 0; k < blockMeta.m_shape[2]; k++)
                    {
                        //std::cout << "index " << i << " value " << *dataValue << std::endl;
                        double temp = *dataValue;
                        if (temp != (i * 0.1 + j * 0.01 + k * 0.001))
                        {
                            throw std::runtime_error("return value is incorrect for index: " + std::to_string(i) + "," + std::to_string(j) + "," + std::to_string(k));
                        }
                        dataValue++;
                    }
                }
            }
        }
        else
        {

            std::cout << "data pointer 3d is nullptr for block " << blockID << std::endl;
            return 1;
        }
    }
    return 0;
}

void runputget_double(MemCache *mcache)
{

    //put double
    testput_double_1d(mcache);

    std::cout << "ok for put 1d double" << std::endl;

    //get double
    int status = testget_double_1d(mcache, 0);
    if (status != 0)
    {
        throw std::runtime_error("failed to get data with blockID " + std::to_string(0));
    }

    status = testget_double_1d(mcache, 1);
    if (status != 1)
    {
        throw std::runtime_error("expect status equals to 1");
    }

    //put double
    testput_double_3d(mcache);

    //get double
    status = testget_double_3d(mcache, 0);
    if (status != 0)
    {
        throw std::runtime_error("failed to get data with blockID " + std::to_string(0));
    }
}

int main(int ac, char *av[])
{

    //init memory cache
    MemCache *mcache = new MemCache(4);

    runputget_double(mcache);
    
    free(mcache);

    return 0;
}