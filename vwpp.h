// $Id: vwpp.h,v 1.24 2015/05/21 18:28:45 neswold Exp $

#ifndef __VWPP_H
#define __VWPP_H

#include <string>

struct semaphore;
struct msg_q;

namespace vwpp {

    class Uncopyable {
	Uncopyable(Uncopyable const&);
	Uncopyable& operator=(Uncopyable const&);

     public:
	Uncopyable() {}
	virtual ~Uncopyable() {}
    };

    class Lockable : public Uncopyable {
	friend class Lock;

	virtual void lock(int) = 0;
	virtual void unlock() = 0;

     public:
	~Lockable();
    };

    // **** This section defines several classes that use the
    // **** semaphore interface provided by VxWorks.

    // This is the base class for the sempahore classes. All VxWorks
    // mutex-like objects use the same SEM_ID to refer to them, so it
    // gets encapsulated in this base class. The whole purpose of this
    // class is to make sure the semaphore handle gets freed up
    // properly when the object is destroyed.

    class SemBase : public Lockable {
	semaphore* const id;

	SemBase();

	void lock(int);
	void unlock();

     public:
	explicit SemBase(semaphore*);
	virtual ~SemBase();
    };

    // This class uses the "Counting" Semaphore as its underlying
    // implementation. Pending tasks are queued by priority.

    class CountingSemaphore : public SemBase {
     public:
	explicit CountingSemaphore(int = 1);
    };

    // This class uses the Mutex semaphore as its underlying
    // implementation. Pending tasks are queued by priority and are
    // protected from deletion while owning the mutex. This class also
    // protects against "priority inversion".

    class Mutex : public SemBase {
     public:
	Mutex();
    };

    // Lock objects are objects that lock a semaphore during creation
    // and release them when destroyed.

    class Lock : public Uncopyable {
	Lockable& obj;

	Lock();

     public:
	explicit Lock(Lockable& ll, int tmo = -1) : obj(ll) { obj.lock(tmo); }
	~Lock() { obj.unlock(); }
    };

    // This class is used by tasks to signal each other when something
    // of interest has happened.

    class Event : public Uncopyable {
	semaphore* id;

     public:
	Event();
	~Event();

	bool wait(int = -1);
	void wakeOne();
	void wakeAll();
    };

    // **** This section defines several classes that support the
    // **** message queue interface provided by VxWorks.

    class QueueBase : public Uncopyable {
	msg_q* const id;

	bool _msg_send(void const*, size_t, int, int);

     protected:
	bool _pop_front(void*, size_t, int);
	bool _push_front(void const*, size_t, int);
	bool _push_back(void const*, size_t, int);

     public:
	QueueBase(size_t, size_t);
	virtual ~QueueBase();

	size_t total() const;
    };

    // This template version of the Queue is what applications should
    // use. It allows better type-safety than the QueueBase.

    template <class T, size_t nn>
    class Queue : public QueueBase {
     public:
	Queue() : QueueBase(sizeof(T), nn) {}

	inline bool pop_front(T& tt, int tmo = -1)
	{
	    return _pop_front(&tt, sizeof(T), tmo);
	}

	inline bool push_front(T const& tt, int tmo = -1)
	{
	    return _push_front(&tt, sizeof(T), tmo);
	}

	inline bool push_back(T const& tt, int tmo = -1)
	{
	    return _push_back(&tt, sizeof(T), tmo);
	}
    };

    // **** This section defines classes that implement the VxWorks
    // **** task interfaces.

    // This class is used to create VxWorks tasks.

    class Task : public Uncopyable {
     protected:
	class Interrupts : public Lockable {
	    int oldValue;

	    void lock(int);
	    void unlock();

	 public:
	    Interrupts();
	    ~Interrupts();
	};

	class Scheduler : public Lockable {
	    void lock(int);
	    void unlock();

	 public:
	    Scheduler();
	    ~Scheduler();
	};

	class Safety : public Lockable {
	    void lock(int);
	    void unlock();

	 public:
	    Safety();
	    ~Safety();
	};

	class AbsPriority : public Lockable {
	    int const newValue;
	    int oldValue;

	    void lock(int);
	    void unlock();

	 public:
	    explicit AbsPriority(int nv) : newValue(nv) {}
	    ~AbsPriority();
	};

	class RelPriority : public Lockable {
	    int const newValue;
	    int oldValue;

	    void lock(int);
	    void unlock();

	 public:
	    explicit RelPriority(int nv) : newValue(nv) {}
	    ~RelPriority();
	};

     private:
	int id;

	static void initTask(Task*);

     protected:
	virtual void taskEntry() = 0;

	void delay(int) const;
	void yieldCpu() const { delay(0); }

	static Scheduler scheduler;
	static Interrupts interrupt;
	static Safety safety;

     public:
	Task();
	virtual ~Task();

	bool isReady() const;
	bool isSuspended() const;
	char const* name() const;

	int priority() const;
	void priority(int);
	void resume() const;
	void suspend() const;

	void run(char const*, unsigned char, int);
    };

    // Other prototypes...

    int ms_to_tick(int);
};

#endif

// Local Variables:
// mode: c++
// End:
