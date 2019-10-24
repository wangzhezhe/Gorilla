
#ifndef DATAMETA_H
#define DATAMETA_H

#include <string>
#include <iostream>
#include <array>
#include <tuple>
#include <typeinfo>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/array.hpp>

// the metadata for every data block
struct BlockMeta
{
    //for empty meta data, the initial value is 0
    size_t m_dimension=0;
    std::string m_typeName="";
    size_t m_elemSize=0;
    std::array<size_t, 3> m_shape{{0,0,0}};

    BlockMeta(){};
    BlockMeta(size_t dimension,
              std::string typeName,
              size_t elemSize,
              std::array<size_t, 3> shape) : m_dimension(dimension),
                                             m_typeName(typeName),
                                             m_elemSize(elemSize),
                                             m_shape(shape){};

    size_t getValidDimention()
    {
        size_t d = 0;
        for (int i = 0; i < 3; i++)
        {
            if (m_shape[i] != 0)
            {
                d++;
            }
        }
        return d;
    }

    size_t getBlockMallocSize()
    {
        if (m_shape[0] == 0 && m_shape[1] == 0 && m_shape[2] == 0)
        {
            return 0;
        }
        if (m_elemSize == 0)
        {
            std::cout << "failed to get malloc size, elem size is 0" << std::endl;
        }

        size_t elemNum = 1;
        for (int i = 0; i < 3; i++)
        {
            if (m_shape[i] != 0)
            {
                elemNum = elemNum * m_shape[i];
            }
        }

        return m_elemSize * elemNum;
    }

    void printMeta()
    {
        std::cout << "m_dimention " << m_dimension
                  << ", m_typeName " << m_typeName
                  << ", m_elemSize " << m_elemSize
                  //<<", lower bound " << m_baseoffset[0] << " " << m_baseoffset[1] << " " << m_baseoffset[2]
                  << ", shape " << m_shape[0] << " " << m_shape[1] << " " << m_shape[2] << std::endl;
        return;
    }

    ~BlockMeta(){};

    template<typename A> void serialize(A& ar)
    {
        ar &m_dimension;
        ar &m_typeName;
        ar &m_elemSize;
        ar &m_shape;
    }
};

//TODO change name to the metadata
//the meta is for all the blocks for specific timestep
//this info is use for put the data, the server only store the BlockMeta for every data block
//return the BlockMeta for dsget interface
//Although the blockMeta is part of the DataMeta split them for easy serialize / deserialize
struct DataMeta
{

    //varname and ts is suitable for all the block data
    std::string m_varName;
    size_t m_iteration;
    size_t m_elemSize;
    size_t m_dimension;
    std::string m_typeName;
    std::array<size_t, 3> m_shape;
    DataMeta(){};
    DataMeta(std::string varName,
             size_t iteration,
             size_t dimension,
             std::string typeName,
             size_t elemSize,
             std::array<size_t, 3> shape) : m_varName(varName),
                                            m_iteration(iteration),
                                            m_elemSize(elemSize),
                                            m_dimension(dimension),
                                            m_typeName(typeName),
                                            m_shape(shape){};

    void printMeta()
    {
        std::cout << "varName " << m_varName
                  << ", m_iteration " << m_iteration
                  << ", m_dimention " << m_dimension
                  << ", m_typeName " << m_typeName
                  << ", m_elemSize " << m_elemSize
                  //<<", lower bound " << m_baseoffset[0] << " " << m_baseoffset[1] << " " << m_baseoffset[2]
                  << ", shape " << m_shape[0] << " " << m_shape[1] << " " << m_shape[2] << std::endl;
        return;
    }

    BlockMeta extractBlockMeta()
    {
        return BlockMeta(this->m_dimension, this->m_typeName, this->m_elemSize, this->m_shape);
    }

    template<typename A> void serialize(A& ar)
    {
        ar &m_varName;
        ar &m_iteration;
        ar &m_elemSize;
        ar &m_dimension;
        ar &m_typeName;
        ar &m_shape;
    }

    ~DataMeta(){};
};

#endif