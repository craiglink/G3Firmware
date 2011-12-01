#ifndef ATOMICCIRCULARBUFFER_h
#define ATOMICCIRCULARBUFFER_h

#ifdef ATOMICCIRCULARBUFFER_EMPTY_CHECK
#define ATOMICCIRCULARBUFFER_NOT_EMPTY if(!empty())
#else
#define ATOMICCIRCULARBUFFER_NOT_EMPTY 
#endif

//template <class T, class SizeType, unsigned int Size>
template <class T, uint8_t Size>
class AtomicCircularBuffer {
public:
  AtomicCircularBuffer() :
    _back(0), _front(0) {}
  ~AtomicCircularBuffer() {}

  bool push(const T& value) {

    uint8_t back = (_back+1)%Size;
    if ( back != _front )
    {
        _buf[_back] = value;
        _back = back;
        return true;
    }
    else
    {
        return false;
    }
  }

  void pop() {
    ATOMICCIRCULARBUFFER_NOT_EMPTY 
    {
        _front = (_front+1)%Size;
    }
  }

  bool insert(const T& value) {

    uint8_t front = (_front-1)%Size;
    if ( front != _back )
    {
        _buf[front] = value;
        _front = front;
        return true;
    }
    else
    {
        return false;
    }
  }

  inline const T& front() {
    ATOMICCIRCULARBUFFER_NOT_EMPTY 
    {
        return _buf[_front];
    }
  }

  inline const T& back() {
    ATOMICCIRCULARBUFFER_NOT_EMPTY 
    {
        return _buf[_back];
    }
  }

  inline void clear() {
      _front = _back;
  }

  inline bool empty() const {
    return _front == _back;
  }


private:
  T _buf[Size];
  uint8_t _back;
  uint8_t _front;

};

#endif
