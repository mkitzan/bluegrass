#ifndef __BLUEGRASS_SERVICE__
#define __BLUEGRASS_SERVICE__

#include <functional>
#include <type_traits>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>

namespace bluegrass {
	
	/*
	 * Description: queue_t is an enum type passed as a template argument to
	 * service. If ENQUEUE is passed, then the programmer has 
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
	 * Class template service has two template parameters:
	 *	 T - the service element type
	 *	 Q - the queue type of the associated service
	 * 
	 * Description: service provides asynchronous queuing functionality.
	 * Programmer has access to either enqueue or dequeue while the internal
	 * service threads have access to the other function. With this 
	 * scheme, threads asynchronously processes the queue as programmer 
	 * performs other work. All service functions are thread safe.
	 * service has RAII functionality as it safely shuts-down on 
	 * destruction. At shutdown, a service will process all remaining 
	 * elements before joining service threads.
	 */
	template <class T, queue_t Q>
	class service {
	public:
		/*
		 * Function service constructor has three parameters:
		 *	 routine - the function threads execute to create or utilize T
		 *	 thread_count > 0 - specifies the number of internal threads 
		 *	 queue_size > 0 - specifies the max size of the internal queue
		 */
		template <queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE != NOQUEUE, bool> = true>
		service(std::function<void(T&)> routine, size_t thread_count, size_t queue_size) : 
			max_ {queue_size} 
		{		
			// reserved vector will ensure stable threads
			threads_.reserve(thread_count);

			while (threads_.size() < thread_count) {
				if constexpr (Q == DEQUEUE) {
					threads_.emplace_back(std::thread(
					[this, routine] { enqueue(routine); }));
				} else {
					threads_.emplace_back(std::thread(
					[this, routine] { dequeue(routine); }));
				}
			}	
		}
		
		/*
		 * Function service constructor has three parameters:
		 *	 enq_routine - the function threads execute to create T elements
		 *	 deq_routine - the function threads execute to utilize T elements
		 *	 enq_count > 0 - specifies the number of threads enqueue-ing T
		 *	 deq_count > 0 - specifies the number of threads dequeue-ing T
		 *	 queue_size > 0 - specifies the max size of the internal queue
		 */
		template <queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE == NOQUEUE, bool> = true>
		service(
			std::function<void(T&)> enq_routine, std::function<void(T&)> deq_routine,
			size_t enq_count, size_t deq_count, size_t queue_size) : 
			max_ {queue_size} 
		{	
			// reserved vector will ensure stable threads
			threads_.reserve(enq_count + deq_count);

			for (; enq_count; --enq_count) {
				threads_.emplace_back(std::thread(
				[this, enq_routine] { enqueue(enq_routine); }));
			}
			for (; deq_count; --deq_count) {
				threads_.emplace_back(std::thread(
				[this, deq_routine] { dequeue(deq_routine); }));
			}	
		}
		
		// service is not copyable or movable
		service(const service&) = delete;
		service(service&&) = delete;
		service& operator=(const service&) = delete;
		service& operator=(service&&) = delete;

		// RAII destructor performs shutdown and join
		~service() 
		{
			shutdown();
			for (std::thread &t : threads_) {
				if (t.joinable()) { 
					t.join(); 
				}
			}
		}
		
		/*
		 * Function enqueue has a single parameter:
		 *	 element - the element to be moved into the service
		 *
		 * Description: enqueue is a blocking function. If the internal queue
		 * is full, the call will block. enqueue returns a bool representing
		 * whether the service is open or not. enqueue will fail to 
		 * insert the element if the service is shutdown at the time it
		 * has mutually exclusive access to the internal queue.
		 */
		template <queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE == ENQUEUE, bool> = true>
		bool enqueue(T& element) 
		{
			std::unique_lock<std::mutex> lock(m_);
			while (open_ && queue_.size() == max_) { 
				enqcv_.wait(lock); 
			}
			
			if (open_) {
				queue_.push(std::move(element));
				deqcv_.notify_one();
			}
			
			return open_;
		}
		
		/*
		 * Function dequeue has a single parameter:
		 *	 element - the location to move the dequeued element into
		 *
		 * Description: dequeue is a blocking function. If the internal queue
		 * is empty, the call will block. dequeue returns a bool representing
		 * whether the service is open or not. dequeue will fail to
		 * remove an element if the service is shutdown and empty at the 
		 * time it has mutually exclusive access to the internal queue.
		 */
		template <queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE == DEQUEUE, bool> = true>
		bool dequeue(T& element) 
		{
			std::unique_lock<std::mutex> lock(m_);
			while (open_ && queue_.empty()) { 
				deqcv_.wait(lock); 
			}
			
			if (!queue_.empty()) {
				element = std::move(queue_.front());
				queue_.pop();
				enqcv_.notify_one();
			}
			
			return open_ || !queue_.empty();
		}
		
		/*
		 * Description: shutdown places the service into a closed state.
		 * No further elements will be queued after this call returns including
		 * elements blocked and waiting to enqueue. Elements may be dequeued 
		 * from the service while it is shutdown. Once shutdown, a 
		 * service can not be re-opened. 
		 */
		void shutdown() 
		{
			std::unique_lock<std::mutex> lock(m_);
			if (open_) {
				open_ = false;
				deqcv_.notify_all();
				enqcv_.notify_all();
			}
		}
		
	private:
		/*
		 * Function enqueue has one function parameter:
		 *	 routine - the function threads invoke to utilize T
		 * 
		 * Description: enqueue enqueues a new T element into the 
		 * the queue. This service is utilized by ENQUEUE
		 * and NOQUEUE. The core while loops terminate when their
		 * calls to service return false indicating the service has been
		 * shutdown (and empty for dequeue calls).
		 */
		template <queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE != DEQUEUE, bool> = true>
		void dequeue(std::function<void(T&)> routine) 
		{
			T data;
			while (dequeue(data)) { 
				routine(data); 
			}
		}
		
		/*
		 * Function enqueue has one function parameter:
		 *	 routine - the function threads invoke to utilize T
		 * 
		 * Description: enqueue enqueues a new T element into the 
		 * the queue. This service is utilized by DEQUEUE
		 * and NOQUEUE. The core while loops terminate when their
		 * calls to service return false indicating the service has been
		 * shutdown (and empty for dequeue calls).
		 */
		template <queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE != ENQUEUE, bool> = true>
		void enqueue(std::function<void(T&)> routine) 
		{
			T data;
			do { 
				routine(data); 
			} while (enqueue(data));
		}
		
		// Private redefinition of enqueue instantiated for DEQUEUE
		template <queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE != ENQUEUE, bool> = true>
		bool enqueue(T& element) 
		{
			std::unique_lock<std::mutex> lock(m_);	
			while (open_ && queue_.size() == max_) { 
				enqcv_.wait(lock); 
			}
			
			if (open_) {
				queue_.push(std::move(element));
				deqcv_.notify_one();
			}
			
			return open_;
		}
		
		// Private redefinition of dequeue instantiated for ENQUEUE
		template <queue_t Q_TYPE = Q, 
		typename std::enable_if_t<Q_TYPE != DEQUEUE, bool> = true>
		bool dequeue(T& element) 
		{
			std::unique_lock<std::mutex> lock(m_); 
			while (open_ && queue_.empty()) { 
				deqcv_.wait(lock); 
			}
			
			if (!queue_.empty()) {
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
		bool open_ {true};
	};

} // namespace bluegrass 

#endif
