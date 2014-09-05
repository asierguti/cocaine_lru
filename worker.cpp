/**
Asier Gutierrez <asierguti@gmail.com>

Yandex 2014

**/
#include "worker.hpp"

worker::worker(cocaine::framework::dispatch_t &d) {
  log = d.service_manager()->get_system_logger();
  COCAINE_LOG_INFO(log, "Another process");
  COCAINE_LOG_DEBUG(log, "Another process");
  COCAINE_LOG_WARNING(log, "Another process");

  storage = d.service_manager()
      ->get_service<cocaine::framework::storage_service_t>("storage");

  d.on<on_get>("get", *this);
}

void on_get::on_chunk(const char *chunk, size_t size) {

  COCAINE_LOG_INFO(parent().log, "Processing on_chunk callback");







  /*  using boost::property_tree::ptree;

  ptree pt;

  read_json(filename, pt);

 auto  shmem_size = pt.get<int>("shmem_size");
  auto total_size = pt.get<int>("total_size");

  */


    ioremap::elliptics::data_pointer data_owner =
      ioremap::elliptics::data_pointer::copy(chunk, size);
  ioremap::elliptics::error_info error;  
  ioremap::elliptics::exec_context context =
      ioremap::elliptics::exec_context::parse(data_owner, &error);
  
    std::ofstream file;
  file.open("received.log", std::ios::out | std::ios::binary);

  file << context.data().to_string();

  file.close();

  std::string output;

  struct shm_remove {
    shm_remove() {
      boost::interprocess::shared_memory_object::remove("SharedMemory");
    }
    ~shm_remove() {
      boost::interprocess::shared_memory_object::remove("SharedMemory");
    }
  } remover;

  struct mutex_remove {
    mutex_remove() { boost::interprocess::named_mutex::remove("SharedMutex"); }
    ~mutex_remove() { boost::interprocess::named_mutex::remove("SharedMutex"); }

  } mutex_remover;




  auto received_data = context.data().to_string();

  msgpack::unpacked msg;    // includes memory pool and deserialized object
  msgpack::unpack(&msg, received_data.c_str(), received_data.size());
  msgpack::object obj = msg.get();

  // Print the deserialized object to stdout.
  

  //std::cout << obj << std::endl;    // ["Hello," "World!"]

  // Convert the deserializecontext.data().to_string();d object to staticaly typed object.
  


  std::vector<std::string> result;
  obj.convert(&result);

  output = "Received size " + result.at(1);

    COCAINE_LOG_INFO(parent().log, output.c_str());






    auto key = result.at(0);//context.data().to_string();
    auto received_size = atol(result.at(1).c_str());


  boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create,
                                         "SharedMutex");

  boost::interprocess::managed_shared_memory segment(
      boost::interprocess::open_or_create, "SharedMemory", 65536);

  typedef boost::interprocess::allocator<
      void, boost::interprocess::managed_shared_memory::segment_manager>
      void_allocator;
  typedef boost::interprocess::allocator<
      char, boost::interprocess::managed_shared_memory::segment_manager>
      char_allocator;
  typedef boost::interprocess::basic_string<char, std::char_traits<char>,
                                            char_allocator> char_string;
  typedef boost::interprocess::basic_string<char> shared_string;
  typedef boost::interprocess::allocator<
      char_string, boost::interprocess::managed_shared_memory::segment_manager>
      string_allocator;


  char_allocator c_allocator(segment.get_segment_manager());


  struct ListNode {
    int TimeStamp;
    shared_string key;
    int count;
    long size;
  };


  // If the type is mismatched, it throws msgpack::type_error.
  // obj.as<int>();  // type is mismatched, msgpack::type_error is thrown


    //////CHECK THAT SIZE IS 2!!!!






  //auto a = cocaine::framework::unpack<std::vector<std::string>>(chunk, size);





  typedef boost::interprocess::allocator<
      ListNode, boost::interprocess::managed_shared_memory::segment_manager>
      CustomListAllocator;
  typedef boost::interprocess::list<ListNode, CustomListAllocator> CustomList;
  typedef boost::interprocess::list<ListNode>::iterator CustomListIterator;

  typedef boost::interprocess::allocator<
      std::pair<char_string, CustomList::iterator>,
      boost::interprocess::managed_shared_memory::segment_manager>
      HashAllocator;
  typedef boost::unordered_map<
      char_string, CustomList::iterator, boost::hash<char_string>,
      std::equal_to<char_string>, HashAllocator> CustomHashTable;

  void_allocator alloc_inst(segment.get_segment_manager());

  CustomList *SharedList;

  {
    //    COCAINE_LOG_INFO(parent().log, "Test2");
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(
        mutex);

    SharedList =
        segment.find_or_construct<CustomList>("SharedList")(alloc_inst);

    COCAINE_LOG_INFO(parent().log, "Test3");

    output = "Starting size " + std::to_string(SharedList->size());

    COCAINE_LOG_INFO(parent().log, output.c_str());

    CustomHashTable *SharedHashTable =
        segment.find_or_construct<CustomHashTable>("SharedHashTable")(
            3, boost::hash<char_string>(), std::equal_to<char_string>(),
            segment
                .get_allocator<std::pair<char_string, CustomListIterator> >());

    COCAINE_LOG_INFO(parent().log, "Test3");

    char_allocator c_allocator(segment.get_segment_manager());
    string_allocator s_allocator(segment.get_segment_manager());

    char_string s(c_allocator);

    struct timeval tv;
    gettimeofday(&tv, NULL);

    double time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;

COCAINE_LOG_INFO(parent().log, "Test3");

    auto found_it = SharedHashTable->find(s);
    CustomList::iterator node;

    if (found_it != SharedHashTable->end())
      node = found_it->second;

COCAINE_LOG_INFO(parent().log, "Test3");

    if (found_it == SharedHashTable->end() || node->size != received_size) {

      if (found_it != SharedHashTable->end()){
	COCAINE_LOG_INFO(parent().log, "Element has changed. Recaching");

	SharedHashTable->erase(found_it);
	SharedList->erase(node);
      }

      COCAINE_LOG_INFO(parent().log, "Adding a new element");

      ListNode node;
      node.TimeStamp = time_in_mill;
      node.key = result.at(0).c_str();
      node.count = 1;
      node.size = received_size;

      SharedList->push_back(node);

      auto it = SharedList->end();

      SharedHashTable->insert(
          std::pair<char_string, CustomList::iterator>(s, it));
    } else {

      COCAINE_LOG_INFO(parent().log, "Element found in the cache");
      //      auto first = found_it->first;

      //auto converted = first.c_str();

      gettimeofday(&tv, NULL);

      time_in_mill =
          (tv.tv_sec) * 1000 +
          (tv.tv_usec) / 1000; // convert tv_sec & tv_usec to millisecond

      node->TimeStamp = time_in_mill;
      ++node->count;

      output = "Hit: " + std::to_string(node->count);
      COCAINE_LOG_INFO(parent().log, output.c_str());

      output = "Date: " + std::to_string(node->TimeStamp);
      COCAINE_LOG_INFO(parent().log, output.c_str());

      SharedList->splice(SharedList->end(), *SharedList, node);
    }

    output = "Ending size " + std::to_string(SharedList->size());
    COCAINE_LOG_INFO(parent().log, output.c_str());
    usleep(1000000);

    auto start = clock();
    auto end = clock();
    /*		while (((double) (end - start)) / CLOCKS_PER_SEC < 1)
   	  {
   	    end = clock();
   	    }
   	*/

    output = "Ending size " + std::to_string(SharedList->size());

    COCAINE_LOG_INFO(parent().log, output.c_str());

    cocaine::framework::http_headers_t headers;
    headers.add_header("Content-Length", "0");

    //response()->write_headers(200, headers);
    //response()->close();

    //auto = cocaine::framework::generator <std::string>& g
    response()->write("");
    response()->close();
  }

  //	response ()->write (gen.next());
}

int main(int argc, char *argv[]) {
  return cocaine::framework::run<worker>(argc, argv);
}
