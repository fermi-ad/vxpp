// $Id: sem.cpp,v 1.12 2014/08/01 15:59:47 neswold Exp $

#include <vxWorks.h>
#include <intLib.h>
#include <semLib.h>
#include <sysLib.h>
#include <stdexcept>
#include <cassert>
#include "vwpp.h"

using namespace vwpp;

// SemBase constructor -- sets the id to the passed semaphore
// pointer. In VxWorks, a pointer to a sempahore structure is
// typedef'ed as SEM_ID. If we were passed a bad SEM_ID, we throw an
// exception.

SemBase::SemBase(semaphore* ii) : id(ii)
{
    if (!id)
	throw std::bad_alloc();
}

// When any derived object gets destroyed, we need to destroy the
// semaphore object we own.

SemBase::~SemBase()
{
    semDelete(id);
}

// CountingSemaphore constructor -- Pass the SEM_ID of a counting
// semaphore to the base class.

CountingSemaphore::CountingSemaphore(int initialCount) :
    SemBase(semCCreate(SEM_Q_PRIORITY, initialCount))
{
}

// Mutex constructor -- Pass the SEM_ID of the mutex to the base
// class.

Mutex::Mutex() :
    SemBase(semMCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE))
{
}

void SemBase::lock(int tmo)
{
    if (ERROR == semTake(id, ms_to_tick(tmo)))
	switch (errno) {
	 case S_intLib_NOT_ISR_CALLABLE:
	    throw std::logic_error("couldn't lock semaphore -- inside "
				   "interrupt!");

	 case S_objLib_OBJ_ID_ERROR:
	    throw std::logic_error("couldn't lock semaphore -- bad handle");

	 case S_objLib_OBJ_UNAVAILABLE:
	    throw std::logic_error("couldn't lock semaphore -- unavailable");

	 case S_objLib_OBJ_TIMEOUT:
	    throw std::runtime_error("couldn't lock semaphore -- timeout");

	 default:
	    throw std::logic_error("couldn't lock semaphore -- unknown "
				   "reason");
	}
}

void SemBase::unlock()
{
    semGive(id);
}

Lockable::~Lockable()
{
}

// Lock constructor -- The whole purpose of this class is to tie the
// semTake() and semGive() period to the lifetime of the object. If we
// can't "take" the semaphore in the constructor, we throw an
// exception.

Lock::Lock(Lockable& ll, int tmo) :
    obj(ll)
{
    obj.lock(tmo);
}

// Lock destructor -- Give up the sempahore.

Lock::~Lock()
{
    obj.unlock();
}

// Event constructor -- Builds a binary semaphore used for
// synchronization.

Event::Event() :
    id(semBCreate(SEM_Q_PRIORITY, SEM_EMPTY))
{
    if (!id)
	throw std::bad_alloc();
}

Event::~Event()
{
    semDelete(id);
}

// Wait for an event to occur. The caller will be blocked until it
// gets signalled (by another task calling wakeOne() or wakeAll()) or
// until the timeout occurs. If the function is terminated by a
// timeout, it returns false, otherwise it returns true.

bool Event::wait(int tmo)
{
    if (ERROR == semTake(id, ms_to_tick(tmo)))
	switch (errno) {
	 case S_intLib_NOT_ISR_CALLABLE:
	 case S_objLib_OBJ_TIMEOUT:
	    return false;

	 case S_objLib_OBJ_ID_ERROR:
	    throw std::logic_error("couldn't lock semaphore -- bad handle");

	 case S_objLib_OBJ_UNAVAILABLE:
	    throw std::logic_error("couldn't lock semaphore -- unavailable");

	 default:
	    throw std::logic_error("couldn't lock semaphore -- unknown "
				   "reason");
	}
    return true;
}

// Wakes only one task that's currently blocked on the event. This
// method is safe to call from an interrupt routine.

void Event::wakeOne()
{
    semGive(id);
}

// Wakes all tasks that are blocked on the event. This method is safe
// to call from an interrupt routine.

void Event::wakeAll()
{
    semFlush(id);
}

#ifndef NDEBUG

#include <iostream>

// Regression test for semaphore support.

STATUS vwppTestSemaphores()
{
    try {
	Mutex a;

	{
	    Lock lock(a);

	    try {
		Lock lock(a, 60);
	    }
	    catch (std::runtime_error& e) {
		std::cout << e.what() << std::endl;
	    }
	}

	return OK;
    }
    catch (std::exception& e) {
	std::cerr << "caught unhandled exception : " << e.what() << std::endl;
	return ERROR;
    }
}

#endif
