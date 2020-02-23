
#include "../../server/unimosserver.hpp"
#include "../../server/settings.h"
#include "../../utils/matrixtool.h"
#include "../../utils/uuid.h"

#include <spdlog/spdlog.h>
#include <vector>

#include "sim_single.h"

namespace tl = thallium;

Settings gloablSettings;
std::vector<UniServer *> serverList;

void initDHT(UniServer *uniServer)
{
    int dataDims = gloablSettings.lenArray.size();
    //config the dht manager
    if (gloablSettings.partitionMethod.compare("SFC") == 0)
    {
        int maxLen = 0;
        for (int i = 0; i < dataDims; i++)
        {
            maxLen = std::max(maxLen, gloablSettings.lenArray[i]);
        }

        BBX *globalBBX = new BBX(dataDims);
        for (int i = 0; i < dataDims; i++)
        {
            Bound *b = new Bound(0, maxLen - 1);
            globalBBX->BoundList.push_back(b);
        }
        uniServer->m_dhtManager->initDHTBySFC(dataDims, gloablSettings.metaserverNum, globalBBX);
    }
    else if (gloablSettings.partitionMethod.compare("manual") == 0)
    {
        //check the metaserverNum match with the partitionLayout
        int totalPartition = 1;
        for (int i = 0; i < gloablSettings.partitionLayout.size(); i++)
        {
            totalPartition = totalPartition * gloablSettings.partitionLayout[i];
        }
        if (totalPartition != gloablSettings.metaserverNum)
        {
            throw std::runtime_error("metaserverNum should equals to the product of partitionLayout[i]");
        }

        uniServer->m_dhtManager->initDHTManually(gloablSettings.lenArray, gloablSettings.partitionLayout);
    }
    else
    {
        throw std::runtime_error("unsuported partition method " + gloablSettings.partitionMethod);
    }

    return;
}

void initAddrandDHT(int current, int totalSerNum, UniServer *uniServer)
{
    //init current addr
    uniServer->m_addrManager->nodeAddr = std::to_string(current);

    //init the list for other addr
    for (int i = 0; i < totalSerNum; i++)
    {
        std::string mockAddr = std::to_string(i);
        uniServer->m_addrManager->m_endPointsLists.push_back(mockAddr);
    }

    //update the DHT for each addr tell the server, what is the addr for each id
    for (auto it = uniServer->m_addrManager->m_endPointsLists.begin(); it != uniServer->m_addrManager->m_endPointsLists.end(); it++)
    {
        std::string serverAddr = *it;

        std::vector<MetaAddrWrapper> metaAddrWrapperList;

        for (int i = 0; i < totalSerNum; i++)
        {
            MetaAddrWrapper mdw(i, uniServer->m_addrManager->m_endPointsLists[i]);
            metaAddrWrapperList.push_back(mdw);
        }

        //this is the code at the server end actually
        //the main operation is to put the data into the map(metaServerIDToAddr)
        int size = metaAddrWrapperList.size();

        for (int i = 0; i < size; i++)
        {
            //TODO init dht
            spdlog::debug("rank {} add meta server, index {} and addr {}", current, metaAddrWrapperList[i].m_index, metaAddrWrapperList[i].m_addr);
            //TODO add lock here
            uniServer->m_dhtManager->metaServerIDToAddr[metaAddrWrapperList[i].m_index] = metaAddrWrapperList[i].m_addr;
        }
    }
}

