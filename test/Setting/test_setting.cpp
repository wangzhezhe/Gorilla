
#include "../../server/settings.h"
#include <string>
#include <vector>

int main()
{

  std::string jsonFile("~/cworkspace/src/Gorilla/server/settings_gni.json");

  Settings gloablSettings = Settings::from_json(jsonFile);

  gloablSettings.printsetting();
  return 0;
}