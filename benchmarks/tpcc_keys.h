#pragma once

#include "../record/encoder.h"
#include "../record/inline_str.h"
#include "../macros.h"
#include "../third-party/lz4/xxhash.h"

struct __attribute__((packed)) customer_key {
    inline customer_key() {
    }
    inline customer_key(int32_t w_id, int32_t d_id, int32_t c_id)
        : c_w_id(w_id), c_d_id(d_id), c_id(c_id) {
    }
    inline bool operator==(const customer_key& other) const {
        return c_w_id == other.c_w_id && c_d_id == other.c_d_id && c_id == other.c_id;
    }
    inline bool operator!=(const customer_key& other) const {
        return !(*this == other);
    }
    int32_t c_w_id;
    int32_t c_d_id;
    int32_t c_id;
};

namespace std
{
  template<>
  struct hash<customer_key>
  {
     typedef customer_key argument_type;
     typedef std::size_t result_type;

     result_type operator()(argument_type const& s) const {
       //return XXH32(&s, sizeof(argument_type), 11);
       return ((size_t)s.c_id) | (((size_t)s.c_d_id & ((1 << 16) - 1)) << 32) | (((size_t)s.c_w_id & ((1 << 16) -1)) << 48);
     }
  };
}

struct __attribute__((packed)) history_key {
    inline history_key() {
    }
    inline history_key(int32_t h_c_id, int32_t h_c_d_id, int32_t h_c_w_id,
			int32_t h_d_id, int32_t h_w_id, uint32_t h_date)
        : h_c_id(h_c_id), h_c_d_id(h_c_d_id), h_c_w_id(h_c_w_id), h_d_id(h_d_id), h_w_id(h_w_id), h_date(h_date) {
    }
    inline bool operator==(const history_key& other) const {
        return h_c_id == other.h_c_id && h_c_d_id == other.h_c_d_id && h_c_w_id == other.h_c_w_id && h_d_id == other.h_d_id && h_w_id == other.h_w_id && h_date == other.h_date;
    }
    inline bool operator!=(const history_key& other) const {
        return !(*this == other);
    }
    int32_t h_c_id;
    int32_t h_c_d_id;
    int32_t h_c_w_id;
    int32_t h_d_id;
    int32_t h_w_id;
    uint32_t h_date;
};

namespace std
{
  template<>
  struct hash<history_key>
  {
     typedef history_key argument_type;
     typedef std::size_t result_type;

     result_type operator()(argument_type const& s) const {
       return XXH32(&s, sizeof(argument_type), 11);
     }
  };
}


struct __attribute__((packed)) district_key {
    inline district_key() {
    }
    inline district_key(int32_t w_id, int32_t d_id)
        : d_w_id(w_id), d_id(d_id) {
    }
    inline bool operator==(const district_key& other) const {
        return d_w_id == other.d_w_id && d_id == other.d_id;
    }
    inline bool operator!=(const district_key& other) const {
        return !(*this == other);
    }
    int32_t d_w_id;
    int32_t d_id;
};

namespace std
{
  template<>
  struct hash<district_key>
  {
     typedef district_key argument_type;
     typedef std::size_t result_type;

     result_type operator()(argument_type const& s) const {
       //return XXH32(&s, sizeof(argument_type), 11);
       return ((size_t)s.d_id) | ((size_t)s.d_w_id  << 32) ;
     }
  };
}


struct __attribute__((packed)) item_key {
    inline item_key() {
    }
    inline item_key(int32_t i_id)
        : i_id(i_id) {
    }
    inline bool operator==(const item_key& other) const {
        return i_id == other.i_id;
    }
    inline bool operator!=(const item_key& other) const {
        return !(*this == other);
    }
    int32_t i_id;
};

struct __attribute__((packed)) oorder_key {
    inline oorder_key() {
    }
    inline oorder_key(int32_t w_id, int32_t d_id, int32_t o_id)
        : o_w_id(w_id), o_d_id(d_id), o_id(o_id) {
    }
    inline bool operator==(const oorder_key& other) const {
        return o_w_id == other.o_w_id && o_d_id == other.o_d_id && o_id == other.o_id;
    }
    inline bool operator!=(const oorder_key& other) const {
        return !(*this == other);
    }
    int32_t o_w_id;
    int32_t o_d_id;
    int32_t o_id;
};

namespace std
{
  template<>
  struct hash<oorder_key>
  {
     typedef oorder_key argument_type;
     typedef std::size_t result_type;

     result_type operator()(argument_type const& s) const {
       //return XXH32(&s, sizeof(argument_type), 11);
       return ((size_t)s.o_id) | (((size_t)s.o_d_id & ((1 << 16) - 1)) << 32) | (((size_t)s.o_w_id & ((1 << 16) -1)) << 48);
     }
  };
}

struct __attribute__((packed)) stock_key {
    inline stock_key() {
    }
    inline stock_key(int32_t w_id, int32_t i_id)
        : s_w_id(w_id), s_i_id(i_id) {
    }
    inline bool operator==(const stock_key& other) const {
        return s_w_id == other.s_w_id && s_i_id == other.s_i_id;
    }
    inline bool operator!=(const stock_key& other) const {
        return !(*this == other);
    }
    int32_t s_w_id;
    int32_t s_i_id;
};

namespace std
{
  template<>
  struct hash<stock_key>
  {
     typedef stock_key argument_type;
     typedef std::size_t result_type;

     result_type operator()(argument_type const& s) const {
       //return XXH32(&s, sizeof(argument_type), 11);
       return ((size_t)s.s_i_id) | ((size_t)s.s_w_id  << 32) ;
     }
  };
}


struct __attribute__((packed)) warehouse_key {
    inline warehouse_key() {
    }
    inline warehouse_key(int32_t w_id)
        : w_id(w_id) {
    }
    inline bool operator==(const warehouse_key& other) const {
        return w_id == other.w_id;
    }
    inline bool operator!=(const warehouse_key& other) const {
        return !(*this == other);
    }
    int32_t w_id;
};


