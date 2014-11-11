/**
Asier Gutierrez <asierguti@gmail.com>

Yandex 2014

**/
#include "worker.hpp"

worker::worker(cocaine::framework::dispatch_t &d)
    : m_lo(file_logger("/dev/stderr", DNET_LOG_ERROR)) {
  m_log = d.service_manager()->get_system_logger();
  COCAINE_LOG_INFO(m_log, "Starting the LRU worker");

  m_storage = d.service_manager()
      ->get_service<cocaine::framework::storage_service_t>("storage");

  d.on<on_get>("get", *this);
  d.on<on_put>("put", *this);
  d.on<flush>("flush", *this);

  m_TotalSize = 0;

  init();
}

void worker::init() {
  COCAINE_LOG_INFO(m_log, "Initializing or grabbing the shared memory");

  Json::Value root;
  std::ifstream file("config.json");
  file >> root;

  int memory_size = root.get("shared_memory", "").asInt();
  std::string log = "Shared memory: " + std::to_string(memory_size);

  COCAINE_LOG_INFO(m_log, log);

  m_hit_limit = root.get("hit_limit", "").asInt();

  log = "Hit limit: " + std::to_string(m_hit_limit);

  COCAINE_LOG_INFO(m_log, log);

  m_MaxSize = root.get("max_limit", "").asInt64();

  m_segment.reset(new boost::interprocess::managed_shared_memory(
      boost::interprocess::open_or_create, "SharedMemory",  //open_or_create
      memory_size * 1024 * 1024));

  char_allocator c_allocator(m_segment->get_segment_manager());
  void_allocator alloc_inst(m_segment->get_segment_manager());

  m_SharedList =
      m_segment->find_or_construct<CustomList>("SharedList")(alloc_inst);

  m_SharedHashTable =
      m_segment->find_or_construct<CustomHashTable>("SharedHashTable")(
          3, boost::hash<shared_string>(), std::equal_to<shared_string>(),
          m_segment
              ->get_allocator<std::pair<shared_string, CustomListIterator> >());

  //////////////////////////

  m_remote_port = root.get("remote_port", "").asInt();
  m_remote_address = root.get("remote_address", "").asString();

  for (int i = 0; i < root.get("group", "").size(); ++i) {
    m_groups.emplace_back(root.get("group", "")[i].asInt());
  }

  using namespace ioremap::elliptics;

  m_no = node(logger(m_lo, blackhole::log::attributes_t()));
  m_no.add_remote(address(m_remote_address, m_remote_port));

  session sess(m_no);
  sess.set_exceptions_policy(0x00);

  //  std::vector <int> groups;

  sess.set_groups(m_groups);

  auto async_result = sess.read_data(std::string("metadata"), 0, 0);
  async_result.wait();

  if (!async_result.error()) {
    std::string elliptics_data = async_result.get_one().file().to_string();

    Json::Reader reader;
    reader.parse(elliptics_data, root, false);
    //  elliptics_data >> root;

    int size = root.get("List", "").size();
    for (int i = 0; i < size; ++i) {
      ListNode node;

      shared_string sh_string;

      node.TimeStamp = root.get("List", "")[i][0].asInt64();
      sh_string = shared_string(root.get("List", "")[i][1].asString().c_str());
      node.key = sh_string;
      node.count = root.get("List", "")[i][2].asInt();
      node.size_element = root.get("List", "")[i][3].asInt64();

      m_SharedList->push_back(node);

      auto it = m_SharedList->end();
      --it;

      m_SharedHashTable->insert(
          std::pair<shared_string, CustomList::iterator>(sh_string, it));
    }
  }

  COCAINE_LOG_DEBUG(m_log, "Finished initializing the LRU");
}

std::shared_ptr<cocaine::framework::logger_t> worker::getLogger() {
  return m_log;
}

CustomList *worker::getList() { return m_SharedList; }

CustomHashTable *worker::getHash() { return m_SharedHashTable; }

