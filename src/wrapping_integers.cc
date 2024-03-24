#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 isn )
{
  // Your code here.
    return isn + static_cast<uint32_t>(n);
}

uint64_t Wrap32::unwrap( Wrap32 isn, uint64_t checkpoint ) const
{
  // Your code here.
    Wrap32 c = wrap(checkpoint, isn);
    int32_t offset = raw_value_ - c.raw_value_;
    int64_t ret = checkpoint + offset;
    return ret >= 0 ? ret : ret + (1ul << 32);
}
