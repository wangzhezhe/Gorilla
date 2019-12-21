#include "dhtmanager.h"
#include "../hilbert/hilbert.h"
#include <string>
#include <algorithm>

int computeBits(int n)
{
    int nr_bits = 0;

    while (n)
    {
        n = n >> 1;
        nr_bits++;
    }

    return nr_bits;
}

BBX *getOverlapBBX(BBOX &a, BBOX &b)
{

    if (a.m_dim != b.m_dim)
    {
        throw std::runtime_error("the dimention of two bounding box is not equal " + "a.m_dim  is " + std::to_string(a.m_dim) + " b.m_dim is " + std::to_string(b.m_dim));
        return NULL;
    }

    if (a.m_dim == 0 || a.m_dim > 3)
    {
        throw std::runtime_error("the dimention of bounding box can not be 0 or value larger than 3, current value is: " + std::to_string(a.m_dim));
        return NULL;
    }

    BBX *overlapBBX = new BBX(a.m_dim);

    for (int i = 1; i <= 3; i++)
    {

        //get overlap for one dim

        //update the ovelapBBX
    }

    if (a.m_dim == 1)
    {
    }
    else if (a.m_dim == 2)
    {
    }
    else if (a.m_dim == 3)
    {
    }
    else
    {
        throw std::runtime_error("unsupport the bounding box that is larger than 3 dims");
    }
}

//init the metaServerBBOXList according to the partitionNum and the bbox of the global domain
//The hilbert DHT is only suitable for the cubic
void DHTManager::initDHT(int ndim, int metaServerNum, BBOX &globalBBOX)
{
    //get max dim value
    int maxDimValue = INT_MIN;
    for (int i = 0; i < 3; i++)
    {
        maxDimValue = std::max(maxDimValue, globalBBOX.m_ub[i]);
    }
    int bits = computeBits(maxDimValue);

    if (ndim == 1)
    {

        for (int i = 0; i < maxDimValue; i++)
        {

            int sfc_coord[1] = {i};
            bitmask_t index = hilbert_c2i(ndim, nBits, sfc_coord);
            int serverID = index % metaServerNum;

            auto iter = this->metaServerIDToBBX.find(serverID);
            if (iter == mapStudent.end())
            {
                this->metaServerIDToBBX[serverID] = new BBX();
            }

            //update the lowerbound and the upper bound for specific partition
            this->metaServerIDToBBX[serverID][0].m_lb = std::min(this->metaServerIDToBBX[serverID][0].m_lb, i);
            this->metaServerIDToBBX[serverID][0].m_ub = std::max(this->metaServerIDToBBX[serverID][0].m_ub, i);
        }
    }
    else if (ndim == 2)
    {
        for (int i = 0; i < maxDimValue; i++)
        {
            for (int j = 0; j < maxDimValue; j++)
            {

                int sfc_coord[2] = {i, j};
                bitmask_t index = hilbert_c2i(ndim, nBits, sfc_coord);
                int serverID = index % metaServerNum;

                if (iter == mapStudent.end())
                {
                    this->metaServerIDToBBX[serverID] = new BBX();
                }

                //update the lowerbound and the upper bound for specific partition
                this->metaServerIDToBBX[serverID][0].m_lb = std::min(this->metaServerIDToBBX[serverID][0].m_lb, i);
                this->metaServerIDToBBX[serverID][1].m_lb = std::min(this->metaServerIDToBBX[serverID][1].m_lb, j);

                this->metaServerIDToBBX[serverID][0].m_ub = std::max(this->metaServerIDToBBX[serverID][0].m_ub, i);
                this->metaServerIDToBBX[serverID][1].m_ub = std::max(this->metaServerIDToBBX[serverID][0].m_ub, j);
            }
        }
    }
    else if (ndim == 3)
    {
        for (int i = 0; i < maxDimValue; i++)
        {
            for (int j = 0; j < maxDimValue; j++)
            {
                for (int k = 0; k < maxDimValue)
                {
                    int sfc_coord[3] = {i, j, k};
                    bitmask_t index = hilbert_c2i(ndim, nBits, sfc_coord);
                    if (iter == mapStudent.end())
                    {
                        this->metaServerIDToBBX[serverID] = new BBX();
                    }

                    //update the lowerbound and the upper bound for specific partition
                    this->metaServerIDToBBX[serverID][0].m_lb = std::min(this->metaServerIDToBBX[serverID][0].m_lb, i);
                    this->metaServerIDToBBX[serverID][1].m_lb = std::min(this->metaServerIDToBBX[serverID][1].m_lb, j);
                    this->metaServerIDToBBX[serverID][2].m_lb = std::min(this->metaServerIDToBBX[serverID][2].m_lb, k);

                    this->metaServerIDToBBX[serverID][0].m_ub = std::max(this->metaServerIDToBBX[serverID][0].m_ub, i);
                    this->metaServerIDToBBX[serverID][1].m_ub = std::max(this->metaServerIDToBBX[serverID][1].m_ub, j);
                    this->metaServerIDToBBX[serverID][2].m_ub = std::max(this->metaServerIDToBBX[serverID][2].m_ub, k);
                }
            }
        }
    }
    else
    {
        throw std::runtime_error("unsupported ndim value " + std::to_string(ndim));
    }

    return;
}

//get the corresponding metaserver according to the input bbox
std::vector<responsibleMetaServer> DHTManager::getMetaServerID(int dim, BBX &BBXQuery)
{
    //go through the metaServerIDToBBX
    //save the corresponding BBX
    for (auto it = this->metaServerIDToBBX.begin(); it != this->metaServerIDToBBX.end(); ++it)
    {
        //compare the BBXQuery with every item and store the overlapping part into the vector

        int overlaplbx, overlaplby, overlaplbz;
        int overlapubx, overlapuby, overlapubz;

        overlaplbx =

            it.second->m_lb
                it.second->m_ub

                    responsibleMetaServer rbs;
        rbs.metaServerID = it.first;

        //caculate the overlapping region
        rbs.bbx.m_lb =
            rbs.bbx.m_ub =
    }
}
