#pragma once

#include <list>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include<iostream>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <memory>

#include "base/all.hpp"


namespace rrr {

#ifdef RPC_STATISTICS
void stat_marshal_in(int fd, const void* buf, size_t nbytes, ssize_t ret);
void stat_marshal_out(int fd, const void* buf, size_t nbytes, ssize_t ret);
#endif // RPC_STATISTICS

// not thread safe, for better performance
class Marshal;

class Marshallable {
 public:
  int32_t kind_{0};
  bool bypass_to_socket_ = false;
  size_t written_to_socket = 0;
//  int32_t __debug_{10};
  Marshallable() = delete;
  explicit Marshallable(int32_t k): kind_(k) {};
  virtual ~Marshallable() {
//    if (__debug_ != 10) {
//      verify(0);
//    }
//    __debug_ = 30;
//    Log_debug("destruct marshallable.");
  };
  virtual Marshal& ToMarshal(Marshal& m) const;
  virtual Marshal& FromMarshal(Marshal& m);
  virtual size_t EntitySize() {
    verify(0);
    return 0;
  }
  virtual size_t WriteToFd(int fd, size_t written_to_socket) {
    verify(0);
    return 0;
  }

  // virtual size_t need_to_write(){
  //   return EntitySize() ; - written_to_socket;
  // }

  // virtual void reset_write_offsets(){
  //    written_to_socket = 0;
  // }
};

class MarshallDeputy {
  public:
    typedef std::unordered_map<int32_t, std::function<Marshallable*()>> MarContainer;
    static MarContainer& Initializers();
    static int RegInitializer(int32_t, std::function<Marshallable*()>);
    static std::function<Marshallable*()> GetInitializer(int32_t);

  public:
    bool bypass_to_socket_ = false;
    // size_t written_to_socket = 0;
    std::shared_ptr<rrr::Marshallable> sp_data_{nullptr};
    int32_t kind_{0};
    enum Kind {
      UNKNOWN=0,
      EMPTY_GRAPH=1,
      RCC_GRAPH=2,
      CONTAINER_CMD=3,
      CMD_TPC_PREPARE=4,
      CMD_TPC_COMMIT=5,
      CMD_VEC_PIECE=6,
      CMD_BLK_PXS=7,
      CMD_BLK_PREP_PXS=8,
      CMD_HRTBT_PXS=9,
      CMD_SYNCREQ_PXS=10,
      CMD_SYNCRESP_PXS=11,
      CMD_SYNCNOOP_PXS=12,
      CMD_PREP_PXS=13
    };
    /**
     * This should be called by the rpc layer.
     */
    MarshallDeputy() : kind_(UNKNOWN){}
    /**
     * This should be called by inherited class as instructor.
     * @param kind
     */
    explicit MarshallDeputy(std::shared_ptr<rrr::Marshallable> m): sp_data_(std::move(m)) {
      kind_ = sp_data_->kind_;
      if(sp_data_.get()->bypass_to_socket_){
        bypass_to_socket_ = true;
      }
      //written_to_socket = 0;
    }

    // virtual void reset_write_offsets(){
    //   written_to_socket = 0;
    //   sp_data_->reset_write_offsets();
    // }

    rrr::Marshal& CreateActualObjectFrom(rrr::Marshal& m);
    void SetMarshallable(std::shared_ptr<rrr::Marshallable> m) {
      verify(sp_data_ == nullptr);
      sp_data_ = m;
      kind_ = m->kind_;
    }

    virtual size_t EntitySize() const {
      return sizeof(int32_t) + sp_data_.get()->EntitySize();
    }

    size_t track_write_2(int fd, const void* p, size_t len, size_t offset){
      const char* x = (const char*)p;
      size_t sz = ::write(fd, x + offset, len - offset);
      if(sz > len - offset || sz <= 0){
         return 0;
      }
      return sz;
    }

    // virtual size_t need_to_write(){
    //   // for marshalldeputy we only write headers. The rest is handled by Marshallable
    //   return EntitySize() - written_to_socket;
    // }

