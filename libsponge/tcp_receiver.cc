#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    DUMMY_CODE(seg);

    // SYN and FIN in one segment
    if (seg.header().syn) {
        _isn = seg.header().seqno;
    }

    if (!_isn.has_value()) {
        return;
    }

    uint64_t abs_seqno;
    bool eof;

    // abs_seqno >= 1
    abs_seqno = unwrap(seg.header().seqno, _isn.value(), _checkpoint);

    // the segment has been received
    if (abs_seqno + static_cast<uint64_t>(seg.length_in_sequence_space()) <= _checkpoint) {
        return;
    }

    if (seg.header().fin) {
        eof = true;
    } else {
        eof = false;
    }

    uint64_t data_index;
    // SYN occpy one bit and the data start from abs_seqno = 1
    if (seg.header().syn) {
        data_index = abs_seqno;
    } else {
        data_index = abs_seqno - 1;
    }

    // can't be sure the segment's payload has been put into reassembled.
    _reassembler.push_substring(seg.payload().copy(), data_index, eof);

    // get the number of reassembled bytes.
    size_t written = _reassembler.stream_out().bytes_written();

    // _checkpoint is the abs index that has been received
    _checkpoint = 0 + static_cast<uint64_t>(written);

    // once FIN get accepted then the ackno should add one bit
    // because need to reply on ack segment
    // FIN <-  ACK ->
    if (_reassembler.stream_out().input_ended()) {
        _checkpoint += 1;
    }

    _ackno = wrap(_checkpoint + 1, _isn.value());

    return;
}

optional<WrappingInt32> TCPReceiver::ackno() const { return _ackno; }

size_t TCPReceiver::window_size() const {
    size_t win_size;
    win_size = _capacity - _reassembler.stream_out().buffer_size();
    return win_size;
}