#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return sequenceNumbersFli;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return consecutiveRetrans;
}

void TCPSender::push( const TransmitFunction& transmit )
{
   // Your code here.
  // bs.reader() = outbound_stream;
  while(true){
    uint64_t equalWindowSize = max(1UL, static_cast<uint64_t>(window_size));
    if(static_cast<uint64_t>(equalWindowSize) <= sequenceNumbersFli){
      //cout << "return empty" << endl;
      return;
    }
    
    bool SYN = seqno == isn_;
    //bool FIN = outbound_stream.is_finished();
    //cout << "SYN: " << SYN <<  "stream_bytes: "  << outbound_stream.bytes_buffered() << endl;
    // uint64_t equalWindowSize = max(1UL, static_cast<uint64_t>(window_size));
    uint64_t availableSent =  equalWindowSize - sequenceNumbersFli;
    uint64_t sendLen = min(input_.reader().bytes_buffered(), availableSent - SYN);
    sendLen = min(sendLen, TCPConfig::MAX_PAYLOAD_SIZE);
    // if(window_size > 0){
    //   window_size -= (sendLen + SYN);
    // }
    
    string data;
    //cout << "reader address: " << &outbound_stream << endl;
    //cout << "reader outer size before: " << outbound_stream.bytes_buffered() << endl;
    read( input_.reader(), sendLen, data );
    //cout << "reader outer size: " << outbound_stream.bytes_buffered() << endl;
    
    bool FIN = input_.reader().is_finished() && availableSent - SYN - sendLen > 0 && !FINSent;
    FINSent |= FIN;
    //cout << "FIN: " << FIN << " window_size: " << window_size << endl;
    // window_size -= FIN;

    if(data.length() == 0 && !SYN && !FIN){
      //cout << "return empty" << endl;
      return;
    }

    TCPSenderMessage message = {
      seqno,
      SYN,
      data,
      FIN
    };

    seqno = seqno + message.sequence_length();
    sequenceNumbersFli += message.sequence_length();
    checkpoint += message.sequence_length();

    outstandingSeg.push_back(message);
    //outstandingSeg.sort(compareSeg);

    if(!isTimerRunning){
      isTimerRunning = true;
      timer = cur_RTO_ms;
    }

    transmit(message);
  }
 
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage message;
 
  bool SYN = false;
  bool FIN = false;
 
  string data = "";
 
  
  message = {
    seqno,
    SYN,
    data,
    FIN
  };

  return message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  window_size = msg.window_size;
  
  bool isNewData = false;
  if(msg.ackno.has_value()){
    
    // ignore impossible ackno
    if(msg.ackno.value().unwrap(isn_, checkpoint) > seqno.unwrap(isn_, checkpoint)){
      return;
    }

    
    for (auto it = outstandingSeg.begin(); it != outstandingSeg.end(); /* no increment here */) {
      if (msg.ackno.value().unwrap(isn_, checkpoint) >= it->seqno.unwrap(isn_, checkpoint) + it->sequence_length()) {
          sequenceNumbersFli -= it->sequence_length();
          it = outstandingSeg.erase(it);
          isNewData = true;
      } else {
          ++it;
      }
    }
  }
  
  // outstandingSeg.remove_if([ackno](TCPSenderMessage& messgae) { return ackno > messgae.seqno.WrappingInt32() + messgae.sequence_length();})
  if(outstandingSeg.empty()){
    isTimerRunning = false;
  }

  if(isNewData){
    cur_RTO_ms = initial_RTO_ms_;
    if(!outstandingSeg.empty()){
      isTimerRunning = true;
      timer = cur_RTO_ms;
    }
    consecutiveRetrans = 0;
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  if(timer < ms_since_last_tick){
    timer = 0;
  }else{
    timer -= ms_since_last_tick;
  }

  //cout << "timer: " << timer << " isRunning: " << isTimerRunning << endl;
  if(timer == 0){
    if(isTimerRunning){
      TCPSenderMessage message = outstandingSeg.front();
      transmit(message);
      //cout << "resent message seqno: " << message.seqno.WrappingInt32() << endl;
      if(window_size > 0){
        consecutiveRetrans++;
        cur_RTO_ms *= 2;
      }

      timer = cur_RTO_ms;
    }
  }
}
