#include "string"
#include "../server/endpointManagement.h"

void testep()
{
    endPointsManager* epManager = new endPointsManager();

    std::vector<std::string> ipLists = {"ip0","ip1","ip2","ip3","ip4"};

    epManager->m_endPointsLists = ipLists;

    epManager->m_serverNum = 3;

    for (int i=0;i<5;i++){
        for(int j=0;j<10;j++){
            std::string varName = "testName_"+std::to_string(i);
            int ts = j;
            std::cout << " getip " << epManager->getByVarTs(varName,ts) << std::endl;
        }
    }
    
    std::cout << "test same key" << std::endl;
    
    for (int i=0;i<5;i++){
        for(int j=0;j<10;j++){
            std::string varName = "testName";
            int ts = 123;
            std::cout << " getip " << epManager->getByVarTs(varName,ts) << std::endl;
        }
    }
    return;

}

int main(int ac, char *av[])
{

    testep();

    return 0;
}