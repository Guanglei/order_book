#! /usr/bin/python

import sys, random

if(len(sys.argv) != 2):
  print "Usage: {} <number of message to generate>".format(sys.argv[0])
  sys.exit(-1);

random.seed()

g_order_id = 0
g_order_map = {}

for i in range(int(sys.argv[1])):
  type = random.randint(1,13)
  if type >=1 and type <=7:
    side = random.randint(1,2)
    side = 'B' if side == 1 else 'S'
    qty = random.randint(1,10)
    price = 0.0
    if side == 'S':
      price = random.randint(188,200)/2.0
    else:
      price = random.randint(180, 192)/2.0
    
    g_order_id += 1
    g_order_map[g_order_id] = [side, qty, price]
    print "A,{},{},{},{}".format(g_order_id, side, qty, price)
  elif type >=8 and type <= 10 and len(g_order_map) > 0:
    index = random.randint(0, len(g_order_map)-1)
    order_id = g_order_map.items()[index][0]
    order_to_cancel = g_order_map.items()[index][1]
    print "X,{},{},{},{}".format(order_id, order_to_cancel[0], order_to_cancel[1], order_to_cancel[2])
    del g_order_map[order_id]
  elif type >=11 and type <=12:
    index = random.randint(0, len(g_order_map)-1)
    order_id = g_order_map.items()[index][0]
    order_to_amend = g_order_map.items()[index][1]
    order_to_amend[1] += random.randint(1,10);
    need_amend_price = random.randint(1,2)
    if need_amend_price == 1:
      if order_to_amend[0] == 'B':
        order_to_amend[2] = random.randint(180,192)/2.0
      else:
        order_to_amend[2] = random.randint(188,200)/2.0

    print "M,{},{},{},{}".format(order_id, order_to_amend[0], order_to_amend[1], order_to_amend[2])
  elif type == 13:
    print "T,{},{}".format(random.randint(1,10), random.randint(180,200)/2.0)
 
    


