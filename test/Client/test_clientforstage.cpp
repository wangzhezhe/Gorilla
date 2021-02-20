

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

  exit(0);

  return 0;
}