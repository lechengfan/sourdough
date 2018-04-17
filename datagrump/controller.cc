#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    cwnd(1.0),
    min_rtt(100000000),
    rtt_estimates{0},
    rtt_estimates_index(0),
    velocity(.5)
{
  for (int i = 0; i < RTT_STANDING_ESTIMATE_WINDOW; i++) {
     rtt_estimates[i] = 10000000;
  }
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size()
{
  unsigned int the_window_size = (unsigned int) cwnd;

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
  }

  cout << "Window size is " << the_window_size << endl;

  return the_window_size;
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
  /* Default: take no action */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }

  uint64_t rtt = timestamp_ack_received - send_timestamp_acked;
  rtt_estimates[rtt_estimates_index] = rtt;
  rtt_estimates_index = (rtt_estimates_index + 1) % RTT_STANDING_ESTIMATE_WINDOW;

  min_rtt = min(rtt, min_rtt);

  uint64_t min_window_rtt = 10000000;
  for (int i = 0; i < RTT_STANDING_ESTIMATE_WINDOW; i++) {
    if (rtt_estimates[i] < min_window_rtt) {
      min_window_rtt = rtt_estimates[i];
    }
  }

  uint64_t queueing_delay_estimate = min_window_rtt - min_rtt;

  double lambda = (1.0)/(DELTA*queueing_delay_estimate);

  if (cwnd/min_window_rtt <= lambda) {
    cwnd += velocity/(DELTA*cwnd);
  } else {
    cwnd -= 2*velocity/(DELTA*cwnd);
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms()
{
  return 1000; /* timeout of one second */
}
