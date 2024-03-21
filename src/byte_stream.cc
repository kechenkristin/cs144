#include "byte_stream.hh"
#include <cstring>
#include <vector>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}



void Writer::push( string data )
{
  // Your code here.
  if (data.empty() || available_capacity() == 0) return;
  uint64_t len_to_push = min(available_capacity(), data.size());
  buffer.insert(buffer.end(), data.begin(), data.begin() + len_to_push);
  bytes_written += len_to_push;
}

void Writer::close()
{
  // Your code here.
    close_flag = true;
}


bool Writer::is_closed() const
{
  // Your code here.
  return close_flag;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - buffer.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_written;
}




string_view Reader::peek() const
{
  // Your code here.
  if (!buffer.empty()) {
    // Get the reference to the first element of the deque
    const char& nextByte = buffer.front();
    // Create a string_view from the character
    return {&nextByte, 1};
  } else {
    // Return an empty string_view if the deque is empty
    return {};
  }
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t len_to_pop = min(len, buffer.size());
  buffer.erase(buffer.begin(), buffer.begin() + len_to_pop);
  bytes_read += len_to_pop;
}


bool Reader::is_finished() const
{
  // Your code here.
  return writer().is_closed() && bytes_read == bytes_written;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_read;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer.size();
}
