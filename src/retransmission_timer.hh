// file: retransmission_timer.hh
#ifndef SPONGE_LIBSPONGE_RETRANSMISSION_TIMER
#define SPONGE_LIBSPONGE_RETRANSMISSION_TIMER

#include <cstddef>

enum class RetransmissionTimerState {
  RUNNING,
  STOPPED,
};

//! A timer keep track each packet's retransmission timeout
class RetransmissionTimer {
private:
  RetransmissionTimerState state;  //! the state of the timer
  size_t _initial_rto;             //! the initial retransmission timeout
  size_t _rto;                     //! current retransmission timeout
  size_t _accumulate_time = 0;     //! the accumulate time

public:
  //! \brief constructor
  RetransmissionTimer(const size_t retx_timeout);

  //! \brief check whether the time is expired
  bool timer_expired(const size_t ms_since_last_tick);

  //! \brief when receiving a valid ack, reset the timer
  void reset_timer();

  //! \brief when the timer is expired we should handle this situation
  void handle_expired();

  //! \brief start the timer
  void start_timer();

  //! \brief stop the timer
  void stop_timer();
};

#endif  // SPONGE_LIBSPONGE_RETRANSMISSION_TIMER