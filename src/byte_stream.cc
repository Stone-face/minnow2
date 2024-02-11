#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  // Your code here.
  
  

  for(uint64_t i = 0; i < data.length(); i++){
    if(byteQueue.size() == capacity_){
      break;
    }
    byteQueue.push(data[i]);
    total_pushed ++;
  }


}

void Writer::close()
{
  // Your code here.
  is_wr_closed_ = true;
}

void Writer::set_error()
{
  // Your code here.
  is_wr_error_ = true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return is_wr_closed_;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - byteQueue.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return total_pushed;
}

string_view Reader::peek() const
{
  // Your code here.
  if(byteQueue.size() == 0){
    return std::string_view("");;
  }

  // unsigned char peekByte = byteQueue.front();
  std::string_view byteStringView(reinterpret_cast<const char*>(&byteQueue.front()), 1);
  return byteStringView;
}

bool Reader::is_finished() const
{
  // Your code here.
  return is_wr_closed_ && byteQueue.size() == 0;
}

bool Reader::has_error() const
{
  // Your code here.
  return is_wr_error_;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  
  for(uint64_t i = 0; i < len; i++){
    if(byteQueue.size() == 0){
      break;
    }
    byteQueue.pop();
    total_poped++;
  }

 
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return byteQueue.size();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return total_poped;
}
