#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    
    EthernetFrame frame;
    if(_ip_to_mac.count(next_hop_ip)) {
        frame.header().type = EthernetHeader::TYPE_IPv4;
        
        BufferList buflist = dgram.serialize();
        frame.payload().append(buflist);
        
        frame.header().src = _ethernet_address;
        frame.header().dst = _ip_to_mac[next_hop_ip];
    } else {
        _ip_to_datagram[next_hop_ip].push_back(dgram);

        // record the ARP request send time
        if(_ip_save_time.count(next_hop_ip)) {
            size_t save_time = _ip_save_time[next_hop_ip];
            if(save_time + 5000 > _ms_since_start) {
                return;
            }
        }
        
        _ip_save_time[next_hop_ip] = _ms_since_start;

        ARPMessage arp_req;
        arp_req.opcode = ARPMessage::OPCODE_REQUEST;
        arp_req.sender_ip_address = _ip_address.ipv4_numeric();
        arp_req.sender_ethernet_address = _ethernet_address;
        arp_req.target_ip_address = next_hop.ipv4_numeric();
        
        BufferList buflist = arp_req.serialize();

        frame.header().src = _ethernet_address;
        frame.header().dst = ETHERNET_BROADCAST;
        frame.header().type = EthernetHeader::TYPE_ARP;
        
        frame.payload().append(buflist);
    }
    
    _frames_out.push(std::move(frame));

    DUMMY_CODE(dgram, next_hop, next_hop_ip);
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    DUMMY_CODE(frame);
    if(frame.header().dst != _ethernet_address 
        && frame.header().dst != ETHERNET_BROADCAST) {
        return nullopt;
    }

    if(frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram dgram;
        
        ParseResult res;
        res = dgram.parse(frame.payload());
        if(res == ParseResult::NoError) {
            return dgram;
        } else {
            return nullopt;
        }
    } else if(frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp;

        ParseResult res;
        res = arp.parse(frame.payload());
        if(res != ParseResult::NoError) {
            return nullopt;
        }

        uint32_t sender_ip = arp.sender_ip_address;
        EthernetAddress sender_mac = arp.sender_ethernet_address;

        _ip_to_mac[sender_ip] = sender_mac; 
        _ip_save_time[sender_ip] = _ms_since_start;

        for(size_t i = 0;i < _ip_to_datagram[sender_ip].size();i++) {
            InternetDatagram dgram = _ip_to_datagram[sender_ip][i];
            Address addr = Address::from_ipv4_numeric(sender_ip);
            send_datagram(dgram,addr);
        }

        _ip_to_datagram[sender_ip].clear();
        

        if(arp.opcode == ARPMessage::OPCODE_REQUEST) {
            if(arp.target_ip_address == _ip_address.ipv4_numeric()) {
                ARPMessage arp_reply;
                arp_reply.sender_ip_address = _ip_address.ipv4_numeric();
                arp_reply.sender_ethernet_address = _ethernet_address;
                arp_reply.target_ip_address = arp.sender_ip_address;
                arp_reply.target_ethernet_address = arp.sender_ethernet_address;
                arp_reply.opcode = ARPMessage::OPCODE_REPLY;

                EthernetFrame send_frame;
                send_frame.header().src = _ethernet_address;
                send_frame.header().dst = arp.sender_ethernet_address;
                send_frame.header().type = EthernetHeader::TYPE_ARP;
                BufferList buflist = arp_reply.serialize();
                send_frame.payload().append(buflist);
                _frames_out.push(send_frame);
            }
        }
        return nullopt;
    }
    return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { 
    DUMMY_CODE(ms_since_last_tick); 
    _ms_since_start += ms_since_last_tick;
    unordered_map<uint32_t,size_t>::iterator it = _ip_save_time.begin();

    vector<uint32_t>deleted_ip;

    while(it != _ip_save_time.end()) {
        uint32_t ip = it->first;
        size_t save_time = it->second;
        if(save_time + 30000 < _ms_since_start) {
            _ip_to_mac.erase(ip);
            deleted_ip.push_back(ip);
        }
        it++;
    }

    for(size_t i = 0;i < deleted_ip.size();i++) {
        uint32_t ip = deleted_ip[i];
        _ip_save_time.erase(ip);
    }
}