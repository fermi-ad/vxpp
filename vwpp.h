// $Id: vwpp.h,v 1.24 2015/05/21 18:28:45 neswold Exp $

#ifndef __VWPP_H
#define __VWPP_H

#include <string>
#include <stdexcept>

#if VX_VERSION > 61
#define NOTHROW		__attribute__((nothrow))
#define NOTHROW_IMPL
#else
#define NOTHROW		throw()
#define NOTHROW_IMPL	throw()
#endif

#ifndef __INCsemLibh

// These forward-declared structures and functions are found in
// <semLib.h>. We don't want to require users of this library to
// include VxWorks' headers, if they don't need to.

struct semaphore;

extern "C" {
    int semDelete(struct semaphore*) NOTHROW;
    int semFlush(struct semaphore*) NOTHROW;
    int semGive(struct semaphore*) NOTHROW;
    int semTake(struct semaphore*, int) NOTHROW;
}

#endif

#ifndef __INCmsgQLibh

struct msg_q;

#endif

#ifndef __INCintLibh

extern "C" {
    int intLock() NOTHROW;
    int intUnlock(int) NOTHROW;
}

#endif

#ifndef __INCtaskLibh

extern "C" {
    int taskIdSelf() NOTHROW;
    int taskLock() NOTHROW;
    int taskPriorityGet(int, int*) NOTHROW;
    int taskPrioritySet(int, int) NOTHROW;
    int taskSafe() NOTHROW;
    int taskUnlock() NOTHROW;
    int taskUnsafe() NOTHROW;
}

#endif

// All identifiers of this module are located in the vwpp name space.

namespace vwpp {

    // Any class derived from Uncopyable will cause a compile-time
    // error if the code tries to copy an instance the class. Even
    // though this is only used as a base class, we aren't making the
    // destructor virtual. This is intention since no valid code will
    // be upcasting to an Uncopyable. By keeping it non-virtual, these
    // classes become more lightweight and have more opportunities of
    // being inlined.

    class Uncopyable {
	Uncopyable(Uncopyable const&);
	Uncopyable& operator=(Uncopyable const&);

     public:
	Uncopyable() NOTHROW {}
	~Uncopyable() NOTHROW {}
    };

    // Forward declaration so resources can use it to declare who
    // their friends are.

    template <class Resource> class Hold;

    // Base class for semaphore-like resources. Again, no methods are
    // virtual to help the compile inline operations on them.

    class SemaphoreBase : public Uncopyable {
	semaphore* res;
	SemaphoreBase();

	friend class Hold<SemaphoreBase>;

	void acquire(int);
	void release() NOTHROW { semGive(res); }

     protected:
	explicit SemaphoreBase(semaphore* tmp) : res(tmp) {}

     public:
	~SemaphoreBase() NOTHROW { semDelete(res); }
    };

    // Mutexes are mutual exclusion locks. They can be locked multiple
    // times by the same process. They also support priority inversion
    // and, while a task owns the mutex, it cannot be deleted.

    class Mutex : public SemaphoreBase {
     public:
	Mutex();
    };

    // This class uses the "Counting" Semaphore as its underlying
    // implementation. Pending tasks are queued by priority.

    class CountingSemaphore : public SemaphoreBase {
     public:
	explicit CountingSemaphore(int = 1);
    };

    // A Hold object holds a resource for its lifetime.

    template <class Resource>
    class Hold : public Uncopyable {
	Hold();

	Resource& res;

     public:
	explicit Hold(Resource& r, int tmo = -1) : res(r) { res.acquire(tmo); }
	~Hold() NOTHROW { res.release(); }
    };

    typedef Hold<SemaphoreBase> SemLock;

    // IntLock objects will disable interrupts during their lifetime.
    // Due to VxWorks' semantics, if a task that created the IntLock
    // gets blocked, interrupts will get re-enabled (which is a very
    // useful thing!)

    class IntLock : public Uncopyable {
	int oldValue;

     public:
	IntLock() NOTHROW : oldValue(intLock()) {}
	~IntLock() NOTHROW { intUnlock(oldValue); }
    };

    // SchedLock objects will disable the task scheduler during the
    // object's lifetime.

    class SchedLock : public Uncopyable {
     public:
	SchedLock() NOTHROW { taskLock(); }
	~SchedLock() NOTHROW { taskUnlock(); }
    };

    // ProtLock objects prevent the task that created it from being
    // destroyed during the object's lifetime.

    class ProtLock : public Uncopyable {
     public:
	ProtLock() NOTHROW { taskSafe(); }
	~ProtLock() NOTHROW { taskUnsafe(); }
    };

    template <unsigned Prio>
    class AbsPriority : public Uncopyable {
	int oldValue;

     public:
	AbsPriority()
	{
	    int const id = taskIdSelf();

	    if (OK == taskPriorityGet(id, &oldValue)) {
		if (ERROR == taskPrioritySet(id, Prio))
		    throw std::runtime_error("couldn't set task priority");
	    } else
		throw std::runtime_error("couldn't get current task priority");
	}

	~AbsPriority() NOTHROW { taskPrioritySet(taskIdSelf(), oldValue); }
    };

    template <int RelPrio>
    class RelPriority : public Uncopyable {
	int oldValue;

     public:
	RelPriority()
	{
	    int const id = taskIdSelf();

	    if (OK == taskPriorityGet(id, &oldValue)) {
		int const nv = std::max(std::min(oldValue - RelPrio, 255), 0);

		if (ERROR == taskPrioritySet(id, nv))
		    throw std::runtime_error("couldn't set task priority");
	    } else
		throw std::runtime_error("couldn't get current task priority");
	}

	~RelPriority() NOTHROW { taskPrioritySet(taskIdSelf(), oldValue); }
    };

    // This class is used by tasks to signal each other when something
    // of interest has happened.

    class Event : public Uncopyable {
	semaphore* id;

     public:
	Event();
	~Event() NOTHROW;

	bool wait(int = -1);
	void wakeOne() NOTHROW { semGive(id); }
	void wakeAll() NOTHROW { semFlush(id); }
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
	virtual ~QueueBase() NOTHROW;

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
     private:
	int id;

	static void initTask(Task*);

     protected:
	virtual void taskEntry() = 0;

	void delay(int) const;
	void yieldCpu() const { delay(0); }

     public:
	Task();
	virtual ~Task() NOTHROW;

	bool isReady() const;
	bool isSuspended() const;
	char const* name() const;

	int priority() const;
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
