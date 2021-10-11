#include <iostream>
#include <queue>
#include <sstream>
#include <time.h>
#include <omp.h>

int32_t thread_count, done_sending;

class Source_queue{
public:
  std::queue<char> src_queue;
  int32_t enqueued, dequeued;
  omp_lock_t front_mutex, rear_mutex;

  Source_queue(){
    enqueued = dequeued = 0;
    omp_init_lock(&front_mutex);
    omp_init_lock(&rear_mutex);
  }

  ~Source_queue(){
    omp_destroy_lock(&front_mutex);
    omp_destroy_lock(&rear_mutex);
  }

  int32_t get_size(){
    return enqueued - dequeued;
  }

  void enqueue(const char& src){
    src_queue.push(src);
    ++enqueued;
    std::stringstream console_output;
    console_output << ">>>>THREAD #" << omp_get_thread_num() << ": ENQUEUED(" << src << ")\n";
    std::cout << console_output.str();
  }

  void dequeue(){
    if(!src_queue.empty()){
      char rcv = src_queue.front();
      src_queue.pop();
      ++dequeued;
      std::stringstream console_output;
      console_output << "\t\t<<<<THREAD #" << omp_get_thread_num() << ": DEQUEUED(" << rcv << ")\n";
      std::cout << console_output.str();
    }
  }
};

void try_receive(Source_queue& queue){
  int32_t queue_size = queue.get_size();
  if(queue_size == 0)
    return;
  else if(queue_size == 1){
    omp_set_lock(&queue.front_mutex);
    queue.dequeue();
    omp_unset_lock(&queue.front_mutex);
  }
  else{
    queue.dequeue();
  }
}

void send_msg(Source_queue& queue){
  queue.enqueue(65 + (rand() % 26));
}

bool done(Source_queue& queue){
  int32_t queue_size = queue.get_size();
  if(queue_size == 0 && done_sending == (thread_count / 2))
    return true;
  return false;
}

//./main <threads> <sources>
int main(int argc, char* argv[]) {
  if(argc != 3) exit(1);
  thread_count = atoi(argv[1]);
  int32_t t_sources_count = atoi(argv[2]);
  omp_lock_t insert_mutex;
  omp_init_lock(&insert_mutex);
  srand(time(NULL));
  done_sending = 0;
  int32_t sources_per_thread = t_sources_count / (thread_count / 2);
  Source_queue global_queue;
#pragma omp parallel for
  for (int32_t thread_id = 0; thread_id < thread_count; ++thread_id) {
    if (thread_id < thread_count / 2) {
      int32_t local_spt = sources_per_thread;
      while(local_spt--){
        omp_set_lock(&insert_mutex);
        send_msg(global_queue);
        omp_unset_lock(&insert_mutex);
      }
      #pragma omp atomic
      ++done_sending;
    }
    else {
      while(!done(global_queue)){
        try_receive(global_queue);
      }
    }
  }
  omp_destroy_lock(&insert_mutex);
  return 0;
}
