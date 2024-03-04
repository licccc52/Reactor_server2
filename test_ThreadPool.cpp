#include "ThreadPool.h"
// 只为了测试
ThreadPool::ThreadPool(size_t threadnum):stop_(false) 
{
    // 启动threadnum个线程，每个线程将阻塞在条件变量上。
	for (size_t ii = 0; ii < threadnum; ii++)
    {
        // 用lambda函创建线程。
        /* [this]捕获当前类中的this指针, 让lambda表达式拥有和当前类成员函数同样的访问权限  */
		threads_.emplace_back([this] 
        { //lambda函数内部
            printf("In ThreadPool::ThreadPool, create thread(%ld).\n", syscall(SYS_gettid));     // 显示线程ID。

			while (stop_==false)
			{
                std::cout << __LINE__ << std::endl;
				std::function<void()> task;       //创建一个函数对象 ,用于存放出队的元素。

				{   // 锁作用域的开始。 ///////////////////////////////////
					std::unique_lock<std::mutex> lock(this->mutex_);
                    std::cout << __LINE__ << std::endl;

					// 等待生产者的条件变量。
                    //wait()函数有两个变量 , 锁, 谓词
                    //condition_variable 是 C++ 标准库中用于多线程同步的一种机制。condition_variable 的 wait() 函数有两个参数：
                    //( 1 )lock/mutex: 这是一个 unique_lock 或者 mutex 对象，用于在等待期间锁定，并在等待期间自动解锁。
                    //   wait() 函数在等待前会对这个锁进行解锁操作，然后将线程置于等待状态,在等待期间会被解锁。
                    
                    // 当条件满足时，线程会重新获得锁，并继续执行。 ,调用wait()函数时, lock/mutex会被锁定
                    //( 2 )predicate/condition: 这是一个条件，是一个函数或者函数对象。当调用 wait() 时，线程会等待直到这个条件成立为止。
                    //在等待期间，线程会释放锁()，进入等待状态，直到其他线程通过 notify_one() 或 notify_all() 发送信号，
                    //或者因为假唤醒而被唤醒，此时会重新检查条件是否满足，若满足则继续执行，否则继续等待。
					this->condition_.wait(lock, [this] 
                    { 
                        return ((this->stop_==true) || (this->taskqueue_.empty()==false)); // 非空或者 要求停止的时候返回
                    });
                    std::cout << __LINE__ << std::endl;

                    //跳出条件 : stop_标志为true, 或者 任务队列 taskqueue不为空, 
                    //wait()函数的条件成立, 如果是stop_标志 跳出 的; 需要判断是否还有剩余任务, 如果还有任务(任务队列不为空)

                    // 在线程池停止之前，如果队列中还有任务，执行完再退出。
					if ((this->stop_==true)&&(this->taskqueue_.empty()==true)) return;//如果满足这两个条件, 函数返回, lambda函数退出
                    std::cout << __LINE__ << std::endl;

                    // 出队一个任务。
					task = std::move(this->taskqueue_.front());
					this->taskqueue_.pop();
				}   // 锁作用域的结束。 ///////////////////////////////////
                std::cout << __LINE__ << std::endl;

                printf("thread is %ld.\n",syscall(SYS_gettid));
				task();  // 执行任务。
			}
		});
    }
}

void ThreadPool::addtask(std::function<void()> task)
{
/*  std::lock_guard<std::mutex>：这是C++标准库提供的一个RAII（资源获取即初始化）包装器。
    它用于自动管理互斥锁。当创建std::lock_guard对象时，它会锁定关联的互斥锁，
    在其作用域结束时（例如，在块或函数的结尾），它会自动释放锁。这确保了锁始终会得到适当的释放，即使发生异常。  */

    {   // 锁作用域的开始。 ///////////////////////////////////
    std::cout << "Thread " << syscall(SYS_gettid) << " is about to lock the mutex." << std::endl; 
        std::lock_guard<std::mutex> lock(mutex_);  //构造函数加锁
    //   lock(mutex_)：这一行创建了一个名为lock的std::lock_guard对象，并将其与互斥锁mutex_关联。
    //   因此，在创建std::lock_guard对象时，互斥锁被锁定, 在作用域结束时候, 自动释放锁             
         
        taskqueue_.push(task); //此时触发 构造函数中lambda函数的非空条件, wait()返回, 继续执行
    
    /*  taskqueue_.push(task)：这一行将task推送到名为taskqueue_的队列中。
        这个队列可能是一个用于存储需要由多个线程并发处理的任务的数据结构。
        使用互斥锁和std::lock_guard确保了在多个线程之间对taskqueue_的访问是同步的，防止竞态条件，确保安全的并发访问。*/
    }   // 锁作用域的结束, 调用析构函数解锁

    condition_.notify_one();   // 唤醒一个线程。
;

}

ThreadPool::~ThreadPool()
{
	stop_ = true;

	condition_.notify_all();  // 唤醒全部的线程。

    // 等待全部线程执行完任务后退出。
	for (std::thread &th : threads_) 
        th.join();
}

void show(int no, const std::string &name)
{

    printf("小哥哥们好，我是第%d号超级女生%s,",no,name.c_str());
    printf("thread is %ld.\n",syscall(SYS_gettid));
}

void test()
{
    printf("我有一只小小鸟, ");
    printf("thread is %ld.\n",syscall(SYS_gettid));
}

int main()
{
    ThreadPool threadpool(3);
    
    sleep(2);

    std::cout << std::endl << std::endl;

    std::string name="西施";
    threadpool.addtask(std::bind(show,8,name));
    sleep(1);

    threadpool.addtask(std::bind(test));
    sleep(1);

    // threadpool.addtask(std::bind([]{ printf("我是一只傻傻鸟。\n");}));
    sleep(1);
    std::cout << "全部代码执行结束, 执行析构函数" << std::endl;
}

// g++ -o test ThreadPool.cpp -lpthread