
#include "../../server/settings.h"
#include <string>
#include <vector>

int main(int argc, char** argv)
{

  std::string str = "/global/homes/z/zw241/cworkspace/src/Gorilla/server/settings_gni.json";
  Settings tempsetting = Settings::from_json(str);
  std::cout << "use path: " << str << std::endl;
  tempsetting.printsetting();
  return 0;
}