    virtual size_t WriteToFd(int fd, int written_to_socket) {
        size_t sz = 0, prev = written_to_socket;
        if(written_to_socket < sizeof(kind_)){
          sz = track_write_2(fd, &kind_, sizeof(kind_), written_to_socket);
          //Log_info("Writing the kind of MarshallDeputy %d %d", sz, written_to_socket);
          written_to_socket += sz;
          if(written_to_socket < sizeof(kind_))return sz;
        }
        //Log_info("Written bytes of ghost chunk 1 %d %d %d", sz, kind_, written_to_socket);
        // sp_data_.get()->reset_write_offset();
        sz = sp_data_.get()->WriteToFd(fd, written_to_socket - sizeof(kind_));
	      //std::cout << sz << std::endl;
        //Log_info("Written bytes of ghost chunk 2 %d %d", sz, kind_);
        written_to_socket += sz;
        //Log_info("Written bytes of ghost chunk 3 %d %d %d", written_to_socket, kind_, EntitySize());
        //Log_info("Written bytes of ghost chunk 2 %d %d", written_to_socket, kind_);
        return written_to_socket - prev;
    }

    ~MarshallDeputy() = default;
};

class Marshal: public NoCopy {
  struct raw_bytes: public RefCounted {
    char *ptr = nullptr;
    size_t size = 0;
    static const size_t min_size;
    MarshallDeputy marshallable_entity;
    bool shared_data = false;
    size_t written_to_socket = 0;

    raw_bytes(size_t sz = min_size) {
      size = std::max(sz, min_size);
      ptr = new char[size];
    }
    raw_bytes(const void *p, size_t n) {
      size = std::max(n, min_size);
      ptr = new char[size];
      memcpy(ptr, p, n);
    }

    raw_bytes(MarshallDeputy md, size_t sz){
      marshallable_entity = md;
      size = sz;
      shared_data = true;
      //Log_info("Creating a ghost chunk here of size %d of kind %d", sz, md.kind_);
    }

    size_t resize_to(size_t new_sz){
      size = std::min(size, new_sz);
      //char *x = new char[size];
      //memcpy(x, ptr, size);
      //delete[] ptr;
      //ptr = x;
      return size;
    }

    raw_bytes(const raw_bytes &) = delete;
    raw_bytes &operator=(const raw_bytes &) = delete;
    ~raw_bytes() { if(ptr)delete[] ptr; }
  };

  struct chunk: public NoCopy {

   private:

    chunk(raw_bytes *dt, size_t rd_idx, size_t wr_idx)
        : data((raw_bytes *) dt->ref_copy()), read_idx(rd_idx),
          write_idx(wr_idx), next(nullptr) {
      assert(write_idx <= data->size);
      assert(read_idx <= write_idx);
    }

   public:

    raw_bytes *data;
    size_t read_idx;
    size_t write_idx;
    chunk *next;

    chunk() : data(new raw_bytes), read_idx(0), write_idx(0), next(nullptr) { }
    chunk(MarshallDeputy md, size_t sz) : data(new raw_bytes(md, sz)), read_idx(0),
                                           write_idx(sz), next(nullptr){}

    chunk(size_t sz) : data(new raw_bytes(sz)), read_idx(0), write_idx(0), next(nullptr){}

    chunk(const void *p, size_t n)
        : data(new raw_bytes(p, n)), read_idx(0),
          write_idx(n), next(nullptr) { }
    chunk(const chunk&) = delete;
    chunk& operator=(const chunk&) = delete;
    ~chunk() { data->release(); }

    // NOTE: This function is only intended for Marshal::read_from_marshal.
    chunk *shared_copy() const {
      return new chunk(data, read_idx, write_idx);
    }

    size_t resize_to_current() {
      verify(data->shared_data == false);
      size_t sz = data->resize_to(write_idx);
      verify(data->size == write_idx);
      return sz;
    }

    size_t content_size() const {
      assert(write_idx <= data->size);
      assert(read_idx <= write_idx);
      return write_idx - read_idx;
    }

    char *set_bookmark() {
      assert(write_idx <= data->size);
      assert(read_idx <= write_idx);

      char *p = &data->ptr[write_idx];
      write_idx++;

      assert(write_idx <= data->size);
      assert(read_idx <= write_idx);
      return p;
    }

