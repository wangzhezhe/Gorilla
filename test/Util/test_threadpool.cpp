
#include <unistd.h>
#include "../utils/ArgothreadPool.h"

void hello(int id)
{
    std::cout << "hello thread for id " << id << std::endl;
    sleep(3);
    return;
}

int main()
{
    tl::abt scope;
    ArgoThreadPool *threadPool = new ArgoThreadPool(5);

    for (int i = 0; i < 10; i++)
    {
        int threadid = threadPool->getEssId();
        tl::managed<tl::thread> th = threadPool->m_ess[threadid]->make_thread(
            [i] {
                char str[200];
                sprintf(str, "put lambda function into the pool (%d)\n", i);
                hello(i);
            });
        threadPool->m_userThreadList.push_back(std::move(th));
    }

    delete threadPool;
}