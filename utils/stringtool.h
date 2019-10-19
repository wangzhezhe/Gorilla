

#ifndef STRINGTOOL_H
#define STRINGTOOL_H


#include <string>
#include <iostream>
#include <vector>
#include <cstring>

namespace IPTOOL{
std::vector<std::string> split(const char *s, int size, char seperatorH, char seperatorE)
{
    std::vector<std::string> result;
    typedef std::string::size_type string_size;
    string_size i = 0;
    int flag = 0;

    while (i != size)
    {
        //if flag =0 , estra str
        //if flag =1 , real content
        //int flag = 0;
        while (i != size && flag == 0)
        {
            //caculate start position

            if (s[i] == seperatorH)
            {
                flag = 1;
                ++i;
                break;
            }else{
                ++i;
            }
        }

        //caculate end position
        string_size j = i;

        while (j != size && flag == 1)
        {

            if (s[j] == seperatorE)
            {
                flag = 0;
                break;
            }

            if (flag == 1)
                ++j;
        }

        if (i != j)
        {
            char substr[100];
            std::memcpy(substr, s + i, j - i);
            result.push_back(std::string(substr));
            i = j;
        }
    }
    return result;
}


std::string getClientAdddr(std::string deviceType, std::string MargoAddr){
    

    //get contents after the 
    
    std::string filteredAddr = MargoAddr.substr(MargoAddr.find(":"), MargoAddr.size());
    //combine it together with the deviceType
    std::string addrForClient = deviceType+filteredAddr;
    
    return addrForClient;
}
}




#endif