#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):
    _data(),_capa(capacity),_input_ended(false),_bytes_read(0),_bytes_written(0) {
    DUMMY_CODE(capacity); 
}

size_t ByteStream::write(const string &data) {
    DUMMY_CODE(data);
    size_t lens;
    lens = min(remaining_capacity(),data.size());
    _data = _data + data.substr(0,lens);
    _bytes_written += lens;

    return lens;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    DUMMY_CODE(len);
    return _data.substr(0,len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    DUMMY_CODE(len);
    size_t rlen;
    rlen = min(buffer_size(),len);
    _data = _data.substr(rlen);
    _bytes_read += rlen;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    DUMMY_CODE(len);
    string res;
    res = peek_output(len);
    pop_output(len);
    return res;
}

void ByteStream::end_input() {
    _input_ended = true;
}

bool ByteStream::input_ended() const { 
    return _input_ended;
}

size_t ByteStream::buffer_size() const { 
    return _data.size();
}

bool ByteStream::buffer_empty() const { 
    return _data.empty(); 
}

bool ByteStream::eof() const { 
    return _input_ended && buffer_size() == 0; 
}

size_t ByteStream::bytes_written() const { 
    return _bytes_written;
}

size_t ByteStream::bytes_read() const { 
    return _bytes_read;
}

size_t ByteStream::remaining_capacity() const { 
    return _capa - buffer_size(); 
}