    size_t write(const void *p, size_t n) {
      assert(write_idx <= data->size);
      assert(read_idx <= write_idx);

      size_t n_write = std::min(n, data->size - write_idx);
      if (n_write > 0) {
        memcpy(data->ptr + write_idx, p, n_write);
      }
      write_idx += n_write;

      assert(write_idx <= data->size);
      assert(read_idx <= write_idx);
      return n_write;
    }

    size_t read(void *p, size_t n) {
      assert(write_idx <= data->size);
      assert(read_idx <= write_idx);

      size_t n_read = std::min(n, write_idx - read_idx);
      if (n_read > 0) {
        memcpy(p, data->ptr + read_idx, n_read);
      }
      read_idx += n_read;

      assert(write_idx <= data->size);
      assert(read_idx <= write_idx);
      return n_read;
    }

    bool is_shared_data_chunk(){
      return data->shared_data;
    }

    size_t peek(void *p, size_t n) const {
      assert(write_idx <= data->size);
      assert(read_idx <= write_idx);
      size_t n_peek = std::min(n, write_idx - read_idx);      
      if (n_peek > 0) {
        memcpy(p, data->ptr + read_idx, n_peek);
      }

      return n_peek;
    }

    size_t discard(size_t n) {
      assert(write_idx <= data->size);
      assert(read_idx <= write_idx);

      size_t n_discard = std::min(n, write_idx - read_idx);
      read_idx += n_discard;

      assert(write_idx <= data->size);
      assert(read_idx <= write_idx);
      return n_discard;
    }

    int write_to_fd(int fd) {
      assert(write_idx <= data->size);
      int cnt;
      if(data->shared_data){
        cnt = data->marshallable_entity.WriteToFd(fd, data->written_to_socket);
        data->written_to_socket += cnt;
	//Log_info("wrote %d bytes of ghost %d", cnt, fd);
      }
      else{
        cnt = ::write(fd, data->ptr + read_idx, write_idx - read_idx);
	//Log_info("wrote %d bytes of normal %d", cnt, fd);
      }
#ifdef RPC_STATISTICS
      if(!data->shared_data)stat_marshal_out(fd, data->ptr + write_idx, data->size - write_idx, cnt);
      else{
        Log_debug("Missed RPC stats, shared data used in raw_bytes");
      }
#endif // RPC_STATISTICS
      //if(cnt == -1)verify(0);
      if (cnt > 0) {
        read_idx += cnt;
      }

      assert(write_idx <= data->size);
      return cnt;
    }

    int read_from_fd(int fd, size_t bytes = -1) {
      if(bytes == -1)bytes = data->size - write_idx;
      assert(write_idx <= data->size);
      assert(read_idx <= write_idx);

      int cnt = 0;
      if (write_idx < data->size) {
        cnt = ::read(fd, data->ptr + write_idx, bytes);

#ifdef RPC_STATISTICS
        stat_marshal_in(fd, data->ptr + write_idx, bytes, cnt);
#endif // RPC_STATISTICS

        if (cnt > 0) {
          write_idx += cnt;
        }
      }

      assert(write_idx <= data->size);
      assert(read_idx <= write_idx);
      return cnt;
    }

    // check if it is not possible to write to the chunk anymore.
    bool fully_written() const {
      assert(write_idx <= data->size);
      assert(read_idx <= write_idx);
      return write_idx == data->size;
    }

    // check if it is not possible to read any data even if retry later
    bool fully_read() const {
      assert(write_idx <= data->size);
      assert(read_idx <= write_idx);
      //Log_info("fully read %d %d", read_idx, data->size);
      return read_idx == data->size;
    }

    void reset() {
      read_idx = write_idx = 0;
    }
  };

  chunk *head_;
  chunk *tail_;
  i32 write_cnt_;
  size_t content_size_;

  // for debugging purpose
  size_t content_size_slow() const;

 public:

  struct bookmark: public NoCopy {
    size_t size;
    char **ptr;

    ~bookmark() {
      delete[] ptr;
    }
  };

