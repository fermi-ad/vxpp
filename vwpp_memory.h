#if !defined(__VWPP_MEMORY_H)
#define __VWPP_MEMORY_H

namespace vwpp {
    namespace v2_7 {

	// This name space defines an API to access VME entities:
	//
	//  * Serialized access to A16/A24/A32 spaces.

	namespace VME {
	    enum AddressSpace { A16 = 0x29, A24 = 0x39, A32 = 0x09 };

	    uint8_t* calcBaseAddr(AddressSpace, uint32_t);

	    enum ReadAccess { NoRead, Read, SyncRead };

	    // This section declares a small API to read memory using
	    // different access methods. These templates are used to
	    // define the characteristics of hardware registers.

	    template <typename T, size_t Offset, enum ReadAccess = NoRead>
	    struct ReadAPI { };

	    // Reads a value from "volatile" memory. We prevent the
	    // compiler from optimizing around this call.

	    template <typename T, size_t Offset>
	    struct ReadAPI<T, Offset, Read> {
		static T readMem(uint8_t volatile* const base)
		{
		    VXPP_MEMORY_SYNC;

		    T const val =
			*reinterpret_cast<T volatile*>(base + Offset);

		    asm volatile ("" ::: "memory");
		    return val;
		}
	    };

	    template <typename T, size_t Offset>
	    struct ReadAPI<T, Offset, SyncRead> {
		static T readMem(uint8_t volatile* const base)
		{
		    VXPP_INSTRUCTION_SYNC;

		    T const val =
			*reinterpret_cast<T volatile*>(base + Offset);

		    asm volatile ("" ::: "memory");
		    return val;
		}
	    };

	    // This section declare a small API to write to memory.

	    enum WriteAccess { NoWrite, Write, SyncWrite };

	    template <typename T, size_t Offset, WriteAccess W = NoWrite>
	    struct WriteAPI { };

	    template <typename T, size_t Offset>
	    struct WriteAPI<T, Offset, Write> {
		static void writeMem(uint8_t volatile* const base, T const& v)
		{
		    VXPP_MEMORY_SYNC;
		    *reinterpret_cast<T volatile*>(base + Offset) = v;
		    asm volatile ("" ::: "memory");
		}

		static void writeMemField(uint8_t volatile* const base,
					  T const& mask, T const& v)
		{
		    T volatile* const ptr =
			reinterpret_cast<T volatile*>(base + Offset);

		    VXPP_MEMORY_SYNC;
		    *ptr = (*ptr & ~mask) | (v & mask);
		    asm volatile ("" ::: "memory");
		}
	    };

	    template <typename T, size_t Offset>
	    struct WriteAPI<T, Offset, SyncWrite> {
		static void writeMem(uint8_t volatile* const base, T const& v)
		{
		    T volatile* const ptr =
			reinterpret_cast<T volatile*>(base + Offset);

		    VXPP_MEMORY_SYNC;
		    *ptr = v;
		    *ptr;
		    VXPP_MEMORY_SYNC;
		}

		static void writeMemField(uint8_t volatile* const base,
					  T const& mask, T const& v)
		{
		    T volatile* const ptr =
			reinterpret_cast<T volatile*>(base + Offset);

		    VXPP_MEMORY_SYNC;
		    *ptr = (*ptr & ~mask) | (v & mask);
		    *ptr;
		    VXPP_MEMORY_SYNC;
		}
	    };

	    template <typename T, size_t Offset, ReadAccess R, WriteAccess W>
	    struct Register {
		typedef T Type;
		typedef T AtomicType;

		enum { RegOffset = Offset, RegEntries = 1 };

		static Type read(uint8_t volatile* const base)
		{
		    return ReadAPI<T, Offset, R>::readMem(base);
		}

		static void write(uint8_t volatile* const base, Type const& v)
		{
		    WriteAPI<T, Offset, W>::writeMem(base, v);
		}

		static void writeField(uint8_t volatile* const base,
				       Type const& mask, Type const& v)
		{
		    WriteAPI<T, Offset, W>::writeMemField(base, mask, v);
		}
	    };

	    enum DataAccess {
		D8 = 1, D16, D8_D16, D32, D8_D32, D16_D32, D8_D16_D32
	    };

	    // This is the generalized template of a class that
	    // controls access to VME memory space. It is given as a
	    // forward declaration and gets defined later in the
	    // header.

