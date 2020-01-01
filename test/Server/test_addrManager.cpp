#include "../server/addrManager.h"

void test_addrManager()
{
    tl::abt scope;
    AddrManager* arrmng = new AddrManager();
     
     for(int i=0;i<5;i++){
         std::string serverAddr = "test" + std::to_string(i);
         arrmng->m_endPointsLists.push_back(serverAddr);
     }

     for(int i=0;i<25;i++){
         std::string serverAddr = arrmng->getByRRobin();
         std::cout << "index " << i << " serverAddr " << serverAddr << std::endl;
         std::string correctStr = "test" + std::to_string(i%5);
         if(serverAddr.compare(correctStr)!=0){
             throw std::runtime_error("error to get server addr");
         }
     }

    return;
}

int main()
{
    test_addrManager();
}