//size_t &step, std::string &varName, BlockSummary &blockSummary, tl::bulk &dataBulk
std::vector<MetaDataWrapper> mockAPIPutrawdata(UniServer *uniServer,
                                               size_t &step,
                                               std::string &varName,
                                               BlockSummary &blockSummary,
                                               void *inputRawData)
{

    std::vector<MetaDataWrapper> metadataWrapperList;
    //caculate the blockid by uuid
    std::string blockID = UUIDTOOL::generateUUID();

    spdlog::debug("blockID is {} on server {} ", blockID, uniServer->m_addrManager->nodeAddr);

    //get source id
    //tl::endpoint ep = req.get_endpoint();

    //assign the memory
    size_t mallocSize = blockSummary.m_elemSize * blockSummary.m_elemNum;

    spdlog::debug("malloc size is {}", mallocSize);

    void *localContainer = (void *)malloc(mallocSize);
    if (localContainer == NULL)
    {
        blockSummary.printSummary();
        spdlog::info("failed to malloc data");
        return metadataWrapperList;
    }

    //copy into the localContainer
    //to simulate the process of the data transfer
    memcpy(localContainer, inputRawData, mallocSize);

    int status = uniServer->m_blockManager->putBlock(blockID, blockSummary, localContainer);

    spdlog::debug("---put block {} on server {} map size {}", blockID, uniServer->m_addrManager->nodeAddr, uniServer->m_blockManager->DataBlockMap.size());
    if (status != 0)
    {
        blockSummary.printSummary();
        throw std::runtime_error("failed to put the raw data");
        return metadataWrapperList;
    }

    //put into the Block Manager

    //get the meta server according to bbx
    BBXTOOL::BBX *BBXQuery = new BBXTOOL::BBX(blockSummary.m_dims, blockSummary.m_indexlb, blockSummary.m_indexub);

    std::vector<ResponsibleMetaServer> metaserverList = uniServer->m_dhtManager->getMetaServerID(BBXQuery);

    //update the coresponding metadata server, send information to corespond meta
    if (gloablSettings.logLevel > 0)
    {
        spdlog::debug("step {} for var {} size of metaserverList {}", step, varName, metaserverList.size());
        for (int i = 0; i < metaserverList.size(); i++)
        {
            std::cout << "metaserver index " << metaserverList[i].m_metaServerID << std::endl;
            metaserverList[i].m_bbx->printBBXinfo();
        }
    }
    if (BBXQuery != NULL)
    {
        free(BBXQuery);
    }

    //update the meta data by async way to improve the performance of the put operation
    //the return value should be a vector of the metaDataWrapper
    for (auto it = metaserverList.begin(); it != metaserverList.end(); it++)
    {
        std::array<int, 3> indexlb = it->m_bbx->getIndexlb();
        std::array<int, 3> indexub = it->m_bbx->getIndexub();

        int metaServerId = it->m_metaServerID;
        if (uniServer->m_dhtManager->metaServerIDToAddr.find(metaServerId) == uniServer->m_dhtManager->metaServerIDToAddr.end())
        {
            throw std::runtime_error("faild to get the coresponding server id in dhtManager");
        }
        RawDataEndpoint rde(
            uniServer->m_addrManager->nodeAddr,
            blockID,
            blockSummary.m_dims, indexlb, indexub);
        //if dest of server is current one
        std::string destAddr = uniServer->m_dhtManager->metaServerIDToAddr[metaServerId];
        if (destAddr.compare(uniServer->m_addrManager->nodeAddr) == 0)
        {
            uniServer->m_metaManager->updateMetaData(step, varName, rde);
        }
        else
        {
            MetaDataWrapper mdw(destAddr, step, varName, rde);
            metadataWrapperList.push_back(mdw);
        }
    }
    return metadataWrapperList;
}

int mockServerPutmetadata(UniServer *uniServer, size_t &step, std::string &varName, RawDataEndpoint &rde)
{

    spdlog::debug("server {} put meta", uniServer->m_addrManager->nodeAddr);
    if (gloablSettings.logLevel > 0)
    {
        rde.printInfo();
    }
    uniServer->m_metaManager->updateMetaData(step, varName, rde);

    //execute init trigger
    //if the trigger is true
    if (gloablSettings.addTrigger)
    {
        uniServer->m_dtmanager->initstart("InitTrigger", step, varName, rde);
    }
    return 0;
}

std::vector<std::string> mockServergetmetaServerList(UniServer *uniServer, size_t dims, std::array<int, 3> indexlb, std::array<int, 3> indexub)
{
    std::vector<std::string> metaServerAddr;

    BBXTOOL::BBX *BBXQuery = new BBXTOOL::BBX(dims, indexlb, indexub);
    std::vector<ResponsibleMetaServer> metaserverList = uniServer->m_dhtManager->getMetaServerID(BBXQuery);
    for (auto it = metaserverList.begin(); it != metaserverList.end(); it++)
    {
        int metaServerId = it->m_metaServerID;
        if (uniServer->m_dhtManager->metaServerIDToAddr.find(metaServerId) == uniServer->m_dhtManager->metaServerIDToAddr.end())
        {
            throw std::runtime_error("faild to get the coresponding server id in dhtManager for getmetaServerList");
        }
        metaServerAddr.push_back(uniServer->m_dhtManager->metaServerIDToAddr[metaServerId]);
    }
    free(BBXQuery);
    return metaServerAddr;
}

