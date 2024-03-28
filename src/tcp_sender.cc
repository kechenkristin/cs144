#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
    return _next_abs_seqno - _receive_ack;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return _consecutive_retransmissions;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  //
  // special case1: if we already sent the FIN
    if ( _end )
      return;

    // get the window_size
    // special case2: if the _received_window set to be 0
    int64_t window_size = _receive_window_size == 0 ? 1 : _receive_window_size;

    TCPSenderMessage tcpSenderMessage{};
    // read from the input stream to build TCPSendMessage
    // special case3: TCP Connection initializing
    if ( _next_abs_seqno == 0) {
      tcpSenderMessage.SYN = true;
      // TODO: debug here
      tcpSenderMessage.seqno = isn_ + _next_abs_seqno;
      _next_abs_seqno += 1;
    }
    else {
      // calcalate the string length
      uint64_t length = min(min(window_size - sequence_numbers_in_flight(), input_.reader().bytes_buffered()), TCPConfig::MAX_PAYLOAD_SIZE);

      // construct the payload_str
      string payload_str{};

      for (uint64_t i = 0; i < length; i++) {
        payload_str.append(input_.reader().peek());
      }

      tcpSenderMessage.payload = payload_str;

      tcpSenderMessage.seqno = isn_ + _next_abs_seqno;
      _next_abs_seqno += length;

      // special case4: FIN set to be false
      if (input_.writer().is_closed() && window_not_full(window_size)) {
        tcpSenderMessage.FIN = true;
        _end = true;
        _next_abs_seqno++;
      }

      // Do nothing: either receiver does not have space or bytestream does not have input yet
      if (length == 0 && !_end)
        return;
    }

    // STEP2: push the msg to the queue
    transmit(tcpSenderMessage);
    _outstanding_msgs.push_back(tcpSenderMessage);
    // start the timer
    _retransmission_timer.start_timer();

    //  // if we have engough window size, we can send out the segment again
    if ( window_not_full(window_size))
      push(transmit);
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
    TCPSenderMessage tcpSenderMessage{};
    tcpSenderMessage.seqno = isn_ + _next_abs_seqno;
    return tcpSenderMessage;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
    // STEP1: check the boundary for ackno
    uint64_t abs_ackno = msg.ackno->unwrap(isn_, _next_abs_seqno);
    // (received_ackno, next_seqno)
    if (abs_ackno < _receive_ack || abs_ackno > _next_abs_seqno)
      return;

    _receive_window_size = msg.window_size;
    bool is_ack_update = false;

    // STEP2: erase TCPSenderMessage in outstanding
    for (auto current_sender_msg = _outstanding_msgs.begin(); current_sender_msg != _outstanding_msgs.end();) {
      uint64_t abs_seqno = current_sender_msg->seqno.unwrap(isn_, _next_abs_seqno);

      uint64_t ackno_offset = abs_seqno + current_sender_msg->sequence_length();
      if (abs_ackno >= ackno_offset) {
        _receive_ack = ackno_offset;
        current_sender_msg = _outstanding_msgs.erase(current_sender_msg);
        is_ack_update = true;
      } else {
        current_sender_msg++;
      }
    }

    // When there is no outstanding segments, we should stop the timer
    if (_outstanding_msgs.empty()) _retransmission_timer.stop_timer();

    // _retransmission_timer 记录的是在RTO时间内有没有收到合法的ACK,如果没有收到合法的ACK，那么就需要重传

    // When the receiver gives the sender an ackno that acknowledges
    // the successful receipt of new data
    if (is_ack_update) {
      _retransmission_timer.reset_timer();
      _consecutive_retransmissions = 0;
    }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
    _consecutive_retransmissions++;
    if (_retransmission_timer.timer_expired(ms_since_last_tick)) {
      transmit(_outstanding_msgs.front());
      if (_receive_window_size == 0)
        _retransmission_timer.reset_timer();
      else
        _retransmission_timer.handle_expired();
    }
}
