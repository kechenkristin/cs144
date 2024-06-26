#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.

  // STEP1: Check the boundary
  // case1
  if ( first_index >= _capacity + _first_unassembled_index
       || _first_unassembled_index > first_index + data.size() ) {
    return;
  }

  // Corner case: when the data is empty. This is important
  // Because the below iteration does not consider
  if ( data.empty() && is_last_substring ) {
    _should_eof = true;
  }

  uint64_t data_index { 0 }; // The index of start of valid data, only apply to the passed string param
  uint64_t actual_index {
    first_index }; // The actual start index of the string in the axis(initialize as
                   // first_index in case 1, 2, 4; equals to _first_unassembled_index in case 3)

  // case 3
  if ( first_index < _first_unassembled_index ) {
    actual_index = _first_unassembled_index;
    data_index += _first_unassembled_index - first_index;
  }

  uint64_t start_index = _first_unassembled_index % _capacity;
  uint64_t loop_index = actual_index % _capacity;
  uint64_t first_unacceptable_index = _first_unassembled_index + _capacity - reader().bytes_buffered();
  if ( first_unacceptable_index <= actual_index ) {
    return;
  }

  // loop_size is the valid string length, "window"
  uint64_t loop_size = min( data.size() - data_index, first_unacceptable_index - actual_index );

  // STEP2: The real processing logic
  // first iterate, iterate the data(string) with the window(loop_size)
  for ( uint64_t i = loop_index, j = data_index; j < data_index + loop_size; i = next( i ), j++ ) {
    if ( !_dirty[i] ) {
      _window[i] = data[j];
      _unassembled_bytes++;
      _dirty[i] = true;
    }

    // j is the last index of the data and if we have eof is true then we should set the flag to be true
    if ( j + 1 == data.size() && is_last_substring ) {
      _should_eof = true;
    }

    if ( next( i ) == start_index ) {
      break;
    }
  }

  // second iterate, iterate through the _wait_map to construct the send_str
  string send_str {};
  // using the next util to smartly captivate consecutive string in window
  for ( size_t i = start_index; _dirty[i]; i = next( i ) ) {
    send_str.push_back( _window[i] );
    // we pushed all the data into to send_str
    if ( next( i ) == start_index )
      break;
  }

  if ( !send_str.empty() ) {
    output_.writer().push( send_str );
    uint64_t written_num = send_str.size();
    // third iterate, erase data in wait_map
    for ( size_t i = start_index, j = 0; j < written_num; i = next( i ), ++j ) {
      _dirty[i] = false;
    }
    _unassembled_bytes -= written_num;
    _first_unassembled_index += written_num;
  }

  if ( _should_eof && bytes_pending() == 0 ) {
    output_.writer().close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return _unassembled_bytes;
}
