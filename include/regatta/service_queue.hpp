#ifndef __SERVICE_QUEUE__
#define __SERVICE_QUEUE__

#include <functional>
#include <type_traits>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>

namespace regatta {
	
	/*
	 * Description: queue_t is an enum type passed as a template argument to
	 * service_queue. If ENQUEUE is passed, then the programmer has 
	 * access to the enqueue function and the service threads have access to 
	 * dequeue. If DEQUEUE is passed, then the programmer has access 
	 * to the dequeue function and the service threads have access to enqueue.
	 * If NOQUEUE is passed, then the programmer has no access to
	 * queuing functions but the constructor takes an extra function pointer.
	 */
	enum queue_t { 
		ENQUEUE, 
		DEQUEUE,
		NOQUEUE,
	};
	
	/*
	 * Class template service_queue has two template parameters:
	 *     T - the service_queue element type
	 *     Q - the queue type of the associated service_queue
	 * 
	 * Description: service_queue provides asynchronous queuing functionality.
	 * Programmer has access to either enqueue or dequeue while the internal
	 * service_queue threads have access to the other function. With this 
	 * scheme, threads asynchronously processes the queue as programmer 
	 * performs other work. All service_queue functions are thread safe.
	 * service_queue has RAII functionality as it safely shuts-down on 
	 * destruction. At shutdown, a service_queue will process all remaining 
	 * elements before joining service threads.
	 */
	template <class T, queue_t Q>
	class service_queue {
	public:
		/*
		 * Function service_queue constructor has three parameters:
		 *     queue_size > 0 - specifies the max size of the internal queue
		 *     thread_count > 0 - specifies the number of internal threads 
		 *     service - the function threads execute to create or utilize T
		 */
		template<queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE != NOQUEUE, bool> = true>
		service_queue(std::size_t queue_size, std::size_t thread_count, 
		std::function<void(T&)> service) : max_(queue_size) 
		{		
			threads_.reserve(thread_count);
			while(threads_.size() < thread_count) {
				if constexpr(Q == DEQUEUE) {
					threads_.emplace_back(std::thread(
					[this, service]{ enqueue_service(service); }));
				} else {
					threads_.emplace_back(std::thread(
					[this, service]{ dequeue_service(service); }));
				}
			}	
		}
		
		/*
		 * Function service_queue constructor has three parameters:
		 *     queue_size > 0 - specifies the max size of the internal queue
		 *     enq_count > 0 - specifies the number of threads enqueue-ing T
		 *     deq_count > 0 - specifies the number of threads dequeue-ing T
		 *     service_enq - the function threads execute to create T elements
		 *     service_deq - the function threads execute to utilize T elements
		 */
		template<queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE == NOQUEUE, bool> = true>
		service_queue(std::size_t queue_size, std::size_t enq_count, 
		std::size_t deq_count, std::function<void(T&)> service_enq, 
		std::function<void(T&)> service_deq) : max_(queue_size) 
		{	
			threads_.reserve(enq_count + deq_count);
			for(; enq_count; --enq_count) {
				threads_.emplace_back(std::thread(
				[this, service_enq]{ enqueue_service(service_enq); }));
			}
			for(; deq_count; --deq_count) {
				threads_.emplace_back(std::thread(
				[this, service_deq]{ dequeue_service(service_deq); }));
			}	
		}
		
		// service_queue is not copyable or movable
        service_queue(const service_queue&) = delete;
        service_queue& operator=(const service_queue&) = delete;
        service_queue(service_queue&&) = delete;
        service_queue& operator=(service_queue&&) = delete;
		
		// RAII destructor performs shutdown and join
		~service_queue() 
		{
			shutdown();
			for(std::thread &t : threads_) {
                if(t.joinable()) { 
					t.join(); 
				}
            }
		}
		
		/*
		 * Function enqueue has a single parameter:
		 *     element - the element to be moved into the service_queue
		 *
		 * Description: enqueue is a blocking function. If the internal queue
		 * is full, the call will block. enqueue returns a bool representing
		 * whether the service_queue is open or not. enqueue will fail to 
		 * insert the element if the service_queue is shutdown at the time it
		 * has mutually exclusive access to the internal queue.
		 */
		template<queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE == ENQUEUE, bool> = true>
		bool enqueue(T& element) 
		{
			std::unique_lock<std::mutex> lock(m_);
            while(open_ && queue_.size() == max_) { 
				enqcv_.wait(lock); 
			}
            
			if(open_) {
				queue_.push(std::move(element));
				deqcv_.notify_one();
			}
			
			return open_;
		}
		
