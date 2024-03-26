#include "reassembler.hh"
#include <iostream>

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
  if ( data.empty() ) {
    if ( is_last_substring ) {
      _should_eof = true;
    }
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
    if ( !_wait_map.contains( i ) ) {
      _wait_map.insert( make_pair( i, data[j] ) );
    }

    // j is the last index of the data and if we have eof is true then we should set the flag to be true
    if ( j + 1 == data.size()  && is_last_substring) {
        _should_eof = true;
    }

    if ( next( i ) == start_index ) {
      break;
    }
  }

  string send_str {};
  for ( uint64_t i = start_index; _wait_map.contains( i ); i = next( i ) ) {
    send_str.push_back( _wait_map[i] );
    if ( next( i ) == start_index ) {
      break;
    }
  }

  if ( !send_str.empty() ) {
    output_.writer().push( send_str );
    uint64_t written_num = send_str.size();
    for ( uint64_t i = start_index, j = 0; j < written_num; i = next( i ), ++j ) {
      _wait_map.erase( i );
    }
    _first_unassembled_index += written_num;
  }

  if ( _should_eof && bytes_pending() == 0 ) {
    output_.writer().close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return _wait_map.size();
}
