#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

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
  uint32_t next_hop_ip_address = next_hop.ipv4_numeric();
  auto target_kv = _ARP_mapping.find( next_hop_ip_address );

  if ( target_kv != _ARP_mapping.end() ) {
    EthernetAddress target_ethernet_address = target_kv->second.first;
    EthernetFrame target_mac_frame
      = make_frame( ethernet_address_, target_ethernet_address, EthernetHeader::TYPE_IPv4, serialize( dgram ) );
    transmit( target_mac_frame );
  } else {
    // queue the IP datagram
    _waiting_dgrams[next_hop_ip_address].emplace_back(dgram);

    if (_arp_time_tracker.contains(next_hop_ip_address))
      return;

    _arp_time_tracker.emplace(next_hop_ip_address, Timer{});
    // broadcast and ARP request for the next hop's ethernet address
    ARPMessage arp_request = make_arp( ARPMessage::OPCODE_REQUEST, {}, next_hop_ip_address );
    EthernetFrame broadcast = make_frame(ethernet_address_, ETHERNET_BROADCAST, EthernetHeader::TYPE_ARP, serialize(arp_request));
    transmit( broadcast );
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
  (void)frame;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Update and check ARP mapping timers
  erase_if(_ARP_mapping, [&](auto&& item) noexcept -> bool {
    Timer timer = item.second.second;
    timer.timer_tick(ms_since_last_tick);
    return timer.timer_expired(ARP_ENTRY_TTL_ms);
  });

  // Update and check ARP time tracker timers
  erase_if(_arp_time_tracker, [&](auto&& item) noexcept -> bool {
    Timer timer = item.second;
    timer.timer_tick(ms_since_last_tick);
    return timer.timer_expired(ARP_RESPONSE_TTL_ms);
  });
}

/* util methods */

ARPMessage NetworkInterface::make_arp( const uint16_t opcode,
                                       const EthernetAddress& target_ethernet_address,
                                       const uint32_t target_ip_address )
{
  ARPMessage arp;
  arp.opcode = opcode;
  arp.sender_ethernet_address = ethernet_address_;
  arp.sender_ip_address = ip_address_.ipv4_numeric();
  arp.target_ethernet_address = target_ethernet_address;
  arp.target_ip_address = target_ip_address;
  return arp;
}

EthernetFrame NetworkInterface::make_frame( const EthernetAddress& src,
                                            const EthernetAddress& dst,
                                            const uint16_t type,
                                            vector<string> payload )
{
  EthernetFrame frame;
  frame.header.src = src;
  frame.header.dst = dst;
  frame.header.type = type;
  frame.payload = std::move( payload );
  return frame;
}
