
#include "../../server/settings.h"
#include <string>
#include <vector>

int main(){
    
    std::string jsonFile = "./settings.json";

    Settings gloablSettings = Settings::from_json(jsonFile.c_str());

    gloablSettings.printsetting();
    return 0;
}