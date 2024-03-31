#pragma once

#include "byte_stream.hh"
#include <map>
#include <unordered_map>
#include <vector>

class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output ) : output_( std::move( output ) )
  {
    _window.resize( output_.capacity(), 0 ); // Initialize the window and make it full of 0 (empty)
    _dirty.resize( output_.capacity(), false );
  }

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the _end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  [[nodiscard]] uint64_t bytes_pending() const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  [[nodiscard]] const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  [[nodiscard]] const Writer& writer() const { return output_.writer(); }
  [[nodiscard]] uint64_t first_unassembled_index() const { return _first_unassembled_index; }

private:
  ByteStream output_; // the Reassembler writes to this ByteStream

  // additional attributes
  uint64_t _capacity { output_.writer().available_capacity() }; // the overall capacity, shared by green and red
  uint64_t _first_unassembled_index { 0 };                      // The index of the next byte expected in the stream
  std::vector<char> _window {};                                 //!< The window
  std::vector<bool> _dirty {};       //!< A bitmap to indicate whether the element is stored
  uint64_t _unassembled_bytes { 0 }; //!< The number of bytes in the substrings stored but not yet reassembled
  [[nodiscard]] uint64_t next( uint64_t ptr ) const { return ( ptr + 1 ) % _capacity; }
  bool _should_eof { false };
};
