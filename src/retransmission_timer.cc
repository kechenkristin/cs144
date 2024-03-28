#include "retransmission_timer.hh"

RetransmissionTimer::RetransmissionTimer( const size_t retx_timeout )
  : state { RetransmissionTimerState ::STOPPED }, _initial_rto { retx_timeout }, _rto { retx_timeout }
{}

bool RetransmissionTimer::timer_expired( const size_t ms_since_last_tick )
{
  //! Only when the timer is running, we add the `_accumulate_time`.
  if ( state == RetransmissionTimerState::RUNNING ) {
    _accumulate_time += ms_since_last_tick;
    //! Check whether the timer has elapsed.
    return _rto <= _accumulate_time;
  }
  return false;
}

void RetransmissionTimer::reset_timer()
{
  _rto = _initial_rto;
  _accumulate_time = 0;
}

void RetransmissionTimer::start_timer()
{
  if ( state == RetransmissionTimerState::STOPPED ) {
    state = RetransmissionTimerState::RUNNING;
    reset_timer();
  }
}

void RetransmissionTimer::stop_timer()
{
  if ( state == RetransmissionTimerState::RUNNING ) {
    state = RetransmissionTimerState::STOPPED;
  }
}

void RetransmissionTimer::handle_expired()
{
  _rto *= 2;
  _accumulate_time = 0;
}