/**
This is the driver main file to get message from file and feed into order book
**/

#include <string>
#include <iostream>
#include <fstream>

#include "order_book.h"

using namespace order_book;

class FeedHandler
{
  public:
    FeedHandler() : order_book_(invalid_stats_)
    {
    }
    
    //dispatch the raw message to each handling method
    void processMessage(const std::string &line)
    {
      if(UNLIKELY(line.size() <= 2 || line[1] != ','))
      {
        //invalid short message, should not happen
        ++ invalid_stats_.num_corrupted_msg;
        return;
      }
      
      MessageType msg_type = static_cast<MessageType>(line[0]);
      const char* msg = line.c_str() + 2;
      switch(msg_type)
      {
        case MessageType::add:
        {
          processOrderAdd(msg);
          break;
        }
        case MessageType::mod:
        {
          processOrderMod(msg);
          break;
        }
        case MessageType::del:
        {
          processOrderDel(msg);
          break;
        }
        case MessageType::trade:
        {
          processTrade(msg);
          break;
        }
        default:
        {
          //unknown message type, should not happen;
          std::cout << "Error, unknown message type, skip..." << std::endl;
          ++ invalid_stats_.num_corrupted_msg;
        }
      }

    }

    void printCurrentOrderBook(std::ostream &os) const
    {
      order_book_.print(os);
      os << "*** Last trade -> " << last_trade_.second 
                  << " @ " << last_trade_.first << std::endl;
    }
    
    void printInvadStat(std::ostream& os) const
    {
      os << "Corrupted Msg : " << invalid_stats_.num_corrupted_msg
         << " Duplicate Order Id : " << invalid_stats_.num_duplicate_order
         << " Unknown Trade : " << invalid_stats_.num_unknown_trade
         << " Unknown order modify or cancel : " << invalid_stats_.num_unknown_mod
         << " Top of book crossed : " << invalid_stats_.num_crossed
         << " Invalid Negative Msg Field : " << invalid_stats_.num_invalid_neg << std::endl;
    }

  private:
    
    bool processOrderMsg(const char* msg, order_id_t& id, SideType& side, qty_t& qty, price_t& price)
    {
      id = parseUnsignedField(msg, ',');
      if(UNLIKELY(id == std::numeric_limits<uint32_t>::max() || id == 0))
      {
        ++ invalid_stats_.num_corrupted_msg;
        return false;
      }

      ++msg;
      auto side_char = parseChar(msg, ',');
      if(!side_char)
      {
        ++ invalid_stats_.num_corrupted_msg;
        return false;
      }

      side  = ToSide(side_char);
      if(UNLIKELY(side == SideType::unknown))
      {
        ++ invalid_stats_.num_corrupted_msg;
        return false;
      }

      ++msg;
      qty = parseUnsignedField(msg, ','); 
      if(UNLIKELY(qty == std::numeric_limits<uint32_t>::max() || qty == 0))
      {
        ++ invalid_stats_.num_corrupted_msg;
        return false;
      }

      ++msg;
      price = parseDouble(msg, '\0');
      if(UNLIKELY(price == std::numeric_limits<double>::infinity()))
      {
        ++ invalid_stats_.num_corrupted_msg;
        return false;
      }

      if(UNLIKELY(price <= 0))
      {
        ++ invalid_stats_.num_invalid_neg;
        return false;
      }

      return true;
    }

    void processOrderAdd(const char* msg)
    {
      order_id_t order_id = 0;
      SideType side = SideType::unknown;
      qty_t qty = 0;
      price_t price = 0;

      if(LIKELY(processOrderMsg(msg, order_id, side, qty, price)))
      {
        //std::cout << "processOrderAdd: " << order_id << " " << ToStr(side) << " " << qty << " @ " << price << std::endl;
        order_book_.add_order(order_id, side, qty, price);   
      }

    }

    void processOrderMod(const char* msg)
    { 
      order_id_t order_id = 0;
      SideType side = SideType::unknown;
      qty_t qty = 0;
      price_t price = 0;

      if(LIKELY(processOrderMsg(msg, order_id, side, qty, price)))
      {
        //std::cout << "processOrderMod: " << order_id << " " << ToStr(side) << " " << qty << " @ " << price << std::endl;
        order_book_.amend_order(order_id, side, qty, price);   
      }
    }
    
    void processOrderDel(const char* msg)
    {
      order_id_t order_id = 0;
      SideType side = SideType::unknown;
      qty_t qty = 0;
      price_t price = 0;

      if(LIKELY(processOrderMsg(msg, order_id, side, qty, price)))
      {
        //std::cout << "processOrderCancel: " << order_id << " " << ToStr(side) << " " << qty << " @ " << price << std::endl;
        order_book_.cancel_order(order_id);   
      }
    }

    void processTrade(const char* msg)
    {
      auto qty = parseUnsignedField(msg, ','); 
      if(UNLIKELY(qty == std::numeric_limits<uint32_t>::max()))
      {
        ++ invalid_stats_.num_corrupted_msg;
        return;
      }

      ++msg;

      auto price = parseDouble(msg, '\0');
      if(UNLIKELY(price == std::numeric_limits<double>::infinity()))
      {
        ++ invalid_stats_.num_corrupted_msg;
        return;
      }

      if(UNLIKELY(price <= 0))
      {
        ++ invalid_stats_.num_invalid_neg;
        return;
      }
      
      //std::cout << "processTrade: " << qty << " @ " << price << std::endl;
      if(price == last_trade_.first)
      {
        last_trade_.second += qty;
      }
      else
      {
        last_trade_.first = price;
        last_trade_.second = qty;
      }
    }

  private:
    InvalidStats invalid_stats_;
    OrderBook<boost::object_pool<Order>, 
        boost::object_pool<PriceLevel>> order_book_;
    std::pair<price_t, qty_t> last_trade_= {0,0};
};

int main(int argc, char **argv)
{
  if(argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <feed message file>" << std::endl;
    return -1;
  }

  FeedHandler feed;
  std::string line;
  const std::string filename(argv[1]);
  std::ifstream infile(filename.c_str(), std::ios::in);
  int counter = 0;
  while (std::getline(infile, line)) 
  {
    feed.processMessage(line);
    if (++counter % 10 == 0) {
      feed.printCurrentOrderBook(std::cerr);
    }
  }
  
  feed.printCurrentOrderBook(std::cerr);
  feed.printInvadStat(std::cout);

  return 0;
}