void on_get::on_chunk(const char *chunk, size_t size) {

  using namespace ioremap::elliptics;

  std::shared_ptr<cocaine::framework::logger_t> client_logger =
      parent().getLogger();

  CustomList *SharedList = parent().getList();
  CustomHashTable *SharedHashTable = parent().getHash();

  COCAINE_LOG_INFO(client_logger, "Processing on_chunk callback");

  ioremap::elliptics::data_pointer data_owner =
      ioremap::elliptics::data_pointer::copy(chunk, size);
  ioremap::elliptics::error_info error;
  ioremap::elliptics::exec_context context =
      ioremap::elliptics::exec_context::parse(data_owner, &error);

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

  output = "Received size " + received_data.size();

  COCAINE_LOG_DEBUG(client_logger, output.c_str());

  boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create,
                                         "SharedMutex");
  {
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(
        mutex);

    output = "Starting size " + std::to_string(SharedList->size());
    COCAINE_LOG_DEBUG(client_logger, output.c_str());

    shared_string sh_string = dnet_dump_id_str(context.src_id()->id);

    output = "Received key: ";
    output.append(dnet_dump_id_str(context.src_id()->id));
    COCAINE_LOG_DEBUG(client_logger, output.c_str());

    struct timeval tv;
    gettimeofday(&tv, NULL);

    double time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;

    auto found_it = SharedHashTable->find(sh_string);
    CustomList::iterator node;

    if (found_it != SharedHashTable->end()) {
      node = found_it->second;

      output = "Original size: " + std::to_string(node->size_element) +
               " Received size: " + std::to_string(received_data.size());

      COCAINE_LOG_DEBUG(client_logger, output);
    }

    if (found_it == SharedHashTable->end()) {  // ||
      //        node->size_element != received_data.size()) {

      COCAINE_LOG_DEBUG(client_logger, "Adding a new element");

      ListNode node;
      node.TimeStamp = time_in_mill;
      node.key = sh_string;
      node.count = 1;
      node.size_element = 0;

      SharedList->push_back(node);

      auto it = SharedList->end();
      --it;

      SharedHashTable->insert(
          std::pair<shared_string, CustomList::iterator>(sh_string, it));

      response()->write("0", 1);  //0

    } else {

      COCAINE_LOG_DEBUG(client_logger, "Element found in the cache");

      gettimeofday(&tv, NULL);

      time_in_mill =
          (tv.tv_sec) * 1000 +
          (tv.tv_usec) / 1000;  // convert tv_sec & tv_usec to millisecond

      if (node->count > parent().m_hit_limit) {
        COCAINE_LOG_DEBUG(client_logger, "Hit count limit");

        session sess(parent().m_no);
        sess.set_exceptions_policy(0x00);

        sess.set_groups(parent().m_groups);

        auto async_result = sess.read_data(key(*context.src_id()), 0, 0);
        async_result.wait();

        if (async_result.error()) {
          COCAINE_LOG_DEBUG(
              client_logger,
              "Data not found in elliptics. Need to get it somehow");
          response()->write("1", 1);
        } else {
          COCAINE_LOG_DEBUG(client_logger, "Sending the data from elliptics");
          std::string elliptics_data =
              async_result.get_one().file().to_string();

          response()->write(elliptics_data.c_str(), elliptics_data.size());
        }

        //response()->write("aaaa",4);

      } else {
        response()->write("0", 1);

      }
      node->count++;

      SharedList->splice(SharedList->end(), *SharedList, node);
    }
  }

}

