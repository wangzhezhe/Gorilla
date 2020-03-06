

#include <server/FunctionManager/functionManager.h>
#include <server/TriggerManager/triggerManager.h>
#include <commondata/metadata.h>
#include <unistd.h>
#include <iostream>

void hello(int i)
{
    char str[256];
    sprintf(str, "test hello %d\n", i);    
    std::cout << str << std::endl;
    usleep(2.0*1000000);
    return;
}

int main()
{
    tl::abt scope;
    FunctionManagerMeta *m_fmetamanager = new FunctionManagerMeta();
    DynamicTriggerManager *m_dtmanager = new DynamicTriggerManager(m_fmetamanager, 8, NULL);

    for (int i = 0; i < 128; i++)
    {
        int essid = m_dtmanager->m_threadPool->getEssId();
        std::cout << "essid " << essid << std::endl;
        tl::managed<tl::thread> th = m_dtmanager->m_threadPool->m_ess[essid]->make_thread([i]() {
            hello(i);
        });
        m_dtmanager->m_threadPool->m_userThreadList.push_back(std::move(th));
    }

    delete m_dtmanager;
    delete m_fmetamanager;
}