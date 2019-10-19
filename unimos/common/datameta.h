
#ifndef DATAMETA_H
#define DATAMETA_H

#include <string>
#include <iostream>
#include <array>
#include <typeinfo>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/array.hpp>


//TODO change name to the metadata
struct DataMeta
{
    std::string m_varName;
    size_t m_timeStep;
    size_t m_dimention;
    std::string m_typeName;
    size_t m_elemSize;
    std::array<size_t, 3> m_baseoffset;
    std::array<size_t, 3> m_shape;

    DataMeta(){};

    DataMeta(std::string varName,
             size_t timeStep,
             size_t dimention,
             std::string typeName,
             size_t elemSize, 
             std::array<size_t, 3> baseoffset,
             std::array<size_t, 3> shape) : m_varName(varName),
                                            m_timeStep(timeStep),
                                            m_elemSize(elemSize),
                                            m_dimention(dimention),
                                            m_typeName(typeName),
                                            m_baseoffset(baseoffset),
                                            m_shape(shape){};
    
    
    void printMeta(){
        std::cout<<"varName "<<m_varName 
        << ", m_timeStep " << m_timeStep 
        <<", m_dimention "<< m_dimention
        <<", m_typeName " << m_typeName
        <<", m_elemSize " << m_elemSize
        <<", lower bound " << m_baseoffset[0] << " " << m_baseoffset[1] << " " << m_baseoffset[2]
        << ", shape " <<m_shape[0] << " " << m_shape[1] << " " << m_shape[2] << std::endl;
        return;
    }

    size_t getDataMallocSize(){
        if(m_shape[0]==0 && m_shape[1]==0 && m_shape[2]==0){
            return 0;
        }
        if(m_elemSize==0){
            std::cout << "failed to get malloc size, elem size is 0" << std::endl;    
        }
        size_t elemNum = 1;

        for(int i=0;i<3;i++){
            if(m_shape[i]!=0){
                elemNum = elemNum*m_shape[i];
            }
        }

        return m_elemSize*elemNum;


    }

    template<typename A> void serialize(A& ar){
        ar & m_varName;
        ar & m_timeStep;
        ar & m_dimention;
        ar & m_typeName;
        ar & m_elemSize;
        ar & m_baseoffset;
        ar & m_shape;
    }

    ~DataMeta(){};

};


#endif