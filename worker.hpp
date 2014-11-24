/**
Asier Gutierrez <asierguti@gmail.com>

Yandex 2014

**/

#include "utils.hpp"

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/unordered_map.hpp>

#include <time.h>
#include <unistd.h>
#include <stdio.h>

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include <cocaine/framework/handler.hpp>
#include <cocaine/framework/handlers/http.hpp>
#include <cocaine/framework/dispatch.hpp>
#include <cocaine/framework/services/storage.hpp>

#include <cocaine/api/stream.hpp>


#include <elliptics/utils.hpp>
#include <elliptics/result_entry.hpp>
#include <elliptics/session.hpp>
#include <elliptics/cppdef.h>

#include <msgpack.hpp>

#include <json/json.h>

  using namespace ioremap::elliptics;

  typedef boost::interprocess::basic_string<char> shared_string;

  struct ListNode {
    double TimeStamp;
    shared_string key;
    int count;
    long size_element;
  };

  typedef boost::interprocess::allocator<
      void, boost::interprocess::managed_shared_memory::segment_manager>
      void_allocator;
  typedef boost::interprocess::allocator<
      char, boost::interprocess::managed_shared_memory::segment_manager>
      char_allocator;

  typedef boost::interprocess::allocator<
      ListNode, boost::interprocess::managed_shared_memory::segment_manager>
      CustomListAllocator;
  typedef boost::interprocess::list<ListNode, CustomListAllocator> CustomList;
  typedef boost::interprocess::list<ListNode, CustomListAllocator>::iterator CustomListIterator;



  typedef boost::interprocess::allocator<
    std::pair<shared_string, CustomList::iterator>,
      boost::interprocess::managed_shared_memory::segment_manager>
      HashAllocator;
  typedef boost::unordered_map<
    shared_string, CustomList::iterator, boost::hash<shared_string>,
      std::equal_to<shared_string>, HashAllocator> CustomHashTable;


class worker {
private:
  void init ();

public:
  worker(cocaine::framework::dispatch_t &d);

  virtual ~worker () {
    COCAINE_LOG_INFO (m_log, "worker destructor");
    boost::interprocess::shared_memory_object::remove("SharedMemory");
  }

  std::shared_ptr<cocaine::framework::logger_t> getLogger ();
  CustomList * getList();
  CustomHashTable * getHash();

  std::shared_ptr<cocaine::framework::logger_t> m_log;
  std::shared_ptr<cocaine::framework::storage_service_t> m_storage;

  CustomList *m_SharedList;
  CustomHashTable *m_SharedHashTable;

  long m_MaxSize;
  long m_TotalSize;

  std::shared_ptr<boost::interprocess::managed_shared_memory> m_segment;

  int m_hit_limit;

  std::string m_remote_address;
  int m_remote_port;
  std::vector <int> m_groups;

  file_logger m_lo;
  ioremap::elliptics::node m_no;
};

struct on_get : public cocaine::framework::handler <worker>,
		public std::enable_shared_from_this<on_get> {
  on_get(worker &w) : cocaine::framework::handler<worker>(w) {
    COCAINE_LOG_INFO(parent().m_log, "Constructor");
    //    boost::interprocess::named_mutex::remove("SharedMutex");
  }

  virtual ~on_get() {
    COCAINE_LOG_INFO(parent().m_log, "Destructor");
    //boost::interprocess::named_mutex::remove("SharedMutex");
  }

  void on_chunk(const char *chunk, size_t size);

  void send(cocaine::framework::generator<std::string> &g) {}
};

struct on_put : public cocaine::framework::handler <worker>,
		public std::enable_shared_from_this<on_put> {
  on_put(worker &w) : cocaine::framework::handler<worker>(w) {
    COCAINE_LOG_INFO(parent().m_log, "Constructor");
    // boost::interprocess::named_mutex::remove("SharedMutex");
  }

  virtual ~on_put (){
    //    boost::interprocess::named_mutex::remove("SharedMutex");
  }

  void on_chunk(const char *chunk, size_t size);
};


struct flush : public cocaine::framework::handler <worker>,
		public std::enable_shared_from_this<flush> {
  flush(worker &w) : cocaine::framework::handler<worker>(w) {
    COCAINE_LOG_INFO(parent().m_log, "Constructor");
    // boost::interprocess::named_mutex::remove("SharedMutex");
  }

  virtual ~flush (){
    //    boost::interprocess::named_mutex::remove("SharedMutex");
  }

  void on_chunk(const char *chunk, size_t size);
};