void on_put::on_chunk(const char *chunk, size_t size) {

  using namespace ioremap::elliptics;

  std::shared_ptr<cocaine::framework::logger_t> logger = parent().getLogger();

  CustomList *SharedList = parent().getList();
  CustomHashTable *SharedHashTable = parent().getHash();

  COCAINE_LOG_DEBUG(logger, "Processing on_chunk callback on_put");

  ioremap::elliptics::data_pointer data_owner =
      ioremap::elliptics::data_pointer::copy(chunk, size);
  ioremap::elliptics::error_info error;
  ioremap::elliptics::exec_context context =
      ioremap::elliptics::exec_context::parse(data_owner, &error);

  std::string output;

  struct mutex_remove {
    mutex_remove() { boost::interprocess::named_mutex::remove("SharedMutex"); }
    ~mutex_remove() { boost::interprocess::named_mutex::remove("SharedMutex"); }

  } mutex_remover;

  std::string received_data = context.data().to_string();

  boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create,
                                         "SharedMutex");
  {
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(
        mutex);

    output = "Received size " + std::to_string(SharedList->size());

    COCAINE_LOG_DEBUG(logger, output.c_str());

    shared_string sh_string =
        dnet_dump_id_str(context.src_id()->id);  //received_data.c_str();

    struct timeval tv;
    gettimeofday(&tv, NULL);

    double time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;

    auto found_it = SharedHashTable->find(sh_string);
    CustomList::iterator node;

    if (found_it != SharedHashTable->end()) {

      auto received_size = std::stol(received_data);
      node = found_it->second;

      auto new_size = parent().m_TotalSize - node->size_element + received_size;

      if (new_size > parent().m_MaxSize) {
        auto to_remove = SharedList->begin();
        auto index_to_remove = SharedHashTable->find(to_remove->key);

        using namespace ioremap::elliptics;

        session sess(parent().m_no);
        sess.set_exceptions_policy(0x00);

        sess.set_groups(parent().m_groups);

        sess.remove(ioremap::elliptics::key(to_remove->key.c_str()));

        SharedHashTable->erase(index_to_remove);
        SharedList->erase(to_remove);
      }

      node = found_it->second;

      COCAINE_LOG_DEBUG(logger, "Adding metadata");

      node->TimeStamp = time_in_mill;
      node->count++;
      node->size_element = received_size;

      SharedList->splice(SharedList->end(), *SharedList, node);

      gettimeofday(&tv, NULL);

      time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
      response()->write(" ", 1);

      parent().m_TotalSize = new_size;

    } else {
      response()->write(" ", 1);
    }
  }
}

void flush::on_chunk(const char *chunk, size_t size) {

  std::shared_ptr<cocaine::framework::logger_t> logger = parent().getLogger();

  auto begin = time(NULL);

  std::string data;

  Json::Value root;

  struct mutex_remove {
    mutex_remove() { boost::interprocess::named_mutex::remove("SharedMutex"); }
    ~mutex_remove() { boost::interprocess::named_mutex::remove("SharedMutex"); }

  } mutex_remover;

  boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create,
                                         "SharedMutex");
  {
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(
        mutex);

    CustomList *SharedList = parent().getList();

    CustomListIterator it = SharedList->begin();

    while (it++ != SharedList->end()) {
      Json::Value jsonVect;
      jsonVect.append(static_cast<Json::Value::Int64>(it->TimeStamp));
      jsonVect.append(it->key.c_str());
      jsonVect.append(it->count);
      jsonVect.append(static_cast<Json::Value::Int64>(it->size_element));
      root["List"].append(jsonVect);

      std::string log11 = "key ";
      log11.append(it->key.c_str());

      COCAINE_LOG_INFO(logger, log11);
    }
  }

  Json::StyledWriter writer;

  data = writer.write(root);

  using namespace ioremap::elliptics;

  session sess(parent().m_no);
  sess.set_groups(parent().m_groups);
  sess.set_exceptions_policy(0x01);

  try {
    sess.write_data(std::string("metadata"), data, 0).wait();
  }
  catch (error & e) {
    std::cerr << "Error while write execution: " << e.error_message();
    return;
  }

  std::string log = "Time elapsed " + std::to_string(time(NULL) - begin);

  COCAINE_LOG_INFO(logger, log);

  response()->write(log.c_str(), log.size());

}

int main(int argc, char *argv[]) {
  return cocaine::framework::run<worker>(argc, argv);
}