  Marshal()
      : head_(nullptr), tail_(nullptr), write_cnt_(0), content_size_(0) { }
  Marshal(const Marshal &) = delete;
  Marshal &operator=(const Marshal &) = delete;
  ~Marshal();

  void init_block_read(size_t block_size){
    head_ = tail_ = new chunk(block_size);
  }

  bool empty() const {
    assert(content_size_ == content_size_slow());
    return content_size_ == 0;
  }
  size_t content_size() const {
    assert(content_size_ == content_size_slow());
    return content_size_;
  }

  size_t write(const void *p, size_t n);
  size_t read(void *p, size_t n);
  size_t peek(void *p, size_t n) const;

  size_t read_from_fd(int fd);

  size_t chnk_read_from_fd(int fd, size_t bytes);

  size_t read_reuse_chnk(Marshal& m, size_t nbytes);

  size_t read_chnk(void* p, size_t n);

  // NOTE: This function is only used *internally* to chop a slice of marshal object.
  // Use case 1: In C++ server io thread, when a compelete packet is received, read it off
  //             into a Marshal object and hand over to worker threads.
  // Use case 2: In Python extension, buffer message in Marshal object, and send to network.
  size_t read_from_marshal(Marshal &m, size_t n);

  size_t write_to_fd(int fd);

  void reset(){
    head_->reset();
    content_size_ = 0;
    write_cnt_ = 0;
  }

  bookmark *set_bookmark(size_t n);
  void write_bookmark(bookmark *bm, const void *p) {
    const char *pc = (const char *) p;
    assert(bm != nullptr && bm->ptr != nullptr && p != nullptr);
    for (size_t i = 0; i < bm->size; i++) {
      *(bm->ptr[i]) = pc[i];
    }
  }

  i32 get_and_reset_write_cnt() {
    i32 cnt = write_cnt_;
    write_cnt_ = 0;
    return cnt;
  }

