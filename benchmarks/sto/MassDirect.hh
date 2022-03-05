#pragma once

#include "compiler.hh"
#include "kvthread.hh"
#include "circular_int.hh"
#include "masstree_tcursor.hh"
#include "masstree_insert.hh"
//#include "masstree_print.hh"
//#include "masstree_remove.hh"
//#include "masstree_scan.hh"
#include "string.hh"

//#include "StringWrapper.hh"
//#include "versioned_value.hh"
#include "stuffed_str.hh"

#define DEFAULT_TABLE_ID 10002


template <typename T, typename=void>
struct base_versioned_value_struct {
  typedef T value_type;

  base_versioned_value_struct() : value_() {}
  // XXX Yihe: I made it public; is there any reason why it should be private?
  explicit base_versioned_value_struct(const value_type& val) : value_(val) {}
  
  static base_versioned_value_struct* make(const value_type& val) {
    return new base_versioned_value_struct<T>(val);
  }
  
  bool needsResize(const value_type&) {
    return false;
  }
  
  base_versioned_value_struct* resizeIfNeeded(const value_type&) {
    return NULL;
  }
  
  inline void set_value(const value_type& v) {
    value_ = v;
  }
  
  inline const value_type& read_value() const {
    return value_;
  }

  inline value_type& writeable_value() {
    return value_;
  }

  inline void deallocate_rcu(threadinfo& ti) {
    ti.deallocate_rcu(this, sizeof(base_versioned_value_struct), memtag_value);
  }
  
private:
  value_type value_;
};

static inline unsigned long long int keystore_decode34(const std::string& s, bool first= false){
	unsigned long long int x=0;
	size_t LEN_ULL = sizeof (unsigned long long int );
	if(first) {
	  // unwrap value
	  memcpy (&x, (void *) (s.data ()), LEN_ULL);
	  return x;
	}else {
	  // unwrap commit ID
	  memcpy (&x, (void *) (s.data () + LEN_ULL), sizeof (unsigned long long int));
	  return x;
	}
}

typedef stuffed_str<uint64_t> versioned_str;

struct base_versioned_str_struct : public versioned_str {
  typedef Masstree::Str value_type;

  bool needsResize(const value_type& v) {
    return needs_resize(v.length());
  }
  bool needsResize(const std::string& v) {
    return needs_resize(v.length());
  }

  base_versioned_str_struct* resizeIfNeeded(const value_type& potential_new_value) {
    // TODO: this cast is only safe because we have no ivars or virtual methods
    return (base_versioned_str_struct*)this->reserve(versioned_str::size_for(potential_new_value.length()));
  }
  base_versioned_str_struct* resizeIfNeeded(const std::string& potential_new_value) {
    // TODO: this cast is only safe because we have no ivars or virtual methods
    return (base_versioned_str_struct*)this->reserve(versioned_str::size_for(potential_new_value.length()));
  }

  template <typename StringType>
  inline void set_value(const StringType& v) {
    auto *ret = this->replace(v.data(), v.length());
    // we should already be the proper size at this point
    (void)ret;
    assert(ret == this);
  }
  
  // responsibility is on the caller of this method to make sure this read is atomic
  value_type read_value() {
    return Masstree::Str(this->data(), this->length());
  }
  
  inline void deallocate_rcu(threadinfo& ti) {
//    ti.deallocate_rcu(this, this->capacity() + sizeof(base_versioned_str_struct), memtag_value);
  }
};

template <typename V,typename Box = base_versioned_value_struct<V>>
class MassDirect {
 protected:
  typedef Box tvalue;
 private:
  struct ti_wrapper {
    threadinfo *ti;
  };
  typedef ti_wrapper threadinfo_type;
  static __thread threadinfo_type mythreadinfo;
  typedef V write_value_type;
  typedef std::string key_write_value_type;
  
  typedef Masstree::Str Str;
  
  unsigned long long int table_name_id;

 public:
	MassDirect(){
	  	this->table_name_id = DEFAULT_TABLE_ID;
	  	
	  	if (!mythreadinfo.ti) {
	  	   //auto* ti = threadinfo::make(threadinfo::TI_PROCESS, TThread::id());
		   //mythreadinfo.ti = ti;
		    auto* ti = threadinfo::make(threadinfo::TI_MAIN, -1);
                    mythreadinfo.ti = ti;
    		}
	  	// root leaf created for this node
	  	table_.initialize(*mythreadinfo.ti);
	}
	
	void set_table_name(const unsigned long long int tid){
    	table_name_id = tid;
  	}
  	
  	static void thread_init() {
  	  if (!mythreadinfo.ti) {
		auto *ti = threadinfo::make (threadinfo::TI_PROCESS, TThread::id ());
		mythreadinfo.ti = ti;
	  }
  	}
 
 private:
	template <typename StringType, typename ValueType>
	bool non_trans_write(const StringType& key,
						 const ValueType& new_value,
						 bool(*compar)(const std::string& newValue,const std::string& oldValue),
						 threadinfo_type& ti = mythreadinfo) {
	  locked_cursor_type lp (table_, key);
	  bool found = lp.find_insert(*ti.ti);
	  if (found) {
//	    std::cout << "Found key " << key << std::endl;
		tvalue *e = lp.value ();
		
		if(!compar(new_value, e->read_value())){
		  lp.finish (0, *ti.ti);
		  return false;
		}else{
		  e->set_value(new_value);
		  lp.finish (1, *ti.ti);
		}
		return true;
	  } else {
//	    std::cout << "Key not found " << key << std::endl;
		auto *val = (tvalue *) tvalue::make (new_value, 0);
		lp.value () = val;
		lp.finish (1, *ti.ti);
		fence (); // jay : not sure how to handle
//		std::cout << "return true " << key << std::endl;
		return true;
	  }
	}
  public:
	template <typename KT, typename VT>
	bool put(const KT& k,
		const VT& v,
		bool(*compar)(const KT& newValue,const KT& oldValue),
		threadinfo_type& ti = mythreadinfo) {
	  return non_trans_write(k, v, compar, ti);
	}

    template <typename StringType>
    bool exists(const StringType key,
                 threadinfo_type& ti = mythreadinfo) {
        unlocked_cursor_type lp (table_, key);
        return lp.find_unlocked(*ti.ti);
    }

 private:
  	struct table_params : public Masstree::nodeparams<15,15> {
	  typedef threadinfo threadinfo_type;
	  typedef tvalue* value_type;
  	};
	
	typedef Masstree::basic_table<table_params> table_type;
	typedef Masstree::tcursor<table_params> locked_cursor_type;
	typedef Masstree::unlocked_tcursor<table_params> unlocked_cursor_type;
  	table_type table_;
};

template <typename V, typename Box>
__thread typename MassDirect<V, Box>::threadinfo_type MassDirect<V, Box>::mythreadinfo;
