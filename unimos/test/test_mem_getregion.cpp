
#include "../server/memcache.h"
#include <cmath>

size_t elemInOnedim = 100;

bool AreSame(double a, double b)
{
    return std::fabs(a - b) < 0.000001;
}

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
    DataMeta datameta = DataMeta(varName, ts, 1, typeid(double).name(), sizeof(double), shape);
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
    DataMeta datameta = DataMeta(varName, ts, 1, typeid(double).name(), sizeof(double), shape);
    mcache->putIntoCache<double>(datameta, blockID, array);
}

//this is supposed to be called by the rpc
int testget_double_1d(MemCache *mcache, size_t blockID)
{
    //get variable type from the data
    void *data = nullptr;

    std::string varName = "testName1d";
    int ts = 0;

    BlockMeta blockmeta = mcache->getFromCache(varName, ts, blockID, data);

    //check the datameta
    if (blockmeta.m_dimension != 0)
    {
        blockmeta.printMeta();
    }
    else
    {
        std::cout << "get data with blockID " << blockID << " is empty" << std::endl;
        return 1;
    }

    std::string typeString = typeid(double).name();

    if (blockmeta.m_typeName.compare(typeString) == 0)
    {
        //There is bug if use static_cast<int*> here, not sure why
        if (data != nullptr)
        {
            double *dataValue = (double *)(data);
            std::cout << "check output..." << std::endl;
            //assume there is only one dimention
            //TODO add function to get index from shape
            for (int i = 0; i < blockmeta.m_shape[0]; i++)
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

    BlockMeta blockmeta = mcache->getFromCache(varName, ts, blockID, data);

    //check the datameta
    if (blockmeta.m_dimension != 0)
    {
        blockmeta.printMeta();
    }
    else
    {
        std::cout << "get data with blockID " << blockID << " is empty" << std::endl;
        return 1;
    }

    std::string typeString = typeid(double).name();

    if (blockmeta.m_typeName.compare(typeString) == 0)
    {
        //There is bug if use static_cast<int*> here, not sure why
        if (data != nullptr)
        {
            double *dataValue = (double *)(data);
            std::cout << "check output..." << std::endl;
            //assume there is only one dimention
            //TODO add function to get index from shape
            for (int i = 0; i < blockmeta.m_shape[0]; i++)
            {
                for (int j = 0; j < blockmeta.m_shape[1]; j++)
                {
                    for (int k = 0; k < blockmeta.m_shape[2]; k++)
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

void runputgetregion1d_double(MemCache *mcache)
{

    //generate the data and put it in memcache
    std::vector<double> array;

    for (int i = 0; i < elemInOnedim; i++)
    {
        array.push_back(i * 1.1);
    }

    std::string varName = "testName1d_double";
    int ts = 0;
    //TODO distinguish local info and the global info
    //Block id could be calculated by the simulation
    //the minidataspace only know the data block id but it don't know how user partition the data domain
    //this two parameters is for the data variable in specific time step
    //std::array<size_t, 3> baseoffset = {0, 0, 0};
    std::array<size_t, 3> shape = {elemInOnedim, 0, 0};
    size_t blockID = 0;

    DataMeta datameta = DataMeta(varName, ts, 1, typeid(double).name(), sizeof(double), shape);
    mcache->putIntoCache<double>(datameta, blockID, array);

    //get from the cache
    std::array<size_t, 3> offset = {5, 0, 0};
    std::array<size_t, 3> queryshape = {3, 0, 0};
    void *data = nullptr;
    BlockMeta blockmeta = mcache->getRegionFromCache(varName, ts, blockID, offset, queryshape, data);
    blockmeta.printMeta();
    if (AreSame(*((double *)data),5.5) && AreSame(*((double *)data+1),6.6) && AreSame(*((double *)data+2),7.7))
    {
        std::cout << "ok for double 3" << std::endl;
    }
    else
    {
        std::cout << *((double *)data) << ", " << *((double *)data + 1) << ", " << *((double *)data + 2) << std::endl;
        std::cout << AreSame(*((double *)data+1),6.6) << std::endl;
        throw std::runtime_error("failed to check the data value");
    }
    free(data);

    offset = {98, 0, 0};
    queryshape = {2, 0, 0};

    blockmeta = mcache->getRegionFromCache(varName, ts, blockID, offset, queryshape, data);
    if (AreSame(*((double *)data),98*1.1) && AreSame(*((double *)data+1),99*1.1))
    {
        std::cout << "ok for double 2" << std::endl;
    }
    else
    {
        throw std::runtime_error("failed to check the data value for offset = {98, 0, 0}");
    }
    free(data);

    offset = {98, 0, 0};
    queryshape = {5, 0, 0};

    try
    {
        blockmeta = mcache->getRegionFromCache(varName, ts, blockID, offset, queryshape, data);
    }
    catch (const std::runtime_error &e)
    {
        if (std::string(e.what()).find("smaller than") != std::string::npos)
        {
            std::cout << "expect exception: " << e.what() << std::endl;
        }
        else
        {
            std::cerr << "failed to catch the exception" << std::endl;
        }
    }
}

void runputgetregion1d_int(MemCache *mcache)
{

    //generate the data and put it in memcache
    std::vector<int> array;

    for (int i = 0; i < elemInOnedim; i++)
    {
        array.push_back(i);
    }

    std::string varName = "testName1d_int";
    int ts = 0;
    //TODO distinguish local info and the global info
    //Block id could be calculated by the simulation
    //the minidataspace only know the data block id but it don't know how user partition the data domain
    //this two parameters is for the data variable in specific time step
    //std::array<size_t, 3> baseoffset = {0, 0, 0};
    std::array<size_t, 3> shape = {elemInOnedim, 0, 0};
    size_t blockID = 0;

    DataMeta datameta = DataMeta(varName, ts, 1, typeid(int).name(), sizeof(int), shape);
    mcache->putIntoCache<int>(datameta, blockID, array);

    //get from the cache
    std::array<size_t, 3> offset = {5, 0, 0};
    std::array<size_t, 3> queryshape = {3, 0, 0};
    void *data = nullptr;
    BlockMeta blockmetaget = mcache->getRegionFromCache(varName, ts, blockID, offset, queryshape, data);
    blockmetaget.printMeta();
    if (*((int *)data) != 5 || *((int *)data + 1) != 6 || *((int *)data + 2) != 7)
    {
        throw std::runtime_error("failed to check the data value for integer");
    }
}

int main(int ac, char *av[])
{

    //init memory cache
    MemCache *mcache = new MemCache();

    runputgetregion1d_double(mcache);

    runputgetregion1d_int(mcache);

    //todo add 2d and 3d test

    //runputgetregion2d_double(mcache);

    //runputgetregion3d_double(mcache);

    free(mcache);

    return 0;
}