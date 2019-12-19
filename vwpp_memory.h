#if !defined(__VWPP_MEMORY_H)
#define __VWPP_MEMORY_H

namespace vwpp {
    namespace v2_7 {

	// This name space defines an API to access VME entities:
	//
	//  * Serialized access to A16/A24/A32 spaces.

	namespace VME {
	    enum AddressSpace { A16 = 0x29, A24 = 0x39, A32 = 0x09 };

	    char* calcBaseAddr(AddressSpace, uint32_t);

	    // This is the generalized template of a class that
	    // controls access to VME memory space. It is given as a
	    // forward declaration and gets defined later in the
	    // header.

	    template <AddressSpace tag, size_t size, typename LockType>
	    class Memory;

	    // This is a partially-specialized version where the lock
	    // type is 'void'. This changes the API to not include the
	    // lock parameter and requires an implementer to do
	    // serialization at a higher level.

	    template <AddressSpace tag, size_t size>
	    class Memory<tag, size, void> {
		char volatile* const baseAddr;

		// This template expands to a type that defines
		// 'allowed' only if an offset into a VME::Memory
		// register bank is accessible.

		template <typename RegType, size_t n, size_t offset,
			  bool = (offset % sizeof(RegType) == 0 &&
				  offset + sizeof(RegType) * n <= size)>
		struct Accessible { };

		template <typename RegType, size_t n, size_t offset>
		struct Accessible<RegType, n, offset, true> {
		    typedef Accessible allowed; };

	     protected:
		Memory(Memory const& o) : baseAddr(o.baseAddr) {}

	     public:
		explicit Memory(uint32_t const offset) :
		    baseAddr(calcBaseAddr(tag, offset)) {}

		void* getBaseAddr() const { return baseAddr; }

		template <typename RT, size_t offset>
		RT get() const
		{
		    typedef typename Accessible<RT, 1, offset>::allowed type;

		    return *reinterpret_cast<RT volatile*>(baseAddr + offset);
		}

		template <typename RT, size_t N, size_t offset>
		RT getItem(size_t const index) const
		{
		    typedef typename Accessible<RT, N, offset>::allowed type;

		    if (LIKELY(index < N))
			return reinterpret_cast<RT volatile*>(baseAddr + offset)[index];
		    else
			throw std::range_error("out of bounds array access");
		}

		template <typename RT>
		RT get(size_t const offset) const
		{
		    if (LIKELY(offset + sizeof(RT) < size))
			return *reinterpret_cast<RT volatile*>(baseAddr + offset);
		    else
			throw std::range_error("reading outside register bank");
		}

		template <typename RT, size_t offset>
		void set(RT const v)
		{
		    typedef typename Accessible<RT, 1, offset>::allowed type;

		    *reinterpret_cast<RT volatile*>(baseAddr + offset) = v;
		}

		template <typename RT>
		void set(size_t const offset, RT const v)
		{
		    if (LIKELY(offset + sizeof(RT) < size))
			*reinterpret_cast<RT volatile*>(baseAddr + offset) = v;
		    else
			throw std::range_error("writing outside register bank");
		}

		template <typename RT, size_t N, size_t offset>
		void setItem(size_t const index, RT const v)
		{
		    typedef typename Accessible<RT, N, offset>::allowed type;

		    if (LIKELY(index < N))
			reinterpret_cast<RT volatile*>(baseAddr + offset)[index] = v;
		    else
			throw std::range_error("out of bounds array access");
		}
	    };

	    // This is the "fleshed-out", generalized version of the
	    // template. It requires a LockType to be given (and it
	    // verifies the LockType is a valid type of lock.)

	    template <AddressSpace tag, size_t size, typename LockType>
	    class Memory :
		protected Memory<tag, size, void>,
		private vwpp::v2_7::Uncopyable
	    {
		typedef Memory<tag, size, void> Base;

		// Validate the lock type.

		typedef typename DetermineLock<LockType>::type Lock;

	     public:
		explicit Memory(uint32_t const offset) :
		    Base(offset) {}

		// This allows a memory bank to convert to a new
		// LockType. This is usually only necessary in
		// constructors and destructors.

		template <typename OldLockType>
		Memory(Memory<tag, size, OldLockType> const& o) :
		    Base(o)
		{}

		void* getBaseAddr() const
		{ return Base::getBaseAddr(); }

		template <typename RT, size_t offset>
		RT get(Lock const&) const
		{ return this->Base::template get<RT, offset>(); }

		template <typename RT, size_t N, size_t offset>
		RT getItem(Lock const&, size_t const index) const
		{ return this->Base::template getItem<RT, offset>(index); }

		template <typename RT>
		RT get(Lock const&, size_t const offset) const
		{ return this->Base::template get<RT>(offset); }

		template <typename RT, size_t offset>
		void set(Lock const&, RT const v)
		{ this->Base::template set<RT, offset>(v); }

		template <typename RT>
		void set(Lock const&, size_t const offset, RT const v)
		{ this->Base::template set<RT>(offset, v); }

		template <typename RT, size_t N, size_t offset>
		void setItem(Lock const&, size_t const index, RT const v)
		{ this->Base::template setItem<RT, N, offset>(index, v); }
	    };
	};
    };
};

#endif

// Local Variables:
// mode:c++
// End:
