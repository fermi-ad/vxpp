#if !defined(__VWPP_H)
#define __VWPP_H

// With our build environment, we have an awkward situation when our
// project's header files include each other. While building the
// project, we want to use the headers in the local directory because
// we can't guarantee the most up-to-date version is in the install
// area. The project's Makefile defines __BUILDING_VWPP so we know
// when it's being built and, therefore, can include the local version
// of the header.
//
// NOTE: The version number for vwpp_types.h needs to change when the
// project's version changes so the correct header gets used!

#ifdef __BUILDING_VWPP
#include "./vwpp_types.h"
#else
#include <vwpp_types-3.0.h>
#endif

// These macros emit assembly instructions which implement "barriers"
// used to force ordering.
//
// MEMORY_SYNC prevents the processor from performing load or store
// operations until all previous load and store operations complete.
//
// INSTRUCTION_SYNC prevents the processor from proceeding until all
// previous instructions have completed their execution (which
// includes load or store operations.)
//
// ALL_SYNC is the most expensive sync primitive (in terms of
// performance.) It prevents the CPU pipeline from doing loads/stores
// and instruction fetches until all previous activity is complete.

#if defined(PPC603) || defined(PPC604) || defined(PPC750) || defined(PPC7400)
#define	VXPP_MEMORY_SYNC	asm volatile ("eieio" ::: "memory")
#define	VXPP_INSTRUCTION_SYNC	asm volatile ("isync" ::: "memory")
#define	VXPP_ALL_SYNC		asm volatile ("sync" ::: "memory")

