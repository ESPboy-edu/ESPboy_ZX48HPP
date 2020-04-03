#ifndef STRCONST_HPP
#define STRCONST_HPP

#if !defined(WIN32) && !defined(WIN64)
	#define __int16 short
	#define __int8 char
#endif

class str_const { // is constexpr string class
private:
	const char* const p_;
	const unsigned __int16 sz_;
public:

	typedef unsigned __int16 HashType;
	typedef unsigned __int8 HashTypeHalf;

	template<HashType N>
	constexpr str_const(const char(&a)[N]) : // ctor
		p_(a), sz_(N - 1) {}
	constexpr char operator[](HashType n) const { // []
		return n < sz_ ? p_[n] : 0;
	}
	constexpr HashType size() const { return sz_; } // size()

	constexpr HashType hashout(HashType sum0, HashType sum1, HashType n) const
	{
		return n < sz_ ? hashout((sum0 + p_[n]) % 255, (((sum0 + p_[n]) % 255) + sum1) % 255, n + 1) : sum1 * 256 + sum0;
	}
	constexpr HashType fletcher16hash() const
	{
		return hashout(0, 0, 0);
	}
};

template<typename ___A, ___A T> constexpr ___A force_compile_time() { return T; }
template<str_const::HashType N> constexpr str_const::HashType fletcher16hash(const char(&a)[N]) { return str_const(a).fletcher16hash(); }

#define HASH16(___STR) (force_compile_time<str_const::HashType, str_const(___STR).fletcher16hash()>())
#define UNUSED(expr) do { (void)(expr); } while (0)

enum class Tags : str_const::HashType;

union HASH
{
	str_const::HashType hash;
	struct SubHash {
		str_const::HashTypeHalf sum0;
		str_const::HashTypeHalf sum1;
	} t;
	HASH() :hash(0L) {}
	inline HASH& clear() { hash = 0L; return *this; }
	HASH& hashout(unsigned char c)
	{
		t.sum0 = ((str_const::HashType)(t.sum0) + (str_const::HashType)(c)) % 255;
		t.sum1 = ((str_const::HashType)(t.sum0) + (str_const::HashType)(t.sum1)) % 255;
		return *this;
	}

	inline bool operator== (const HASH& h) const
	{
		return hash == h.hash;
	}
	operator str_const::HashType() const { return hash; }

	operator Tags() const
	{
		return (Tags)hash;
	}

};

// Вычисление Хеша по алгоритму Fletcher16 (https://en.wikipedia.org/wiki/Fletcher%27s_checksum)
HASH hashout(HASH shash, unsigned char c);

#endif