#include <iostream>
#include <thallium.hpp>
#include "point.hpp"

namespace tl = thallium;

int main(int argc, char **argv)
{

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);

    std::cout << "Server running at address " << myEngine.self() << std::endl;

    tl::engine myEngineClient("tcp", THALLIUM_CLIENT_MODE);

    std::function<void(const tl::request &, const point &, const point &)> dot_product =
        [&myEngine](const tl::request &req, const point &p, const point &q) {
            std::cout << "inner " << p.m_i.innerx << " " << p.m_i.innery << " " << p.m_i.innerz << std::endl;

            tl::endpoint ep = req.get_endpoint();
            hg_addr_t client_addr = ep.get_addr();
            char addr_str[128];
            size_t addr_str_size = 128;
            
            margo_addr_to_string(myEngine.get_margo_instance(), addr_str, &addr_str_size, client_addr);

            std::cout << "client addr is " << addr_str << std::endl;

            req.respond(p * q);
            myEngine.finalize();
        };

    myEngine.define("dot_product", dot_product);

    return 0;
}
