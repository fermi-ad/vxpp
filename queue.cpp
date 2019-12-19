#include <vxWorks.h>
#include <msgQLib.h>
#include <stdexcept>
#include <cassert>
#include "./vwpp.h"

using namespace vwpp::v2_7;

static void xlatErrno(int e)
{
    switch (e) {
     case S_objLib_OBJ_ID_ERROR:
	throw std::logic_error("invalid message queue ID");

     case S_objLib_OBJ_DELETED:
	throw std::logic_error("message queue has been deleted");

     case S_objLib_OBJ_UNAVAILABLE:
	throw std::logic_error("message queue is unavailable");

     case S_objLib_OBJ_TIMEOUT:
	throw std::runtime_error("time expired waiting for queue data");

     case S_msgQLib_INVALID_MSG_LENGTH:
	throw std::logic_error("bad message length specified");

     case S_msgQLib_NON_ZERO_TIMEOUT_AT_INT_LEVEL:
	throw std::logic_error("non zero timeout given the msg queue in "
			       "interrupt handler");

     default:
	throw std::logic_error("returned an usupported error code");
    }
}

QueueBase::QueueBase(size_t sz, size_t nn) :
    id(::msgQCreate(nn, sz, MSG_Q_PRIORITY))
{
    if (!id)
	throw std::bad_alloc();
}

QueueBase::~QueueBase() NOTHROW_IMPL
{
    ::msgQDelete(id);
}

size_t QueueBase::total() const
{
    int const result = ::msgQNumMsgs(id);

    if (LIKELY(ERROR != result))
	return static_cast<size_t>(result);

    throw std::runtime_error("queue couldn't return total");
}

bool QueueBase::_pop_front(void* buf, size_t nn, int tmo)
{
    int const result = ::msgQReceive(id, reinterpret_cast<char*>(buf), nn,
				     ms_to_tick(tmo));

    if (LIKELY(ERROR != result)) {
	if ((size_t) result < nn)
	    throw std::logic_error("too little data pulled from queue");
	return true;
    } else if (UNLIKELY(errno != S_objLib_OBJ_TIMEOUT))
	xlatErrno(errno);
    return false;
}

bool QueueBase::_msg_send(void const* buf, size_t nn, int tmo, int pri)
{
    int const result =
	::msgQSend(id, const_cast<char*>(reinterpret_cast<char const*>(buf)),
		   nn, ms_to_tick(tmo), pri);

    if (LIKELY(ERROR != result)) {
	if ((size_t) result < nn)
	    throw std::logic_error("too little data sent to queue");
	return true;
    } else if (UNLIKELY(errno != S_objLib_OBJ_TIMEOUT))
	xlatErrno(errno);
    return false;
}

bool QueueBase::_push_front(void const* buf, size_t nn, int tmo)
{
    return _msg_send(buf, nn, tmo, MSG_PRI_URGENT);
}

bool QueueBase::_push_back(void const* buf, size_t nn, int tmo)
{
    return _msg_send(buf, nn, tmo, MSG_PRI_NORMAL);
}
