#ifndef CIRCULARDOUBLEARRAY_H
#define CIRCULARDOUBLEARRAY_H

// this is a template to store different types of data into the template
// this class can be inherited by other class for specific goals
#include <cfloat>
#include <limits>
#include <thallium.hpp>
#include <vector>

namespace tl = thallium;
namespace GORILLA
{

// the thread pool based on a circular Array
// init tail = head = 0 (empty)
// tail is empty position that do not store real element
// head + 1 = tail full
// tail + 1 = head the valid element is 1
// tail = head empty
struct CircularDoubleArray
{
  CircularDoubleArray(int slotNum)
    : m_slotNum(slotNum)
  {
    if (this->m_slotNum < 1)
    {
      // the actual avalible position is slotNum - 1
      throw std::runtime_error("the length of the circular array should larger than 1");
    }

    // init the slots
    // it is empty slot currently
    for (int i = 0; i < slotNum; i++)
    {
      this->m_circularArraybuffer.push_back(DBL_MAX);
    }
  };

  ~CircularDoubleArray(){};

  // buffer is in charge of storing the elemtens
  std::vector<double> m_circularArraybuffer;
  // use the mochi mutex
  tl::mutex m_indexMutex;

  int m_head = 0;
  int m_tail = 0;
  const int m_slotNum;

  // add an actual element into the slot array
  int addElement(double element)
  {
    this->m_indexMutex.lock();
    // if (((this->m_head + 1) % this->m_slotNum) == this->m_tail)
    //{
    // use m_head == m_tail to represent the empty or full
    // use m_head+1 == m_tail to represent the full
    // this will be convenient to caculate the length
    // current head position is supposed to be empty
    // current tail position is supposed to be accupied
    // this->m_indexMutex.unlock();
    // std::cout << "the buffer is full, remove last one" << std::endl;
    //}

    // if the current position is not empty
    // if we want to do the autonomic cover, we do not need to bother this
    //if (m_circularArraybuffer[this->m_head] != DBL_MAX)
    //{
      // if entry is not null
      // this->m_indexMutex.unlock();
      // throw std::runtime_error("the element at the new position " + std::to_string(this->m_head)
      // +
      //  " should be empty before moving head ptr");
      //std::cout << "the buffer is full, remove last one" << std::endl;
    //}
    // else
    //{
    // cover existing one automatically
    // do not care if it is full
    this->m_circularArraybuffer[this->m_head] = element;
    // }

    // head move to the next position
    // the full case have been considered at the begining
    this->m_head = (this->m_head + 1) % (this->m_slotNum);

    this->m_indexMutex.unlock();
    return 0;
  };

  std::vector<double> getLastN(uint32_t number)
  {
    if (number <= 0)
    {
      throw std::runtime_error("the number should be larger than 1");
    }
    uint32_t bufferLen = this->bufferLen();
    if (number >= bufferLen)
    {
      // return all value
      throw std::runtime_error("the required number is larger than actual buffer length");
    }
    // go back to number-1 position to get the start position
    // the current head position is empty
    int startPosition = (this->m_head - number + bufferLen) % bufferLen;
    std::vector<double> results;
    // the first one is at the startPosition
    int tempPosition = startPosition;
    this->m_indexMutex.lock();
    for (int i = 0; i < number; i++)
    {
      if (this->m_circularArraybuffer[tempPosition] == DBL_MAX)
      {
        throw std::runtime_error("try to get an empty element");
      }
      results.push_back(this->m_circularArraybuffer[tempPosition]);
      tempPosition = (tempPosition + 1) % bufferLen;
    }
    this->m_indexMutex.unlock();
    return results;
  };

  std::vector<double> getAll()
  {
    int bufferLen = this->bufferLen();
    return getLastN(bufferLen);
  };

  int removeElement()
  {

    this->m_indexMutex.lock();

    // check empty
    if (this->m_tail == this->m_head)
    {
      throw std::runtime_error("current array is empty");
    }

    // remove current tail position
    // delete the entry for the removePos position and make it empty
    if (this->m_circularArraybuffer[this->m_tail] == DBL_MAX)
    {
      // the entry supposed to be remove is already null
      this->m_indexMutex.unlock();
      throw std::runtime_error("entry supposed to be removed is already nullptr");
    }
    else
    {
      // set the empty position into the DBL_MAX value
      this->m_circularArraybuffer[this->m_tail] = (DBL_MAX);
    }

    this->m_tail = (this->m_tail + 1) % (this->m_slotNum);
    this->m_indexMutex.unlock();
    return 0;
  };

  // get the length of the non empty es
  uint32_t bufferLen()
  {
    uint32_t len;
    this->m_indexMutex.lock();
    if (this->m_head == this->m_tail)
    {
      len = 0;
    }
    else if (this->m_head > this->m_tail)
    {
      len = this->m_head - this->m_tail;
    }
    else
    {
      len = m_slotNum + this->m_head - this->m_tail;
    }
    this->m_indexMutex.unlock();
    return len;
  }
};
}
#endif