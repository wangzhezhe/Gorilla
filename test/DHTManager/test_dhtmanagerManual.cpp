

#include "../server/DHTManager/dhtmanager.h"
#include <vector>
using namespace GORILLA;

void test1d()
{

    std::cout << "---test1d dht manual---" << std::endl;

    std::vector<int> lenArray = {9};

    std::vector<int> partitionLayout = {2};

    DHTManager *dhtm = new DHTManager();

    dhtm->initDHTManually(lenArray, partitionLayout);

    dhtm->printDTMInfo();
}

void test2d()
{

    std::cout << "---test2d dht manual---" << std::endl;

    std::vector<int> lenArray = {9, 10};

    std::vector<int> partitionLayout = {2, 2};

    DHTManager *dhtm = new DHTManager();

    dhtm->initDHTManually(lenArray, partitionLayout);

    dhtm->printDTMInfo();
}

void test3d()
{
    std::cout << "---test3d dht manual---" << std::endl;

    std::vector<int> lenArray = {9, 10, 11};

    std::vector<int> partitionLayout = {2, 2, 3};

    DHTManager *dhtm = new DHTManager();

    dhtm->initDHTManually(lenArray, partitionLayout);

    dhtm->printDTMInfo();

    std::cout << "---test3d dht manual B---" << std::endl;

    lenArray = {512, 512, 512};

    partitionLayout = {1, 1, 1};

    DHTManager *dhtm2 = new DHTManager();

    dhtm2->initDHTManually(lenArray, partitionLayout);

    dhtm2->printDTMInfo();
}

int main()
{
    test1d();
    test2d();
    test3d();
}