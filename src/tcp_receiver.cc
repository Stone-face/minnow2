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

  if(message.RST) {
    reassembler_.reader().set_error();
  }
  
  Wrap32 payloadSeqno = message.SYN ? message.seqno + 1 : message.seqno;
  uint64_t absSeqno = payloadSeqno.unwrap(ISN, checkpoint);
  string data = message.payload;
  reassembler_.insert(absSeqno - 1, data, message.FIN); // string_view??
  checkpoint = absSeqno - 1 + data.length() - 1; //  index of the last reassembled byte 
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  std::optional<Wrap32> ack;
  if(isISNSet){
    uint64_t ackNo = reassembler_.writer().bytes_pushed() + 1;  // +1: convert stream index to absolute index
    //  the first (unreceived)byte the receiver is interested in receiving
    ackNo += reassembler_.writer().is_closed() ? 1 : 0;
    ack = Wrap32::wrap(ackNo, ISN);
  }

  uint16_t windowSize = reassembler_.writer().available_capacity() > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(reassembler_.writer().available_capacity());
  // error = reassembler_.writer().has_error();
  TCPReceiverMessage message{
    ack,
    windowSize,
    reassembler_.writer().has_error() ||  reassembler_.reader().has_error()
  };
  
  return message;
}
