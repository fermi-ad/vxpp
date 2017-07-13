#if !defined(__VWPP_TYPES_H)
#define __VWPP_TYPES_H

#if !defined(__VWPP_VERSION)
#define __VWPP_VERSION  0x204
#endif

#if __VWPP_VERSION != 0x204
#error "Mismatched VWPP headers."
#endif

// "Recent" GNU compilers support some built-in directives that affect
// code generation. One built-in, __builtin_expect, assists the
// compiler in choosing which branch in a conditional is more likely
// to occur.

#if VX_VERSION > 55
#define LIKELY(x)	__builtin_expect(!!(x), 1)
#define UNLIKELY(x)	__builtin_expect(!!(x), 0)
#else
#define LIKELY(x)	(x)
#define UNLIKELY(x)	(x)
#endif

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
};

#elif __VWPP_VERSION != 0x204
#error "Already included different vwpp_types.h"
#endif

// Local Variables:
// mode:c++
// End:
