#ifndef _UTIL_H_
#define _UTIL_H_

#include <iostream>
#include <sstream>
#include <string>

// not thread-safe
//
// taken from java:
//   http://developer.classpath.org/doc/java/util/Random-source.html
class fast_random
{
public:
    fast_random(unsigned long seed)
        : seed(0)
    {
        set_seed0(seed);
    }

    inline unsigned long
    next()
    {
        return ((unsigned long)next(32) << 32) + next(32);
    }

    inline uint32_t
    next_u32()
    {
        return next(32);
    }

    inline uint16_t
    next_u16()
    {
        return next(16);
    }

    /** [0.0, 1.0) */
    inline double
    next_uniform()
    {
        return (((unsigned long)next(26) << 27) + next(27)) / (double)(1L << 53);
    }

    inline char
    next_char()
    {
        return next(8) % 256;
    }

    inline char
    next_readable_char()
    {
        static const char readables[] = "0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
        return readables[next(6)];
    }

    inline std::string
    next_string(size_t len)
    {
        std::string s(len, 0);
        for (size_t i = 0; i < len; i++)
            s[i] = next_char();
        return s;
    }

    inline std::string
    next_readable_string(size_t len)
    {
        std::string s(len, 0);
        for (size_t i = 0; i < len; i++)
            s[i] = next_readable_char();
        return s;
    }

    inline unsigned long
    get_seed()
    {
        return seed;
    }

    inline void
    set_seed(unsigned long seed)
    {
        this->seed = seed;
    }

private:
    inline void
    set_seed0(unsigned long seed)
    {
        this->seed = (seed ^ 0x5DEECE66DL) & ((1L << 48) - 1);
    }

    inline unsigned long
    next(unsigned int bits)
    {
        seed = (seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
        return (unsigned long)(seed >> (48 - bits));
    }

    unsigned long seed;
};

static size_t
NumDistrictsPerWarehouse()
{
  return 10;
}

static size_t
NumItems()
{
    return 100000;
}

static int RandomNumber(fast_random &r, int min, int max)
{
    return (int)(r.next_uniform() * (max - min + 1) + min);
}

static int NonUniformRandom(fast_random &r, int A, int C, int min, int max)
{
    return (((RandomNumber(r, 0, A) | RandomNumber(r, min, max)) + C) % (max - min + 1)) + min;
}

static int
GetItemId(fast_random &r)
{
    return NonUniformRandom(r, 8191, 7911, 1, NumItems());
}

static size_t
NumCustomersPerDistrict()
{
    return 3000;
}

static int GetCustomerId(fast_random &r)
{
    return NonUniformRandom(r, 1023, 259, 1, NumCustomersPerDistrict());
}

#endif /* _UTIL_H_ */