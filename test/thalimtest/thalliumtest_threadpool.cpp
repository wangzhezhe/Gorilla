#include <iostream>
#include <thallium.hpp>
#include <unistd.h>

namespace tl = thallium;

void hello(int i) {
    tl::xstream es = tl::xstream::self();
    std::cout << "Hello World from ES " 
        << es.get_rank() << ", ULT " 
        << tl::thread::self_id() << std::endl;
    sleep(3);
}

int main(int argc, char** argv) {

    tl::abt scope;

    std::vector<tl::managed<tl::xstream>> ess;

    for(int i=0; i < 4; i++) {
        tl::managed<tl::xstream> es = tl::xstream::create();
        ess.push_back(std::move(es));
    }

    std::vector<tl::managed<tl::thread>> ths;
    for(int i=0; i < 256; i++) {
        tl::managed<tl::thread> th = ess[i % ess.size()]->make_thread(
            [i] {
                char str[200];
                sprintf(str, "put lambda function into the pool (%d)\n", i);
                std::cout << str;
                hello(i);
            });
        ths.push_back(std::move(th));
    }

    for(auto& mth : ths) {
        mth->join();
    }

    sleep(10);

    for(int i=0; i <256 ; i++) {
        ess[i]->join();
    }

    return 0;
}
