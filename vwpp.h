// $Id: vwpp.h,v 1.24 2015/05/21 18:28:45 neswold Exp $

#ifndef __VWPP_H
#define __VWPP_H

#if VX_VERSION > 55
#define LIKELY(x)	__builtin_expect(!!(x), 1)
#define UNLIKELY(x)	__builtin_expect(!!(x), 0)
#else
#define LIKELY(x)	(x)
#define UNLIKELY(x)	(x)
#endif

#include <string>
#include <stdexcept>

// The throw() specification should actually produce *more* code
// (because the compiler needs to wrap the function with a try/catch
// to make sure it doesn't throw anything), but the compilers included
// with VxWorks 6.1, and earlier, seem to produce better code.
// Compilers after 6.1 support the GNU attribute which guarantees to
// produce tight code.

#if VX_VERSION > 61
#define NOTHROW		__attribute__((nothrow))
#define NOTHROW_IMPL
#else
#define NOTHROW		throw()
#define NOTHROW_IMPL	throw()
#endif

// These forward-declared structures and functions are found in
// <semLib.h>. We don't want to require users of this library to
// include VxWorks' headers, if they don't need to.

#ifndef __INCsemLibh

struct semaphore;

extern "C" {
    int semDelete(struct semaphore*) NOTHROW;
    int semFlush(struct semaphore*) NOTHROW;
    int semGive(struct semaphore*) NOTHROW;
    int semTake(struct semaphore*, int) NOTHROW;
}

#endif

// These forward-declared structures and functions are found in
// <msgQLib.h>. We don't want to require users of this library to
// include VxWorks' headers, if they don't need to.

#ifndef __INCmsgQLibh

struct msg_q;

#endif

// These forward-declared structures and functions are found in
// <intLib.h>. We don't want to require users of this library to
// include VxWorks' headers, if they don't need to.

#ifndef __INCintLibh

extern "C" {
    int intLock() NOTHROW;
    int intUnlock(int) NOTHROW;
}

#endif