std::vector<RawDataEndpoint> mockServergetrawDataEndpointList(UniServer *uniServer,
                                                              size_t step,
                                                              std::string varName,
                                                              size_t dims,
                                                              std::array<int, 3> indexlb,
                                                              std::array<int, 3> indexub)
{
    std::vector<RawDataEndpoint> rawDataEndpointList;
    BBXTOOL::BBX *BBXQuery = new BBXTOOL::BBX(dims, indexlb, indexub);
    rawDataEndpointList = uniServer->m_metaManager->getOverlapEndpoints(step, varName, BBXQuery);
    free(BBXQuery);
    return rawDataEndpointList;
}

int mockServergetSubregionData(UniServer *uniServer,
                               std::string &blockID,
                               size_t &dims,
                               std::array<int, 3> &subregionlb,
                               std::array<int, 3> &subregionub,
                               void *outputRawData)
{

    try
    {

        spdlog::debug("map size on server {} is {}", uniServer->m_addrManager->nodeAddr, uniServer->m_blockManager->DataBlockMap.size());
        if (uniServer->m_blockManager->checkDataExistance(blockID) == false)
        {
            throw std::runtime_error("failed to get block id " + blockID + " on server with rank id " + uniServer->m_addrManager->nodeAddr);
        }
        BlockSummary bs = uniServer->m_blockManager->getBlockSummary(blockID);
        size_t elemSize = bs.m_elemSize;
        BBXTOOL::BBX *bbx = new BBXTOOL::BBX(dims, subregionlb, subregionub);
        size_t allocSize = elemSize * bbx->getElemNum();
        spdlog::debug("alloc size at server {} is {}", uniServer->m_addrManager->nodeAddr, allocSize);

        //simulate the process of data copy
        uniServer->m_blockManager->getBlockSubregion(blockID, dims, subregionlb, subregionub, outputRawData);

        free(bbx);
        return 0;
    }
    catch (std::exception &e)
    {
        spdlog::info("exception for getDataSubregion block id {} lb {},{},{} ub {},{},{}", blockID,
                     subregionlb[0], subregionlb[1], subregionlb[2],
                     subregionub[0], subregionub[1], subregionub[2]);
        spdlog::info("exception for getDataSubregion: {}", std::string(e.what()));
        return -1;
    }
}

void mockServerGetrawdata(UniServer *uniServer)
{
}

int mockClientPutRawData(UniServer *uniServer,
                         size_t &step,
                         std::string &varName,
                         BlockSummary &blockSummary,
                         void *inputRawData)
{

    //get the List
    std::vector<MetaDataWrapper> metadataWrapperList = mockAPIPutrawdata(uniServer,
                                                                         step,
                                                                         varName,
                                                                         blockSummary,
                                                                         inputRawData);

    //update the server according to this list
    for (auto it = metadataWrapperList.begin(); it != metadataWrapperList.end(); it++)
    {
        //get dest server
        int destSerId = std::stoi(it->m_destAddr);
        UniServer *destServer = serverList[destSerId];
        int status = mockServerPutmetadata(destServer, it->m_step, it->m_varName, it->m_rde);
        if (status != 0)
        {
            spdlog::error("failed to put metadata");
            it->printInfo();
            return -1;
        }
    }
    return 0;
}

