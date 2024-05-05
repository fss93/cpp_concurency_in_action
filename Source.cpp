#include<thread>

/****************************/
/* 2.1.1 Launching a thread */
/****************************/

// Basic launching. Every thread takes a function (or callable object) as a first parameter.
// This function is an entry point for the thread.
// main function is the entry point for main thread
void do_some_work();
std::thread my_thread(do_some_work);

// Example of launching a thread with a callable object
class background_task
{
public:
    void operator() () const
    {
        do_something();
        do_something_else();
    }
};
background_task f;
std::thread my_thread(f);

// Avoid "C++’s most vexing parse"
std::thread my_thread(background_task());
// declares a my_thread function that takes a single parameter
// (of type pointer - to - afunction - taking - no - parameters - and -returning - a - background_task - object)
// and returns a std::thread object, rather than launching a new thread

std::thread my_thread((background_task())); // Correct
std::thread my_thread{ background_task() }; // Correct

// Lambda expressions are useful for thread declarations
std::thread my_thread([] {
    do_something();
    do_something_else();
});

// Beware of dangling references
struct func
{
    int& i;
    func(int& i_) : i(i_){}
    void operator() ()
    {
        for (unsigned j = 0; j < 1000000; j++)
        {
            do_something(i); // Potential access to dangling reference
        }
    }
};
void oops()
{
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread my_thread(my_func);
    my_thread.detach(); // Don't wait for thread to finish 
} // some_local_state has been destructed but my_thread still uses it





/**********************************************/
/* 2.1.3 Waiting in exceptional circumstances */
/**********************************************/

// Using join in catch block is verbose. It's better to use RAII concept
// Resource Aqcuisition Is Initialization
class thread_guard
{
    std::thread& t;
public:
    explicit thread_guard(std::thread& t_) : t(t_) {}
    ~thread_guard()
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    thread_guard(const thread_guard&) = delete;
    thread_guard& operator=(const thread_guard&) = delete;
};
struct func; // Defined above
void f()
{
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread t(my_func);
    thread_guard g(t);
    do_something_in_current_thread(); // When f reaches the end or throws an exception, its objects are being destroyed.
}
