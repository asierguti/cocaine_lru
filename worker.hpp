/**
Asier Gutierrez <asierguti@gmail.com>

Yandex 2014

**/

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

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include <cocaine/framework/handler.hpp>
#include <cocaine/framework/handlers/http.hpp>
#include <cocaine/framework/dispatch.hpp>
#include <cocaine/framework/services/storage.hpp>

#include <elliptics/utils.hpp>
#include <elliptics/result_entry.hpp>

#include <msgpack.hpp>

class worker {
public:
  worker(cocaine::framework::dispatch_t &d);

  std::shared_ptr<cocaine::framework::logger_t> log;
  std::shared_ptr<cocaine::framework::storage_service_t> storage;

};

struct on_get : public cocaine::framework::handler<worker>,
                public std::enable_shared_from_this<on_get> {
  on_get(worker &w) : cocaine::framework::handler<worker>(w) {
    COCAINE_LOG_INFO(parent().log, "Constructor");

}

  void on_chunk(const char *chunk, size_t size);

  void send(cocaine::framework::generator<std::string> &g) {}
};
