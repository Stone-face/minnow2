#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

void Router::uint32ToBitsArray(uint32_t num, bool bitsArray[32]) {
    for (int i = 0; i < 32; i++) {
        bitsArray[i] = (num & 1) == 1;
        num >>= 1;
    }
}

bool Router::match(uint32_t route_prefix, uint8_t prefix_length, uint32_t ip) {

  // std::cout << "route_prefix: " << Address::from_ipv4_numeric(route_prefix).to_string() << endl;
  // std::cout << "prefix_length: " << prefix_length << endl;
  // std::cout << "ip: " << Address::from_ipv4_numeric(ip).to_string() << endl;
  bool bits1[32];
  bool bits2[32];
  uint32ToBitsArray(route_prefix, bits1);
  uint32ToBitsArray(ip, bits2);

  for(uint8_t i = 0; i < prefix_length; i++) {
    if (bits1[31 - i] != bits2[31 - i]) {
      //std::cout << "res: false" << endl;
      return false;
    }
  }
  //std::cout << "res: true" << endl;
  return true;
}

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  // cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
  //      << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
  //      << " on interface " << interface_num << "\n";

  // Your code here.
  RouteItem item;
  item.route_prefix = route_prefix;
  item.prefix_length = prefix_length;
  item.next_hop = next_hop;
  item.interface_num = interface_num;
  routeTable.push_back(item);
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
  // std::cout << "route() entered " << endl;
  for (size_t i = 0; i < _interfaces.size(); i++) {
    std::queue<InternetDatagram>& que = interface(i)->datagrams_received();
    if (que.size() > 0) {
      std::cout << "interfaces num: "  << i << " que size: " << que.size() << endl;
    }
    
    while (!que.empty()) {
      InternetDatagram datagram = que.front();  
      int maxLen = -1;
      int maxInd = -1;

      std::cout << "routeTable size: " << routeTable.size() << endl;
      for (size_t j = 0; j < routeTable.size(); j++) {
        RouteItem item = routeTable[j];
        if (match(item.route_prefix, item.prefix_length, datagram.header.dst)) {
          if (item.prefix_length > maxLen) {
            maxLen = item.prefix_length;
            maxInd = j;
          }
        }
      }

      if (maxInd >= 0 && datagram.header.ttl > 1) {
        datagram.header.ttl--;
        RouteItem item = routeTable[maxInd];

        InternetDatagram revData;
        bool parseRes = parse(revData, serialize(datagram));
        if (!parseRes) {
          std::cout << "Couldn't be parsed in router.cc before transmitting." << endl;
        } else {
          std::cout << "Could be parsed in router.cc before transmitting." << endl;
        }


        if (item.next_hop.has_value()) {
          interface(item.interface_num)->send_datagram(datagram, item.next_hop.value());
          std::cout << "interface_num: " << item.interface_num << " next_hop: " << item.next_hop.value().to_string() << endl;
        } else {
          interface(item.interface_num)->send_datagram(datagram, Address::from_ipv4_numeric(datagram.header.dst));
          std::cout << "interface_num: " << item.interface_num << " next_hop: " << Address::from_ipv4_numeric(datagram.header.dst).to_string() << endl;
        }
      }

      que.pop(); 
    }
  }
}




