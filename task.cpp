// $Id: task.cpp,v 1.8 2014/10/22 16:04:51 neswold Exp $

#include <vxWorks.h>
#include <taskLib.h>
#include <intLib.h>
#include <sysLib.h>
#include <stdexcept>
#include <algorithm>
#include "vwpp.h"

using namespace vwpp;

Task::Interrupts Task::interrupt;
Task::Scheduler Task::scheduler;
Task::Safety Task::safety;

Task::Interrupts::Interrupts()
{
}

Task::Interrupts::~Interrupts()
{
}

Task::Scheduler::Scheduler()
{
}

Task::Scheduler::~Scheduler()
{
}

void Task::Interrupts::lock(int)
{
    oldValue = intLock();
}

void Task::Interrupts::unlock()
{
    intUnlock(oldValue);
}

void Task::Scheduler::lock(int)
{
    STATUS const result = taskLock();

    if (ERROR == result && errno != S_intLib_NOT_ISR_CALLABLE)
	throw std::runtime_error("can't taskLock()!?");
}

void Task::Scheduler::unlock()
{
    if (ERROR == taskUnlock())
	throw std::logic_error("can't manipulate task scheduler in interrupt "
			       "routine");
}

Task::Safety::Safety()
{
}

Task::Safety::~Safety()
{
}

void Task::Safety::lock(int)
{
    taskSafe();
}

void Task::Safety::unlock()
{
    taskUnsafe();
}

Task::AbsPriority::~AbsPriority()
{
}

void Task::AbsPriority::lock(int)
{
    int const id = taskIdSelf();

    if (OK == taskPriorityGet(id, &oldValue)) {
	if (ERROR == taskPrioritySet(id, newValue))
	    throw std::runtime_error("couldn't set task priority");
    } else
	throw std::runtime_error("couldn't get current task priority");
}

void Task::AbsPriority::unlock()
{
    taskPrioritySet(taskIdSelf(), oldValue);
}

Task::RelPriority::~RelPriority()
{
}

void Task::RelPriority::lock(int)
{
    int const id = taskIdSelf();

    if (OK == taskPriorityGet(id, &oldValue)) {
	int const nv = std::max(std::min(oldValue - newValue, 255), 0);

	if (ERROR == taskPrioritySet(id, nv))
	    throw std::runtime_error("couldn't set task priority");
    } else
	throw std::runtime_error("couldn't get current task priority");
}

void Task::RelPriority::unlock()
{
    taskPrioritySet(taskIdSelf(), oldValue);
}

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
	taskSuspend(0);
    }
    tt->id = ERROR;
}

Task::Task() : id(ERROR)
{
}

Task::~Task()
{
    Lock lock(Scheduler());

    if (ERROR != id) {
	taskDelete(id);
	id = ERROR;
    }
}

// Delays the current task. The delay is given in milliseconds. If,
// after scaling the delay into timer ticks, the result is zero, the
// task will delay for 1 tick.

void Task::delay(int const dly) const
{
    taskDelay(ms_to_tick(dly));
}

void Task::run(char const* name, unsigned char const pri, int const ss)
{
    if (ERROR == (id = taskSpawn(const_cast<char*>(name), pri, VX_FP_TASK, ss,
				 reinterpret_cast<FUNCPTR>(initTask),
				 reinterpret_cast<int>(this), 0, 0, 0, 0, 0, 0,
				 0, 0, 0)))
	throw std::runtime_error("couldn't start new task");
}

int Task::priority() const
{
    int tmp;

    if (OK == taskPriorityGet(id, &tmp))
	return tmp;
    throw std::runtime_error("couldn't get task priority");
}

void Task::priority(int pri)
{
    if (ERROR == taskPrioritySet(id, pri))
	throw std::runtime_error("couldn't set task priority");
}

bool Task::isReady() const
{
    return TRUE == taskIsReady(id);
}

bool Task::isSuspended() const
{
    return TRUE == taskIsSuspended(id);
}

char const* Task::name() const
{
    return ::taskName(id);
}

void Task::suspend() const
{
    taskSuspend(id);
}

void Task::resume() const
{
    taskResume(id);
}

#ifndef NDEBUG

STATUS vwppTestTasks()
{
    Lock lock(Task::Scheduler());

    return OK;
}

#endif
