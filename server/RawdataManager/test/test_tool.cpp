#include "string"
#include "../utils/stringtool.h"

void testip()
{

    std::string results = IPTOOL::getClientAdddr("verbs", "sdfsd://1.2.3.4");
    if (results.compare(std::string("verbs://1.2.3.4")) != 0)
    {
        throw std::runtime_error("failed to get client addr  " + results);
    }
}

int main(int ac, char *av[])
{

    testip();

    return 0;
}