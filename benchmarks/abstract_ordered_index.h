#ifndef _ABSTRACT_ORDERED_INDEX_H_
#define _ABSTRACT_ORDERED_INDEX_H_

#include <stdint.h>
#include <string>
#include <utility>
#include <map>
#include "../masstree/str.hh"

#include "../macros.h"
#include "../str_arena.h"
#include "tpcc_keys.h"

/**
 * The underlying index manages memory for keys/values, but
 * may choose to expose the underlying memory to callers
 * (see put() and inesrt()).
 */
class abstract_ordered_index {
public:

  virtual ~abstract_ordered_index() {}

  /**
   * Get a key of length keylen. The underlying DB does not manage
   * the memory associated with key. Returns true if found, false otherwise
   */
  virtual bool get(
      void *txn,
      lcdf::Str key,
      std::string &value,
      size_t max_bytes_read = std::string::npos) = 0;

  virtual bool get(
      void *txn,
      const std::string &key,
      std::string &value,
      size_t max_bytes_read = std::string::npos) {
      return get(txn, lcdf::Str(key), value, max_bytes_read);
  }

  virtual bool get(
      void *txn,
      int32_t key,
      std::string &value,
      size_t max_bytes_read = std::string::npos) {
      return get(txn, lcdf::Str(reinterpret_cast<const char*>(&key), sizeof(key)), value, max_bytes_read);
  }
  
  virtual bool get(
      void *txn,
      customer_key key,
      std::string &value,
      size_t max_bytes_read = std::string::npos) {
      return get(txn, lcdf::Str(reinterpret_cast<const char*>(&key), sizeof(key)), value, max_bytes_read);
  }

  class scan_callback {
  public:
    virtual ~scan_callback() {}
    // XXX(stephentu): key is passed as (const char *, size_t) pair
    // because it really should be the string_type of the underlying
    // tree, but since abstract_ordered_index is not templated we can't
    // really do better than this for now
    //
    // we keep value as std::string b/c we have more control over how those
    // strings are generated
    virtual bool invoke(const char *keyp, size_t keylen,
                        const std::string &value) = 0;
  };

  /**
   * Search [start_key, *end_key) if end_key is not null, otherwise
   * search [start_key, +infty)
   */
  virtual void scan(
      void *txn,
      const std::string &start_key,
      const std::string *end_key,
      scan_callback &callback,
      str_arena *arena = nullptr) = 0;

    virtual std::string scan_iter(
            void *txn,
            const std::string &start_key,
            const std::string *end_key,
            scan_callback &callback,
            str_arena *arena = nullptr) = 0;

  /**
   * Search (*end_key, start_key] if end_key is not null, otherwise
   * search (-infty, start_key] (starting at start_key and traversing
   * backwards)
   */
  virtual void rscan(
      void *txn,
      const std::string &start_key,
      const std::string *end_key,
      scan_callback &callback,
      str_arena *arena = nullptr) = 0;

  /**
   * Put a key of length keylen, with mapping of length valuelen.
   * The underlying DB does not manage the memory pointed to by key or value
   * (a copy is made).
   *
   * If a record with key k exists, overwrites. Otherwise, inserts.
   *
   * If the return value is not NULL, then it points to the actual stable
   * location in memory where the value is located. Thus, [ret, ret+valuelen)
   * will be valid memory, bytewise equal to [value, value+valuelen), since the
   * implementations have immutable values for the time being. The value
   * returned is guaranteed to be valid memory until the key associated with
   * value is overriden.
   */
  virtual const char *
  put(void *txn,
      lcdf::Str key,
      const std::string &value) = 0;

  virtual const char *
  put(void *txn,
      const std::string &key,
      const std::string &value) {
      return put(txn, lcdf::Str(key), value);
  }

    virtual const char *
    put_mbta(void *txn,
        lcdf::Str key,
        bool(*compar)(const std::string& newValue,const std::string& oldValue),
        const std::string &value) = 0;

    virtual const char *
    put_mbta(void *txn,
        const std::string &key,
        bool(*compar)(const std::string& newValue,const std::string& oldValue),
        const std::string &value) {
        return put_mbta(txn, lcdf::Str(key), compar, value);
    }

