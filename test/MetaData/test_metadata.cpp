

#include "../server/MetadataManager/metadataManager.h"
using namespace GORILLA;

void testMetaDataOverlap()
{
  std::cout << "---testMetaDataOverlap1---" << std::endl;
  tl::abt scope;
  MetaDataManager metam;
  std::string varname = "testVarName";
  std::string serverAddr = "testserverAddr";
  std::string dataID1 = "testDataID1";
  std::string dataID2 = "testDataID2";
  std::string dataID3 = "testDataID3";
  std::string dataID4 = "testDataID4";

  BlockDescriptor rde1(serverAddr, dataID1, DATATYPE_CARGRID, 2, { { 0, 0 } }, { { 5, 5 } });
  BlockDescriptor rde2(serverAddr, dataID2, DATATYPE_CARGRID, 2, { { 0, 6 } }, { { 5, 11 } });
  BlockDescriptor rde3(serverAddr, dataID3, DATATYPE_CARGRID, 2, { { 6, 6 } }, { { 11, 11 } });
  BlockDescriptor rde4(serverAddr, dataID4, DATATYPE_CARGRID, 2, { { 6, 0 } }, { { 11, 5 } });

  metam.updateMetaData(0, varname, rde1);
  metam.updateMetaData(0, varname, rde2);
  metam.updateMetaData(0, varname, rde3);
  metam.updateMetaData(0, varname, rde4);

  std::array<int, DEFAULT_MAX_DIM> indexlb = { { 3, 3, 0 } };
  std::array<int, DEFAULT_MAX_DIM> indexub = { { 8, 8, 0 } };

  BBX queryBBX(2, indexlb, indexub);

  std::vector<BlockDescriptor> endpointList = metam.getOverlapEndpoints(0, varname, queryBBX);
  for (auto it = endpointList.begin(); it != endpointList.end(); ++it)
  {
    it->printInfo();
  }

  if (endpointList.size() != 4)
  {
    throw std::runtime_error("failed for metaDataMap testMetaDataOverlap\n");
  }

  std::cout << "---testMetaDataOverlap2---" << std::endl;
  indexlb = { { 0, 0, 0 } };
  indexub = { { 3, 3, 0 } };
  BBX queryBBX2(2, indexlb, indexub);

  std::vector<BlockDescriptor> endpointList2 = metam.getOverlapEndpoints(0, varname, queryBBX2);
  for (auto it = endpointList2.begin(); it != endpointList2.end(); ++it)
  {
    it->printInfo();
  }
  if (endpointList2.size() != 1)
  {
    throw std::runtime_error("failed for metaDataMap testMetaDataOverlap\n");
  }
}

void testMetaData()
{
  tl::abt scope;
  MetaDataManager metam(10, 5, 1);
  for (int i = 0; i < 15; i++)
  {
    std::string varname = "testVarName" + std::to_string(i);
    std::string serverAddr1 = "testserverAddr1" + std::to_string(i);
    std::string serverAddr2 = "testserverAddr2" + std::to_string(i);
    std::string serverAddr3 = "testserverAddr3" + std::to_string(i);

    std::array<int, DEFAULT_MAX_DIM> indexlb = { { 0, 0, 0 } };
    std::array<int, DEFAULT_MAX_DIM> indexub = { { 1, 1, 1 } };

    BlockDescriptor rde1(serverAddr1, std::to_string(i), DATATYPE_CARGRID, 3, indexlb, indexub);
    BlockDescriptor rde2(serverAddr2, std::to_string(i), DATATYPE_CARGRID, 3, indexlb, indexub);

    BlockDescriptor rde3(serverAddr3, std::to_string(i), DATATYPE_CARGRID, 3, indexlb, indexub);

    metam.updateMetaData(i, varname, rde1);
    metam.updateMetaData(i, varname, rde2);
    metam.updateMetaData(i, varname, rde3);
  }

  auto it = metam.m_metaDataMap.begin();
  while (it != metam.m_metaDataMap.end())
  {
    std::cout << it->first << std::endl;
    auto innerit = it->second.begin();
    while (innerit != it->second.end())
    {
      // range vector
      int size = innerit->second.m_metadataBlock["rawdata"].size();
      std::cout << "varName " << innerit->first << " vector size " << size << std::endl;
      for (int i = 0; i < size; i++)
      {
        std::cout << innerit->first << " "
                  << innerit->second.m_metadataBlock["rawdata"][i].m_rawDataServerAddr << " "
                  << innerit->second.m_metadataBlock["rawdata"][i].m_rawDataID << std::endl;
      }

      innerit++;
    }
    it++;
  }

  /*
  TODO, need to delete the data stored at the raw data server after shrink the metadata map
  if (metam.m_metaDataMap.size() != 10)
  {
    throw std::runtime_error("failed for metaDataMap size\n");
  }

  if ((metam.m_windowlb != 5) || (metam.m_windowub != 14))
  {
    throw std::runtime_error("failed for metaDataMap windlow bound\n");
  }
  */
}