namespace vwpp {
    namespace v3_0 {
	inline void memory_sync() { asm volatile ("eieio" ::: "memory"); }
	inline void instruction_sync() { asm volatile ("isync" ::: "memory"); }
	inline void global_sync() { asm volatile ("sync" ::: "memory"); }
	inline void optimizer_barrier() { asm volatile ("" ::: "memory"); }
    };
};
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
    void intUnlock(int) NOTHROW;
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
    namespace v3_0 {

	class timeout_error : public std::runtime_error {
	 public:
	    explicit timeout_error(char const* msg = 0) :
		std::runtime_error(msg ? msg : "timeout obtaining resource") {}
	};

	class IntLock;

	// Base class for semaphore-like resources.

	class SemaphoreBase : private Uncopyable, private NoHeap {
	    semaphore* const res;
	    SemaphoreBase();

	 protected:
	    void acquire(int);
	    void release() NOTHROW { ::semGive(res); }

	    explicit SemaphoreBase(semaphore* const tmp) : res(tmp) {}

	 public:
	    virtual ~SemaphoreBase() NOTHROW
	    {
		::semTake(res, WAIT_FOREVER);
		::semDelete(res);
	    }
	};

	// Mutexes are mutual exclusion locks. They can be locked
	// multiple times by the same process. They also support
	// priority inversion and, while a task owns the mutex, it
	// cannot be deleted.

	class Mutex : public SemaphoreBase {
	    template <Mutex& mtx> friend class Lock;
	    template <Mutex& mtx> friend class LockWithInt;
	    template <Mutex& mtx> friend class Unlock;
	    template <typename T, Mutex T::*PMtx> friend class PMLock;
	    template <typename T, Mutex T::*PMtx> friend class PMLockWithInt;
	    template <typename T, Mutex T::*PMtx> friend class PMUnlock;

	 public:

	    // Mutex::Lock<> is used to hold ownership of a Mutex
	    // during the object's lifetime. The single parameter of
	    // the template is the mutex with which this lock is
	    // associated.

	    template <Mutex& mtx>
	    class Lock : private Uncopyable, private NoHeap {
	     public:
		explicit Lock(int tmo = -1) { mtx.acquire(tmo); }
		~Lock() NOTHROW { mtx.release(); }
	    };

	    // Mutex::LockWithInt<> is used to hold ownership of a
	    // Mutex during the object's lifetime along with disabling
	    // interrupts. The single parameter of the template is the
	    // mutex with which this lock is associated.

	    template <Mutex& mtx>
	    class LockWithInt : private Uncopyable, private NoHeap {
		int const prevVal;

	     public:
		explicit LockWithInt(int tmo = -1) : prevVal(intLock())
		{ mtx.acquire(tmo); }

		~LockWithInt() NOTHROW
		{
		    mtx.release();
		    intUnlock(prevVal);
		}

		operator Lock<mtx> const& () const
		{ return *reinterpret_cast<Lock<mtx> const*>(0); }

		operator IntLock const& () const
		{ return *reinterpret_cast<IntLock const*>(0); }
	    };

	    // Mutex::Unlock<> is used to release ownership of a Mutex
	    // during the object's lifetime. The single parameter of
	    // the template is the mutex with which this lock is
	    // associated. Since you can't release an un-owned mutex,
	    // the constructor requires you to prove you have a lock
	    // on the mutex, proving at compile-time that you already
	    // own it.

	    template <Mutex& mtx>
	    class Unlock : private Uncopyable, private NoHeap {
	     public:
		explicit Unlock(Lock<mtx>&) { mtx.release(); }
		~Unlock() NOTHROW { mtx.acquire(-1); }
	    };

	    // Mutex::PMLock<> is used to hold ownership of a Mutex
	    // residing in an object during the lock object's
	    // lifetime. The first template parameter is the class
	    // holding the mutex and the second selects the mutex
	    // field in the class.

	    template <typename T, Mutex T::*pmtx>
	    class PMLock : private Uncopyable, private NoHeap {
		Mutex& mtx;

	     public:
		explicit PMLock(T* const obj, int const tmo = -1) :
		    mtx(obj->*pmtx)
		{ mtx.acquire(tmo); }

		~PMLock() NOTHROW { mtx.release(); }
	    };

	    // Mutex::PMLockWithInt<> is used to hold ownership of a
	    // Mutex during the object's lifetime along with disabling
	    // interrupts.

	    template <typename T, Mutex T::*pmtx>
	    class PMLockWithInt : private Uncopyable, private NoHeap {
		Mutex& mtx;
		int const prevVal;

	     public:
		explicit PMLockWithInt(T* const obj, int tmo = -1) :
		    mtx(obj->*pmtx), prevVal(intLock())
		{ mtx.acquire(tmo); }

		~PMLockWithInt() NOTHROW
		{
		    mtx.release();
		    intUnlock(prevVal);
		}

		operator PMLock<T, pmtx> const& () const
		{ return *reinterpret_cast<PMLock<T, pmtx> const*>(0); }

		operator IntLock const& () const
		{ return *reinterpret_cast<IntLock const*>(0); }
	    };

	    // Mutex::PMUnlock<> is used to release ownership of a
	    // Mutex residing in an object during the unlock object's
	    // lifetime. The first template parameter is the class
	    // holding the mutex and the second parameter selects the
	    // mutex field in the class. Since you can't release an
	    // un-owned mutex, the constructor requires you to prove
	    // you have a lock on the mutex, proving at compile-time
	    // that you already own it.

	    template <typename T, Mutex T::*pmtx>
	    class PMUnlock : private Uncopyable, private NoHeap {
		Mutex& mtx;

	     public:
		explicit PMUnlock(T* const obj, PMLock<T, pmtx>&) :
		    mtx(obj->*pmtx)
		{ mtx.release(); }

		~PMUnlock() NOTHROW { mtx.acquire(-1); }
	    };

	    Mutex();
	};

	// Experimental class that associates a variable with a mutex.
	// Access to the variable is only allowed if a lock is
	// provided proving, at compile-time, you own the required
	// mutex.

	template <typename T, Mutex& mtx>
	class MVar : private Uncopyable, private NoHeap {
	    T value;

	 public:
	    MVar() : value(T()) {}
	    explicit MVar(T const& o) : value(o) {}

	    T operator()(Mutex::Lock<mtx>&) const { return value; }
	    void operator()(Mutex::Lock<mtx>&, T const& o) { value = o; }
	};

	// IntLock objects will disable interrupts during their
	// lifetime.  Due to VxWorks' semantics, if a task that
	// created the IntLock gets blocked, interrupts will get
	// re-enabled (which is a very useful thing!)

	class IntLock : private Uncopyable, private NoHeap {
	    int const oldValue;

	 public:
	    IntLock() NOTHROW : oldValue(::intLock()) {}
	    ~IntLock() NOTHROW { ::intUnlock(oldValue); }
	};

	// SchedLock objects will disable the task scheduler during
	// the object's lifetime.

	class SchedLock : private Uncopyable, private NoHeap {
	 public:
	    SchedLock() NOTHROW { ::taskLock(); }
	    ~SchedLock() NOTHROW { ::taskUnlock(); }
	};

	// ProtLock objects prevent the task that created it from
	// being destroyed during the object's lifetime.

	class ProtLock : private Uncopyable, private NoHeap {
	 public:
	    ProtLock() NOTHROW { ::taskSafe(); }
	    ~ProtLock() NOTHROW { ::taskUnsafe(); }
	};

	// This template allows us to detect serialization locks.

	template <typename T>
	struct DetermineLock {
	};

	template <>
	template <Mutex& mtx>
	struct DetermineLock<Mutex::Lock<mtx> > {
	    typedef Mutex::Lock<mtx> type;
	};

	template <>
	template <Mutex& mtx>
	struct DetermineLock<Mutex::LockWithInt<mtx> > {
	    typedef Mutex::LockWithInt<mtx> type;
	};

	template <>
	template <typename T, Mutex T::*pmtx>
	struct DetermineLock<Mutex::PMLock<T, pmtx> > {
	    typedef Mutex::PMLock<T, pmtx> type;
	};

	template <>
	template <typename T, Mutex T::*pmtx>
	struct DetermineLock<Mutex::PMLockWithInt<T, pmtx> > {
	    typedef Mutex::PMLockWithInt<T, pmtx> type;
	};

	template <>
	struct DetermineLock<IntLock> {
	    typedef IntLock type;
	};

	template <>
	struct DetermineLock<SchedLock> {
	    typedef SchedLock type;
	};

	template <>
	struct DetermineLock<ProtLock> {
	    typedef ProtLock type;
	};

	// During this object's lifetime, the priority of the task is
	// changed to 'Prio'.

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

	template <unsigned Prio>
	class MinAbsPriority : private Uncopyable, private NoHeap {
	    int oldValue;

	 public:
	    MinAbsPriority()
	    {
		int const id = ::taskIdSelf();

		if (LIKELY(OK == ::taskPriorityGet(id, &oldValue))) {
		    if (oldValue > Prio)
			if (UNLIKELY(ERROR == ::taskPrioritySet(id, Prio)))
			    throw std::runtime_error("couldn't set task priority");
		} else
		    throw std::runtime_error("couldn't get current task priority");
	    }

	    ~MinAbsPriority() NOTHROW
	    {
		::taskPrioritySet(::taskIdSelf(), oldValue);
	    }
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

	struct IntSignal;
	struct TaskSignal;

	class EventBase : private Uncopyable, private NoHeap {
	    semaphore* id;

	 protected:
	    EventBase();

	    bool _wait(int = -1);

	 public:
	    virtual ~EventBase();

	    void wakeOne() NOTHROW { ::semGive(id); }
	    void wakeAll() NOTHROW { ::semFlush(id); }
	};

	// Default Event template. We only support two types of
	// events. If the programmer tried to specify a third type,
	// they'll get a compiler error.

	template <typename T = IntSignal>
	class Event {};

	// This Event type is used for tasks to signal each other.

	template <>
	class Event<TaskSignal> : private EventBase {

	 public:
	    bool wait(int tmo = -1) { return _wait(tmo); }
	    void wakeOne() NOTHROW { EventBase::wakeOne(); }
	    void wakeAll() NOTHROW { EventBase::wakeAll(); }
	};

	// This Event type is used for an interrupt routine to signal
	// tasks.

	template <>
	class Event<IntSignal> : private EventBase {

	 public:
	    bool wait(IntLock&, int tmo = -1) { return _wait(tmo); }
	    void wakeOne() NOTHROW { EventBase::wakeOne(); }
	    void wakeAll() NOTHROW { EventBase::wakeAll(); }
	};

	// Non-POSIX implementation of conditional variables. This
	// version works with global mutexes.

	template <Mutex& mtx>
	class CondVar : private Uncopyable, private NoHeap {
	    Event<TaskSignal> ev;

	 public:
	    bool wait(Mutex::Lock<mtx>& lock, int tmo = -1)
	    {
		SchedLock sLock;
		Mutex::Unlock<mtx> uLock(lock);

		return ev.wait(tmo);
	    }

	    void signal(Mutex::Lock<mtx> const&) NOTHROW { ev.wakeOne(); }
	};

	// Non-POSIX implementation of conditional variables. This
	// version works inside classes.

	template <typename T, Mutex T::*pmtx>
	class PMCondVar : private Uncopyable, private NoHeap {
	    Event<TaskSignal> ev;

	 public:
	    bool wait(T* const obj, Mutex::PMLock<T, pmtx>& lock,
		      int tmo = -1)
	    {
		SchedLock sLock;
		Mutex::PMUnlock<T, pmtx> uLock(obj, lock);

		return ev.wait(tmo);
	    }

	    void signal(Mutex::PMLock<T, pmtx> const&) NOTHROW { ev.wakeOne(); }
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

	// This template version of the Queue is what applications
	// should use. It allows better type-safety than the
	// QueueBase.

	template <typename T, size_t nn>
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
};

#ifdef __BUILDING_VWPP
#include "./vwpp_memory.h"
#else
#include <vwpp_memory-3.0.h>
#endif

#endif

// Local Variables:
// mode: c++
// End:
