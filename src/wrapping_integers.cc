#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  uint32_t n32 = static_cast<uint32_t>(n);
  return zero_point + n32;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.

  uint64_t remain = ((1ULL << 32) + raw_value_ - zero_point.WrappingInt32()) % (1ULL << 32);
  uint64_t closeVal = ((checkpoint >> 32) << 32) + remain;
  if(closeVal <= checkpoint){
    if(checkpoint - closeVal < (1ULL << 31)){
      return closeVal;
    }else{
      return closeVal + (1ULL << 32);
    }
  }else{
    if(closeVal - checkpoint < (1ULL << 31) || closeVal < (1ULL << 32)){
      return closeVal;
    }else{
      return closeVal - (1ULL << 32);
    }
  }
}
