#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"
#include <vector>
#include <numeric>

using namespace std;
// inline std::string concat( const std::vector<std::string>& buffers )
// {
//   return std::accumulate( buffers.begin(), buffers.end(), std::string {} );
  
// }

// std::string summary( const EthernetFrame& frame )
// {
//   std::string out = frame.header.to_string() + " payload: ";
//   switch ( frame.header.type ) {
//     case EthernetHeader::TYPE_IPv4: {
//       InternetDatagram dgram;
//       if ( parse( dgram, frame.payload ) ) {
//         out.append( dgram.header.to_string() + " payload=\"" 
//                     + "\"" );
//       } else {
//         out.append( "bad IPv4 datagram" );
//       }
//     } break;
//     case EthernetHeader::TYPE_ARP: {
//       ARPMessage arp;
//       if ( parse( arp, frame.payload ) ) {
//         out.append( arp.to_string() );
//       } else {
//         out.append( "bad ARP message" );
//       }
//     } break;
//     default:
//       out.append( "unknown frame type" );
//       break;
//   }
//   return out;
// }

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // Your code here.

  uint32_t ipaddress = next_hop.ipv4_numeric();
  struct EthernetFrame frame;  
  struct EthernetHeader fheader;
  
  fheader.src = ethernet_address_;
  fheader.type = EthernetHeader::TYPE_IPv4;
  std::vector<std::string> fpayload = serialize(dgram);
  frame.payload = std::move(fpayload);


  if (ipMap.find(ipaddress) != ipMap.end() && timer <= ipMap[ipaddress].time + 30000) {  
    fheader.dst = ipMap[ipaddress].ethernetAddress;
    frame.header = fheader;

    transmit(frame);


  } else {
    frame.header = fheader;
    waitingFrames.push_back({frame, ipaddress});
   
    if (lastArpTime.find(ipaddress) == lastArpTime.end() || lastArpTime[ipaddress] + 5000 < timer) {
      struct EthernetFrame arpFrame;
      struct EthernetHeader arpFheader;

      arpFheader.src = ethernet_address_;
      arpFheader.dst = ETHERNET_BROADCAST;
      arpFheader.type = EthernetHeader::TYPE_ARP;

      ARPMessage request;
      request.opcode = ARPMessage::OPCODE_REQUEST;
      request.sender_ethernet_address = ethernet_address_;
      request.sender_ip_address = ip_address_.ipv4_numeric();
      request.target_ethernet_address = {0, 0, 0, 0, 0, 0};
      request.target_ip_address = ipaddress;
      
      arpFrame.header = arpFheader;
      arpFrame.payload = serialize(request);

      // cout << "achieve here to transmit arp?" << endl;

      transmit(arpFrame);

      if (lastArpTime.find(ipaddress) == lastArpTime.end()) {
        lastArpTime.insert(make_pair(ipaddress, timer));
      } else {
        lastArpTime[ipaddress] = timer;
      }
    }
    
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  
  if (frame.header.type == EthernetHeader::TYPE_ARP) {
    ARPMessage arp;
    parse(arp, frame.payload);
    struct macInfo thisMacInfo;
    thisMacInfo.ethernetAddress =  arp.sender_ethernet_address;
    thisMacInfo.time = timer;

    if (ipMap.find(arp.sender_ip_address) == ipMap.end()) {
      ipMap.insert(make_pair(arp.sender_ip_address, thisMacInfo));
    } else {
      ipMap[arp.sender_ip_address] = thisMacInfo;
    }
    
    
    for (auto it = waitingFrames.begin(); it != waitingFrames.end(); /* no increment here */) {
        // Check if the current element should be removed
        // std::cout << "it->ip: " << it->ip << " arp.sender_ip_address: " << arp.sender_ip_address << endl;
        if (it->ip == arp.sender_ip_address) {
            // Erase the element and obtain the iterator to the next element
            it->frame.header.dst = arp.sender_ethernet_address;
            //std::cout << "parsedData: " << summary(it->frame) << endl;
            transmit(it->frame);
            it = waitingFrames.erase(it);
        } else {
            // Move to the next element
            ++it;
        }
    }

    
    if (arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == ip_address_.ipv4_numeric()) {

      
      struct EthernetFrame sendFrame;
    
      struct EthernetHeader fheader;
      fheader.src = ethernet_address_;
      fheader.dst =  arp.sender_ethernet_address;
      fheader.type = EthernetHeader::TYPE_ARP;

      ARPMessage reply;
      reply.opcode = ARPMessage::OPCODE_REPLY;
      reply.sender_ethernet_address = ethernet_address_;
      reply.sender_ip_address = ip_address_.ipv4_numeric();
      reply.target_ethernet_address = arp.sender_ethernet_address;
      reply.target_ip_address = arp.sender_ip_address;


      sendFrame.header = fheader;
      sendFrame.payload = serialize(reply);


      transmit(sendFrame);
    }

  } else if (frame.header.type == EthernetHeader::TYPE_IPv4 && frame.header.dst == ethernet_address_) {
    // std::cout << "parsedData before pushing to received queue: " << summary(frame) << endl;
    InternetDatagram revData;
    bool parseRes = parse(revData, frame.payload);
    if (parseRes) {
      
      datagrams_received_.push(revData);
      // InternetDatagram revData2;
      // bool parseRes2 = parse(revData2, serialize(revData));
      // std::cout << "-------------parseRes2--------------" << endl;
      // if (!parseRes2) {
      //   std::cout << "Couldn't be parsed" << endl;
      // } else {
      //   std::cout << "Could be parsed." << endl;
      //   std::cout << "datagram: " << revData.header.to_string() + " payload=\"" + concat( revData.payload ) 
      //               + "\""<< endl;
      // }
      // std::cout << "-------------parseRes2--------------" << endl;
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  timer += ms_since_last_tick;

}
