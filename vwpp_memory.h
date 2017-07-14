#if !defined(__VWPP_MEMORY_H)
#define __VWPP_MEMORY_H

#if !defined(__VWPP_VERSION)
#define __VWPP_VERSION	0x204
#endif

#if __VWPP_VERSION != 0x204
#error "Mismatched VWPP headers."
#endif

namespace vwpp {

    // This name space defines an API to access VME entities:
    //
    //  * Serialized access to A16 space

    namespace vme {

	char* calcA16BaseAddr(uint32_t);

	// This class controls access to VME A16 space. It requires a
	// mutex to be held during access to the hardware and it
	// range-checks all accesses so they don't exceed the
	// hardware's span.

	template <typename LockType, size_t size>
	class A16 {
	    char* const baseAddr;

	    // This template expands to a type that defines 'type'
	    // only if an offset into a VME::A16 register bank is
	    // accessible.

	    template <typename RT, size_t offset,
		      bool = (offset % sizeof(RT) == 0 &&
			      offset + sizeof(RT) <= size)>
	    struct Accessible { typedef Accessible allowed; };

	    template <typename RT, size_t offset>
	    struct Accessible<RT, offset, false> {};

	 public:
	    explicit A16(uint32_t const a16_offset) :
		baseAddr(calcA16BaseAddr(a16_offset)) {}

	    void* getBaseAddr() const { return baseAddr; }

	    typedef typename DetermineLock<LockType>::type Lock;

	    template <typename RT, size_t offset>
	    struct Reg {
		typedef typename Accessible<RT, offset>::allowed type;

		RT get(Lock const&) const
		{
		    return *reinterpret_cast<RT volatile*>(baseAddr + offset);
		}

		void set(Lock const&, RT const& v)
		{
		    *reinterpret_cast<RT volatile*>(baseAddr + offset) = v;
		}
	    };

	    template <size_t offset>
	    struct Reg<void, offset> {
	    };

	    template <typename RT, size_t offset>
	    struct Reg<RT*, offset> {
	    };

	    template <typename RT, size_t offset>
	    struct Reg<RT&, offset> {
	    };
	};
    };
};

#elif __VWPP_VERSION != 0x204
#error "Already included different vwpp_memory.h"
#endif

// Local Variables:
// mode:c++
// End:
