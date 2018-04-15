#include <iostream>
#include <algorithm>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : cwnd(10.0),
    debug_( debug ),
    BtlBwFilter{0},
    BtlBwFilterCurrIndex(0),
    RtPropFilter{0},
    RtPropFilterCurrIndex(0),
    latest_sequence_number_sent(0)
{
  for (int i = 0; i < RtPropFilterCapacity; i++) {
     RtPropFilter[i] = 10000000;
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

  latest_sequence_number_sent = max(sequence_number, latest_sequence_number_sent);
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
  RtPropFilter[RtPropFilterCurrIndex] = rtt;
  RtPropFilterCurrIndex = (RtPropFilterCurrIndex + 1) % RtPropFilterCapacity;

  cout << "rtt is " << rtt << endl;

  uint64_t minRtt = 10000000;
  for (int i = 0; i < RtPropFilterCapacity; i++) {
    if (RtPropFilter[i] < minRtt) {
      minRtt = RtPropFilter[i];
    }
  }

  cout << "min rtt is " << minRtt << endl;


  double deliveryRate = ((double)latest_sequence_number_sent - sequence_number_acked)/(rtt);
  cout << "delivery rate is " << deliveryRate << endl;

  BtlBwFilter[BtlBwFilterCurrIndex] = deliveryRate;
  BtlBwFilterCurrIndex = (BtlBwFilterCurrIndex + 1) % BtlBwFilterCapacity;

  double maxBw = 0;
  for (int i = 0; i < BtlBwFilterCapacity; i++) {
    if (BtlBwFilter[i] > maxBw) {
      maxBw = BtlBwFilter[i];
    }
  }

  cout << "max bw is " << maxBw << endl;

  cwnd = 1.25*minRtt*maxBw;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms()
{
  return 500;
}
