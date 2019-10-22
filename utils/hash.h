
#ifndef HASH_H
#define HASH_H

#include <string>

namespace HASHFUNC{
//the hash function may used by both client and the server
size_t getIdByVarAndTs(std::string varName, int ts){
    std::string identifier = varName + "_" + std::to_string(ts);
    std::hash<std::string> hashFun;
    size_t strHashID = hashFun(identifier);
    return strHashID;
}

}

#endif