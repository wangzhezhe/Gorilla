
#include "ClientForStaging.hpp"

namespace GORILLA
{

// currently we basic ask every stage if there are required data
std::vector<vtkSmartPointer<vtkPolyData> > ClientForStaging::aggregatePolyBySuffix(
  std::string blockIDSuffix)
{
  // send request to all stages nodes
  tl::remote_procedure remotegetBlockSummayBySuffix =
    this->m_clientEnginePtr->define("getBlockSummayBySuffix");

  tl::remote_procedure remotegetArraysExplicit =
    this->m_clientEnginePtr->define("getArraysExplicit");

  std::vector<vtkSmartPointer<vtkPolyData> > polylist;

  // a vector based on the poydata

  for (auto& kv : this->m_serverToEndpoints)
  {
    tl::endpoint serverEndpoint = kv.second;
    std::vector<BlockSummary> tempbsList =
      remotegetBlockSummayBySuffix.on(serverEndpoint)(blockIDSuffix);
    // std::cout << "check server: " << kv.first << std::endl;
    for (auto& v : tempbsList)
    {
      // v.printSummary();
      // maybe try to transfer data here
      // assign the space
      // create the bulk
      // get the poly data back
      // points, normals, cells
      // allocate memory space
      // go through array
      if (v.m_arrayListLen != 3)
      {
        throw std::runtime_error(
          "the array len is supposed to be 3, but now it is: " + std::to_string(v.m_arrayListLen));
      }
      std::vector<std::pair<void*, std::size_t> > segments(3);
      for (int i = 0; i < 3; i++)
      {
        size_t transferSize = v.m_arrayList[i].m_elemSize * v.m_arrayList[i].m_elemNum;
        void* memspace = ::operator new(transferSize);

        if (strcmp(v.m_arrayList[i].m_arrayName, "points") == 0)
        {
          segments[0].first = memspace;
          segments[0].second = transferSize;
        }
        else if (strcmp(v.m_arrayList[i].m_arrayName, "normals") == 0)
        {
          segments[1].first = memspace;
          segments[1].second = transferSize;
        }
        else if (strcmp(v.m_arrayList[i].m_arrayName, "cells") == 0)
        {
          segments[2].first = memspace;
          segments[2].second = transferSize;
        }
        else
        {
          throw std::runtime_error(
            "the array list name is unexpected" + std::string(v.m_arrayList[i].m_arrayName));
        }
      }

      // create the bulk
      // transfer the data back
      tl::bulk clientBulk = this->m_clientEnginePtr->expose(segments, tl::bulk_mode::write_only);
      int status = remotegetArraysExplicit.on(serverEndpoint)(std::string(v.m_blockid), clientBulk);
      if (status != 0)
      {
        throw std::runtime_error("failed to get the data back");
      }

      // create the polydata
      // generate the points array
      void* pointArrayptr = segments[0].first;
      void* normalArrayPtr = segments[1].first;
      void* polyArrayPtr = segments[2].first;

      int nPoints = (segments[0].second / sizeof(double)) / 3;
      auto points = vtkSmartPointer<vtkPoints>::New();
      points->SetNumberOfPoints(nPoints);
      for (vtkIdType i = 0; i < nPoints; i++)
      {
        const double* tempp = (double*)pointArrayptr;
        points->SetPoint(i, tempp + (i * 3));
      }

      // generate normal array
      auto normals = vtkSmartPointer<vtkDoubleArray>::New();
      normals->SetNumberOfComponents(3);
      const double* tempn = (double*)normalArrayPtr;

      for (vtkIdType i = 0; i < nPoints; i++)
      {
        normals->InsertNextTuple(tempn + (i * 3));
      }

      // generate cell array
      int nCells = (segments[2].second / sizeof(int)) / 3;
      auto polys = vtkSmartPointer<vtkCellArray>::New();
      const int* tempc = (int*)polyArrayPtr;

      for (vtkIdType i = 0; i < nCells; i++)
      {
        vtkIdType a = *(tempc + (i * 3 + 0));
        vtkIdType b = *(tempc + (i * 3 + 1));
        vtkIdType c = *(tempc + (i * 3 + 2));

        polys->InsertNextCell(3);
        polys->InsertCellPoint(a);
        polys->InsertCellPoint(b);
        polys->InsertCellPoint(c);
      }

      auto polyData = vtkSmartPointer<vtkPolyData>::New();
      polyData->SetPoints(points);
      polyData->SetPolys(polys);
      polyData->GetPointData()->SetNormals(normals);

      // polyData->PrintSelf(std::cout, vtkIndent(5));
      // std::cout << "aggregate blockid " << v.m_blockid
      //          << " cell number: " << polyData->GetNumberOfPolys() << std::endl;
      polylist.push_back(polyData);

      // segment can be deleted here
      // since we already have the poly data that use the memory space of the poly array
      ::operator delete(segments[0].first);
      ::operator delete(segments[1].first);
      ::operator delete(segments[2].first);
    }
  }

  // getBlockSummayBySuffix
  return polylist;
}

void ClientForStaging::cacheClientAddr(const std::string& clientAddr)
{
  // std::cout << " debug cacheClientAddr, addr: " << clientAddr << std::endl;
  // if exist, reutrn

  std::lock_guard<tl::mutex> g(this->m_mutex_clientAddrToID);
  if (this->m_clientAddrToID.find(clientAddr) == this->m_clientAddrToID.end())
  {
    // not found
    this->m_base++;
    this->m_clientAddrToID[clientAddr] = this->m_base;
  }
  // found
  return;
}

int ClientForStaging::getIDFromClientAddr(const std::string& clientAddr)
{
  // std::cout << " debug getIDFromClientAddr, addr: " << clientAddr << std::endl;
  // if not exist, return -1
  std::lock_guard<tl::mutex> g(this->m_mutex_clientAddrToID);
  if (this->m_clientAddrToID.find(clientAddr) == this->m_clientAddrToID.end())
  {
    // not found
    // there the putrawdata operation is not called in this case
    return -1;
  }
  // found
  return this->m_clientAddrToID[clientAddr];
}

}