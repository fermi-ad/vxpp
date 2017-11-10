#if !defined(__VWPP_MEMORY_H)
#define __VWPP_MEMORY_H

#if !defined(__VWPP_VERSION)
#define __VWPP_VERSION	0x205
#endif

#if __VWPP_VERSION != 0x205
#error "Mismatched VWPP headers."
#endif

namespace vwpp {

    // This name space defines an API to access VME entities:
    //
    //  * Serialized access to A16 space

    namespace VME {

	char* calcA16BaseAddr(uint32_t);

	// This class controls access to VME A16 space. It requires a
	// mutex to be held during access to the hardware and it
	// range-checks all accesses -- at compile time -- so they
	// don't exceed the hardware's span.

	template <typename LockType, size_t size>
	class A16 {
	    char volatile* const baseAddr;

	    // This template expands to a type that defines 'type'
	    // only if an offset into a VME::A16 register bank is
	    // accessible.

	    template <typename RT, size_t offset,
		      bool = (offset % sizeof(RT) == 0 &&
			      offset + sizeof(RT) <= size)>
	    struct Accessible { typedef Accessible allowed; };

	    template <typename RT, size_t offset>
	    struct Accessible<RT, offset, false> {};

	    typedef typename DetermineLock<LockType>::type Lock;

	 public:
	    explicit A16(uint16_t const a16_offset) :
		baseAddr(calcA16BaseAddr(a16_offset)) {}

	    void* getBaseAddr() const { return baseAddr; }

	    template <typename RT, size_t offset>
	    RT get(Lock const&) const
	    {
		typedef typename Accessible<RT, offset>::allowed type;

		return *reinterpret_cast<RT volatile*>(baseAddr + offset);
	    }

	    template <typename RT>
	    RT get(Lock const&, size_t const offset) const
	    {
		if (offset + sizeof(RT) < size)
		    return *reinterpret_cast<RT volatile*>(baseAddr + offset);
		else
		    throw std::range_error("reading outside register bank");
	    }

	    template <typename RT, size_t offset>
	    void set(Lock const&, RT const v)
	    {
		typedef typename Accessible<RT, offset>::allowed type;

		*reinterpret_cast<RT volatile*>(baseAddr + offset) = v;
	    }

	    template <typename RT>
	    void set(Lock const&, size_t const offset, RT const v)
	    {
		if (offset + sizeof(RT) < size)
		    *reinterpret_cast<RT volatile*>(baseAddr + offset) = v;
		else
		    throw std::range_error("writing outside register bank");
	    }
	};
    };
};

#elif __VWPP_VERSION != 0x205
#error "Already included different vwpp_memory.h"
#endif

// Local Variables:
// mode:c++
// End:
