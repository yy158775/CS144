#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : 
    _next_index(0),_output(capacity), _capacity(capacity),_unassembled_bytes(0),_datagrams() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    DUMMY_CODE(data, index, eof);
    uint64_t left,right; // [left,right)
    left = _next_index;

    list<datagram>::iterator it = _datagrams.begin();
    
    while(it != _datagrams.end()) {
        right = it->_index; // [left,right)
        uint64_t data_left,data_right;
        data_left = max(left,index);
        data_right = min(right,index+data.size()); // [)

        if(data_left < data_right) {
            bool datagram_eof;
            if(data_right == index+data.size()) {
                datagram_eof = eof;
            } else {
                datagram_eof = false;
            }
            datagram new_datagram(data.substr(data_left-index,data_right-data_left),data_left,datagram_eof);
            // before
            _datagrams.insert(it,new_datagram);
            _unassembled_bytes += new_datagram._data.size();
        }

        left = it->_index + it->_data.size();
        
        if(left >= index+data.size()) {
            break;
        }

        it++;
    }

    // <=
    if(left <= index+data.size()) {
        uint64_t data_left;
        data_left = max(left,index);
        // std::move
        datagram new_datagram(data.substr(data_left-index),data_left,eof); // EOF
        _datagrams.push_back(new_datagram);
        _unassembled_bytes += new_datagram._data.size();
    }

    // Capacity limit
    size_t all_bytes = _unassembled_bytes + _output.buffer_size();
    size_t remove_bytes;
    
    if(all_bytes > _capacity) {
        remove_bytes = int(all_bytes) - int(_capacity);
    } else {
        remove_bytes = 0;
    }

    while(remove_bytes > 0) {
        if(_datagrams.back()._data.size() <= remove_bytes) {
            remove_bytes -= _datagrams.back()._data.size();
            _unassembled_bytes -= _datagrams.back()._data.size();
            _datagrams.pop_back();
        } else {
            size_t pos = _datagrams.back()._data.size()-remove_bytes;
            _datagrams.back()._data.erase(pos);
            _datagrams.back()._eof = false;
            _unassembled_bytes -= remove_bytes;
            remove_bytes = 0;
        }
    }

    // Merge
    list<datagram>::iterator prev,cur;
    cur = _datagrams.begin();
    cur++;
    prev = _datagrams.begin();

    while(cur != _datagrams.end()) {
        if(prev->_index + prev->_data.size() == cur->_index) {
            
            prev->_data = prev->_data + cur->_data;
            prev->_eof = cur->_eof;

            _datagrams.erase(cur);
            prev++;
            cur = prev;
            prev--;
        } else {
            prev = cur;
            cur++;
        }
    }

    // Write to ByteStream
    it = _datagrams.begin();
    if(it != _datagrams.end() && it->_index == _next_index) {
        _output.write(it->_data);
        _next_index += it->_data.size();
        _unassembled_bytes -= it->_data.size();
        
        // EOF
        if(it->_eof) {
            _output.end_input();
        }
        
        _datagrams.pop_front();
    }
    
    return;
}

size_t StreamReassembler::unassembled_bytes() const { 
    return _unassembled_bytes; 
}

bool StreamReassembler::empty() const { 
    return _output.buffer_empty() && _unassembled_bytes == 0;
}
