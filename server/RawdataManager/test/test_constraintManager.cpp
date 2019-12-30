#include <string>
#include "../server/constraintManager.h"
#include <iostream>


int main(){
    //init manager
    std::vector<std::string> contentFilterName;
    contentFilterName.push_back("default");
    constraintManager cm = constraintManager("default","default","default");

    //call the specific function

    bool cmResult = cm.execute(0, 0, NULL);

    std::cout << "cm test results " << cmResult <<std::endl;
    
    //range map call specific function

    if(cmResult==false){
        throw std::runtime_error("failed to check results\n");
    }

    return 0;
}