		/*
		 * Function dequeue has a single parameter:
		 *     element - the location to move the dequeued element into
		 *
		 * Description: dequeue is a blocking function. If the internal queue
		 * is empty, the call will block. dequeue returns a bool representing
		 * whether the service_queue is open or not. dequeue will fail to
		 * remove an element if the service_queue is shutdown and empty at the 
		 * time it has mutually exclusive access to the internal queue.
		 */
		template<queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE == DEQUEUE, bool> = true>
		bool dequeue(T& element) 
		{
			std::unique_lock<std::mutex> lock(m_);
			while(open_ && queue_.empty()) { 
				deqcv_.wait(lock); 
			}
			
			if(!queue_.empty()) {
				element = std::move(queue_.front());
				queue_.pop();
				enqcv_.notify_one();
			}
			
			return open_ || !queue_.empty();
		}
		
		/*
		 * Description: shutdown places the service_queue into a closed state.
		 * No further elements will be queued after this call returns including
		 * elements blocked and waiting to enqueue. Elements may be dequeued 
		 * from the service_queue while it is shutdown. Once shutdown, a 
		 * service_queue can not be re-opened. 
		 */
		void shutdown() 
		{
			std::unique_lock<std::mutex> lock(m_);
			if(open_) {
				open_ = false;
				deqcv_.notify_all();
				enqcv_.notify_all();
			}
		}
		
	private:
		/*
		 * Function enqueue_service has one function parameter:
		 *     service - the function threads invoke to utilize T
		 * 
		 * Description: enqueue_service enqueues a new T element into the 
		 * the service_queue. This service is utilized by ENQUEUE
		 * and NOQUEUE. The core while loops terminate when their
		 * calls to service return false indicating the service_queue has been
		 * shutdown (and empty for dequeue calls).
		 */
		template<queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE != DEQUEUE, bool> = true>
		void dequeue_service(std::function<void(T&)> service) 
		{
			T data;
			while(dequeue(data)) { 
				service(data); 
			}
		}
		
		/*
		 * Function enqueue_service has one function parameter:
		 *     service - the function threads invoke to utilize T
		 * 
		 * Description: enqueue_service enqueues a new T element into the 
		 * the service_queue. This service is utilized by DEQUEUE
		 * and NOQUEUE. The core while loops terminate when their
		 * calls to service return false indicating the service_queue has been
		 * shutdown (and empty for dequeue calls).
		 */
		template<queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE != ENQUEUE, bool> = true>
		void enqueue_service(std::function<void(T&)> service) 
		{
			T data;
			do { 
				service(data); 
			} while(enqueue(data));
		}
		
		// Private redefinition of enqueue instantiated for DEQUEUE
		template<queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE != ENQUEUE, bool> = true>
		bool enqueue(T& element) 
		{
			std::unique_lock<std::mutex> lock(m_);	
            while(open_ && queue_.size() == max_) { 
				enqcv_.wait(lock); 
			}
            
			if(open_) {
				queue_.push(std::move(element));
				deqcv_.notify_one();
			}
			
			return open_;
		}
		
		// Private redefinition of dequeue instantiated for ENQUEUE
		template<queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE != DEQUEUE, bool> = true>
		bool dequeue(T& element) 
		{
			std::unique_lock<std::mutex> lock(m_); 
			while(open_ && queue_.empty()) { 
				deqcv_.wait(lock); 
			}
			
			if(!queue_.empty()) {
				element = std::move(queue_.front());
				queue_.pop();
				enqcv_.notify_one();
			}
			
			return open_ || !queue_.empty();
		}
		
		mutable std::condition_variable enqcv_, deqcv_;
		mutable std::mutex m_;
		std::queue<T> queue_;
		std::vector<std::thread> threads_;
		std::size_t max_;
		bool open_ = true;
	};

}

#endif
