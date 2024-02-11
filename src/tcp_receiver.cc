#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  // Your code here.
  if(message.SYN){
    ISN = message.seqno;  // ??
    isISNSet = true;
  }
  Wrap32 payloadSeqno = message.SYN ? message.seqno + 1 : message.seqno;
  uint64_t absSeqno = payloadSeqno.unwrap(ISN, checkpoint);
  string data = message.payload.release();
  reassembler_.insert(absSeqno - 1, data, message.FIN, reassembler_.writer()); // string_view??
  checkpoint = absSeqno - 1 + data.length() - 1; //  index of the last reassembled byte 
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  std::optional<Wrap32> ack;
  if(isISNSet){
    uint64_t ackNo = inbound_stream.bytes_pushed() + 1;  // +1: convert stream index to absolute index
    //  the first (unreceived)byte the receiver is interested in receiving
    ackNo += inbound_stream.writer().is_closed() ? 1 : 0;
    ack = Wrap32::wrap(ackNo, ISN);
  }

  uint16_t windowSize = reassembler_.writer().available_capacity() > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(inbound_stream.available_capacity());

  TCPReceiverMessage message{
    ack,
    windowSize
  };
  
  return message;
}
