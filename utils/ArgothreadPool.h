#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thallium.hpp>

namespace tl = thallium;

struct ArgoThreadPool
{
    //the number of the es controls the number of the thread running in concurent way
    ArgoThreadPool(int esNumber): m_poolSize (esNumber)
    {
        //set the user thread number as 4 times of the system thread number
        if(esNumber<=0){
            throw std::runtime_error("pool number should larger than 0");
        }
        for (int i = 0; i < esNumber; i++)
        {
            tl::managed<tl::xstream> es = tl::xstream::create();
            m_ess.push_back(std::move(es));
        }
    };

    //put this separately, init this when using the pool
    //tl::abt scope;
    //the system thread number
    int m_poolSize=0;
    
    tl::mutex m_Mutex;
    int m_currentUserThreadId = 0;

    //single producer and multiple consumer pattern https://xgitlab.cels.anl.gov/sds/thallium/blob/master/include/thallium/pool.hpp
    //tl::managed<tl::pool> m_myPool = tl::pool::create(tl::pool::access::spmc);
    std::vector<tl::managed<tl::xstream>> m_ess;

    //TODO update this part, delete thread when one finish
    tl::mutex m_threadmutex;

    //Attention we only need to store the thread if we want to merge it or revive it
    //otherwise, we can use the anonyous as the second parameter when making a new thread
    //this is a example
    /*
    m_pool.make_thread(
      [data, size, src, tag, &req, this]() {
        recv(data, size, src, tag);
        req.m_eventual.set_value();
      },
      tl::anonymous())
    */
    //std::vector<tl::managed<tl::thread>> m_userThreadList;

    int getEssId(){
        m_Mutex.lock();
        int essId = m_currentUserThreadId % m_poolSize;
        //assume there are less than 256 ess running concurrently
        m_currentUserThreadId = (m_currentUserThreadId+1) % 256;
        m_Mutex.unlock();
        return essId;
    }

    int deleteOneEss(){
        //maybe just delete one (maybe use a queue)
        //delete which
    }

    int addOneEss(){
        //lock the vector and put one into the vector
    }

    void essjoin(){
        for (int i = 0; i < m_poolSize; i++)
        {
             m_ess[i]->join();
        }
        return;
    }

    ~ArgoThreadPool(){
        std::cout << "delete thread pool\n" << std::endl;
        essjoin();
    }
};

#endif