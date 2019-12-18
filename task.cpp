#include <vxWorks.h>
#include <taskLib.h>
#include <intLib.h>
#include <sysLib.h>
#include <stdexcept>
#include "./vwpp.h"

using namespace vwpp;

// This is the entry point for all tasks created with the Task
// class. It is a static function, so it has no object instance. By
// convention, the object context for the new task is passed as the
// first argument.

void Task::initTask(Task* tt)
{
    try {
	tt->taskEntry();
    }
    catch (...) {
	::taskSuspend(0);
    }
    tt->id = ERROR;
}

Task::Task() : id(ERROR)
{
}

Task::~Task() NOTHROW_IMPL
{
    if (ERROR != id)
	::taskDelete(id);
}

// Delays the current task. The delay is given in milliseconds. If,
// after scaling the delay into timer ticks, the result is zero, the
// task will delay for 1 tick.

void Task::delay(int const dly) const
{
    ::taskDelay(ms_to_tick(dly));
}

void Task::run(char const* const name, unsigned char const pri, int const ss)
{
    if (ERROR == id) {
	if (ERROR == (id = ::taskSpawn(const_cast<char*>(name), pri,VX_FP_TASK,
				       ss, reinterpret_cast<FUNCPTR>(initTask),
				       reinterpret_cast<int>(this), 0, 0, 0, 0,
				       0, 0, 0, 0, 0)))
	    throw std::runtime_error("couldn't start new task");
    } else
	throw std::logic_error("task is already started");
}

int Task::priority() const
{
    int tmp;

    if (LIKELY(OK == ::taskPriorityGet(id, &tmp)))
	return tmp;
    throw std::runtime_error("couldn't get task priority");
}

bool Task::isReady() const
{
    return TRUE == ::taskIsReady(id);
}

bool Task::isSuspended() const
{
    return TRUE == ::taskIsSuspended(id);
}

bool Task::isValid() const
{
    return OK == ::taskIdVerify(id);
}

char const* Task::name() const
{
    return ::taskName(id);
}

void Task::suspend() const
{
    ::taskSuspend(id);
}

void Task::resume() const
{
    ::taskResume(id);
}

#ifndef NDEBUG

STATUS vwppTestTasks()
{
    SchedLock lock();

    return OK;
}

#endif
