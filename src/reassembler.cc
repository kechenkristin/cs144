#include "reassembler.hh"
#include <iostream>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  // case1
  if ( first_index + data.size() < _next_index || first_index > _next_index + data.size()) {
    return;
  }

  // Corner case: when the data is empty. This is important
  // Because the below iteration does not consider
  if (data.empty()) {
    if (is_last_substring) {
      _should_eof = true;
    }
  }

  uint64_t data_index{0}; // The index of start of valid data, only apply to the passed string param
  uint64_t actual_index(first_index); // The actual start index of the string in the axis(initialize as first_index in case 1, 2, 4; equals to _next_index in case 3)


  uint64_t start_index = _next_index % _capacity;
  // case 3
//  if (first_index < _next_index) {
//    actual_index = _next_index;
//    data_index = _next_index - first_index;
//  }

  if (first_index < start_index) {
    actual_index = start_index;
    data_index = start_index - first_index;
  }

  uint64_t first_unacceptable_index = _next_index + _capacity + reader().bytes_buffered();
  if (first_unacceptable_index <= actual_index) {
    return;
  }

  uint64_t loop_size = min(data.size() - data_index, first_unacceptable_index - actual_index);

  // Here we need to find the start index and loop index
  // this is the start index of the `_stream`, which is the `_next_index` nothing change just different name
  // this is the loop index of the `_stream`, which is the index where we should start to store the data,
  uint64_t loop_index = actual_index % _capacity;
  for (uint64_t i = loop_index, j = data_index; j < data_index + loop_size; i = next(i), j++) {
    if (!_wait_map.contains(i)) {
      _wait_map.insert( make_pair(i, data[j]));
    }

    // j is the last index of the data and if we have eof is true then we should set the flag to be true
    if (j + 1 == data.size()) {
      if (is_last_substring) {
        _should_eof = true;
      }
    }

    if ( next(i) == start_index) {
      break;
    }
  }

  string send_str{};
  for (uint64_t i = start_index; _wait_map.contains(i); i = next(i)) {
    send_str.push_back(_wait_map[i]);
    if ( next(i) == start_index) {
      break;
    }
  }

  if (!send_str.empty()) {
      output_.writer().push(send_str);
      uint64_t written_num = send_str.size();
      for (uint64_t i = start_index, j = 0; j < written_num; i = next(i), ++j) {
        _wait_map.erase(i);
      }
      _next_index += written_num;
     // _next_index = _next_index % _capacity;
  }

  if (_should_eof && bytes_pending() == 0) {
      output_.writer().close();
  }

}



uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return _wait_map.size();
}


