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

	    enum ReadAccess { NoRead, Read };

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
		    T volatile* const ptr =
			reinterpret_cast<T volatile*>(reinterpret_cast<char volatile*>(base) + Offset);

		    MEMORY_SYNC;
		    return *ptr;
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
		    MEMORY_SYNC;
		    *reinterpret_cast<T volatile*>(base + Offset) = v;
		    asm volatile ("" ::: "memory");
		}

		static void writeMemField(uint8_t volatile* const base,
					  T const& mask, T const& v)
		{
		    T volatile* const ptr =
			reinterpret_cast<T volatile*>(base + Offset);

		    MEMORY_SYNC;
		    *ptr = v;
		}
	    };

	    template <typename T, size_t Offset>
	    struct WriteAPI<T, Offset, SyncWrite> {
		static void writeMem(uint8_t volatile* const base, T const& v)
		{
		    T volatile* const ptr =
			reinterpret_cast<T volatile*>(base + Offset);

		    MEMORY_SYNC;
		    *ptr = v;
		    *ptr;
		    MEMORY_SYNC;
		}
	    };

	    template <typename T, size_t Offset, bool R, WriteAccess W>
	    struct Register {
		typedef T Type;
		typedef ReadAPI<T, Offset, R> ReadInterface;
		typedef WriteAPI<T, Offset, W> WriteInterface;

		enum { RegOffset = Offset };

		static T read(void volatile* const base)
		{
		    return ReadInterface::readMem(base);
		}

		static void write(void volatile* const base, T const& v)
		{
		    WriteInterface::writeMem(base, v);
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

	     protected:
		Memory(Memory const& o) : baseAddr(o.baseAddr) {}

	     public:
		explicit Memory(uint32_t const offset) :
		    baseAddr(calcBaseAddr(tag, offset)) {}

		void volatile* getBaseAddr() const { return baseAddr; }

		template <typename R>
		typename R::Type get() const
		{
		    typedef typename Accessible<typename R::Type, 1,
						R::RegOffset>::allowed type;

		    return R::read(baseAddr);
		}

		template <typename R>
		void set(typename R::Type const& v) const
		{
		    typedef typename Accessible<typename R::Type, 1,
						R::RegOffset>::allowed type;

		    R::write(baseAddr, v);
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

		void volatile* getBaseAddr() const
		{ return Base::getBaseAddr(); }

		template <typename R>
		typename R::Type get(Lock const&) const
		{ return Base::template get<R>(); }

		template <typename R>
		void set(Lock const&, typename R::Type const& v) const
		{ return Base::template set<R>(v); }
	    };
	};
    };
};

#endif

// Local Variables:
// mode:c++
// End:
