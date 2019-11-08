#include <iostream>
#include <thread>
#include "../utils/ArgothreadPool.h"

namespace tl = thallium;


void test(int i){

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    
    char str[200];
    sprintf(str, "test for input %d\n", i);
    std::cout << str;
}

void hello()
{
    tl::xstream es = tl::xstream::self();
    std::cout << "Hello World from ES "
              << es.get_rank() << ", ULT "
              << tl::thread::self_id() << std::endl;
}

int main(int argc, char **argv)
{

    ArgoThreadPool abpool(4);

    for (int i = 0; i < 16; i++)
    {
        int id = abpool.getEssId();
        tl::managed<tl::thread> th = abpool.m_ess[id]->make_thread(
            [i] {
                test(i);
            }

        );
        abpool.m_userThreadList.push_back(std::move(th));
    }

    std::cout << "wait the ess"<<std::endl;

    abpool.essjoin();

    return 0;
}
