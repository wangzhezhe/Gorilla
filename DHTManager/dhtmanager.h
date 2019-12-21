#ifndef DHTMANAGER_H
#define DHTMANAGER_H

#include <vector>
#include <climits>

//the bound value for specific dimention
struct Bound{
    Bound(){};
    int m_lb = INT_MAX;
    int m_ub = INT_MIN;
    ~Bound(){};
}

//the bbox for the application domain
//the lb/ub represents the lower bound and the upper bound of every dimention
//Attention, if there is 1 elements for every dimention, the lower bound is same with the upper bound
//TODO update, use the vector of the Bound
struct BBX{
    //the bound for every dimention
    std::vector<Bound> BBX;
    ~BBX();


    void printBBXinfo(){
        int dim = BBX.size();
        std::cout << "dim: " << dim << std::endl;
        for(i=0;i<dim;i++){
            std::cout << "lb: " << BBX[i].m_lb << "," 
        }
        std::cout<<std::endl;
        for(i=0;i<dim;i++){
            std::cout << "ub: " << BBX[i].m_ub << "," 
        }
        std::cout<<std::endl;
    }
};

struct responsibleMetaServer{
    int metaServerID;
    BBX bbx;
}


struct DHTManager{

    DHTManager(){};

    //init the metaServerBBOXList according to the partitionNum and the bbox of the global domain
    void initDHT(int metaServerNum, BBOX& globalBBX);

    //get the corresponding metaserver according to the input bbox
    std::vector<responsibleMetaServer> getMetaServerID(BBX& BBXQuery);

    std::map<int, BBX> metaServerIDToBBX;

    //TODO, aggregate bounding box, input a list that contains several BBX and then aggregate them into a large one
};



#endif