MATRIXTOOL::MatrixView mockClientGetArbitratyData(
    UniServer *uniServer,
    size_t step,
    std::string varName,
    size_t elemSize,
    size_t dims,
    std::array<int, 3> indexlb,
    std::array<int, 3> indexub)
{

    //TODO use the mock service
    std::vector<std::string> metaList = mockServergetmetaServerList(uniServer, dims, indexlb, indexub);

    std::vector<MATRIXTOOL::MatrixView> matrixViewList;

    for (auto it = metaList.begin(); it != metaList.end(); it++)
    {
        int metaServerID = std::stoi(*it);
        //TODO use the mock service
        //get the dest server
        UniServer *destuniServer = serverList[metaServerID];
        std::vector<RawDataEndpoint> rweList =
            mockServergetrawDataEndpointList(destuniServer, step, varName, dims, indexlb, indexub);

        //std::cout << "metadata server id" << metaServerID << " size of rweList " << rweList.size() << std::endl;
        if (rweList.size() == 0)
        {
            throw std::runtime_error("failed to get the overlap raw data endpoint for " + std::to_string(metaServerID));
        }
        for (auto itrwe = rweList.begin(); itrwe != rweList.end(); itrwe++)
        {
            //itrwe->printInfo();

            //get the subrigion according to the information at the list
            //it is better to use the multithread here

            //get subrigion of the data
            //allocate size (use differnet strategy is the vtk object is used)
            BBXTOOL::BBX *bbx = new BBXTOOL::BBX(dims, itrwe->m_indexlb, itrwe->m_indexub);
            size_t allocSize = sizeof(double) * bbx->getElemNum();
            //std::cout << "alloc size is " << allocSize << std::endl;

            void *subDataContainer = (void *)malloc(allocSize);

            int serverId = std::stoi(itrwe->m_rawDataServerAddr);
            UniServer *datauniServer = serverList[serverId];
            int status = mockServergetSubregionData(datauniServer,
                                                    itrwe->m_rawDataID,
                                                    dims,
                                                    itrwe->m_indexlb,
                                                    itrwe->m_indexub,
                                                    subDataContainer);

            if (status != 0)
            {
                throw std::runtime_error("failed for get subrigion data for current raw data endpoint");
            }

            MATRIXTOOL::MatrixView mv(bbx, subDataContainer);
            matrixViewList.push_back(mv);
        }
    }

    //stage 1
    //clock_gettime(CLOCK_REALTIME, &end1);
    //diff1 = (end1.tv_sec - start.tv_sec) * 1.0 + (end1.tv_nsec - start.tv_nsec) * 1.0 / BILLION;
    //std::cout<<"get data stage 1: " << diff1 << std::endl;

    //assemble the matrix View
    BBX *intactbbx = new BBX(dims, indexlb, indexub);
    MATRIXTOOL::MatrixView mvassemble = MATRIXTOOL::matrixAssemble(elemSize, matrixViewList, intactbbx);

    //free the element in matrixViewList
    for (auto it = matrixViewList.begin(); it != matrixViewList.end(); it++)
    {
        if (it->m_bbx != NULL)
        {
            free(it->m_bbx);
            it->m_bbx = NULL;
        }
        if (it->m_data != NULL)
        {
            free(it->m_data);
            it->m_data = NULL;
        }
    }

    //stage 2
    //clock_gettime(CLOCK_REALTIME, &end2);
    //diff2 = (end2.tv_sec - end1.tv_sec) * 1.0 + (end2.tv_nsec - end1.tv_nsec) * 1.0 / BILLION;
    //std::cout<<"get data stage 2: " << diff2 << std::endl;

    return mvassemble;
}

void print_settings(const SingleSettings &s)
{
    std::cout << "grid:             " << s.lenArray[0] << "x" << s.lenArray[1] << "x" << s.lenArray[2] << std::endl;
    std::cout << "steps:            " << s.steps << std::endl;
    std::cout << "plotgap:          " << s.plotgap << std::endl;
    std::cout << "calcutime:        " << s.calcutime << std::endl;
    std::cout << "output:           " << s.output << std::endl;
    std::cout << "processLayout:    " << s.processLayout[0] << "x" << s.processLayout[1] << "x" << s.processLayout[2] << std::endl;
}

