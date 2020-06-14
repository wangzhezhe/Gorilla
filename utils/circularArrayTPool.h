
#ifndef CIRCULARARRAY_H
#define CIRCULARARRAY_H

#include <thallium.hpp>
#include <vector>

namespace tl = thallium;

// the thread pool based on a circular Array
struct CircularArrayTP {
  CircularArrayTP(int slotNum) : m_slotNum(slotNum) {
    if (m_slotNum < 1) {
      throw std::runtime_error("the length of the array should larger than 1");
    }
    // init the slots with the empty xstream
    for (int i = 0; i < slotNum; i++) {
      // the constructor of the managed element will call the default
      // constructor of the tl::xstream
      this->m_circularArray.push_back(tl::managed<tl::xstream>());
    }
    // the ES with #0 could execute the task that motitor the status and adjust
    // the size of the thread pool
    addES();
  }

  ~CircularArrayTP() {
    std::cout << "CircularArrayTP is destroyed" << std::endl;
    int size = m_circularArray.size();
    for (int i = 0; i < size; i++) {
      if (!m_circularArray[i]->is_null()) {
        m_circularArray[i]->join();
      }
      // free it manually since the destroy function is private one
      // the xstream will be deleted by the managed class automatically
    }
  }

  std::vector<tl::managed<tl::xstream>> m_circularArray;

  tl::mutex m_indexMutex;
  int m_head = -1;
  int m_tail = -1;
  const int m_slotNum;
  // add an actual es into the slot array
  int addES() {
    m_indexMutex.lock();
    bool ifInit = false;
    if (m_head == -1 && m_tail == -1) {
      // the start position
      m_head = 0;
      m_tail = 0;

    } else if (this->m_head == this->m_slotNum - 1) {
      //jump out the #0 element
      this->m_head = 1;
    } else {
      this->m_head++;
    }

    if (!m_circularArray[m_head]->is_null()) {
      throw std::runtime_error("the ess at the new position " +
                               std::to_string(m_head) +
                               " should be empty before moving head ptr");
    } else {
      m_circularArray[m_head] = std::move(tl::xstream::create());
    }

    m_indexMutex.unlock();
  };

  int removeES() {
    if (m_head == -1 || m_tail == -1) {
      throw std::runtime_error("fail to be initlized before removeES");
    }
    m_indexMutex.lock();
    int temptail = this->m_tail;
    if (this->m_tail == this->m_slotNum - 1) {
      //jump out the #0 element which is a master one 
      this->m_tail = 1;
    } else {
      this->m_tail++;
    }
    // delete the es for the current position and make it empty
    if (m_circularArray[temptail]->is_null()) {
      throw std::runtime_error(
          "the current slot should be an avalible es (not empty) before "
          "deleting");
    }
    // wait the finish of the ess
    m_circularArray[temptail]->join();
    // make this slot empty
    // the previous es is deleted in the move constructor
    m_circularArray[temptail] = std::move(tl::managed<tl::xstream>());
    std::cout << "debug, reset for position " << temptail << std::endl;
    m_indexMutex.unlock();
  };

  // get the length of the non empty es
  int essLen() {
    m_indexMutex.lock();

    m_indexMutex.unlock();
  }
};

#endif