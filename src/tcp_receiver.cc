#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if (message.RST) {
    reassembler_.reader().set_error();
    return;
  }

  if (!_isn.has_value()) {
    if (!message.SYN) return;
    _isn = message.seqno;
    SYN_ = true;
  }

  if (message.FIN) FIN_ = true;

  // seqno -> abs seqno -> stream index
  // checkpoint(64 bit) is the first unassembled byte index
  uint64_t checkpoint = reassembler_.reader().bytes_buffered();
  uint64_t absolute_seqno = message.seqno.unwrap(_isn.value(), checkpoint);
  if (message.SYN) absolute_seqno += 1;
  uint64_t stream_index = absolute_seqno - 1;
  reassembler_.insert(stream_index, message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  if (!_isn.has_value()) return {};
  TCPReceiverMessage ret {};

  // get the RST flag The RST (reset) flag. If set, the stream has suffered an error and the connection should be aborted.
  ret.RST = ( reassembler_.reader().has_error() || reassembler_.writer().has_error() );

  // get the window size
  uint64_t window_size_64 = reassembler_.writer().available_capacity();
  ret.window_size = window_size_64 < UINT16_MAX ? window_size_64 : UINT16_MAX;


  // get the ackno(The index of first unassembled byte, the first byte that the receiver needs from the sender
  // stream index -> abs seqno -> seqno
  // return seqno after the last byte of the stream, the seqno after FIN
  auto offset = 0;
  if (FIN_ && reassembler_.bytes_pending() == 0) offset += 1;
  if (SYN_) {
    offset += 1;
    uint64_t stream_index = reassembler_.reader().bytes_buffered();
    ret.ackno = Wrap32::wrap(stream_index + offset, _isn.value());
  }
  return ret;
}
