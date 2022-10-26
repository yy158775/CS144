#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    ,_retransmission_timeout(retx_timeout)
    ,_timer(nullopt) {
}

uint64_t TCPSender::bytes_in_flight() const { 
    return _bytes_in_flight;
}

void TCPSender::fill_window() {
    uint64_t right_edge;
    right_edge = _ack_seqno+static_cast<uint64_t>(_win_size);

    // how to exit the while gracefully
    while(1) {
        TCPSegment seg;
    
        if(_next_seqno == 0) {
            seg.header().syn = true;
        }

        // window size
        if(right_edge <= _next_seqno) {
            break;
        }
        
        if(_fin_sended) {
            break;
        }
        // syn occpy one bit
        // len is read data
        size_t len = min(right_edge-(_next_seqno + (seg.header().syn ? 1:0)),TCPConfig::MAX_PAYLOAD_SIZE);
        string send_data = _stream.read(len);

        seg.payload() = std::move(send_data);
        seg.header().seqno = wrap(_next_seqno,_isn);
        
        // EOF
        // winsize = 100
        // data = 100 fin = 1 
        if(_stream.eof() && seg.length_in_sequence_space()+_next_seqno < right_edge) {
            seg.header().fin = true;
        }

        if(seg.length_in_sequence_space() == 0) {
            break;
        }

        _segments_out.push(seg);
        _segments_in_flight.push(seg);
        
        _next_seqno += seg.length_in_sequence_space();        
        _bytes_in_flight += seg.length_in_sequence_space();

        if(_timer == nullopt) {
            retransmission_timer r_timer;
            r_timer.rto = _retransmission_timeout;
            r_timer.start_time = _ms_since_start;
            _timer = r_timer;
        }

        // FIN
        if(seg.header().fin) {
            // done't send data to peer
            _fin_sended = true;
            break;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    DUMMY_CODE(ackno, window_size);
    
    if(window_size == 0) {
        _win_size = 1;
        _zero_win = true;
    } else {
        _win_size = window_size;
        _zero_win = false;
    }

    uint64_t cur_ack_seqno = unwrap(ackno,_isn,_next_seqno);
    
    // invalid
    if(cur_ack_seqno > _next_seqno || cur_ack_seqno < _ack_seqno) {
        return;
    }

    _ack_seqno = cur_ack_seqno;

    TCPSegment seg;
    
    while(!_segments_in_flight.empty()) {
        seg = _segments_in_flight.front();
        uint64_t end_index = unwrap(seg.header().seqno,_isn,_next_seqno) + seg.length_in_sequence_space();
        if(cur_ack_seqno >= end_index) {
            _bytes_in_flight -= seg.length_in_sequence_space();
            _segments_in_flight.pop();
            _timer = nullopt;
        } else {
            break;
        }
    }

    // Set the RTO back to its "initial value."
    _retransmission_timeout = _initial_retransmission_timeout;

    // restart the retransmission timer
    if(!_segments_in_flight.empty() && _timer == nullopt) {
        retransmission_timer tr;
        tr.rto = _retransmission_timeout;
        tr.start_time = _ms_since_start;
        _timer = tr;
    }

    // reset
    _consecutive_retransmissions = 0;
    return;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    DUMMY_CODE(ms_since_last_tick);
    if(_timer == nullopt) {
        return;
    }
    _ms_since_start += ms_since_last_tick;

    size_t expire_time = (*_timer).start_time + (*_timer).rto;
    

    // retransmit
    // update _consecutive_retransmissions
    // double RTO
    // start new timer
    if(expire_time <= _ms_since_start) {
        _timer = nullopt;
        if(_segments_in_flight.empty()) {
            return;
        }
        
        TCPSegment seg;
        seg = _segments_in_flight.front();
        _segments_out.push(seg);

        _consecutive_retransmissions += 1;
        
        if(!_zero_win) {
            _retransmission_timeout *= 2;
        }

        retransmission_timer rt;
        rt.rto = _retransmission_timeout;
        rt.start_time = _ms_since_start;
        _timer = rt;
    }
    return;
}

unsigned int TCPSender::consecutive_retransmissions() const { 
    return _consecutive_retransmissions;
}

void TCPSender::send_empty_segment() {

}
