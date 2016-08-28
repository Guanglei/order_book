#pragma once

#include "types.h"
#include "utils.h"

#include <unordered_map>
#include <vector>
#include <cassert>

#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/object_pool.hpp>

namespace order_book
{
  struct PriceLevel;

  struct Order : Node<Order>
  {
    Order() {}

    Order(order_id_t _id, SideType _side, qty_t _qty, price_t _price, PriceLevel* _level):
          id(_id), side(_side), qty(_qty), price(_price), level(_level)
    {
    }

    order_id_t id = std::numeric_limits<order_id_t>::max();
    SideType side = SideType::unknown;
    qty_t qty = 0;
    price_t price = 0;

    PriceLevel* level = nullptr;
  };
  
  using price_level_map_alloc_t = 
            boost::fast_pool_allocator<std::pair<const price_t, PriceLevel*>, 
                        boost::default_user_allocator_new_delete, boost::details::pool::null_mutex, 64, 0>;
  
  using price_level_map_t = std::unordered_map<
          price_t, PriceLevel*, std::hash<price_t>, std::equal_to<price_t>, price_level_map_alloc_t>;

  struct PriceLevel : Node<PriceLevel>
  {
    uint64_t total_qty = 0;
    Order*   head_order = nullptr; 
    Order*   tail_order = nullptr;
    typename price_level_map_t::iterator iter_in_map;

    void add_order(Order& order)
    {   
      order.level = this;
      total_qty += order.qty;

      if(tail_order == nullptr)
      {
        head_order = &order;
        tail_order = &order;
      }
      else
      {
        order.insert_after(*tail_order);
        tail_order = &order;
      }
    }
    
    void cancel_order(Order& order)
    {
      if(head_order == &order)
      {
        head_order = order.get_next();
      }

      if(tail_order == &order)
      {
        tail_order = order.get_prev();
      }
      
      total_qty -= order.qty;
      order.detach();
    }

    price_t get_price() const
    {
      assert(head_order);
      return head_order->price;
    }

    qty_t get_qty() const
    {
      return total_qty;
    }
    
    bool empty() const
    {
      return head_order == nullptr;
    }

    void print(std::ostream& os) const
    {
      os << get_qty() << " @ " << get_price() << " - ";
      assert(head_order);
      auto temp = head_order;
      os << "[";
      while(temp)
      {
        os << "(" << temp->id << "," << temp->qty << ")";
        temp = temp->get_next();
      }
      os << "]" << std::endl;
    }
  };

  template<typename price_level_constructor_t>
  class PriceBook
  {
    public:
      
      PriceBook(SideType s, price_level_constructor_t& plc) : side_(s), price_level_constructor_(plc)
      {
        price_level_map_.reserve(128);
        //warm up pool
        price_level_map_.emplace(1.1, nullptr);
        price_level_map_.erase(1.1);
      }

      void add_order(Order& order)
      {
        //find and update level, insert order into the list, update order with the level
        auto price_level = get_and_update_level(order.side, order.price);
        assert(price_level);
        price_level->add_order(order);
      }
      
      void cancel_order(Order& order)
      {
        assert(order.level);
        assert(order.level->get_price() == order.price);
        assert(order.level->get_qty() >= order.qty);

        order.level->cancel_order(order);
        //if this level is empth after the order is cancelled, need to remove the level;
        if(order.level->empty())
        {
          if(top_level_ == order.level)
          {
            top_level_ = order.level->get_next();
          }

          if(last_level_ == order.level)
          {
            last_level_ = order.level->get_prev();
          }

          order.level->detach();
          price_level_map_.erase(order.level->iter_in_map);
          price_level_constructor_.destroy(order.level);
        }
      }
    
      void print(std::ostream& os) const
      {
        if(top_level_ == nullptr)
        {
          os << "* EMPTY *" << std::endl;
          return;
        }
        
        if(side_ == SideType::ask)
        {
          auto temp = last_level_;
          while(temp)
          {
            temp->print(os);
            temp = temp->get_prev();
          }
        }
        else
        {
          auto temp = top_level_;
          while(temp)
          {
            temp->print(os);
            temp = temp->get_next();
          }
        }
      }
      
      bool empty() const
      {
        return !top_level_;
      }
      
      double get_tob() const
      {
        if(top_level_)
        {
          return top_level_->get_price();
        }

        return std::numeric_limits<double>::quiet_NaN();
      }

    private:

      PriceLevel* get_and_update_level(SideType side, price_t price)
      {
        if(top_level_ == nullptr)
        {
          top_level_ = price_level_constructor_.construct();
          last_level_ = top_level_;
          top_level_->iter_in_map = price_level_map_.emplace(price, top_level_).first;
          return top_level_;
        }
        
        //check if it is an existing price level
        auto iter1 = price_level_map_.find(price);
        if(iter1 != price_level_map_.end())
        {
          return iter1->second;
        }
        
        //now need to search through the book to find the position for the new level
        auto iter = top_level_;
        if(side == SideType::bid)
        {
          while(iter)
          {
            if(iter->get_price() == price)
            {
              //find the match level, just return it
              return iter;
            }
            else if(iter->get_price() < price)
            {
              //find the lower level, since it is bid, insert before it
              auto new_level = price_level_constructor_.construct();
              new_level->insert_before(*iter);
              if(new_level->prev == nullptr)
              {
                top_level_ = new_level;
              }

              new_level->iter_in_map = price_level_map_.emplace(price, new_level).first;
              return new_level;
            } 
            
            iter = iter->get_next();
          }
        }
        else
        {
          while(iter)
          {
            if(iter->get_price() == price)
            {
              //find the match level, just return it
              return iter;
            }
            else if(iter->get_price() > price)
            {
              //find the higer level, since it is ask, insert before it
              auto new_level = price_level_constructor_.construct();
              new_level->insert_before(*iter);
              if(new_level->prev == nullptr)
              {
                top_level_ = new_level;
              }
              new_level->iter_in_map = price_level_map_.emplace(price, new_level).first;
              return new_level;
            } 
            
            iter = iter->get_next();
          }
        }

        //need insert a level at tail
        auto new_level = price_level_constructor_.construct();
        assert(last_level_);
        new_level->insert_after(*last_level_);
        last_level_ = new_level;

        new_level->iter_in_map = price_level_map_.emplace(price, new_level).first;
        return new_level;
      }

