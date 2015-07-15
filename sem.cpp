// $Id: sem.cpp,v 1.12 2014/08/01 15:59:47 neswold Exp $

#include <vxWorks.h>
#include <intLib.h>
#include <semLib.h>
#include <sysLib.h>
#include <cassert>
#include "vwpp.h"

using namespace vwpp;

Mutex::Mutex() :
    SemaphoreBase(semMCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE |
			     SEM_INVERSION_SAFE))
{
}

// CountingSemaphore constructor -- Pass the SEM_ID of a counting
// semaphore to the base class.

CountingSemaphore::CountingSemaphore(int initialCount) :
    SemaphoreBase(semCCreate(SEM_Q_PRIORITY, initialCount))
{
}

void SemaphoreBase::acquire(int tmo)
{
    if (ERROR == semTake(res, ms_to_tick(tmo)))
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

#ifndef NDEBUG

#include <iostream>

// Regression test for semaphore support.

STATUS vwppTestSemaphores()
{
    try {
	// Shouldn't ever throw an exception.

	Mutex a;

	// Create a scope for the Lock object's lifetime.

	{
	    // Lock the mutex. This shouldn't ever throw an exception
	    // because the mutex isn't accessible to any other task.

	    Lock lock(a);

	    // Create another scope of a Lock object.

	    {
		// This shouldn't fail because Mutexes can be locked
		// several times by the same task.

		Lock lock(a, 60);

		return OK;
	    }
	}
    }
    catch (std::exception& e) {
	std::cerr << "vwppTestSemaphores() : caught unhandled exception : " <<
	    e.what() << std::endl;
	return ERROR;
    }
}

#endif
