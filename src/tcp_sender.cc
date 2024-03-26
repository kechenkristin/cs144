#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
    return _next_abs_seqno - _receiver_ack;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return {};
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  // special case1: if we already sent the FIN
  (void) transmit;
    if (end)
      return;

    // get the window_size
    // special case2: if the _received_window set to be 0
    int64_t window_size = _receive_window_size == 0 ? 1 : _receive_window_size;

    TCPSenderMessage tcpSenderMessage{};
    // read from the input stream to build TCPSendMessage
    // special case3: TCP Connection initializing
    if ( _next_abs_seqno == 0) {
      tcpSenderMessage.SYN = true;
    }

}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  return {};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  (void)msg;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  (void)ms_since_last_tick;
  (void)transmit;
  (void)initial_RTO_ms_;
}
