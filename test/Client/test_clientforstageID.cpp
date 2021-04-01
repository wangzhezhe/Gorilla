

#include "../../client/ClientForStaging.hpp"
#include <string>
#include <thallium.hpp>
#include <vector>

using namespace GORILLA;
int main(int argc, char** argv)
{

  // Initialize the thallium server
  tl::engine engine("tcp", THALLIUM_SERVER_MODE);

  std::string addressServer = engine.self();

  // start a client for this server
  ClientForStaging ClientForStaging(&engine, addressServer, 0);

  // shut down the server
  std::cout << "ok to create the ClientForStaging" << std::endl;

  for (int i = 0; i < 500; i++)
  {
    std::string testkey = "testid" + std::to_string(i);
    ClientForStaging.cacheClientAddr(testkey);
  }

  for (int i = 0; i < 1000; i++)
  {
    std::string testkey = "testid" + std::to_string(i);
    int id = ClientForStaging.getIDFromClientAddr(testkey);
    std::cout << "testkey " << testkey << " id " << id << std::endl;
  }
  std::cout << "--------------" << std::endl;
  for (int i = 0; i < 500; i++)
  {
    int j = i % 10;
    std::string testkey = "testid" + std::to_string(j);
    int id = ClientForStaging.getIDFromClientAddr(testkey);
    std::cout << "testkey " << testkey << " id " << id << std::endl;
  }

  exit(0);

  return 0;
}