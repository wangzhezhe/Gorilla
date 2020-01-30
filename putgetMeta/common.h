
#ifndef __COMMON_H__
#define __COMMON_H__

#include <string>
#include <thallium/serialization/stl/string.hpp>

struct Input
{
    Input(){};
    Input(std::string recordKey) : m_recordKey(recordKey){}
    std::string m_recordKey;
    ~Input(){};

    template <typename A>
    void serialize(A &ar)
    {
        ar &m_recordKey;
    }
};

#endif