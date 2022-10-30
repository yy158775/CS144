#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { 
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const { 
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const { 
    return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const { 
    return _ms_since_start - _ms_last_segment_received;
}

void TCPConnection::segment_received(const TCPSegment &seg) { 
    DUMMY_CODE(seg);
    _ms_last_segment_received = _ms_since_start;
    
    // kills the connection
    if(seg.header().rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _active = false;
        return;
    }

    // not has value
    if(_receiver.ackno() == nullopt) {
        if(!seg.header().syn) {
            return;
        }
    }

    // give it to receiver
    // seqno SYN payload FIN
    _receiver.segment_received(seg);
    
    // give it to the sender
    // ackno winsize
    if(seg.header().ack) {
        _sender.ack_received(seg.header().ackno,seg.header().win);
    }

    // if _receiver: end and _sender : not end
    // 
    // else _sender: end
    // set the wait start time
    if(_receiver.stream_out().input_ended() && _receiver.unassembled_bytes() == 0) {
        if(!_sender.stream_in().eof()) {
            _linger_after_streams_finish = false;
        } else {
            if(_sender.bytes_in_flight() == 0 && !_both_streams_ended.has_value()) {
                _both_streams_ended = _ms_since_start;
                if(!_linger_after_streams_finish) {
                    _active = false;
                }
            }
        }
    }

    // receive the same data from peer and send ack seg
    // receive the syn from peer
    // receive only ack
    // receive seqno = value-1
    if(seg.length_in_sequence_space() > 0) {
        _sender.fill_window();
        if(_sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
    } else {
        if(seg.header().seqno == _receiver.ackno().value()-1) {
            _sender.send_empty_segment();
        } else if(seg.header().ack) {
            _sender.fill_window();
        }
    }
    
    push_outgoing_segments();
}

bool TCPConnection::active() const { 
    return _active;
}

size_t TCPConnection::write(const string &data) {
    DUMMY_CODE(data);
    size_t size = _sender.stream_in().write(data);
    _sender.fill_window();
    push_outgoing_segments();
    return size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    DUMMY_CODE(ms_since_last_tick);
    // time has passed
    _ms_since_start += ms_since_last_tick;
    
    // in TIME_WAIT STATE
    // To wait the ACK for FIN is been received by the peer
    if(_both_streams_ended.has_value() && _linger_after_streams_finish) {
        if(_ms_since_start-*_both_streams_ended >= 10*_cfg.rt_timeout) {
            _active = false;
        }
        return;
    }

    // send RST
    if(_sender.consecutive_retransmissions() >= TCPConfig::MAX_RETX_ATTEMPTS) {
        send_rst();
    } else {
        _sender.tick(ms_since_last_tick);
    }

    push_outgoing_segments();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    push_outgoing_segments();
}

// send a syn
void TCPConnection::connect() {
    _sender.fill_window();
    push_outgoing_segments();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            send_rst();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}


void TCPConnection::push_outgoing_segments() {
    while(!_sender.segments_out().empty()) {
        TCPSegment seg = std::move(_sender.segments_out().front());
        _sender.segments_out().pop();
        
        // ack and ackno
        std::optional<WrappingInt32> ackno = _receiver.ackno();
        
        if(ackno == nullopt) {
            seg.header().ack = false;
            // how to write
        } else {
            seg.header().ack = true;
            seg.header().ackno = *ackno;
        }
        
        // compare is safe?
        if(_receiver.window_size() > std::numeric_limits<uint16_t>::max()) {
            seg.header().win = std::numeric_limits<uint16_t>::max();
        } else {
            seg.header().win = static_cast<uint16_t>(_receiver.window_size());
        }

        _segments_out.push(seg);
    }
    
    return;
}

void TCPConnection::send_rst() {
    if(_sender.segments_out().empty()) {
        _sender.send_empty_segment();
    }
    // if I call fill_window ?
    _sender.segments_out().front().header().rst = true;

    push_outgoing_segments();
    
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _active = false;
    _linger_after_streams_finish = false;
}