	    template <AddressSpace tag, DataAccess DA, size_t size,
		      typename LockType>
	    class Memory;

	    // This is a partially-specialized version where the lock
	    // type is 'void'. This changes the API to not include the
	    // lock parameter and requires an implementer to do
	    // serialization at a higher level.

	    template <AddressSpace tag, DataAccess DA, size_t size>
	    class Memory<tag, DA, size, void> {
		uint8_t volatile* const baseAddr;

		// This template expands to a type that defines
		// 'allowed' only if an offset into a VME::Memory
		// register bank is accessible.

		template <typename RegType, size_t n, size_t offset,
			  bool = (offset % sizeof(RegType) == 0 &&
				  offset + sizeof(RegType) * n <= size &&
				  (DA & sizeof(RegType)) != 0)>
		struct Accessible { };

		template <typename RegType, size_t n, size_t offset>
		struct Accessible<RegType, n, offset, true> {
		    typedef Accessible allowed; };

		template <typename T>
		T volatile* getAddr(size_t const offset) const
		{
		    return reinterpret_cast<T volatile*>(baseAddr + offset);
		}

	     protected:
		Memory(Memory const& o) : baseAddr(o.baseAddr) {}

	     public:
		explicit Memory(uint32_t const offset) :
		    baseAddr(calcBaseAddr(tag, offset)) {}

		template <typename R>
		typename R::Type get() const
		{
		    typedef typename Accessible<typename R::AtomicType,
						R::RegEntries,
						R::RegOffset>::allowed type;

		    return R::read(baseAddr);
		}

		template <typename R>
		void set(typename R::Type const& v) const
		{
		    typedef typename Accessible<typename R::AtomicType,
						R::RegEntries,
						R::RegOffset>::allowed type;

		    R::write(baseAddr, v);
		}

		template <typename R>
		void set_field(typename R::Type const& mask,
			       typename R::Type const& v) const
		{
		    typedef typename Accessible<typename R::AtomicType,
						R::RegEntries,
						R::RegOffset>::allowed type;

		    R::writeField(baseAddr, mask, v);
		}

		template <typename T>
		T unsafe_get(size_t const offset) const
		{
		    typedef typename Accessible<T, 1, 0>::allowed type;

		    asm volatile ("" ::: "memory");
		    return *getAddr<T>(offset);
		}

		template <typename T>
		void unsafe_set(size_t const offset, T const& v) const
		{
		    typedef typename Accessible<T, 1, 0>::allowed type;

		    asm volatile ("" ::: "memory");
		    *getAddr<T>(offset) = v;
		}
	    };

	    // This is the "fleshed-out", generalized version of the
	    // template. It requires a LockType to be given (and it
	    // verifies the LockType is a valid type of lock.)

	    template <AddressSpace tag, DataAccess DA, size_t size,
		      typename LockType>
	    class Memory :
		protected Memory<tag, DA, size, void>,
		private vwpp::v2_7::Uncopyable
	    {
		typedef Memory<tag, DA, size, void> Base;

		// Validate the lock type.

		typedef typename DetermineLock<LockType>::type Lock;

	     public:
		explicit Memory(uint32_t const offset) :
		    Base(offset) {}

		// This allows a memory bank to convert to a new
		// LockType. This is usually only necessary in
		// constructors and destructors.

		template <DataAccess ODA, typename OldLockType>
		Memory(Memory<tag, ODA, size, OldLockType> const& o) :
		    Base(o)
		{}

		template <typename R>
		typename R::Type get(Lock const&) const
		{ return Base::template get<R>(); }

		template <typename R>
		void set(Lock const&, typename R::Type const& v) const
		{ Base::template set<R>(v); }

		template <typename R>
		void set_field(Lock const&, typename R::Type const& mask,
			       typename R::Type const& v) const
		{ Base::template set_field<R>(mask, v); }

		template <typename T>
		T unsafe_get(Lock const&, size_t const offset) const
		{ return Base::template unsafe_get<T>(offset); }

		template <typename T>
		void unsafe_set(Lock const&, size_t const offset,
			     T const& v) const
		{ Base::template unsafe_set<T>(offset, v); }
	    };
	};
    };
};

#endif

// Local Variables:
// mode:c++
// End:
