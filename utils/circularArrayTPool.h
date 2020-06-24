
#ifndef CIRCULARARRAY_H
#define CIRCULARARRAY_H

#include <thallium.hpp>
#include <vector>

namespace tl = thallium;

// the thread pool based on a circular Array
// init tail = head = 0
// tail is empty position that do not store real element
// head + 1 = tail full
// tail + 1 = head the valid element is 1
// tail = head empty
struct CircularArrayTP {
  CircularArrayTP(int slotNum)
      : m_slotNum(slotNum), m_mainES(tl::xstream::create()) {
    if (this->m_slotNum < 1) {
      throw std::runtime_error(
          "the length of the es array should larger than 1");
    }

    // init the slots with the empty xstream
    for (int i = 0; i < slotNum; i++) {
      // the constructor of the managed element will call the default
      // constructor of the tl::xstream
      this->m_circularESbuffer.push_back(tl::managed<tl::xstream>());
    }
  }

  ~CircularArrayTP() {
    std::cout << "CircularArrayTP is destroyed" << std::endl;
    int size = m_slotNum;
    for (int i = 0; i < size; i++) {
      if (!m_circularESbuffer[i]->is_null()) {
        m_circularESbuffer[i]->join();
      }
      // the xstream will be deleted by the managed class automatically
    }
    this->m_mainES->join();
  }

  // this es is in charge of increasing/decreasing es in circularESbuffer
  tl::managed<tl::xstream> m_mainES;
  // the es in this buffer is in charge of executing the in-memory tasks
  std::vector<tl::managed<tl::xstream>> m_circularESbuffer;

  tl::mutex m_indexMutex;
  int m_head = 0;
  int m_tail = 0;
  const int m_slotNum;
  // add an actual es into the slot array
  int addES() {
    this->m_indexMutex.lock();
    if (((this->m_head + 1) % this->m_slotNum) == this->m_tail) {
      // use m_head == m_tail to represent the empty
      // use m_head+1 == m_tail to represent the full
      // this will be convenient to caculate the length
      this->m_indexMutex.unlock();
      throw std::runtime_error("the es buffer is full");
    }
    // normal move between the 0 to m_slotNum -1

    this->m_head = (this->m_head + 1) % (this->m_slotNum);
    // if the current position is not null
    if (!this->m_circularESbuffer[this->m_head]->is_null()) {
      this->m_indexMutex.unlock();
      throw std::runtime_error("the ess at the new position " +
                               std::to_string(this->m_head) +
                               " should be empty before moving head ptr");
    } else {
      this->m_circularESbuffer[this->m_head] = std::move(tl::xstream::create());
    }
    this->m_indexMutex.unlock();
  };

  int removeES() {
    this->m_indexMutex.lock();
    int temptail = this->m_tail;
    this->m_tail = (this->m_tail + 1) % (this->m_slotNum);
    this->m_indexMutex.unlock();
    // delete the es for the current position and make it empty
    // wait the finish of the ess
    if (!this->m_circularESbuffer[temptail]->is_null()) {
      this->m_circularESbuffer[temptail]->join();
      // make this slot empty
      // the previous es is deleted in the move constructor
      this->m_circularESbuffer[temptail] =
          std::move(tl::managed<tl::xstream>());
    }
  };

  // get the length of the non empty es
  int esbufferLen() {
    int len;
    this->m_indexMutex.lock();
    if (this->m_head == this->m_tail) {
      len = 0;
    } else if (this->m_head > this->m_tail) {
      len = this->m_head - this->m_tail;
    } else {
      len = this->m_head - this->m_tail + m_slotNum;
    }
    this->m_indexMutex.unlock();
    return len;
  }
};

#endif