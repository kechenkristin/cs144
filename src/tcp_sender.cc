#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return outgoing_bytes_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return retransmission_cnt_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  TCPSenderMessage msg_send_;
  Reader& BytesReader = input_.reader();
  msg_send_.seqno = Wrap32::wrap( next_seqno_, isn_ );
  uint16_t wnd_size = wnd_size_ > 0 ? wnd_size_ : 1;
  fin_ |= BytesReader.is_finished();

  while ( window_not_full(wnd_size) ) {
    if ( !set_syn_ ) {
      msg_send_.SYN = true;
      set_syn_ = true;
      outgoing_bytes_ += 1;
      next_seqno_ += 1;
    }

    while ( BytesReader.bytes_buffered() && window_not_full(wnd_size)
            && msg_send_.sequence_length() < TCPConfig::MAX_PAYLOAD_SIZE ) {
      string_view bytes_view = BytesReader.peek();
      uint16_t send_size = min( TCPConfig::MAX_PAYLOAD_SIZE, wnd_size - outgoing_bytes_ );
      if ( bytes_view.size() > send_size ) {
        bytes_view.remove_suffix( bytes_view.size() - send_size - msg_send_.SYN );
      }

      msg_send_.payload.append( bytes_view );
      uint64_t offset = bytes_view.size() + msg_send_.SYN;
      outgoing_bytes_ += offset;
      next_seqno_ += offset;
      BytesReader.pop( bytes_view.size() );
      fin_ |= BytesReader.is_finished();
    }

    if ( !set_fin_ && fin_ && window_not_full(wnd_size) ) {
      set_fin_ = msg_send_.FIN = true;
      outgoing_bytes_ += 1;
      next_seqno_ += 1;
    }

    if ( msg_send_.sequence_length() ) {
      msg_send_.RST = input_.has_error();
      outstanding_queue_.emplace( msg_send_ );
      transmit( msg_send_ );
    }

    if ( msg_send_.sequence_length() == TCPConfig::MAX_PAYLOAD_SIZE ) {
      msg_send_.payload = {};
      msg_send_.seqno
        = Wrap32::wrap( msg_send_.seqno.unwrap( isn_, next_seqno_ ) + TCPConfig::MAX_PAYLOAD_SIZE, isn_ );
    } else {
      return;
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  return TCPSenderMessage { Wrap32::wrap( next_seqno_, isn_ ), false, {}, false, input_.reader().has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  wnd_size_ = msg.window_size;
  const uint64_t abs_seqno = ( msg.ackno )->unwrap( isn_, next_seqno_ );

  if ( !msg.ackno.has_value() ) {
    if ( msg.window_size == 0 )
      input_.set_error();
    return;
  }
  if ( abs_seqno > next_seqno_ )
    return;

  while ( !outstanding_queue_.empty() ) {
    auto& msg_seg = outstanding_queue_.front();
    if ( ack_seqno_ + msg_seg.sequence_length() <= abs_seqno ) {
      outgoing_bytes_ -= msg_seg.sequence_length();
      ack_seqno_ += msg_seg.sequence_length();
      outstanding_queue_.pop();

      RTO_ = initial_RTO_ms_;
      time_passed_ = 0;
      retransmission_cnt_ = 0;
    } else {
      break;
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  time_passed_ += ms_since_last_tick;

  if ( !outstanding_queue_.empty() && time_passed_ >= RTO_ ) {
    auto iter = outstanding_queue_.front();

    if ( wnd_size_ > 0 ) {
      RTO_ <<= 1;
    }
    transmit( iter );
    time_passed_ = 0;
    ++retransmission_cnt_;
  }
}

// utils funtions
bool TCPSender::window_not_full(uint64_t wnd_size) {
  return wnd_size > outgoing_bytes_;
}