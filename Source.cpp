#include<thread>
#include<utility>
#include<stdexcept>
#include<vector>

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
// (of type pointer - to - a function - taking - no - parameters - and - returning - a - background_task - object)
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





/******************************************/
/* 2.1.4 Running threads in the background */
/******************************************/

// Detached threads are often called daemon threads
void do_background_work();
std::thread t(do_background_work);
t.detach();

// Detaching a thread to handle other documents in a text editor
void edit_document(std::string const& filename)
{
    open_document_and_display_gui(filename);
    while (!done_editing())
    {
        user_command cmd = get_user_input();
        if (cmd.type == open_new_document)
        {
            std::string const new_name = get_filename_from_user();
            std::thread t(edit_document, new_name);
            t.detach();
        }
        else
        {
            process_user_input(cmd);
        }
    }
}





/**********************************************/
/* 2.2 Passing arguments to a thread function */
/**********************************************/

// Arguments are copied to the internal storage of a new thread and passed as rvalues to a callable object
void f(int i, std::string const& s);
std::thread t(f, 3, "Hello"); // char const* is copied to the new thread. In the context of new thread it's converted to string

// Content of the pointer copied to thread may be destroyed before used
void f(int i, std::string const& s);
void oops(int some_param)
{
    char buffer[1024];
    sprintf(buffer, "%i", some_param);
    std::thread t(f, 3, buffer); // char const* copied to the new thread
    t.detach(); // buffer may still being converted to string
} // buffer is destroyed before converted to string

// Solution - cast to string before copying to new thread
void f(int i, std::string const& s);
void not_oops(int some_param)
{
    char buffer[1024];
    sprintf(buffer, "%i", some_param);
    std::thread(f, 3, std::string(buffer)); // string is created and copied to the thread's context
    t.detach(); // buffer may be destroyed safely
}

// If function expects a non-const reference, it will fail, because thread passes rvalue to it
void update_data_for_widget(widget_id w, wdiget_data& data);
void oops_again(widget_id w)
{
    widget_data data;
    std::thread t(update_data_for_widget, w, data); // Failes
    display_status();
    t.join();
    process_widget_data(data);
}

// Solution - std::ref
std::thread t(update_data_for_widget, w, std::ref(data)); // Copies reference to data to thread t. thread t updates data in place

// You can pass a member function pointer and object pointer to call obj.member()
class X
{
public:
    void do_lengthy_work(Arg arg);
};
X my_x;
Arg my_arg;
std::thread t(&X::do_lengthy_work, &my_x, my_arg); // Calls my_x.do_lengthy_work(my_arg);

// If object can only be moved but not copied, std::move must be used
void process_big_object(std::unique_ptr<big_object>);
std::unique_ptr<big_object> p(new big_object);
p->prepare_data(42);
std::thread t(process_big_object, std::move(p)); // Ownership of big_object is transferred inside thread t





/******************************************/
/* 2.3 Transferring ownership of a thread */
/******************************************/

// std::thread is movable but not copyable
void some_function();
void some_other_function();
std::thread t1(some_function); // t1 attached to thread runnung some_function
std::thread t2 = std::move(t1); // t2 attached to thread running some_function; t1 is not attached to a thread
t1 = std::thread(some_other_function); // t1 <-> some_other_function
std::thread t3; // t3 <-> none
t3 = std::move(t2); // t3 <-> some_function; t2 <-> none; t1 <-> some_other_function
t1 = std::move(t3); // t1 <-> some_function; some_other_function terminated; t2 <-> none

// std::thread is moveable hence it can be returned from a function
std::thread f()
{
    void some_function();
    return std::thread(some_function);
}

std::thread g()
{
    void some_other_function(int);
    std::thread t(some_other_function, 42);
    return t;
}

// Thread can be passed into function as rvalue
void f(std::thread t);
void g()
{
    void some_function();
    f(std::thread(some_function));
    std::thread t(some_function);
    f(std::move(t));
}

// Move allows to build a thread_guard class and have it take ownership of the thread
// Now thread_guard can't outlive the thread it was referencing
// No one else can join or detach thread
// Listing 2.6 scoped_thread and example usage
class scoped_thread
{
    std::thread t; // instead of std::thread& t
public:
    explicit scoped_thread(std::thread t_) : t(std::move(t_))
    {
        if (!t.joinable())
        {
            throw std::logic_error("No thread");
        }

    }
    ~scoped_thread()
    {
        t.join();
    }
    scoped_thread(const scoped_thread&) = delete;
    scoped_thread& operator=(const scoped_thread&) = delete;
};
struct func; // See line 44
void f()
{
    int some_local_state;
    scoped_thread t{ std::thread(func(some_local_state)) };
    do_something_in_current_thread();
} // scoped_thread t is being destroyed here. It joins the owned thread in destructor
 
// Listing 2.7 A joining_thread class
class joining_thread
{
    std::thread t;
public:
    joining_thread() noexcept = default; // default constructor
    template<typename Callable, typename ... Args>
    explicit joining_thread(Callable&& func, Args&& ... args) : // thread-like constructor accepting callable and args
        t(std::forward<Callable>(func), std::forward<Args>(args)...)
    {}
    explicit joining_thread(std::thread t_) noexcept: // Constructor accepting a thread
        t(std::move(t_))
    {}
    joining_thread(joining_thread&& other) noexcept: // Move constructor
        t(std::move(other.t))
    {}
    joining_thread& operator=(joining_thread&& other) noexcept // Move assignment
    {
        if (joinable())
        {
            join();
        }
        t = std::move(other.t);
        return *this;
    }
    joining_thread& operator=(std::thread other) noexcept // Assignment from thread. Why?
    {
        if (joinable())
        {
            join();
        }
        t = std::move(other);
        return *this;
    }
    ~joining_thread()
    {
        if (joinable())
        {
            join();
        }
    }
    void swap(joining_thread& other) noexcept
    {
        t.swap(other.t);
    }
    std::thread::id get_id() const noexcept
    {
        return t.get_id();
    }
    bool joinable() const noexcept
    {
        return t.joinable();
    }
    void join()
    {
        t.join();
    }
    void detach()
    {
        t.detach();
    }
    std::thread& as_thread() noexcept
    {
        return t;
    }
    const std::thread& as_thread() const noexcept
    {
        return t;
    }
};

// Move semantics allows to make a vector of threads
// Listing 2.8 Spawns some threads and waits for them to finish
void do_work(unsigned id);
void f()
{
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < 20; i++)
    {
        threads.emplace_back(do_work, i);
        for (auto& entry : threads)
        {
            entry.join();
        }
    }
}





/*************************************************/
/* 2.4 Choosing the number of threads at runtime */
/*************************************************/
