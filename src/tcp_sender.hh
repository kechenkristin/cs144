#pragma once

#include "byte_stream.hh"
#include "retransmission_timer.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;  //! outgoing stream of bytes that have not yet been sent
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  // additional variables
  // Keep track of which segments have been sent but not yet acknowledged by the receiver—
  // we call these “outstanding” segments
  std::deque<TCPSenderMessage> _outstanding_msgs{};

  // the absolute seqno for the next byte to be sent
  uint64_t _next_abs_seqno {0};

  // the absolute receiver ack
  uint64_t _receive_ack {0};

  // the initial window size should be
  uint64_t _receive_window_size{1};

  //! the consecutive retransmissions
  uint64_t _consecutive_retransmissions{0};

  // whether the fin is sent
  bool _end { false};

  // the retransmission timer
  RetransmissionTimer _retransmission_timer{initial_RTO_ms_};


  // helper methods
  /* A helper method to tell whether the window is not full. */
  bool window_not_full(uint64_t window_size) const {return window_size > sequence_numbers_in_flight(); }
};
