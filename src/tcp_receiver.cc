#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if ( message.RST ) {
    reassembler_.reader().set_error();
    return;
  }

  if ( message.SYN ) {
    _isn = message.seqno;
    _SYN = true;
    message.seqno = message.seqno + 1;
  }

  if ( !_SYN )
    return;

  if ( message.FIN )
    _FIN = true;

  // seqno -> abs seqno -> stream index
  // checkpoint(64 bit) is the first unassembled byte index
  uint64_t checkpoint = reassembler_.first_unassembled_index();
  uint64_t absolute_seqno = message.seqno.unwrap( _isn.value(), checkpoint );

  uint64_t stream_index = absolute_seqno - 1;

  reassembler_.insert( stream_index, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage ret {};

  // get the RST flag The RST (reset) flag. If set, the stream has suffered an error and the connection should be
  // aborted.
  ret.RST = reassembler_.reader().has_error();

  // get the window size
  uint64_t window_size_64 = reassembler_.writer().available_capacity();
  ret.window_size = window_size_64 < UINT16_MAX ? window_size_64 : UINT16_MAX;

  // get the ackno(The index of first unassembled byte, the first byte that the receiver needs from the sender
  // stream index -> abs seqno -> seqno
  // return seqno after the last byte of the stream, the seqno after FIN
  auto offset = 0;
  if ( _FIN && reassembler_.bytes_pending() == 0 )
    offset += 1;
  // TODO: might be a bug here
  if ( _SYN ) {
    offset += 1;
    uint64_t stream_index = reassembler_.first_unassembled_index();
    ret.ackno = Wrap32::wrap( stream_index + offset, _isn.value() );
  }
  return ret;
}
