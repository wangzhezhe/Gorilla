
#include "../server/memcache.h"
#include "../server/filterManager.h"
#include <unistd.h>
//#include <omp.h>

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
    tl::abt scope;

    std::string varName = "testName1d";

    FilterProfile fp("testProfile", "default", "default", "default", "testsubaddr");

    FilterManager *fmanager = new FilterManager();
    fmanager->profileSubscribe(varName, fp);

    //init memory cache
    MemCache *mcache = new MemCache(8);

    mcache->loadFilterManager(fmanager);

    //first principle for using argobots
    //it is still dangerous to mix the argoboots with the openmp, pthread
    auto self_es = tl::xstream::self();
    std::vector<tl::managed<tl::thread>> ults;
    for (size_t i = 0; i < 1000; i++)
    {
        auto th = self_es.make_thread([i,&varName,&mcache](){
            testput_double_1d(varName, i, mcache);
            std::cout << "ok to put the block " << i << std::endl;
        });
        ults.push_back(std::move(th));
    }
    for(auto& th : ults) {
        th->join();
    }
    
    ults.clear();

    //sleep(20);
    //TODO check the memcache before exit, to see if it finish all the thread then exit
    free(mcache);

    return 0;
}