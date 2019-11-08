#include <thallium.hpp>

namespace tl = thallium;

struct ArgoThreadPool
{
    ArgoThreadPool(int esNumber): m_poolSize (esNumber)
    {
        //set the user thread number as 4 times of the system thread number
        if(esNumber<=0){
            throw std::runtime_error("pool number should larger than 0");
        }
        m_threadNumer = m_poolSize*4;
        for (int i = 0; i < esNumber; i++)
        {
            tl::managed<tl::xstream> es = tl::xstream::create(tl::scheduler::predef::deflt, *m_myPool);
            m_ess.push_back(std::move(es));
        }
    };

    tl::abt scope;
    
    //the system thread number
    int m_poolSize=0;
    
    tl::mutex m_Mutex;
    //the user level thread number
    int m_threadNumer = 0;

    int m_currentUserThreadId = 0;

    //single producer and multiple consumer pattern https://xgitlab.cels.anl.gov/sds/thallium/blob/master/include/thallium/pool.hpp
    tl::managed<tl::pool> m_myPool = tl::pool::create(tl::pool::access::spmc);

    std::vector<tl::managed<tl::xstream>> m_ess;

    std::vector<tl::managed<tl::thread>> m_userThreadList;

    int getEssId(){
        int essId = m_currentUserThreadId % m_poolSize;
        m_Mutex.lock();
        //assume there are less than 256 thread running concurrently
        m_currentUserThreadId = (m_currentUserThreadId+1) % 256;
        m_Mutex.unlock();
        return essId;
    }

    void essjoin(){
        for (int i = 0; i < m_poolSize; i++)
        {
             m_ess[i]->join();
        }
        return;
    }
    

};



/*


        std::vector<tl::managed<tl::thread>> ths;
        for (int i = 0; i < 16; i++)
        {
            tl::managed<tl::thread> th = ess[i % ess.size()]->make_thread(test);

            ths.push_back(std::move(th));
        }

*/