void testMetaDataType()
{

  tl::abt scope;
  MetaDataManager metam(10, 5, 1);
  for (int i = 0; i < 15; i++)
  {
    std::string varname = "testVarName" + std::to_string(i);
    std::string serverAddr1 = "testserverAddr1" + std::to_string(i);
    std::string serverAddr2 = "testserverAddr2" + std::to_string(i);
    std::string serverAddr3 = "testserverAddr3" + std::to_string(i);

    std::array<int, DEFAULT_MAX_DIM> indexlb = { { 0, 0, 0 } };
    std::array<int, DEFAULT_MAX_DIM> indexub = { { 1, 1, 1 } };

    BlockDescriptor rde1(serverAddr1, std::to_string(i), DATATYPE_CARGRID, 3, indexlb, indexub);
    BlockDescriptor rde2(serverAddr2, std::to_string(i), DATATYPE_CARGRID, 3, indexlb, indexub);

    BlockDescriptor rde3(serverAddr3, std::to_string(i), DATATYPE_CARGRID, 3, indexlb, indexub);

    metam.updateMetaData(i, varname, rde1);
    metam.updateMetaData(i, varname, rde2);
    metam.updateMetaData(i, varname, rde3);
  }

  auto it = metam.m_metaDataMap.begin();
  while (it != metam.m_metaDataMap.end())
  {
    std::cout << it->first << std::endl;
    auto innerit = it->second.begin();
    while (innerit != it->second.end())
    {
      // range vector
      int size = innerit->second.m_metadataBlock["type1"].size();
      std::cout << "varName " << innerit->first << " vector size " << size << std::endl;
      for (int i = 0; i < size; i++)
      {
        std::cout << innerit->first << " "
                  << innerit->second.m_metadataBlock["type1"][i].m_rawDataServerAddr << " "
                  << innerit->second.m_metadataBlock["type1"][i].m_rawDataID << std::endl;
      }

      size = innerit->second.m_metadataBlock["type2"].size();
      std::cout << "varName " << innerit->first << " vector size " << size << std::endl;
      for (int i = 0; i < size; i++)
      {
        std::cout << innerit->first << " "
                  << innerit->second.m_metadataBlock["type2"][i].m_rawDataServerAddr << " "
                  << innerit->second.m_metadataBlock["type2"][i].m_rawDataID << std::endl;
      }

      size = innerit->second.m_metadataBlock["type3"].size();
      std::cout << "varName " << innerit->first << " vector size " << size << std::endl;
      for (int i = 0; i < size; i++)
      {
        std::cout << innerit->first << " "
                  << innerit->second.m_metadataBlock["type3"][i].m_rawDataServerAddr << " "
                  << innerit->second.m_metadataBlock["type3"][i].m_rawDataID << std::endl;
      }

      innerit++;
    }
    it++;
  }

  /*
  TODO, need to delete the data stored at the raw data server after shrink the metadata map
  if (metam.m_metaDataMap.size() != 10)
  {
    throw std::runtime_error("failed for metaDataMap size\n");
  }

  if ((metam.m_windowlb != 5) || (metam.m_windowub != 14))
  {
    throw std::runtime_error("failed for metaDataMap windlow bound\n");
  }
  */
}

int main()
{
  testMetaData();
  testMetaDataType();
  testMetaDataOverlap();
  //TODO add test about different backend and dataType
}