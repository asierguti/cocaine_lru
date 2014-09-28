/**
Asier Gutierrez <asierguti@gmail.com>

Yandex 2014

**/
#include "worker.hpp"

worker::worker(cocaine::framework::dispatch_t &d) {
  m_log = d.service_manager()->get_system_logger();
  COCAINE_LOG_INFO(m_log, "Starting the LRU worker");

  m_storage = d.service_manager()
      ->get_service<cocaine::framework::storage_service_t>("storage");

  d.on<on_get>("get", *this);

  init();
}

void worker::init() {
  COCAINE_LOG_INFO(m_log, "Initializing or grabbing the shared memory");

  boost::interprocess::shared_memory_object::remove("SharedMemory"); 

  m_segment.reset(new boost::interprocess::managed_shared_memory(boost::interprocess::open_or_create, "SharedMemory", 65536));

  char_allocator c_allocator(m_segment->get_segment_manager());
  void_allocator alloc_inst(m_segment->get_segment_manager());

    m_SharedList =
      m_segment->find_or_construct<CustomList>("SharedList")(alloc_inst);  

    m_SharedHashTable = m_segment->find_or_construct<CustomHashTable>("SharedHashTable")(
            3, boost::hash<shared_string>(), std::equal_to<shared_string>(),
            m_segment
                ->get_allocator<std::pair<shared_string, CustomListIterator> >());
}

std::shared_ptr<cocaine::framework::logger_t> worker::getLogger () {
  return m_log;
}

CustomList * worker::getList() {
  return m_SharedList;
}

CustomHashTable * worker::getHash() {
  return m_SharedHashTable;
}

void on_get::on_chunk(const char *chunk, size_t size) {

  std::shared_ptr<cocaine::framework::logger_t> logger = parent().getLogger();
  
  CustomList *SharedList = parent().getList();
  CustomHashTable *SharedHashTable = parent().getHash();

  COCAINE_LOG_INFO(logger, "Processing on_chunk callback");



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

  /*  struct shm_remove {
    shm_remove() {
      boost::interprocess::shared_memory_object::remove("SharedMemory");
    }
    ~shm_remove() {
      boost::interprocess::shared_memory_object::remove("SharedMemory");
    }
  } remover;
  */
  struct mutex_remove {
    mutex_remove() { boost::interprocess::named_mutex::remove("SharedMutex"); }
    ~mutex_remove() { boost::interprocess::named_mutex::remove("SharedMutex"); }

  } mutex_remover;
  
  auto received_data = context.data().to_string();

  msgpack::unpacked msg;    // includes memory pool and deserialized object
  msgpack::unpack(&msg, received_data.c_str(), received_data.size());
  msgpack::object obj = msg.get();

  std::vector<std::string> result;
  obj.convert(&result);

  auto key = result.at(0);//context.data().to_string();
  auto received_size = atol(result.at(1).c_str());
  
  output = "Received size " + std::to_string (received_size);
  
  COCAINE_LOG_DEBUG(logger, output.c_str());
    


  boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create,
                                         "SharedMutex");

  /*  boost::interprocess::managed_shared_memory segment(
      boost::interprocess::open_or_create, "SharedMemory", 65536);


  char_allocator c_allocator(segment.get_segment_manager());

  */


  // If the type is mismatched, it throws msgpack::type_error.
  // obj.as<int>();  // type is mismatched, msgpack::type_error is thrown


    //////CHECK THAT SIZE IS 2!!!!






  //auto a = cocaine::framework::unpack<std::vector<std::string>>(chunk, size);


  //  void_allocator alloc_inst(segment.get_segment_manager());

  //CustomList *SharedList;

  {
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(
    mutex);

    //SharedList =
    //  segment.find_or_construct<CustomList>("SharedList")(alloc_inst);
    ////////////////////////////////////////////////

        output = "Starting size " + std::to_string(SharedList->size());

	  COCAINE_LOG_DEBUG(logger, output.c_str());

    /*    CustomHashTable *SharedHashTable =

        segment.find_or_construct<CustomHashTable>("SharedHashTable")(
            3, boost::hash<shared_string>(), std::equal_to<shared_string>(),
            segment
	    .get_allocator<std::pair<shared_string, CustomListIterator> >());*/
    //////////////////////////////////

    //char_allocator c_allocator(segment.get_segment_manager());
    //string_allocator s_allocator(segment.get_segment_manager());

    shared_string sh_string = key.c_str();

    struct timeval tv;
    gettimeofday(&tv, NULL);

    double time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;

    auto found_it = SharedHashTable->find(sh_string);
    CustomList::iterator node;

    if (found_it != SharedHashTable->end()){
      node = found_it->second;

    output = "Original size: "+ std::to_string(node->size_element) + " Received size: " + std::to_string (received_size);

    COCAINE_LOG_DEBUG(logger, output);}

    if (found_it == SharedHashTable->end() || node->size_element != received_size) {

      if (found_it != SharedHashTable->end()){
	COCAINE_LOG_DEBUG(logger, "Element has changed. Recaching");

	SharedList->erase(node);
	COCAINE_LOG_DEBUG(logger, "Element has changed. Recaching");
	SharedHashTable->erase(found_it);
      }

      COCAINE_LOG_DEBUG(logger, "Adding a new element");

      ListNode node;
      node.TimeStamp = time_in_mill;
      node.key = result.at(0).c_str();
      node.count = 1;
      node.size_element = received_size;

      SharedList->push_back(node);

      auto it = SharedList->end();
      --it;

      SharedHashTable->insert(
          std::pair<shared_string, CustomList::iterator>(sh_string, it));
    } else {

      COCAINE_LOG_DEBUG(logger, "Element found in the cache");
      //      auto first = found_it->first;

      //auto converted = first.c_str();

      gettimeofday(&tv, NULL);

      time_in_mill =
          (tv.tv_sec) * 1000 +
          (tv.tv_usec) / 1000; // convert tv_sec & tv_usec to millisecond

      node->TimeStamp = time_in_mill;
      ++node->count;

      output = "Hit: " + std::to_string(node->count);
      COCAINE_LOG_INFO(logger, output.c_str());

      output = "Date: " + std::to_string(node->TimeStamp);
      COCAINE_LOG_INFO(logger, output.c_str());

      SharedList->splice(SharedList->end(), *SharedList, node);
    }

    output = "Ending size " + std::to_string(SharedList->size());
    COCAINE_LOG_INFO(logger, output.c_str());
    usleep(1000000);

    auto start = clock();
    auto end = clock();
    /*		while (((double) (end - start)) / CLOCKS_PER_SEC < 1)
   	  {
   	    end = clock();
   	    }
   	*/

    output = "Ending size " + std::to_string(SharedList->size());

    COCAINE_LOG_DEBUG(logger, output.c_str());

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
