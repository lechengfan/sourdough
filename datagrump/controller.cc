#include <iostream>
#include <cmath>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : cwnd(1.0), 
  estimated_rtt(-1),
  rtt_dev(-1),
  debug_( debug )
{
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size()
{
  unsigned int the_window_size = (unsigned int) cwnd;
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
  }

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
  if (after_timeout) {
    cwnd /= 2;
    if (cwnd < 1)
      cwnd = 1;
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
  cwnd += 1.0/cwnd;
  double sample_rtt = (double) (timestamp_ack_received - send_timestamp_acked);
  if (estimated_rtt == -1) {
    estimated_rtt = sample_rtt;
    rtt_dev = 0;
  }
  else {
    rtt_dev = 0.75*rtt_dev + 0.25*abs(sample_rtt - estimated_rtt);
    estimated_rtt = 0.875 * estimated_rtt + 0.125 * sample_rtt;
  }
  cout << "estimated_rtt " << estimated_rtt << endl;
  cout << "deviation " << rtt_dev << endl;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms()
{
  if (estimated_rtt == -1)
    return 25;
  else {
    return estimated_rtt + 4*rtt_dev;
  }
}