int main(int argc, char **argv)
{

    //get the config file
    if (argc != 3)
    {

        std::cerr << "Usage: unimos_server <path of server_setting.json> <sim_setting.json>" << std::endl;
        exit(0);
    }

    tl::abt scope;

    //the copy operator is called here
    Settings tempsetting = Settings::from_json(argv[1]);
    gloablSettings = tempsetting;

    gloablSettings.printsetting();

    int logLevel = gloablSettings.logLevel;

    if (logLevel == 0)
    {
        spdlog::set_level(spdlog::level::info);
    }
    else
    {
        spdlog::set_level(spdlog::level::debug);
    }

    int totalServerNum = gloablSettings.metaserverNum;

    for (int i = 0; i < totalServerNum; i++)
    {
        UniServer *tempServer = new UniServer();
        tempServer->initManager(totalServerNum, totalServerNum, NULL, false);
        initDHT(tempServer);
        if (i == 0)
        {
            //print metaServerIDToBBX
            for (auto it = tempServer->m_dhtManager->metaServerIDToBBX.begin(); it != tempServer->m_dhtManager->metaServerIDToBBX.end(); it++)
            {
                std::cout << "init DHT, meta id " << it->first << std::endl;
                it->second->printBBXinfo();
            }
        }

        //add addr and dht
        initAddrandDHT(i, totalServerNum, tempServer);
        serverList.push_back(tempServer);
        spdlog::info("init mock server ok, for rank {}", i);
    }

    //init the mock simulation and write the data into the server
    std::cout << "================start sim====================" << std::endl;

    SingleSettings simSettings = SingleSettings::from_json(argv[2]);

    std::vector<SingleSim *> simList;
    int npx = simSettings.processLayout[0];
    int npy = simSettings.processLayout[1];
    int npz = simSettings.processLayout[2];

    int totalNum = npx * npy * npz;

    for (int pz = 0; pz < npz; pz++)
    {
        for (int py = 0; py < npy; py++)
        {
            for (int px = 0; px < npx; px++)
            {
                int id = pz * npy * npx + py * npx + px;
                SingleSim *simInstance = new SingleSim(simSettings, id, totalNum);
                int coord[3] = {px, py, pz};
                simInstance->init(simSettings, coord, 1.0 * id);
                simList.push_back(simInstance);
            }
        }
    }

    print_settings(simSettings);
    std::cout << "========================================" << std::endl;

    // go through all the ranks
    int globalCount = 0;
    for (size_t t = 0; t < (size_t)simSettings.steps; t++)
    {
        std::cout << "sim iterate step " << t << std::endl;
        for (int pz = 0; pz < npz; pz++)
        {
            for (int py = 0; py < npy; py++)
            {
                for (int px = 0; px < npx; px++)
                {
                    int id = pz * npy * npx + py * npx + px;
                    SingleSim *simInstance = simList[id];
                    simInstance->iterate(simSettings.calcutime);
                    if (id % 100 == 0)
                    {
                        std::cout << " rank " << id << std::endl;
                    }

                    //get the id of the server
                    int serverID = globalCount % totalServerNum;
                    globalCount++;
                    //std::cout << "serverID " << serverID << std::endl;
                    UniServer *tempUniServer = serverList[serverID];

                    //write the data into the server    std::vector<double> u = sim.u_noghost();

                    std::string VarNameU = "grascott_u";
                    std::vector<double> u = simInstance->u_noghost();

                    std::array<int, 3> indexlb = {{(int)simInstance->offset_x, (int)simInstance->offset_y, (int)simInstance->offset_z}};
                    std::array<int, 3> indexub = {{(int)(simInstance->offset_x + simInstance->size_x - 1), (int)(simInstance->offset_y + simInstance->size_y - 1), (int)(simInstance->offset_z + simInstance->size_z - 1)}};

                    size_t elemSize = sizeof(double);
                    size_t elemNum = simInstance->size_x * simInstance->size_y * simInstance->size_z;

                    //generate raw data summary block
                    BlockSummary bs(elemSize, elemNum,
                                    DRIVERTYPE_RAWMEM,
                                    3,
                                    indexlb,
                                    indexub);

                    mockClientPutRawData(tempUniServer,
                                         t,
                                         VarNameU,
                                         bs,
                                         u.data());
                }
            }
        }
    }

    npx = simSettings.processLayout[0];
    npy = simSettings.processLayout[1];
    npz = simSettings.processLayout[2];

    // simulate the process of the reader to get the data
    globalCount = 0;
    for (size_t t = 0; t < (size_t)simSettings.steps; t++)
    {
        std::cout << " ana iterate step " << t << std::endl;
        for (int pz = 0; pz < npz; pz++)
        {
            for (int py = 0; py < npy; py++)
            {
                for (int px = 0; px < npx; px++)
                {
                    int id = pz * npy * npx + py * npx + px;

                    int size_x = (simSettings.lenArray[0] + npx - 1) / npx;
                    int size_y = (simSettings.lenArray[1] + npy - 1) / npy;
                    int size_z = (simSettings.lenArray[2] + npz - 1) / npz;

                    int offset_x = size_x * px;
                    int offset_y = size_y * py;
                    int offset_z = size_z * pz;

                    if (px == npx - 1)
                    {
                        size_x -= size_x * npx - simSettings.lenArray[0];
                    }
                    if (py == npy - 1)
                    {
                        size_y -= size_y * npy - simSettings.lenArray[1];
                    }
                    if (pz == npz - 1)
                    {
                        size_z -= size_z * npz - simSettings.lenArray[2];
                    }

                    std::array<int, 3> indexlb = {{offset_x, offset_y, offset_z}};
                    std::array<int, 3> indexub = {{offset_x + size_x - 1, offset_y + size_y - 1, offset_z + size_z - 1}};

                    int serverID = globalCount % totalServerNum;
                    globalCount++;
                    UniServer *tempUniServer = serverList[serverID];
                    //caculate the bbx
                    std::string VarNameU = "grascott_u";
                    MATRIXTOOL::MatrixView mvassemble = mockClientGetArbitratyData(tempUniServer, t, VarNameU, sizeof(double), 3, indexlb, indexub);
                }
            }
        }
    }

    return 0;
}