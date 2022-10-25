#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    DUMMY_CODE(n, isn);
    uint32_t b = static_cast<uint32_t>(n%0x100000000);
    return operator+(isn,b);
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    DUMMY_CODE(n, isn, checkpoint);
    uint32_t offset = operator-(n,isn);
    uint64_t startidx = static_cast<uint64_t>(offset);
    if(checkpoint <= startidx) {
        return startidx;
    } else {
        int64_t dis = static_cast<int64_t>(checkpoint - startidx);
        int64_t res = dis/(0x100000000);
        uint64_t left = static_cast<uint64_t>(startidx + res*(0x100000000));
        uint64_t right = static_cast<uint64_t>(startidx + (res+1)*(0x100000000));
        if((checkpoint-left) <= static_cast<uint64_t>(INT32_MAX)) {
            return left;
        } else if((right-checkpoint) <= static_cast<uint64_t>(INT32_MAX)) {
            return right;
        }
    }
    return {};
}