  size_t bypass_copying(rrr::MarshallDeputy, size_t);
};

inline rrr::Marshal &operator<<(rrr::Marshal &m, const rrr::i8 &v) {
  verify(m.write(&v, sizeof(v)) == sizeof(v));
  return m;
}

inline rrr::Marshal &operator<<(rrr::Marshal &m, const rrr::i16 &v) {
  verify(m.write(&v, sizeof(v)) == sizeof(v));
  return m;
}

inline rrr::Marshal &operator<<(rrr::Marshal &m, const rrr::i32 &v) {
  verify(m.write(&v, sizeof(v)) == sizeof(v));
  return m;
}

inline rrr::Marshal &operator<<(rrr::Marshal &m, const rrr::i64 &v) {
  verify(m.write(&v, sizeof(v)) == sizeof(v));
  return m;
}

inline rrr::Marshal &operator<<(rrr::Marshal &m, const rrr::v32 &v) {
  char buf[5];
  size_t bsize = rrr::SparseInt::dump(v.get(), buf);
  verify(m.write(buf, bsize) == bsize);
  return m;
}

inline rrr::Marshal &operator<<(rrr::Marshal &m, const rrr::v64 &v) {
  char buf[9];
  size_t bsize = rrr::SparseInt::dump(v.get(), buf);
  verify(m.write(buf, bsize) == bsize);
  return m;
}

inline rrr::Marshal &operator<<(rrr::Marshal &m, const uint8_t &u) {
  verify(m.write(&u, sizeof(u)) == sizeof(u));
  return m;
}

inline rrr::Marshal &operator<<(rrr::Marshal &m, const uint16_t &u) {
  verify(m.write(&u, sizeof(u)) == sizeof(u));
  return m;
}

inline rrr::Marshal &operator<<(rrr::Marshal &m, const uint32_t &u) {
  verify(m.write(&u, sizeof(u)) == sizeof(u));
  return m;
}

inline rrr::Marshal &operator<<(rrr::Marshal &m, const uint64_t &u) {
  verify(m.write(&u, sizeof(u)) == sizeof(u));
  return m;
}

inline rrr::Marshal &operator<<(rrr::Marshal &m, const double &v) {
  verify(m.write(&v, sizeof(v)) == sizeof(v));
  return m;
}

inline rrr::Marshal &operator<<(rrr::Marshal &m, const std::string &v) {
  v64 v_len = v.length();
  m << v_len;
  if (v_len.get() > 0) {
    verify(m.write(v.c_str(), v_len.get()) == (size_t) v_len.get());
  }
  return m;
}

template<class T1, class T2>
inline rrr::Marshal &operator<<(rrr::Marshal &m, const std::pair<T1, T2> &v) {
  m << v.first;
  m << v.second;
  return m;
}

template<class T>
inline rrr::Marshal &operator<<(rrr::Marshal &m, const std::vector<T> &v) {
  v64 v_len = v.size();
  m << v_len;
  for (typename std::vector<T>::const_iterator it = v.begin(); it != v.end();
       ++it) {
    m << *it;
  }
  return m;
}

template<class T>
inline rrr::Marshal &operator<<(rrr::Marshal &m, const std::list<T> &v) {
  v64 v_len = v.size();
  m << v_len;
  for (typename std::list<T>::const_iterator it = v.begin(); it != v.end();
       ++it) {
    m << *it;
  }
  return m;
}

template<class T>
inline rrr::Marshal &operator<<(rrr::Marshal &m, const std::set<T> &v) {
  v64 v_len = v.size();
  m << v_len;
  for (typename std::set<T>::const_iterator it = v.begin(); it != v.end();
       ++it) {
    m << *it;
  }
  return m;
}

template<class K, class V>
inline rrr::Marshal &operator<<(rrr::Marshal &m, const std::map<K, V> &v) {
  v64 v_len = v.size();
  m << v_len;
  for (typename std::map<K, V>::const_iterator it = v.begin(); it != v.end();
       ++it) {
    m << it->first << it->second;
  }
  return m;
}

template<class T>
inline rrr::Marshal &operator<<(rrr::Marshal &m,
                                const std::unordered_set<T> &v) {
  v64 v_len = v.size();
  m << v_len;
  for (typename std::unordered_set<T>::const_iterator it = v.begin();
       it != v.end(); ++it) {
    m << *it;
  }
  return m;
}

template<class K, class V>
inline rrr::Marshal &operator<<(rrr::Marshal &m,
                                const std::unordered_map<K, V> &v) {
  v64 v_len = v.size();
  m << v_len;
  for (typename std::unordered_map<K, V>::const_iterator it = v.begin();
       it != v.end(); ++it) {
    m << it->first << it->second;
  }
  return m;
}

inline rrr::Marshal &operator>>(rrr::Marshal &m, rrr::i8 &v) {
  verify(m.read(&v, sizeof(v)) == sizeof(v));
  return m;
}

inline rrr::Marshal &operator>>(rrr::Marshal &m, rrr::i16 &v) {
  verify(m.read(&v, sizeof(v)) == sizeof(v));
  return m;
}

inline rrr::Marshal &operator>>(rrr::Marshal &m, rrr::i32 &v) {
  verify(m.read(&v, sizeof(v)) == sizeof(v));
  return m;
}

inline rrr::Marshal &operator>>(rrr::Marshal &m, rrr::i64 &v) {
  verify(m.read(&v, sizeof(v)) == sizeof(v));
  return m;
}

inline rrr::Marshal &operator>>(rrr::Marshal &m, rrr::v32 &v) {
  char byte0;
  verify(m.peek(&byte0, 1) == 1);
  size_t bsize = rrr::SparseInt::buf_size(byte0);
  char buf[5];
  verify(m.read(buf, bsize) == bsize);
  i32 val = rrr::SparseInt::load_i32(buf);
  v.set(val);
  return m;
}

inline rrr::Marshal &operator>>(rrr::Marshal &m, rrr::v64 &v) {
  char byte0;
  //Log_info("peeking data of %d", m.peek(&byte0, 1));
  verify(m.peek(&byte0, 1) == 1);
  size_t bsize = rrr::SparseInt::buf_size(byte0);
  char buf[9];
  verify(m.read(buf, bsize) == bsize);
  i64 val = rrr::SparseInt::load_i64(buf);
  v.set(val);
  return m;
}

inline rrr::Marshal &operator>>(rrr::Marshal &m, uint8_t &u) {
  verify(m.read(&u, sizeof(u)) == sizeof(u));
  return m;
}

inline rrr::Marshal &operator>>(rrr::Marshal &m, uint16_t &u) {
  verify(m.read(&u, sizeof(u)) == sizeof(u));
  return m;
}

inline rrr::Marshal &operator>>(rrr::Marshal &m, uint32_t &u) {
  verify(m.read(&u, sizeof(u)) == sizeof(u));
  return m;
}

inline rrr::Marshal &operator>>(rrr::Marshal &m, uint64_t &u) {
  verify(m.read(&u, sizeof(u)) == sizeof(u));
  return m;
}

inline rrr::Marshal &operator>>(rrr::Marshal &m, double &v) {
  verify(m.read(&v, sizeof(v)) == sizeof(v));
  return m;
}

inline rrr::Marshal &operator>>(rrr::Marshal &m, std::string &v) {
  v64 v_len;
  m >> v_len;
  v.resize(v_len.get());
  if (v_len.get() > 0) {
    verify(m.read(&v[0], v_len.get()) == (size_t) v_len.get());
  }
  return m;
}

template<class T1, class T2>
inline rrr::Marshal &operator>>(rrr::Marshal &m, std::pair<T1, T2> &v) {
  m >> v.first;
  m >> v.second;
  return m;
}

template<class T>
inline rrr::Marshal &operator>>(rrr::Marshal &m, std::vector<T> &v) {
  v64 v_len;
  m >> v_len;
  v.clear();
  v.reserve(v_len.get());
  for (int i = 0; i < v_len.get(); i++) {
    T elem;
    m >> elem;
    v.push_back(elem);
  }
  return m;
}

template<class T>
inline rrr::Marshal &operator>>(rrr::Marshal &m, std::list<T> &v) {
  v64 v_len;
  m >> v_len;
  v.clear();
  for (int i = 0; i < v_len.get(); i++) {
    T elem;
    m >> elem;
    v.push_back(elem);
  }
  return m;
}

template<class T>
inline rrr::Marshal &operator>>(rrr::Marshal &m, std::set<T> &v) {
  v64 v_len;
  m >> v_len;
  v.clear();
  for (int i = 0; i < v_len.get(); i++) {
    T elem;
    m >> elem;
    v.insert(elem);
  }
  return m;
}

template<class K, class V>
inline rrr::Marshal &operator>>(rrr::Marshal &m, std::map<K, V> &v) {
  v64 v_len;
  m >> v_len;
  v.clear();
  for (int i = 0; i < v_len.get(); i++) {
    K key;
    V value;
    m >> key >> value;
    insert_into_map(v, key, value);
  }
  return m;
}

template<class T>
inline rrr::Marshal &operator>>(rrr::Marshal &m, std::unordered_set<T> &v) {
  v64 v_len;
  m >> v_len;
  v.clear();
  for (int i = 0; i < v_len.get(); i++) {
    T elem;
    m >> elem;
    v.insert(elem);
  }
  return m;
}

template<class K, class V>
inline rrr::Marshal &operator>>(rrr::Marshal &m, std::unordered_map<K, V> &v) {
  v64 v_len;
  m >> v_len;
  v.clear();
  for (int i = 0; i < v_len.get(); i++) {
    K key;
    V value;
    m >> key >> value;
    insert_into_map(v, key, value);
  }
  return m;
}

inline rrr::Marshal& operator>>(rrr::Marshal& m, rrr::MarshallDeputy& rhs) {
  m >> rhs.kind_;
  rhs.CreateActualObjectFrom(m);
  return m;
}

inline rrr::Marshal& operator<<(rrr::Marshal& m,const rrr::MarshallDeputy& rhs) {
  verify(rhs.kind_ != rrr::MarshallDeputy::UNKNOWN);
  verify(rhs.sp_data_);
  if(rhs.bypass_to_socket_){
    m.bypass_copying(rhs, rhs.EntitySize());
  }else{
    //Log_info("size is %d", rhs.EntitySize());
    m << rhs.kind_;
    verify(rhs.sp_data_); // must be non-empty
    rhs.sp_data_->ToMarshal(m);
  }
  return m;
}

} // namespace rrr
