#include "../server/MetadataManager/metadataManager.h"
#include <iostream>
#include <thallium.hpp>

using namespace GORILLA;

void test_overlap2d()
{
    std::cout << "---test_overlap2d---" << std::endl;
    //set rdep list
    std::vector<BlockDescriptor> rdeplist;
    rdeplist.push_back(BlockDescriptor("serverAddr", "dataID1", GORILLA::DATATYPE_CARGRID, 2, {{0, 0}}, {{15, 15}}));
    rdeplist.push_back(BlockDescriptor("serverAddr", "dataID2", GORILLA::DATATYPE_CARGRID,2, {{16, 0}}, {{31, 15}}));
    rdeplist.push_back(BlockDescriptor("serverAddr", "dataID3", GORILLA::DATATYPE_CARGRID,2, {{0, 16}}, {{15, 31}}));

    //set query
    std::cout << "---case0---" << std::endl;
    MetaDataManager metam;
    bool ifcover = metam.ifCovered(rdeplist, BBX(2, std::array<int, DEFAULT_MAX_DIM>({0, 0}), std::array<int, DEFAULT_MAX_DIM>({31, 31})));

    if (ifcover == true)
    {
        throw std::runtime_error("error for ifcovered case0");
    }

    rdeplist.push_back(BlockDescriptor("serverAddr", "dataID4",DATATYPE_CARGRID, 2, std::array<int, DEFAULT_MAX_DIM>({16, 16}), std::array<int, DEFAULT_MAX_DIM>({31, 31})));

    //set query
    std::cout << "---case1---" << std::endl;
    ifcover = metam.ifCovered(rdeplist, BBX(2, std::array<int, DEFAULT_MAX_DIM>({0, 0}), std::array<int, DEFAULT_MAX_DIM>({31, 31})));

    if (ifcover == false)
    {
        throw std::runtime_error("error for ifcovered case1");
    }

    std::cout << "---case2---" << std::endl;
    ifcover = metam.ifCovered(rdeplist, BBX(2, std::array<int, DEFAULT_MAX_DIM>({0, 0}), std::array<int, DEFAULT_MAX_DIM>({64, 64})));

    if (ifcover == true)
    {
        throw std::runtime_error("error for ifcovered case2");
    }

    std::cout << "---case3---" << std::endl;
    ifcover = metam.ifCovered(rdeplist, BBX(2, std::array<int, DEFAULT_MAX_DIM>({0, 0}), std::array<int, DEFAULT_MAX_DIM>({16, 16})));

    if (ifcover == false)
    {
        throw std::runtime_error("error for ifcovered case3");
    }

    std::cout << "---case4---" << std::endl;
    ifcover = metam.ifCovered(rdeplist, BBX(2, std::array<int, DEFAULT_MAX_DIM>({8, 8}), std::array<int, DEFAULT_MAX_DIM>({25, 25})));

    if (ifcover == false)
    {
        throw std::runtime_error("error for ifcovered case4");
    }

    return;
}

void test_overlap3d()
{
    std::cout << "---test_overlap3d---" << std::endl;
    //set rdep list
    std::vector<BlockDescriptor> rdeplist;
    rdeplist.push_back(BlockDescriptor("serverAddr", "dataID1", DATATYPE_CARGRID,3, {{0, 0, 0}}, {{15, 15, 31}}));
    rdeplist.push_back(BlockDescriptor("serverAddr", "dataID2", DATATYPE_CARGRID,3, {{16, 0, 0}}, {{31, 15, 31}}));
    rdeplist.push_back(BlockDescriptor("serverAddr", "dataID3", DATATYPE_CARGRID,3, {{0, 16, 0}}, {{16, 31, 31}}));

    //set query
    MetaDataManager metam;
    std::cout << "---case0---" << std::endl;
    bool ifcover = metam.ifCovered(rdeplist, BBX(3, std::array<int, DEFAULT_MAX_DIM>({0, 0, 0}), std::array<int, DEFAULT_MAX_DIM>({31, 31, 31})));

    if (ifcover == true)
    {
        throw std::runtime_error("error for ifcovered case0");
    }

    rdeplist.push_back(BlockDescriptor("serverAddr", "dataID4", DATATYPE_CARGRID,3, {{16, 16, 0}}, {{31, 31, 31}}));
    std::cout << "---case1---" << std::endl;
     ifcover = metam.ifCovered(rdeplist, BBX(3, std::array<int, DEFAULT_MAX_DIM>({0, 0, 0}), std::array<int, DEFAULT_MAX_DIM>({31, 31, 31})));

    if (ifcover == false)
    {
        throw std::runtime_error("error for ifcovered case1");
    }

    return;
}

int main()
{
    tl::abt scope;
    test_overlap2d();
    test_overlap3d();
}