    private:
       
      PriceLevel* top_level_ = nullptr; 
      PriceLevel* last_level_ = nullptr;
      SideType side_;
      price_level_constructor_t& price_level_constructor_;

      price_level_map_t price_level_map_;
  };

  template<typename order_constructor_t = DefaultConstructor<Order>, 
            typename price_level_constructor_t = DefaultConstructor<PriceLevel>>
  class OrderBook
  {
    public:
      
      OrderBook(InvalidStats& stats) : invalid_stats_(stats), 
                    order_constructor_(8192), price_level_constructor_(128)
      {
        order_map_.reserve(1024);

        //warm up pool
        order_map_.emplace(1,nullptr);
        order_map_.erase(1);

        order_constructor_.destroy(order_constructor_.construct());
        price_level_constructor_.destroy(price_level_constructor_.construct());
      }

      bool add_order(order_id_t order_id, SideType side, qty_t qty, price_t price)
      {
        //if book is cross when receiving a new order, update the stats
        if(is_cross())
        {
          ++ invalid_stats_.num_crossed;
        }

        //check if duplicate order
        auto new_order_iter = order_map_.emplace(order_id, nullptr);
        if(!new_order_iter.second)
        {
          ++ invalid_stats_.num_duplicate_order;
          return false;
        }
        
        //create new order
        auto new_order = order_constructor_.construct();
        if(UNLIKELY(!new_order))
        {
          return false;
        }
        *new_order = {order_id, side, qty, price, nullptr};

        //update the order map with the new order
        new_order_iter.first->second = new_order;

        //add to price book
        book_[static_cast<int>(side)].add_order(*new_order);  
        
        return true;
      }

      bool amend_order(order_id_t order_id, SideType side, qty_t qty, price_t price)
      { 
        //if book is cross when receiving a amend order, update the stats
        if(is_cross())
        {
          ++ invalid_stats_.num_crossed;
        }

        auto iter = order_map_.find(order_id);
        //check if order exists
        if(UNLIKELY(iter == order_map_.end()))
        {
          ++ invalid_stats_.num_unknown_mod;
          return false;
        }

        assert(iter->second);
        auto& order = *(iter->second);
        //if side or price change the previous order should be cancelled and a new order should be added
        if(side != order.side || price != order.price)
        {
          book_[static_cast<int>(order.side)].cancel_order(order);

          order.side = side;
          order.qty = qty;
          order.price = price;
          book_[static_cast<int>(order.side)].add_order(order);  
          return true;
        }
        //otherwise only qty change just update in place
        else if(qty != order.qty)
        {
          assert(order.level);
          order.level->total_qty += qty;
          order.level->total_qty -= order.qty;
          order.qty = qty; 

          return true;
        }
        else
        {
          //nothing change
          return false;
        }
      }

      bool cancel_order(order_id_t order_id)
      {
        auto iter = order_map_.find(order_id);
        //check if order exists
        if(UNLIKELY(iter == order_map_.end()))
        {
          ++ invalid_stats_.num_unknown_mod;
          return false;
        }
        
        assert(iter->second);
        book_[static_cast<int>(iter->second->side)].cancel_order(*(iter->second));
        
        order_constructor_.destroy(iter->second);
        
        order_map_.erase(iter);
        return true;
      }
      
      void print(std::ostream& os) const
      {
          double mid_quote = (book_[static_cast<int>(SideType::bid)].get_tob() + 
                            book_[static_cast<int>(SideType::ask)].get_tob()) /2;

        os << std::endl;
        os << "*** ask ***" << std::endl;
        book_[static_cast<int>(SideType::ask)].print(os);
        std::cout << "========" << mid_quote << "========" << std::endl;
        book_[static_cast<int>(SideType::bid)].print(os);
        os << "*** bid ***" << std::endl;
        os << std::endl;
      }
      
      bool is_cross() const
      {
        auto& bid_book = book_[static_cast<int>(SideType::bid)]; 
        auto& ask_book = book_[static_cast<int>(SideType::ask)]; 
        if(!bid_book.empty() && !ask_book.empty())
        {
          return (bid_book.get_tob() >= ask_book.get_tob());
        }

        return false;
      }
      
      double get_tob(SideType side) const
      {
        assert(side == SideType::bid || side == SideType::ask);
        return book_[static_cast<int>(side)].get_tob();
      }

    private:
      
      using price_book_t = PriceBook<price_level_constructor_t>;
      price_book_t book_[static_cast<int>(SideType::cardinality)] = {{SideType::bid, price_level_constructor_}, 
                                                                          {SideType::ask, price_level_constructor_}};     
      using order_map_alloc_t = 
              boost::fast_pool_allocator<std::pair<const order_id_t, Order*>, 
                        boost::default_user_allocator_new_delete, boost::details::pool::null_mutex, 8192, 0>;
      using order_map_t = std::unordered_map<
          order_id_t, Order*, std::hash<order_id_t>, std::equal_to<order_id_t>, order_map_alloc_t>;
      order_map_t order_map_;

      InvalidStats& invalid_stats_;
      order_constructor_t order_constructor_;
      price_level_constructor_t price_level_constructor_;
  };
}
