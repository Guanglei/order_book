#pragma once

#define LIKELY(x)       __builtin_expect(!!(x),1)
#define UNLIKELY(x)     __builtin_expect(!!(x),0)

#include <cstdlib>
#include <cerrno>
#include <limits>

namespace order_book
{
    template<class derive_t>
    struct Node
    {
      using self_t = Node<derive_t>;

      derive_t* get()
      {
        return static_cast<derive_t*>(this);
      }

      derive_t* get_next()
      {
        return static_cast<derive_t*>(next);
      }

      derive_t* get_prev()
      {
        return static_cast<derive_t*>(prev);
      }

      void insert_before(Node& n)
      {
        prev = n.prev;
        next = &n;
        if(n.prev)
        {
          n.prev->next = this;
        }
        n.prev = this;
      };

      void insert_after(Node& n)
      {
        next = n.next;
        prev = &n;
        if(n.next)
        {
          n.next->prev = this;
        }
        n.next = this;
      }

      void detach()
      {
        if(next)
        {
          next->prev = prev;
        }

        if(prev)
        {
          prev->next = next;
        }

        prev = nullptr;
        next = nullptr;
      }

      bool is_detached() const
      {
        return !prev && !next;
      }

      self_t* prev = nullptr;
      self_t* next = nullptr;
    };

    template<typename T>
    struct DefaultConstructor
    {
      DefaultConstructor(...)
      {
      }

      template<typename... Args>
      T* construct(Args&&... args)
      {
        return new T(std::forward<Args>(args)...);
      }

      void destroy(T* ptr)
      {
        delete ptr;
      }
    };

    //parse the c str to a unsigned 32 bit integer until hit the delimiter
    //either return a valid uint32 and begin stop at delimiter or return the max for invalid case
    uint32_t parseUnsignedField(const char*& begin, char delimiter)
    {
      if(UNLIKELY(!(*begin) || (*begin == delimiter)))
      {
        return std::numeric_limits<uint32_t>::max();
      }

      uint64_t ret = 0;

      while(*begin)
      {
        char c = *begin;
        if(c == delimiter)
        {
          break;
        } 
        
        if(UNLIKELY(c > '9' || c < '0'))
        {
          return std::numeric_limits<uint32_t>::max();
        }
        
        ret = ret * 10 + (c - '0');
        if(UNLIKELY(ret >= std::numeric_limits<uint32_t>::max()))
        {
          return std::numeric_limits<uint32_t>::max();
        }
        ++begin; 
      }

      if(UNLIKELY(*begin != delimiter))
      {
        return std::numeric_limits<uint32_t>::max();
      }
      
      return ret;
    };

    double parseDouble(const char*& begin, char delimiter)
    {
      if(UNLIKELY(!(*begin) || (*begin == delimiter)))
      {
        return std::numeric_limits<double>::infinity();
      }

      char* end = nullptr;
      double ret = std::strtod(begin, &end);
      if(UNLIKELY(errno == ERANGE))
      {
        errno = 0;
        return std::numeric_limits<double>::infinity();
      }

      if(UNLIKELY(end == begin || (end && (*end != delimiter))))
      {
        return std::numeric_limits<double>::infinity();
      }
      
      begin = end;

      return ret;
    }
    
    inline char parseChar(const char*& begin, char delimiter)
    {
      if(UNLIKELY(!(*begin) || (*begin == delimiter) || *(begin+1) != delimiter))
      {
        return '\0';
      }

      return *(begin++);
    }
}
