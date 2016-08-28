
#include "benchmark/benchmark.h"

#include "order_book.h"

using namespace order_book;

static InvalidStats is;
static OrderBook<boost::object_pool<Order>, boost::object_pool<PriceLevel>> g_order_book(is);

static void BM_ORDER_BOOK_ADD_ORDER(benchmark::State& state) 
{
  order_id_t order_id = 0;
  qty_t qty = 1;
  price_t price = 10;
  SideType side = SideType::bid;
  while (state.KeepRunning()) 
  {
    benchmark::DoNotOptimize(g_order_book.add_order(++order_id, side, (++qty)%100, (++price)));
    side = static_cast<SideType>(!static_cast<int>(side));
    if(price >=20)
    {
      price = 10;
    }
  }
}

static void BM_ORDER_BOOK_AMEND_ORDER(benchmark::State& state) 
{
  order_id_t order_id = 0;
  qty_t qty = 1;
  price_t price = 21;
  SideType side = SideType::bid;

  while (state.KeepRunning()) 
  {
    benchmark::DoNotOptimize(g_order_book.amend_order(++order_id, side, (++qty)%50, (++price)));
    if(price >=40)
    {
      price = 21;
    }
  }
}

static void BM_ORDER_BOOK_CANCEL_ORDER(benchmark::State& state) 
{
  order_id_t order_id = 0;
  while (state.KeepRunning()) 
  {
    benchmark::DoNotOptimize(g_order_book.cancel_order(++order_id));
  }
}

BENCHMARK(BM_ORDER_BOOK_ADD_ORDER)->UseRealTime()->MinTime(0.0001);
BENCHMARK(BM_ORDER_BOOK_AMEND_ORDER)->UseRealTime()->MinTime(0.0001);
BENCHMARK(BM_ORDER_BOOK_CANCEL_ORDER)->UseRealTime()->MinTime(0.00005);

BENCHMARK_MAIN()