  virtual const char *
  put(void *txn,
      lcdf::Str key,
      std::string &&value)
  {
      return put(txn, key, static_cast<const std::string &>(value));
  }

  virtual const char *
  put(void *txn,
      const std::string &key,
      std::string &&value)
  {   
      return put(txn, key, static_cast<const std::string &>(value));
  }  


  virtual const char *
  put(void *txn,
         int32_t key,
         const std::string &value)
  {
      return put(txn, lcdf::Str(reinterpret_cast<const char*>(&key), sizeof(key)), value);
  }

  virtual const char *
  put(void *txn,
         int32_t key,
         std::string &&value)
  {
      return put(txn, lcdf::Str(reinterpret_cast<const char*>(&key), sizeof(key)), value);
  }


  virtual const char *
  put(void *txn,
         customer_key key,
         const std::string &value)
  {
      return put(txn, lcdf::Str(reinterpret_cast<const char*>(&key), sizeof(key)), value);
  }

  virtual const char *
  put(void *txn,
         customer_key key,
         std::string &&value)
  {
      return put(txn, lcdf::Str(reinterpret_cast<const char*>(&key), sizeof(key)), value);
  }



  /**
   * Insert a key of length keylen.
   *
   * If a record with key k exists, behavior is unspecified- this function
   * is only to be used when you can guarantee no such key exists (ie in loading phase)
   *
   * Default implementation calls put(). See put() for meaning of return value.
   */
  virtual const char *
  insert(void *txn,
         lcdf::Str key,
         const std::string &value)
  {
    return put(txn, key, value);
  }

  virtual const char *
  insert(void *txn,
         const std::string &key,
         const std::string &value)
  {      
    return insert(txn, lcdf::Str(key), value);
  } 

  virtual const char *
  insert(void *txn,
         lcdf::Str key,
         std::string &&value)
  {
      return insert(txn, key, static_cast<const std::string &>(value));
  }

  virtual const char *
  insert(void *txn,
         const std::string &key,
         std::string &&value)
  {      
      return insert(txn, key, static_cast<const std::string &>(value));
  }   

  virtual const char *
  insert(void *txn,
         int32_t key,
         const std::string &value)
  {
      return insert(txn, lcdf::Str(reinterpret_cast<const char*>(&key), sizeof(key)), value);
  }

  virtual const char *
  insert(void *txn,
         int32_t key,
         std::string &&value)
  {
      return insert(txn, lcdf::Str(reinterpret_cast<const char*>(&key), sizeof(key)), value);
  }

  virtual const char *
  insert(void *txn,
         customer_key key,
         const std::string &value)
  {      
      return insert(txn, lcdf::Str(reinterpret_cast<const char*>(&key), sizeof(key)), value);
  }
  
  virtual const char *
  insert(void *txn,
         customer_key key,
         std::string &&value)
  {      
      return insert(txn, lcdf::Str(reinterpret_cast<const char*>(&key), sizeof(key)), value);
  }


  /**
   * Default implementation calls put() with NULL (zero-length) value
   */
  virtual void remove(
      void *txn,
      lcdf::Str key)
  {
    put(txn, key, "");
  }

  virtual void remove(
      void *txn,
      const std::string &key)
  {   
    remove(txn, lcdf::Str(key));
  } 

  virtual void remove(
      void *txn,
      int32_t key)
  {
    remove(txn, lcdf::Str(reinterpret_cast<const char*>(&key), sizeof(key)));
  }

  virtual void remove(
      void *txn,
      customer_key key)
  {   
    remove(txn, lcdf::Str(reinterpret_cast<const char*>(&key), sizeof(key)));
  } 



  /**
   * Only an estimate, not transactional!
   */
  virtual size_t size() const = 0;

  /**
   * Not thread safe for now
   */
  virtual std::map<std::string, uint64_t> clear() = 0;

  virtual void print_stats() { }
};

#endif /* _ABSTRACT_ORDERED_INDEX_H_ */
