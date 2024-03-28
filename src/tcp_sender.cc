#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  // TODO: fix bug here
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
  Reader& bytes_reader = _input.reader();
  _fin_flag |= bytes_reader.is_finished(); // 允许刚建立就关闭流
  if ( _sent_fin )
    return; // 已经结束了，什么都不该再发

  const size_t window_size = _receive_window_size == 0 ? 1 : _receive_window_size;
  // 不断组装并发送分组数据报，直到达到窗口上限或没数据读，并且在 FIN 发出后不再尝试组装报文
  for ( string payload {}; window_not_full(window_size) && !_sent_fin; payload.clear() ) {
    string_view bytes_view = bytes_reader.peek();
    // 流为空且不需要发出 FIN，并且已经发出了连接请求，则跳过报文发送
    if ( _sent_syn && bytes_view.empty() && !_fin_flag )
      break;

    // 从流中读取数据并组装报文，直到达到报文长度限制或窗口上限
    while ( payload.size() + sequence_numbers_in_flight() + ( !_sent_syn ) < window_size
            && payload.size() < TCPConfig::MAX_PAYLOAD_SIZE ) { // 负载上限
      // 没数据读了，或者流关闭了
      if ( bytes_view.empty() || _fin_flag )
        break;

      // 如果当前读取的字节分组长度超过限制
      if ( const uint64_t available_size
           = min( TCPConfig::MAX_PAYLOAD_SIZE - payload.size(),
                  window_size - ( payload.size() + sequence_numbers_in_flight() + ( !_sent_syn ) ) );
           bytes_view.size() > available_size ) // 那么这个分组需要被截断
        bytes_view.remove_suffix( bytes_view.size() - available_size );

      payload.append( bytes_view );
      bytes_reader.pop( bytes_view.size() );
      // 从流中弹出字符后要检查流是否关闭
      _fin_flag |= bytes_reader.is_finished();
      bytes_view = bytes_reader.peek();
    }
    TCPSenderMessage msg =  make_message( _next_abs_seqno, move( payload ), _sent_syn ? _syn_flag : true, _fin_flag );
    _outstanding_msgs.push_back(msg);

    // 因为 FIN 会在下一个 if-else 中动态改变，所以先计算报文长度调整余量
    const size_t margin = _sent_syn ? _syn_flag : 0;

    // 检查 FIN 字节能否在此次报文传送中发送出去
    if ( _fin_flag && ( msg.sequence_length() - margin ) + sequence_numbers_in_flight() > window_size )
      msg.FIN = false;    // 如果窗口大小不足以容纳 FIN，则不发送
    else if ( _fin_flag ) // 否则发送
      _sent_fin = true;

    // 最后再计算真实的报文长度
    const size_t correct_length = msg.sequence_length() - margin;

    _next_abs_seqno += correct_length;
    _num_bytes_in_flight += correct_length;
    _sent_syn = _sent_syn ? _sent_syn : true;
    transmit( msg );
    if ( correct_length != 0 )
      _retransmission_timer.start_timer();
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
    return make_message(_next_abs_seqno, {}, false, false);
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
    _receive_window_size = msg.window_size;
    if (!msg.ackno.has_value() && msg.window_size == 0) {
      _input.set_error();
      return;
    }

    // STEP1: check the boundary for ackno
    uint64_t expecting_seqno = msg.ackno->unwrap( _isn, _next_abs_seqno);
    // (received_ackno, next_seqno)
    if ( expecting_seqno > _next_abs_seqno)
      return;

    bool is_ack_update = false;

    while ( !_outstanding_msgs.empty() ) {
      TCPSenderMessage buffered_msg = _outstanding_msgs.front();
      uint64_t final_seqno = _receive_ack + buffered_msg.sequence_length() - buffered_msg.SYN;
      if (expecting_seqno <= _receive_ack || expecting_seqno < final_seqno)
      break;

      is_ack_update = true;
      uint64_t offset = buffered_msg.sequence_length() - _syn_flag;
      _num_bytes_in_flight -= offset;
      _receive_ack += offset;
      _syn_flag = _sent_syn ? _syn_flag : expecting_seqno <= _next_abs_seqno;
      _outstanding_msgs.pop_front();
    }


        if (is_ack_update) {
          _consecutive_retransmissions = 0;
          if (_outstanding_msgs.empty()) _retransmission_timer.stop_timer();
          else
            _retransmission_timer.reset_timer();
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

TCPSenderMessage TCPSender::make_message( uint64_t seqno, string payload, bool SYN, bool FIN ) const
{
    return { .seqno = Wrap32::wrap( seqno, _isn ),
             .SYN = SYN,
             .payload = move( payload ),
             .FIN = FIN,
             .RST = _input.reader().has_error() };
}