// These forward-declared structures and functions are found in
// <taskLib.h>. We don't want to require users of this library to
// include VxWorks' headers, if they don't need to.

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
    // destructor virtual. This is intentional since no valid code
    // will be upcasting to an Uncopyable. By keeping it non-virtual,
    // these classes become more lightweight and have more
    // opportunities of being inlined.

    class Uncopyable {
	Uncopyable(Uncopyable const&);
	Uncopyable& operator=(Uncopyable const&);

     public:
	Uncopyable() NOTHROW {}
	~Uncopyable() NOTHROW {}
    };

    // A class derived from NoHeap can never exist on the heap.

    class NoHeap {
	void* operator new(size_t);
	void* operator new[](size_t);

     public:
	NoHeap() NOTHROW {}
	~NoHeap() NOTHROW {}
    };

    // Base class for semaphore-like resources.

    class SemaphoreBase : private Uncopyable, private NoHeap {
	semaphore* const res;
	SemaphoreBase();

     protected:
	void acquire(int);
	void release() NOTHROW { ::semGive(res); }

	explicit SemaphoreBase(semaphore* const tmp) : res(tmp) {}

     public:
	virtual ~SemaphoreBase() NOTHROW { ::semDelete(res); }
    };

    // Mutexes are mutual exclusion locks. They can be locked multiple
    // times by the same process. They also support priority inversion
    // and, while a task owns the mutex, it cannot be deleted.

    class Mutex : public SemaphoreBase {
	template <Mutex& mtx> friend class Lock;
	template <Mutex& mtx> friend class Unlock;
	template <class T, Mutex T::*PMtx> friend class PMLock;
	template <class T, Mutex T::*PMtx> friend class PMUnlock;

     public:
	template <Mutex& mtx>
	class Lock : private Uncopyable, private NoHeap {
	 public:
	    explicit Lock(int tmo = -1) { mtx.acquire(tmo); }
	    ~Lock() NOTHROW { mtx.release(); }
	};

	template <Mutex& mtx>
	class Unlock : private Uncopyable, private NoHeap {
	 public:
	    explicit Unlock(Lock<mtx> const&) { mtx.release(); }
	    ~Unlock() NOTHROW { mtx.acquire(-1); }
	};

	template <class T, Mutex T::*pmtx>
	class PMLock : private Uncopyable, private NoHeap {
	    Mutex& mtx;

	 public:
	    explicit PMLock(T* const obj, int const tmo = -1) :
		mtx(obj->*pmtx)
	    { mtx.acquire(tmo); }

	    ~PMLock() NOTHROW { mtx.release(); }
	};
	
	template <class T, Mutex T::*pmtx>
	class PMUnlock : private Uncopyable, private NoHeap {
	    Mutex& mtx;

	 public:
	    explicit PMUnlock(T* const obj, PMLock<T, pmtx> const&) :
		mtx(obj->*pmtx)
	    { mtx.release(); }

	    ~PMUnlock() NOTHROW { mtx.acquire(-1); }
	};

	Mutex();
    };

    template <class T, Mutex& mtx>
    class MVar : private Uncopyable, private NoHeap {
	T value;

     public:
	MVar() : value(T()) {}
	explicit MVar(T const& o) : value(o) {}

	T operator()(Mutex::Lock<mtx>&) const { return value; }
	void operator()(Mutex::Lock<mtx>&, T const& o) { value = o; }
    };

    // This class uses the "Counting" Semaphore as its underlying
    // implementation. Pending tasks are queued by priority.

    class CountingSemaphore : public SemaphoreBase {
     public:
	explicit CountingSemaphore(int = 1);

	template <CountingSemaphore& sem>
	class Lock : private Uncopyable, private NoHeap {
	 public:
	    explicit Lock(int tmo = -1) { sem.acquire(tmo); }
	    ~Lock() NOTHROW { sem.release(); }
	};

	template <class T, CountingSemaphore T::*PSem>
	class PMLock : private Uncopyable, private NoHeap {
	    CountingSemaphore& sem;

	 public:
	    explicit PMLock(T* const obj, int const tmo = -1) :
		sem(obj->*PSem)
	    { sem.acquire(tmo); }

	    ~PMLock() NOTHROW { sem.release(); }
	};
    };

    // IntLock objects will disable interrupts during their lifetime.
    // Due to VxWorks' semantics, if a task that created the IntLock
    // gets blocked, interrupts will get re-enabled (which is a very
    // useful thing!)

    class IntLock : private Uncopyable, private NoHeap {
	int const oldValue;

     public:
	IntLock() NOTHROW : oldValue(::intLock()) {}
	~IntLock() NOTHROW { ::intUnlock(oldValue); }
    };

    // SchedLock objects will disable the task scheduler during the
    // object's lifetime.

    class SchedLock : private Uncopyable, private NoHeap {
     public:
	SchedLock() NOTHROW { ::taskLock(); }
	~SchedLock() NOTHROW { ::taskUnlock(); }
    };

    // ProtLock objects prevent the task that created it from being
    // destroyed during the object's lifetime.

    class ProtLock : private Uncopyable, private NoHeap {
     public:
	ProtLock() NOTHROW { ::taskSafe(); }
	~ProtLock() NOTHROW { ::taskUnsafe(); }
    };

    template <unsigned Prio>
    class AbsPriority : private Uncopyable, private NoHeap {
	int oldValue;

     public:
	AbsPriority()
	{
	    int const id = ::taskIdSelf();

	    if (LIKELY(OK == ::taskPriorityGet(id, &oldValue))) {
		if (UNLIKELY(ERROR == ::taskPrioritySet(id, Prio)))
		    throw std::runtime_error("couldn't set task priority");
	    } else
		throw std::runtime_error("couldn't get current task priority");
	}

	~AbsPriority() NOTHROW { ::taskPrioritySet(::taskIdSelf(), oldValue); }
    };

    template <int Prio>
    class RelPriority : private Uncopyable, private NoHeap {
	int oldValue;

     public:
	RelPriority()
	{
	    int const id = ::taskIdSelf();

	    if (LIKELY(OK == ::taskPriorityGet(id, &oldValue))) {
		int const nv = oldValue - Prio;
		int const np =
		    (UNLIKELY(nv < 0) ? 0 : (UNLIKELY(nv > 255) ? 255 : nv));

		if (UNLIKELY(ERROR == ::taskPrioritySet(id, np)))
		    throw std::runtime_error("couldn't set task priority");
	    } else
		throw std::runtime_error("couldn't get current task priority");
	}

	~RelPriority() NOTHROW { ::taskPrioritySet(::taskIdSelf(), oldValue); }
    };

    // This class is used by tasks to signal each other when something
    // of interest has happened.

    class Event : private Uncopyable, private NoHeap {
	semaphore* id;

     public:
	Event();
	~Event() NOTHROW;

	bool wait(int = -1);
	void wakeOne() NOTHROW { ::semGive(id); }
	void wakeAll() NOTHROW { ::semFlush(id); }
    };

    template <Mutex& mtx>
    class CondVar : private Uncopyable, private NoHeap {
	Event ev;

     public:
	bool wait(Mutex::Lock<mtx> const& lock, int tmo = -1)
	{
	    SchedLock sLock;
	    Mutex::Unlock<mtx> uLock(lock);

	    return ev.wait(tmo);
	}

	void signal() NOTHROW { ev.wakeOne(); }
    };

    template <class T, Mutex T::*pmtx>
    class PMCondVar : private Uncopyable, private NoHeap {
	Event ev;

     public:
	bool wait(T* const obj, Mutex::PMLock<T, pmtx> const& lock,
		  int tmo = -1)
	{
	    SchedLock sLock;
	    Mutex::PMUnlock<T, pmtx> uLock(obj, lock);

	    return ev.wait(tmo);
	}

	void signal() NOTHROW { ev.wakeOne(); }
    };

    // **** This section defines several classes that support the
    // **** message queue interface provided by VxWorks.

    class QueueBase : private Uncopyable {
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

    class Task : private Uncopyable {
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
	bool isValid() const;
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
