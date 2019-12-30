#include <iostream>
#include <thread>
#include <thallium.hpp>

namespace tl = thallium;


void test(int i){

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    std::cout << "test for input " << i <<std::endl;
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

    tl::abt scope;

    std::vector<tl::managed<tl::xstream>> ess;

    //single producer and multiple consumer pattern https://xgitlab.cels.anl.gov/sds/thallium/blob/master/include/thallium/pool.hpp
    tl::managed<tl::pool> myPool = tl::pool::create(tl::pool::access::spmc);

    for (int i = 0; i < 4; i++)
    {
        tl::managed<tl::xstream> es = tl::xstream::create(tl::scheduler::predef::deflt, *myPool);
        ess.push_back(std::move(es));
    }

    std::vector<tl::managed<tl::thread>> ths;
    for (int i = 0; i < 16; i++)
    {
        tl::managed<tl::thread> th = ess[i % ess.size()]->make_thread(
            [i] {
                test(i);
            }

        );
        ths.push_back(std::move(th));
    }

    std::cout << "wait the ess"<<std::endl;

    //for (auto &mth : ths)
    //{
    //    mth->join();
    //}


    for (int i = 0; i < 4; i++)
    {
        ess[i]->join();
    }

    return 0;
}
