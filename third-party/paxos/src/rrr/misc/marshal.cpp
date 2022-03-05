#include <sstream>

#include <sys/time.h>
#include <mutex>

#include "marshal.hpp"

using namespace std;

namespace rrr {

#ifdef RPC_STATISTICS

// -1, 0~15, 16~31, 32~63, 64~127, 128~255, 256~511, 512~1023, 1024~2047, 2048~4095, 4096~8191, 8192~
static Counter g_marshal_in_stat[12];
static Counter g_marshal_in_stat_cumulative[12];
static Counter g_marshal_out_stat[12];
static Counter g_marshal_out_stat_cumulative[12];
static uint64_t g_marshal_stat_report_time = 0;
static const uint64_t g_marshal_stat_report_interval = 1000 * 1000 * 1000;

static void stat_marshal_report() {
    Log::info("* MARSHAL:     -1 0~15 16~31 32~63 64~127 128~255 256~511 512~1023 1024~2047 2048~4095 4096~8191 8192~");
    {
        ostringstream ostr;
        for (size_t i = 0; i < arraysize(g_marshal_in_stat); i++) {
            i64 v = g_marshal_in_stat[i].peek_next();
            g_marshal_in_stat_cumulative[i].next(v);
            ostr << " " << v;
            g_marshal_in_stat[i].reset();
        }
        Log::info("* MARSHAL IN: %s", ostr.str().c_str());
    }
    {
        ostringstream ostr;
        for (size_t i = 0; i < arraysize(g_marshal_in_stat); i++) {
            ostr << " " << g_marshal_in_stat_cumulative[i].peek_next();
        }
        Log::info("* MARSHAL IN (cumulative): %s", ostr.str().c_str());
    }
    {
        ostringstream ostr;
        for (size_t i = 0; i < arraysize(g_marshal_out_stat); i++) {
            i64 v = g_marshal_out_stat[i].peek_next();
            g_marshal_out_stat_cumulative[i].next(v);
            ostr << " " << v;
            g_marshal_out_stat[i].reset();
        }
        Log::info("* MARSHAL OUT:%s", ostr.str().c_str());
    }
    {
        ostringstream ostr;
        for (size_t i = 0; i < arraysize(g_marshal_in_stat); i++) {
            ostr << " " << g_marshal_out_stat_cumulative[i].peek_next();
        }
        Log::info("* MARSHAL OUT (cumulative): %s", ostr.str().c_str());
    }
}

void stat_marshal_in(int fd, const void* buf, size_t nbytes, ssize_t ret) {
    if (ret == -1) {
        g_marshal_in_stat[0].next();
    } else if (ret < 16) {
        g_marshal_in_stat[1].next();
    } else if (ret < 32) {
        g_marshal_in_stat[2].next();
    } else if (ret < 64) {
        g_marshal_in_stat[3].next();
    } else if (ret < 128) {
        g_marshal_in_stat[4].next();
    } else if (ret < 256) {
        g_marshal_in_stat[5].next();
    } else if (ret < 512) {
        g_marshal_in_stat[6].next();
    } else if (ret < 1024) {
        g_marshal_in_stat[7].next();
    } else if (ret < 2048) {
        g_marshal_in_stat[8].next();
    } else if (ret < 4096) {
        g_marshal_in_stat[9].next();
    } else if (ret < 8192) {
        g_marshal_in_stat[10].next();
    } else {
        g_marshal_in_stat[11].next();
    }

    uint64_t now = base::rdtsc();
    if (now - g_marshal_stat_report_time > g_marshal_stat_report_interval) {
        stat_marshal_report();
        g_marshal_stat_report_time = now;
    }
}

void stat_marshal_out(int fd, const void* buf, size_t nbytes, ssize_t ret) {
    if (ret == -1) {
        g_marshal_out_stat[0].next();
    } else if (ret < 16) {
        g_marshal_out_stat[1].next();
    } else if (ret < 32) {
        g_marshal_out_stat[2].next();
    } else if (ret < 64) {
        g_marshal_out_stat[3].next();
    } else if (ret < 128) {
        g_marshal_out_stat[4].next();
    } else if (ret < 256) {
        g_marshal_out_stat[5].next();
    } else if (ret < 512) {
        g_marshal_out_stat[6].next();
    } else if (ret < 1024) {
        g_marshal_out_stat[7].next();
    } else if (ret < 2048) {
        g_marshal_out_stat[8].next();
    } else if (ret < 4096) {
        g_marshal_out_stat[9].next();
    } else if (ret < 8192) {
        g_marshal_out_stat[10].next();
    } else {
        g_marshal_out_stat[11].next();
    }

    uint64_t now = base::rdtsc();
    if (now - g_marshal_stat_report_time > g_marshal_stat_report_interval) {
        stat_marshal_report();
        g_marshal_stat_report_time = now;
    }
}

#endif // RPC_STATISTICS

/**
 * 8kb minimum chunk size.
 * NOTE: this value directly affects how many read/write syscall will be issued.
 */
const size_t Marshal::raw_bytes::min_size = 8192;

Marshal::~Marshal() {
    chunk* chnk = head_;
    while (chnk != nullptr) {
	//Log_info("wkwkakakak");
        chunk* next = chnk->next;
        delete chnk;
        chnk = next;
    }
}

size_t Marshal::content_size_slow() const {
    assert(tail_ == nullptr || tail_->next == nullptr);

    size_t sz = 0;
    chunk* chnk = head_;
    while (chnk != nullptr) {
	//Log_info("wkwkakakak");
        sz += chnk->content_size();
        chnk = chnk->next;
    }
    return sz;
}

size_t Marshal::write(const void* p, size_t n) {
    assert(tail_ == nullptr || tail_->next == nullptr);

    if (head_ == nullptr) {
        assert(tail_ == nullptr);
        head_ = new chunk(p, n);
        tail_ = head_;
    } else if (tail_->fully_written()) {
        tail_->next = new chunk(p, n);
        tail_ = tail_->next;
    } else {
        size_t n_write = tail_->write(p, n);

        // otherwise the above fully_written() should have returned true
        assert(n_write > 0);

        if (n_write < n) {
            const char* pc = (const char *) p;
            tail_->next = new chunk(pc + n_write, n - n_write);
            tail_ = tail_->next;
        }
    }
    write_cnt_ += n;
    content_size_ += n;
    assert(content_size_ == content_size_slow());

    return n;
}

size_t Marshal::bypass_copying(MarshallDeputy data, size_t sz) {
  //Log_info("bypassing copying %d", sz);
  assert(data.EntitySize() == sz);
  assert(tail_ == nullptr || tail_->next == nullptr);

  if(head_ == nullptr){
    head_ = new chunk(data, sz);
    tail_ = head_;
  } else if (tail_->fully_written()){
    tail_->next = new chunk(data, sz);
    tail_ = tail_->next;
  } else{
    //Log_info("resizing current chunk %d", tail_->write_idx);
    tail_->resize_to_current();
    //verify(tail_->fully_written());
    tail_->next = new chunk(data, sz);
    tail_ = tail_->next;
  }
  write_cnt_ += sz;
  content_size_ += sz;
  assert(content_size_ == content_size_slow());
  //Log_info("final size is %d", write_cnt_);
  return sz;
}

size_t Marshal::read_chnk(void* p, size_t n){
    char* pc = (char *) p;
    size_t n_read = head_->read(pc, n);
    content_size_ -= n_read;
    return n_read;
}

size_t Marshal::read(void* p, size_t n) {
    assert(tail_ == nullptr || tail_->next == nullptr);
    assert(empty() || (head_ != nullptr && !head_->fully_read()));

    char* pc = (char *) p;
    size_t n_read = 0;
    while (n_read < n && head_ != nullptr && head_->content_size() > 0) {
	//Log_info("wkwkakakak");
        size_t cnt = head_->read(pc + n_read, n - n_read);
        if (head_->fully_read()) {
            if (tail_ == head_) {
                // deleted the only chunk
                tail_ = nullptr;
            }
            chunk* chnk = head_;
            head_ = head_->next;
            //delete chnk;
        }
        if (cnt == 0) {
            // currently there's no data available, so stop
            break;
        }
        n_read += cnt;
    }
    assert(content_size_ >= n_read);
    content_size_ -= n_read;
    assert(content_size_ == content_size_slow());

    assert(n_read <= n);
    assert(tail_ == nullptr || tail_->next == nullptr);
    assert(empty() || (head_ != nullptr && !head_->fully_read()));

    return n_read;
}

size_t Marshal::peek(void* p, size_t n) const {
    assert(tail_ == nullptr || tail_->next == nullptr);
    assert(empty() || (head_ != nullptr && !head_->fully_read()));
    //Log_info("is peeking empty %d %d", empty(), n);
    char* pc = (char *) p;
    size_t n_peek = 0;
    chunk* chnk = head_;
    while (chnk != nullptr && n - n_peek > 0) {
	//Log_info("wkwkakakak");
        size_t cnt = chnk->peek(pc + n_peek, n - n_peek);
        if (cnt == 0) {
            // no more data to peek, quit
            break;
        }
        n_peek += cnt;
        chnk = chnk->next;
    }

    assert(n_peek <= n);
    assert(tail_ == nullptr || tail_->next == nullptr);
    assert(empty() || (head_ != nullptr && !head_->fully_read()));
    return n_peek;
}

size_t Marshal::read_from_fd(int fd) {
    assert(empty() || (head_ != nullptr && !head_->fully_read()));

    size_t n_bytes = 0;
    for (;;) {
        if (head_ == nullptr) {
            head_ = new chunk;
            tail_ = head_;
        } else if (tail_->fully_written()) {
            tail_->next = new chunk;
            tail_ = tail_->next;
        }
        int r = tail_->read_from_fd(fd);
        if (r <= 0) {
            break;
        }
        n_bytes += r;
    }
    write_cnt_ += n_bytes;
    content_size_ += n_bytes;
    assert(content_size_ == content_size_slow());

    assert(empty() || (head_ != nullptr && !head_->fully_read()));
    return n_bytes;
}

// the marshal object should have a chunk allocated with necessary size
size_t Marshal::chnk_read_from_fd(int fd, size_t bytes){
    size_t read_bytes = 0;
    read_bytes += head_->read_from_fd(fd, bytes);
    content_size_ += read_bytes;
    write_cnt_ += read_bytes;
    if(read_bytes <= 0)return 0;
    return read_bytes;
}

size_t Marshal::read_reuse_chnk(Marshal& m, size_t n){
    assert(m.content_size() >= n);   // require m.content_size() >= n > 0
    size_t n_fetch = 0;

    while (n_fetch < n) {
        // NOTE: The copied chunk is shared by 2 Marshal objects. Be careful
        //       that only one Marshal should be able to write to it! For the
        //       given 2 use cases, it works.
        chunk* chnk = m.head_->shared_copy();
        if (n_fetch + chnk->content_size() > n) {
            // only fetch enough bytes we need
            chnk->write_idx -= (n_fetch + chnk->content_size()) - n;
        }
        size_t cnt = chnk->content_size();
        assert(cnt > 0);
        n_fetch += cnt;
        verify(m.head_->discard(cnt) == cnt);
        if (head_ == nullptr) {
            head_ = tail_ = chnk;
        } else {
            tail_->next = chnk;
            tail_ = chnk;
        }
    }

    write_cnt_ += n_fetch;
    content_size_ += n_fetch;
    verify(m.content_size_ >= n_fetch);
    m.content_size_ -= n_fetch;
    return n_fetch;
}

size_t Marshal::read_from_marshal(Marshal& m, size_t n) {
    assert(m.content_size() >= n);   // require m.content_size() >= n > 0
    size_t n_fetch = 0;

    if ((head_ == nullptr && tail_ == nullptr) || tail_->fully_written()) {
        // efficiently copy data by only copying pointers
        while (n_fetch < n) {
	   //Log_info("wkwkakakak");
            // NOTE: The copied chunk is shared by 2 Marshal objects. Be careful
            //       that only one Marshal should be able to write to it! For the
            //       given 2 use cases, it works.
            chunk* chnk = m.head_->shared_copy();
            if (n_fetch + chnk->content_size() > n) {
                // only fetch enough bytes we need
                chnk->write_idx -= (n_fetch + chnk->content_size()) - n;
            }
            size_t cnt = chnk->content_size();
            assert(cnt > 0);
            n_fetch += cnt;
            verify(m.head_->discard(cnt) == cnt);
            if (head_ == nullptr) {
                head_ = tail_ = chnk;
            } else {
                tail_->next = chnk;
                tail_ = chnk;
            }
            if (m.head_->fully_read()) {
                if (m.tail_ == m.head_) {
                    // deleted the only chunk
                    m.tail_ = nullptr;
                }
                chunk* next = m.head_->next;
                delete m.head_;
                m.head_ = next;
            }
        }
        write_cnt_ += n_fetch;
        content_size_ += n_fetch;
        verify(m.content_size_ >= n_fetch);
        m.content_size_ -= n_fetch;

    } else {

        // number of bytes that need to be copied
        size_t copy_n = std::min(tail_->data->size - tail_->write_idx, n);
        char* buf = new char[copy_n];
        n_fetch = m.read(buf, copy_n);
        verify(n_fetch == copy_n);
        verify(this->write(buf, n_fetch) == n_fetch);
        delete[] buf;

        size_t leftover = n - copy_n;
        if (leftover > 0) {
            verify(tail_->fully_written());
            n_fetch += this->read_from_marshal(m, leftover);
        }
    }
    assert(n_fetch == n);
    assert(content_size_ == content_size_slow());
    return n_fetch;
}


size_t Marshal::write_to_fd(int fd) {
    size_t n_write = 0;
    bool ok = false;
    while (!empty()) {
        int cnt = head_->write_to_fd(fd);
	//Log_info("written %d bytes of %d", head_->read_idx, head_->write_idx);
        if (head_->fully_read()) {
            if (head_ == tail_) {
                tail_ = nullptr;
            }
	    //Log_info("fully read a chunk of size %d %d", head_->data->size, head_->write_idx);
            chunk* chnk = head_;
            head_ = head_->next;
            delete chnk;
	    ok = true;
        }
        if (cnt <= 0) {
	    //Log_info("written less than 0 bytes, breaking... %d %d %d", head_->data->size, head_->write_idx, head_->read_idx);
            break;
        } else {
	    //Log_info("written %lld bytes of %lld", head_->read_idx, head_->write_idx);	
	}
        assert(content_size_ >= (size_t) cnt);
        content_size_ -= cnt;
        n_write += cnt;
	//if(ok) break;

    }
    assert(content_size_ == content_size_slow());
    return n_write;
}

Marshal::bookmark* Marshal::set_bookmark(size_t n) {
    verify(write_cnt_ == 0);

    bookmark* bm = new bookmark;
    bm->size = n;
    bm->ptr = new char*[bm->size];
    for (size_t i = 0; i < n; i++) {
        if (head_ == nullptr) {
            head_ = new chunk;
            tail_ = head_;
        } else if (tail_->fully_written() || tail_->is_shared_data_chunk()) {
            tail_->next = new chunk;
            tail_ = tail_->next;
        }
        bm->ptr[i] = tail_->set_bookmark();
    }
    content_size_ += n;
    assert(content_size_ == content_size_slow());

    return bm;
}

std::mutex md_mutex_g;
std::mutex mdi_mutex_g;
std::shared_ptr<MarshallDeputy::MarContainer> mc_{nullptr};
thread_local std::shared_ptr<MarshallDeputy::MarContainer> mc_th_{nullptr};

int MarshallDeputy::RegInitializer(int32_t cmd_type,
                                   function<Marshallable * ()> init) {
  md_mutex_g.lock();
  auto pair = Initializers().insert(std::make_pair(cmd_type, init));
  verify(pair.second);
  md_mutex_g.unlock();
  return 0;
}

function<Marshallable * ()>
MarshallDeputy::GetInitializer(int32_t type) {
  if (!mc_th_) {
    mc_th_ = std::make_shared<MarshallDeputy::MarContainer>();
    md_mutex_g.lock();
    *mc_th_ = *mc_;
    md_mutex_g.unlock();
  }
  auto it = mc_th_->find(type);
  verify(it != mc_th_->end());
  auto f = it->second;
  return f;
}

MarshallDeputy::MarContainer&
MarshallDeputy::Initializers() {
  mdi_mutex_g.lock();
  if (!mc_)
    mc_ = std::make_shared<MarshallDeputy::MarContainer>();
  mdi_mutex_g.unlock();
  return *mc_;
};

Marshal &Marshallable::FromMarshal(Marshal &m) {
  verify(0);
  return m;
}

Marshal& MarshallDeputy::CreateActualObjectFrom(Marshal& m) {
  verify(sp_data_ == nullptr);
  switch (kind_) {
    case UNKNOWN:
      verify(0);
      break;
    default:
      auto func = GetInitializer(kind_);
      verify(func);
      sp_data_.reset(func());
      break;
  }
  verify(sp_data_);
  sp_data_->FromMarshal(m);
  verify(sp_data_->kind_);
  verify(kind_);
  verify(sp_data_->kind_ == kind_);
  return m;
}

Marshal &Marshallable::ToMarshal(Marshal &m) const {
  verify(0);
  return m;
}

} // namespace rrr
