
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
    // there should the first one is already exist
    for (int i = 0; i < slotNum; i++) {
      this->m_circularArray.push_back(tl::xstream());
    }
    addES();
  }

  ~CircularArrayTP() {
    // call the destroyed operation
    std::cout << "CircularArrayTP is destroyed" << std::endl;
    for (auto es : m_circularArray) {
      es.join();
      // free it manually since the destroy function is private one
      if (!es.is_null()) {
        // TODO the ess is needed to be destroyed
      }
    }
  }

  std::vector<tl::xstream> m_circularArray;

  tl::mutex m_indexMutex;
  int m_head = -1;
  int m_tail = -1;
  const int m_slotNum;

  int addES() {
    m_indexMutex.lock();
    bool ifInit = false;
    if (m_head == -1 && m_tail == -1) {
      // the first element
      m_head = 0;
      m_tail = 0;

    } else if (this->m_head == this->m_slotNum - 1) {
      this->m_head = 0;
    } else {
      this->m_head++;
    }

    if (!m_circularArray[m_head].is_null()) {
      throw std::runtime_error("the ess at the new position should be empty");
    } else {
      m_circularArray[m_head] = std::move(*tl::xstream::create());
    }
    m_indexMutex.unlock();
  };

  int removeES() {
    if (m_head == -1 || m_tail == -1) {
      throw std::runtime_error("fail to be initlized before removeES");
    }
    m_indexMutex.lock();
    if (this->m_tail == 0) {
      this->m_tail = m_slotNum - 1;
    } else {
      this->m_tail--;
    }
    m_indexMutex.unlock();
  };

  int essLen() {
    m_indexMutex.lock();

    m_indexMutex.unlock();
  }
};

#endif