

#include "../server/DHTManager/dhtmanager.h"
#include <vector>

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
}

int main()
{
    test1d();
    test2d();
    test3d();
}