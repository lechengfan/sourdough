#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : window(9.0),
  min_rtt(100000),
  alpha(0.07),
  beta(0.12),
  last_increase_time(0),
  last_large_decrease_time(0),
  debug_( debug )
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size()
{

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << window << endl;
  }

  return (unsigned int) window;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp,
                                    /* in milliseconds */
				    const bool after_timeout
				    /* datagram was sent because of a timeout */ )
{
  /* Default: take no action */

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << " (timeout = " << after_timeout << ")\n";
  }
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged datagram was sent (sender's clock) */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged datagram was received (receiver's clock)*/
			       const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{
  // uint64_t thresh = 50;
  /* Default: take no action */
  uint64_t actual_rtt = timestamp_ack_received - send_timestamp_acked;
  min_rtt = min(min_rtt, actual_rtt);
  unsigned int rounded_window = (unsigned int) window;
  double expected_rate = (double) rounded_window/min_rtt;
  double actual_rate = (double) rounded_window/actual_rtt;
  double difference = expected_rate - actual_rate;
  // cout << "1/delay is " << 1.0/actual_rtt << endl;
  // cout << "actual rate is  " << actual_rate << endl;
  if (difference < alpha) {
    // window += 2.5/window;
    window += 1.0/window;
    if (difference < alpha/3 &&
      (timestamp_ack_received - last_large_decrease_time) > 300 &&
      (timestamp_ack_received - last_increase_time) > 100) {
      cout << "drastically increase window by " << window/10 << " at time " << timestamp_ack_received << endl;
      window += window/10;
      last_increase_time = timestamp_ack_received;
    }
  }
  else if (difference > beta) {
    window -= 1.0/window;
    if (difference > 1.4*beta && (timestamp_ack_received - last_large_decrease_time) > 100) {
      cout << "drastically reduce by " << window/5 << " at time " << timestamp_ack_received << endl;
      window *= 0.8;
      last_large_decrease_time = timestamp_ack_received;
    }
  }

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms()
{
  return 1000; /* timeout of one second */
}
