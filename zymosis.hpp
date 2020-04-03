/*
 * Z80 CPU emulation engine v0.0.3b
 * coded by Ketmar // Vampire Avalon
 * C++ revision by Dmitry L.
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */
#pragma once

#ifndef __ZYMOSIS_HPP__
#define __ZYMOSIS_HPP__

#pragma GCC optimize ("-Ofast")
#pragma GCC push_options

#include <cstdint>

 /* define either ZYMOSIS_LITTLE_ENDIAN or ZYMOSIS_BIG_ENDIAN */

#if !defined(ZYMOSIS_LITTLE_ENDIAN) && !defined(ZYMOSIS_BIG_ENDIAN)
	#if BYTE_ORDER == LITTLE_ENDIAN
		#define ZYMOSIS_LITTLE_ENDIAN
	#else
		#define ZYMOSIS_BIG_ENDIAN
	#endif
	// # error wtf?! Zymosis endiannes is not defined!
#endif

#if defined(ZYMOSIS_LITTLE_ENDIAN) && defined(ZYMOSIS_BIG_ENDIAN)
# error wtf?! Zymosis endiannes double defined! are you nuts?
#endif

#if defined(__GNUC__)
# ifndef ZYMOSIS_PACKED
#  define ZYMOSIS_PACKED  /*__attribute__((packed)) __attribute__((gcc_struct))*/
# endif
# ifndef ZYMOSIS_INLINE
#  define ZYMOSIS_INLINE  __attribute__((always_inline)) 
# endif
#else
# ifndef ZYMOSIS_PACKED
#  define ZYMOSIS_PACKED
# endif
# ifndef ZYMOSIS_INLINE
#  define ZYMOSIS_INLINE inline
# endif
#endif

#ifndef PROGMEM
#define PROGMEM
#define pgm_read_byte(x) (x)
#define pgm_read_word(x) (x)
#define pgm_read_dword(x) (x)
#endif

namespace zymosis {

	enum {
		Z80_FLAG_C = 0x01,
		Z80_FLAG_N = 0x02,
		Z80_FLAG_PV = 0x04,
		Z80_FLAG_3 = 0x08,
		Z80_FLAG_H = 0x10,
		Z80_FLAG_5 = 0x20,
		Z80_FLAG_Z = 0x40,
		Z80_FLAG_S = 0x80,

		Z80_FLAG_35 = Z80_FLAG_3 | Z80_FLAG_5,
		Z80_FLAG_S35 = Z80_FLAG_S | Z80_FLAG_3 | Z80_FLAG_5
	};

// Five different fSZ53PTAB() function realizations. Lookup function returns zx register flag state for values. Roughly calculated performance.
// 1. Original based on static initialized array.						100.0%
// 2. Const array with compile-time data filling.						100.3%
// 3. Switch-based function generation. (compiler make lookup table).	100.3%
// 4. Bit ops based function (without loops).							100.5%	<-- Carefully chosed. 
// 4. Naive function (with loop).										101.6%
#define ZYMOSIS_FLAGS_IN_FUNCTION2
#if defined(ZYMOSIS_FLAGS_IN_STATIC) | defined(ZYMOSIS_FLAGS_IN_FUNCTION)
	template<uint8_t N>
	struct FA { constexpr static uint8_t val() { return (4 * (!((((N * 0x0101010101010101ULL) & 0x8040201008040201ULL) % 0x1FF) & 1))) | (N & Z80_FLAG_S35) | (N == 0 ? Z80_FLAG_Z : 0); } };
	//template<typename ___A, ___A T> constexpr ___A force_compile_time() { return T; }
	// enum { TT = FA<255>::val() };
#endif
#if defined(ZYMOSIS_FLAGS_IN_STATIC)
	#include <array>

	template<uint16_t INDEX = 0, uint16_t ...D> struct Helper : Helper<INDEX + 1, D..., FA<INDEX>::val()> { };
	template<uint16_t ...D> struct Helper<256, D...> { static constexpr std::array<uint8_t, 256> table = { D... };	};
	constexpr std::array<uint8_t, 256> sz53pTable = Helper<>::table;
#define SZ53PTAB(x) (sz53pTable[(x)])
#elif defined(ZYMOSIS_FLAGS_IN_ARRAY)
#define SZ53PTAB(x) (sz53pTable[(x)])
#elif defined(ZYMOSIS_FLAGS_IN_FUNCTION)
#define _ZYMOSIS_STAMP4(n, x) \
    x(n);                 \
    x(n + 1);             \
    x(n + 2);             \
    x(n + 3)
#define _ZYMOSIS_STAMP16(n, x) \
    _ZYMOSIS_STAMP4(n, x);     \
    _ZYMOSIS_STAMP4(n + 4, x); \
    _ZYMOSIS_STAMP4(n + 8, x); \
    _ZYMOSIS_STAMP4(n + 12, x)
#define _ZYMOSIS_STAMP64(n, x)   \
    _ZYMOSIS_STAMP16(n, x);      \
    _ZYMOSIS_STAMP16(n + 16, x); \
    _ZYMOSIS_STAMP16(n + 32, x); \
    _ZYMOSIS_STAMP16(n + 48, x)
#define _ZYMOSIS_STAMP256(n, x)   \
    _ZYMOSIS_STAMP64(n, x);       \
    _ZYMOSIS_STAMP64(n + 64, x);  \
    _ZYMOSIS_STAMP64(n + 128, x); \
    _ZYMOSIS_STAMP64(n + 192, x)
#define _ZYMOSIS_STAMP(n, x) _ZYMOSIS_STAMP##n(0, x)
#define SZ53PTAB(x) (fSZ53PTAB(x))
#define caseSZ53PTAB(n) case n: return FA<n>::val(); break;
	ZYMOSIS_INLINE uint8_t fSZ53PTAB(uint8_t t)
	{
		switch (t) {
			_ZYMOSIS_STAMP(256, caseSZ53PTAB);
		default: return 0;
		}
		return 0;
	}
#elif defined(ZYMOSIS_FLAGS_IN_FUNCTION2)
	ZYMOSIS_INLINE uint8_t fSZ53PTAB(uint8_t x)
	{
		uint8_t t = x ^ (x >> 4);
		t = t ^ (t >> 2);
		return ((~(t ^ (t >> 1))) & 1) * 4 | (x & Z80_FLAG_S35) | ((!x) * Z80_FLAG_Z);
	}
#define SZ53PTAB(x) (fSZ53PTAB(x))
#elif defined(ZYMOSIS_FLAGS_IN_FUNCTION3)
	ZYMOSIS_INLINE uint8_t fSZ53PTAB(uint8_t x)
	{
		uint_fast8_t t = (x & Z80_FLAG_S35) | ((!x) * Z80_FLAG_Z);
		uint_fast8_t p = 1;
		while (x) { x &= x - 1; p = !p; }
		return t | p*4;
	}
#define SZ53PTAB(x) (fSZ53PTAB(x))
#endif


	union Z80WordReg {
		uint16_t w;
#ifdef ZYMOSIS_LITTLE_ENDIAN
		struct ZYMOSIS_PACKED { uint8_t c, b; };
		struct ZYMOSIS_PACKED { uint8_t e, d; };
		struct ZYMOSIS_PACKED { uint8_t l, h; };
		struct ZYMOSIS_PACKED { uint8_t f, a; };
		struct ZYMOSIS_PACKED { uint8_t xl, xh; };
		struct ZYMOSIS_PACKED { uint8_t yl, yh; };
#else
		struct ZYMOSIS_PACKED { uint8_t b, c; };
		struct ZYMOSIS_PACKED { uint8_t d, e; };
		struct ZYMOSIS_PACKED { uint8_t h, l; };
		struct ZYMOSIS_PACKED { uint8_t a, f; };
		struct ZYMOSIS_PACKED { uint8_t xh, xl; };
		struct ZYMOSIS_PACKED { uint8_t yh, yl; };
#endif
	};

	static_assert(sizeof(Z80WordReg) == 2, "Incorrect size of union ZYMOSIS_PACKED");

	enum Z80MemIOType : uint_fast8_t {
		Z80_MEMIO_OPCODE = 0x00, /* reading opcode */
		Z80_MEMIO_OPCEXT = 0x01, /* 'ext' opcode (after CB/ED/DD/FD prefix) */
		Z80_MEMIO_OPCARG = 0x02, /* opcode argument (jump destination, register value, etc) */
		Z80_MEMIO_DATA = 0x03, /* reading/writing data */
		Z80_MEMIO_OTHER = 0x04, /* other 'internal' reads (for memptr, etc; don't do contention, breakpoints or so) */
		Z80_MEMIO_MASK = 0x0f,
		/* values for memory contention */
		Z80_MREQ_NONE = 0x00,
		Z80_MREQ_WRITE = 0x10,
		Z80_MREQ_READ = 0x20,
		Z80_MREQ_MASK = 0xf0
	};

	enum Z80PIOType : uint_fast8_t {
		Z80_PIO_NORMAL = 0x00,   /* normal call in Z80 execution loop */
		Z80_PIO_INTERNAL = 0x01, /* call from debugger or other place outside of Z80 execution loop */
		/* flags for port contention */
		Z80_PIOFLAG_IN = 0x10,   /* doing 'in' if set */
		Z80_PIOFLAG_EARLY = 0x20 /* 'early' port contetion, if set */
	};

#define INC_PC  (this->pc = (this->pc+1)&0xffff)
#define DEC_PC  (this->pc = ((int32_t)(this->pc)-1)&0xffff)

#define INC_W(n)  ((n) = ((n)+1)&0xffff)
#define DEC_W(n)  ((n) = ((int32_t)(n)-1)&0xffff)

#define XADD_W(n,v)  ((n) = ((n)+(v))&0xffff)
#define XSUB_W(n,v)  ((n) = ((int32_t)(n)-(v))&0xffff)

#define ZADD_W(n,v)  ((n) = ((int32_t)(n)+(v))&0xffff)

#define ZADD_WX(n,v)  (((int32_t)(n)+(v))&0xffff)

#define INC_B(n)  ((n) = ((n)+1)&0xff)
#define DEC_B(n)  ((n) = ((int32_t)(n)-1)&0xff)

#define CBX_REPEATED (opcode&0x10)
#define CBX_BACKWARD (opcode&0x08)

	struct Z80Info {

		/* registers */
		Z80WordReg bc, de, hl, af, sp, ix, iy;
		/* alternate registers */
		Z80WordReg bcx, dex, hlx, afx;
		Z80WordReg* dd; /* pointer to current HL/IX/IY (inside this struct) for the current command */
		Z80WordReg memptr;
		uint16_t pc; /* program counter */
		//uint16_t prev_pc; /* first byte of the previous command */
		uint16_t org_pc; /* first byte of the current command */
		uint8_t regI;
		uint8_t regR;
		bool iff1, iff2; /* boolean */
		uint8_t im; /* IM (0-2) */
		bool halted; /* boolean; is CPU halted? main progam must manually reset this flag when it's appropriate */
		int32_t tstates; /* t-states passed from previous interrupt (0-...) */
		int32_t next_event_tstate; /* Z80Execute() will exit when tstates>=next_event_tstate */
		int8_t prev_was_EIDDR; /* 1: previous instruction was EI/FD/DD? (they blocks /INT); -1: prev vas LD A,I or LD A,R */
							/* Zymosis will reset this flag only if it executed at least one instruction */
		bool evenM1; /* boolean; emulate 128K/Scorpion M1 contention? */
		// void* user; /* arbitrary user data */
	};

	struct Z80CallBacks : public Z80Info
	{
		/* miot: only Z80_MEMIO_xxx, no need in masking */
		inline uint8_t  memReadFn(uint16_t addr, Z80MemIOType miot) { return 0; }
		inline void memWriteFn(uint16_t addr, uint8_t value, Z80MemIOType miot) {}

		/* will be called when memory contention is necessary */
		/* must increase tstates to at least 'tstates' arg */
		/* mio: Z80_MEMIO_xxx | Z80_MREQ_xxx */
		/* Zymosis will never call this CB for Z80_MEMIO_OTHER memory acces */
		inline void contentionFn(uint16_t addr, int _tstates, Z80MemIOType mio) { tstates += _tstates; } /* can be NULL */

		/* port I/O functions should add 4 t-states by themselves */
		/* pio: only Z80_PIO_xxx, no need in masking */
		inline uint8_t portInFn(uint16_t port, Z80PIOType pio) { return 0; } /* in: +3; do read; +1 */
		inline void portOutFn(uint16_t port, uint8_t value, Z80PIOType pio) {} /* out: +1; do out; +3 */

		/* will be called when port contention is necessary */
		/* must increase tstates to at least 'tstates' arg */
		/* pio: can contain only Z80_PIOFLAG_xxx flags */
		/* `tstates` is always 1 when Z80_PIOFLAG_EARLY is set and 2 otherwise */
		inline void portContentionFn(uint16_t port, int _tstates, Z80PIOType pio) { tstates += _tstates; } /* can be NULL */


		/* RETI/RETN traps; called with opcode, *AFTER* iff changed and return address set; return !0 to break execution */
		inline int retiFn(uint8_t trapCode) { return 0; } /* return !0 to exit immediately */
		inline int retnFn(uint8_t trapCode) { return 0; } /* return !0 to exit immediately */


		/***/
		inline int trapEDFn(uint8_t trapCode) { return 0; } /* can be NULL */ /* return !0 to exit immediately */
		/* called when invalid ED command found */
		/* PC points to the next instruction */
		/* trapCode=0xFB: */
		/*   .SLT trap */
		/*    HL: address to load; */
		/*    A: A --> level number */
		/*    return: CARRY complemented --> error */

		inline void pagerFn() {} /* can be NULL */
		/* pagerFn is called before fetching opcode to allow, for example, TR-DOS ROM paging in/out */

		/* return !0 to break */
		inline int checkBPFn() { return 0; } /* can be NULL */
		/* checkBPFn is called just after pagerFn (before fetching opcode) */
		/* emulator can check various breakpoint conditions there */
		/* and return non-zero to immediately stop executing and return from Z80_Execute[XXX]() */

		inline void reset() {}
	};

	/******************************************************************************/
#define Z80_PeekBI(_addr)  this->memReadFn((_addr), Z80_MEMIO_OTHER)
#define Z80_PeekB(_addr)   this->memReadFn((_addr), Z80_MEMIO_DATA)

#define Z80_PokeBI(_addr,_byte)  this->memWriteFn((_addr), (_byte), Z80_MEMIO_OTHER)
#define Z80_PokeB(_addr,_byte)   this->memWriteFn((_addr), (_byte), Z80_MEMIO_DATA)

/* t1: setting /MREQ & /WR */
/* t2: memory write */
#define Z80_PokeB3T(_addr,_byte)  do { \
	  Z80_Contention((_addr), 3, Z80_MREQ_WRITE|Z80_MEMIO_DATA); \
	  Z80_PokeB((_addr), (_byte)); \
	} while (0)

	/******************************************************************************/
#define Z80_EXX()  do { \
	  uint16_t t = this->bc.w; this->bc.w = this->bcx.w; this->bcx.w = t; \
	  t = this->de.w; this->de.w = this->dex.w; this->dex.w = t; \
	  t = this->hl.w; this->hl.w = this->hlx.w; this->hlx.w = t; \
	} while (0)

#define Z80_EXAFAF()  do { \
	  uint16_t t = this->af.w; this->af.w = this->afx.w; this->afx.w = t; \
	} while (0)


	/******************************************************************************/
	/* simulate contented memory access */
	/* (tstates = tstates+contention+1)*cnt */
	/* (Z80Info *z80, uint16_t addr, int tstates, Z80MemIOType mio) */
#define Z80_Contention(_addr,_tstates,_mio)  do { \
	  this->contentionFn((_addr), (_tstates), static_cast<Z80MemIOType>(_mio)); \
	} while (0)


#define Z80_ContentionBy1(_addr,_cnt)  do { \
		for (int _f = (_cnt); _f-- > 0; this->contentionFn((_addr), 1, static_cast<Z80MemIOType>(Z80_MREQ_NONE | Z80_MEMIO_OTHER))) ; \
	} while (0)


#define Z80_ContentionIRBy1(_cnt)  Z80_ContentionBy1((((uint16_t)this->regI)<<8)|(this->regR), (_cnt))
#define Z80_ContentionPCBy1(_cnt)  Z80_ContentionBy1(this->pc, (_cnt))

#define Z80_AND_A(_b) (this->af.f = SZ53PTAB(this->af.a&=(_b))|Z80_FLAG_H)
#define Z80_OR_A(_b)  (this->af.f = SZ53PTAB(this->af.a|=(_b)))
#define Z80_XOR_A(_b) (this->af.f = SZ53PTAB(this->af.a^=(_b)))

#define INC_R  (this->regR = ((this->regR+1)&0x7f)|(this->regR&0x80))

	template<class T = Z80CallBacks>
	class Z80Cpu : public T
	{
		static_assert(std::is_base_of<Z80CallBacks, T>::value, "Type T must be derived from Z80CallBacks");

#if defined(ZYMOSIS_FLAGS_IN_ARRAY)
		static bool tablesInitialized;
		static uint8_t sz53pTable[256]; /* bits 3, 5 and 7 of result, Z and P flags */
		/* Z80InitTables() should be called before anyting else! */
		void Z80_InitTables(void)
		{
			if (!tablesInitialized) {
				int16_t f;
				for (f = 0; f <= 255; ++f) {
					int n, p;
					for (n = f, p = 0; n != 0; n >>= 1) p ^= n & 0x01;
					sz53pTable[f] = (f & Z80_FLAG_S35) | (p ? 0 : Z80_FLAG_PV);
				}
				sz53pTable[0] |= Z80_FLAG_Z;
				
				tablesInitialized = true;
			}
		}
#endif
		
		/*inline bool SET_TRUE_CC(uint8_t opcode)
		{
			bool trueCC = false;
			switch ((opcode >> 3) & 0x07)
			{
			case 0: return (this->af.f & Z80_FLAG_Z) == 0; break;
			case 1: return (this->af.f & Z80_FLAG_Z) != 0; break;
			case 2: return (this->af.f & Z80_FLAG_C) == 0; break;
			case 3: return (this->af.f & Z80_FLAG_C) != 0; break;
			case 4: return (this->af.f & Z80_FLAG_PV) == 0; break;
			case 5: return (this->af.f & Z80_FLAG_PV) != 0; break;
			case 6: return (this->af.f & Z80_FLAG_S) == 0; break;
			case 7: return (this->af.f & Z80_FLAG_S) != 0; break;
			}
			return trueCC;
		}*/

		ZYMOSIS_INLINE bool SET_TRUE_CC(uint8_t opcode)
		{
			uint8_t op = (opcode >> 3) & 0x07;
			if (op < 4)
			{
				if (op < 2)
				{
					if (!op)  return (this->af.f & Z80_FLAG_Z) == 0;
					return (this->af.f & Z80_FLAG_Z) != 0;
				}
				else
				{
					if (op == 2) return (this->af.f & Z80_FLAG_C) == 0;
					return (this->af.f & Z80_FLAG_C) != 0;
				}
			}
			else
			{
				if (op < 6)
				{
					if (op == 4) return (this->af.f & Z80_FLAG_PV) == 0;
					return (this->af.f & Z80_FLAG_PV) != 0;
				}
				else
				{
					if (op == 6) return (this->af.f & Z80_FLAG_S) == 0;
					return (this->af.f & Z80_FLAG_S) != 0;
				}
			}
			return false;
		}
		/* t1: setting /MREQ & /RD */
		/* t2: memory read */
		/* t3, t4: decode command, increment R */
		ZYMOSIS_INLINE void GET_OPCODE(uint8_t& _opc)
		{
			do {
				this->contentionFn(this->pc, 4, static_cast<Z80MemIOType>(Z80_MREQ_READ | Z80_MEMIO_OPCODE));
				if (this->evenM1 && (this->tstates & 0x01))++this->tstates;
				(_opc) = this->memReadFn(this->pc, Z80_MEMIO_OPCODE);
				this->pc = (this->pc + 1) & 0xffff;
				this->regR = ((this->regR + 1) & 0x7f) | (this->regR & 0x80);
			} while (0);
		}

		ZYMOSIS_INLINE void GET_OPCODE_EXT(uint8_t& _opc) {
			do {
				this->contentionFn(this->pc, 4, static_cast<Z80MemIOType>(Z80_MREQ_READ | Z80_MEMIO_OPCEXT));
				_opc = this->memReadFn(this->pc, Z80_MEMIO_OPCEXT);
				this->pc = (this->pc + 1) & 0xffff;
				this->regR = ((this->regR + 1) & 0x7f) | (this->regR & 0x80);
			} while (0);
		}

		/*  t1: setting /MREQ & /RD */
		/*  t2: memory read */
		/*#define Z80_PeekB3T(_z80,_addr)  (Z80_Contention(_(_addr), 3, Z80_MREQ_READ|Z80_MEMIO_DATA), Z80_PeekB(_(_addr))) */
		ZYMOSIS_INLINE uint8_t Z80_PeekB3T(uint16_t addr) {
			Z80_Contention(addr, 3, Z80_MREQ_READ | Z80_MEMIO_DATA);
			return Z80_PeekB(addr);
		}

		ZYMOSIS_INLINE uint8_t Z80_PeekB3TA(uint16_t addr) {
			Z80_Contention(addr, 3, Z80_MREQ_READ | Z80_MEMIO_OPCARG);
			return Z80_PeekB(addr);
		}

		/******************************************************************************/
		ZYMOSIS_INLINE uint8_t Z80_PortIn(uint16_t port) {
			uint8_t value;
			this->portContentionFn(port, 1, static_cast<Z80PIOType>(Z80_PIOFLAG_IN | Z80_PIOFLAG_EARLY));
			this->portContentionFn(port, 2, Z80_PIOFLAG_IN);
			value = this->portInFn(port, Z80_PIO_NORMAL);
			++this->tstates;
			return value;
		}


		ZYMOSIS_INLINE void Z80_PortOut(uint16_t port, uint8_t value) {
			this->portContentionFn(port, 1, Z80_PIOFLAG_EARLY);
			this->portOutFn(port, value, Z80_PIO_NORMAL);
			this->portContentionFn(port, 2, Z80_PIO_NORMAL);
			++this->tstates;
		}

		ZYMOSIS_INLINE uint16_t Z80_PeekW6T(uint16_t addr) {
			uint16_t res = Z80_PeekB3T(addr);
			return res | (((uint16_t)Z80_PeekB3T((addr + 1) & 0xffff)) << 8);
		}

		ZYMOSIS_INLINE void Z80_PokeW6T(uint16_t addr, uint16_t value) {
			Z80_PokeB3T(addr, value & 0xff);
			Z80_PokeB3T((addr + 1) & 0xffff, (value >> 8) & 0xff);
		}

		ZYMOSIS_INLINE void Z80_PokeW6TInv(uint16_t addr, uint16_t value) {
			Z80_PokeB3T((addr + 1) & 0xffff, (value >> 8) & 0xff);
			Z80_PokeB3T(addr, value & 0xff);
		}

		ZYMOSIS_INLINE uint16_t Z80_GetWordPC(int wait1) {
			uint16_t res = Z80_PeekB3TA(this->pc);
			/***/
			this->pc = (this->pc + 1) & 0xffff;
			res |= ((uint16_t)Z80_PeekB3TA(this->pc)) << 8;
			if (wait1) Z80_ContentionPCBy1(wait1);
			this->pc = (this->pc + 1) & 0xffff;
			return res;
		}

		ZYMOSIS_INLINE uint16_t Z80_Pop6T() {
			uint16_t res = Z80_PeekB3T(this->sp.w);
			/***/
			this->sp.w = (this->sp.w + 1) & 0xffff;
			res |= ((uint16_t)Z80_PeekB3T(this->sp.w)) << 8;
			this->sp.w = (this->sp.w + 1) & 0xffff;
			return res;
		}

		/* 3 T states write high byte of PC to the stack and decrement SP */
		/* 3 T states write the low byte of PC and jump to #0066 */
		ZYMOSIS_INLINE void Z80_Push6T(uint16_t value) {
			this->sp.w = (((int32_t)this->sp.w) - 1) & 0xffff;
			Z80_PokeB3T(this->sp.w, (value >> 8) & 0xff);
			this->sp.w = (((int32_t)this->sp.w) - 1) & 0xffff;
			Z80_PokeB3T(this->sp.w, value & 0xff);
		}


		/******************************************************************************/
		ZYMOSIS_INLINE void Z80_ADC_A(uint8_t b) {
			uint16_t _new, o = this->af.a;
			/***/
			this->af.a = (_new = o + b + (this->af.f & Z80_FLAG_C)) & 0xff; /* Z80_FLAG_C is 0x01, so it's safe */
			this->af.f =
				(SZ53PTAB(_new & 0xff) & ~Z80_FLAG_PV) |
				(_new > 0xff ? Z80_FLAG_C : 0) |
				((o ^ (~b)) & (o ^ _new) & 0x80 ? Z80_FLAG_PV : 0) |
				((o & 0x0f) + (b & 0x0f) + (this->af.f & Z80_FLAG_C) >= 0x10 ? Z80_FLAG_H : 0);
		}

		ZYMOSIS_INLINE void Z80_SBC_A(uint8_t b) {
			uint16_t _new, o = this->af.a;
			/***/
			this->af.a = (_new = ((int32_t)o - (int32_t)b - (int32_t)(this->af.f & Z80_FLAG_C)) & 0xffff) & 0xff; /* Z80_FLAG_C is 0x01, so it's safe */
			this->af.f =
				Z80_FLAG_N |
				(SZ53PTAB(_new & 0xff) & ~Z80_FLAG_PV) |
				(_new > 0xff ? Z80_FLAG_C : 0) |
				((o ^ b) & (o ^ _new) & 0x80 ? Z80_FLAG_PV : 0) |
				((int32_t)(o & 0x0f) - (int32_t)(b & 0x0f) - (int32_t)(this->af.f & Z80_FLAG_C) < 0 ? Z80_FLAG_H : 0);
		}


		ZYMOSIS_INLINE void Z80_ADD_A(uint8_t b) {
			this->af.f &= ~Z80_FLAG_C;
			Z80_ADC_A(b);
		}

		ZYMOSIS_INLINE void Z80_SUB_A(uint8_t b) {
			this->af.f &= ~Z80_FLAG_C;
			Z80_SBC_A(b);
		}

		ZYMOSIS_INLINE void Z80_CP_A(uint8_t b) {
			uint8_t o = this->af.a, _new = ((int32_t)o - (int32_t)b) & 0xff;
			/***/
			this->af.f =
				Z80_FLAG_N |
				(_new & Z80_FLAG_S) |
				(b & Z80_FLAG_35) |
				(_new == 0 ? Z80_FLAG_Z : 0) |
				(o < b ? Z80_FLAG_C : 0) |
				((o ^ b) & (o ^ _new) & 0x80 ? Z80_FLAG_PV : 0) |
				((int32_t)(o & 0x0f) - (int32_t)(b & 0x0f) < 0 ? Z80_FLAG_H : 0);
		}

		/* carry unchanged */
		ZYMOSIS_INLINE uint8_t Z80_DEC8(uint8_t b) {
			this->af.f &= Z80_FLAG_C;
			this->af.f |= Z80_FLAG_N |
				(b == 0x80 ? Z80_FLAG_PV : 0) |
				(b & 0x0f ? 0 : Z80_FLAG_H) |
				(SZ53PTAB((((int)b) - 1) & 0xff) & ~Z80_FLAG_PV);
			return (((int)b) - 1) & 0xff;
		}

		/* carry unchanged */
		ZYMOSIS_INLINE uint8_t Z80_INC8(uint8_t b) {
			this->af.f &= Z80_FLAG_C;
			this->af.f |=
				(b == 0x7f ? Z80_FLAG_PV : 0) |
				((b + 1) & 0x0f ? 0 : Z80_FLAG_H) |
				(SZ53PTAB((b + 1) & 0xff) & ~Z80_FLAG_PV);
			return ((b + 1) & 0xff);
		}


		/* cyclic, carry reflects shifted bit */
		ZYMOSIS_INLINE void Z80_RLCA() {
			uint8_t c = ((this->af.a >> 7) & 0x01);
			/***/
			this->af.a = (this->af.a << 1) | c;
			this->af.f = c | (this->af.a & Z80_FLAG_35) | (this->af.f & (Z80_FLAG_PV | Z80_FLAG_Z | Z80_FLAG_S));
		}

		/* cyclic, carry reflects shifted bit */
		ZYMOSIS_INLINE void Z80_RRCA() {
			uint8_t c = (this->af.a & 0x01);
			/***/
			this->af.a = (this->af.a >> 1) | (c << 7);
			this->af.f = c | (this->af.a & Z80_FLAG_35) | (this->af.f & (Z80_FLAG_PV | Z80_FLAG_Z | Z80_FLAG_S));
		}


		/* cyclic thru carry */
		ZYMOSIS_INLINE void Z80_RLA() {
			uint8_t c = ((this->af.a >> 7) & 0x01);
			/***/
			this->af.a = (this->af.a << 1) | (this->af.f & Z80_FLAG_C);
			this->af.f = c | (this->af.a & Z80_FLAG_35) | (this->af.f & (Z80_FLAG_PV | Z80_FLAG_Z | Z80_FLAG_S));
		}

		/* cyclic thru carry */
		ZYMOSIS_INLINE void Z80_RRA() {
			uint8_t c = (this->af.a & 0x01);
			/***/
			this->af.a = (this->af.a >> 1) | ((this->af.f & Z80_FLAG_C) << 7);
			this->af.f = c | (this->af.a & Z80_FLAG_35) | (this->af.f & (Z80_FLAG_PV | Z80_FLAG_Z | Z80_FLAG_S));
		}

		/* cyclic thru carry */
		ZYMOSIS_INLINE uint8_t Z80_RL(uint8_t b) {
			uint8_t c = (b >> 7)& Z80_FLAG_C;
			/***/
			this->af.f = SZ53PTAB((b = ((b << 1) & 0xff) | (this->af.f & Z80_FLAG_C))) | c;
			return b;
		}


		ZYMOSIS_INLINE uint8_t Z80_RR(uint8_t b) {
			uint8_t c = (b & 0x01);
			/***/
			this->af.f = SZ53PTAB((b = (b >> 1) | ((this->af.f & Z80_FLAG_C) << 7))) | c;
			return b;
		}

		/* cyclic, carry reflects shifted bit */
		ZYMOSIS_INLINE uint8_t Z80_RLC(uint8_t b) {
			uint8_t c = ((b >> 7)& Z80_FLAG_C);
			/***/
			this->af.f = SZ53PTAB((b = ((b << 1) & 0xff) | c)) | c;
			return b;
		}

		/* cyclic, carry reflects shifted bit */
		ZYMOSIS_INLINE uint8_t Z80_RRC(uint8_t b) {
			uint8_t c = (b & 0x01);
			/***/
			this->af.f = SZ53PTAB((b = (b >> 1) | (c << 7))) | c;
			return b;
		}

		ZYMOSIS_INLINE uint8_t Z80_SLA(uint8_t b) {
			uint8_t c = ((b >> 7) & 0x01);
			/***/
			b <<= 1;
			this->af.f = SZ53PTAB(b) | c;
			return b;
		}

		ZYMOSIS_INLINE uint8_t Z80_SRA(uint8_t b) {
			uint8_t c = (b & 0x01);
			/***/
			this->af.f = SZ53PTAB((b = (b >> 1) | (b & 0x80))) | c;
			return b;
		}

		ZYMOSIS_INLINE uint8_t Z80_SLL(uint8_t b) {
			uint8_t c = ((b >> 7) & 0x01);
			/***/
			this->af.f = SZ53PTAB((b = (b << 1) | 0x01)) | c;
			return b;
		}

		ZYMOSIS_INLINE uint8_t Z80_SLR(uint8_t b) {
			uint8_t c = (b & 0x01);
			/***/
			this->af.f = SZ53PTAB((b >>= 1)) | c;
			return b;
		}


		/* ddvalue+value */
		ZYMOSIS_INLINE uint16_t Z80_ADD_DD(uint16_t value, uint16_t ddvalue) {
			//static const uint8_t hct[8] = { 0, Z80_FLAG_H, Z80_FLAG_H, Z80_FLAG_H, 0, 0, 0, Z80_FLAG_H };
			uint32_t res = (uint32_t)value + (uint32_t)ddvalue;
			uint8_t b = (((value & 0x0FFF) + (ddvalue & 0x0FFF)) >> 8) & 0x10;
			/***/
			this->memptr.w = (ddvalue + 1) & 0xffff;
			this->af.f =
				(this->af.f & (Z80_FLAG_PV | Z80_FLAG_Z | Z80_FLAG_S)) |
				(res > 0xffff ? Z80_FLAG_C : 0) |
				((res >> 8)& Z80_FLAG_35) | b;
			return res;
		}

		/* ddvalue+value */
		ZYMOSIS_INLINE uint16_t Z80_ADC_DD(uint16_t value, uint16_t ddvalue) {
			uint8_t c = (this->af.f & Z80_FLAG_C);
			uint32_t _new = (uint32_t)value + (uint32_t)ddvalue + (uint32_t)c;
			uint16_t res = (_new & 0xffff);
			/***/
			this->memptr.w = (ddvalue + 1) & 0xffff;
			this->af.f =
				((res >> 8)& Z80_FLAG_S35) |
				(res == 0 ? Z80_FLAG_Z : 0) |
				(_new > 0xffff ? Z80_FLAG_C : 0) |
				((value ^ ((~ddvalue) & 0xffff)) & (value ^ _new) & 0x8000 ? Z80_FLAG_PV : 0) |
				((value & 0x0fff) + (ddvalue & 0x0fff) + c >= 0x1000 ? Z80_FLAG_H : 0);
			return res;
		}

		/* ddvalue-value */
		ZYMOSIS_INLINE uint16_t Z80_SBC_DD(uint16_t value, uint16_t ddvalue) {
			uint16_t res;
			uint8_t tmpB = this->af.a;
			/***/
			this->memptr.w = (ddvalue + 1) & 0xffff;
			this->af.a = ddvalue & 0xff;
			Z80_SBC_A(value & 0xff);
			res = this->af.a;
			this->af.a = (ddvalue >> 8) & 0xff;
			Z80_SBC_A((value >> 8) & 0xff);
			res |= (this->af.a << 8);
			this->af.a = tmpB;
			this->af.f = (res ? this->af.f & (~Z80_FLAG_Z) : this->af.f | Z80_FLAG_Z);
			return res;
		}


		ZYMOSIS_INLINE void Z80_BIT(uint8_t bit, uint8_t num, int mptr) {
			this->af.f =
				Z80_FLAG_H |
				(this->af.f & Z80_FLAG_C) |
				(num & Z80_FLAG_35) |
				(num & (1 << bit) ? 0 : Z80_FLAG_PV | Z80_FLAG_Z) |
				(bit == 7 ? num & Z80_FLAG_S : 0);
			if (mptr) this->af.f = (this->af.f & ~Z80_FLAG_35) | (this->memptr.h & Z80_FLAG_35);
		}


		ZYMOSIS_INLINE void Z80_DAA() {
			uint8_t tmpI = 0, tmpC = (this->af.f & Z80_FLAG_C), tmpA = this->af.a;
			/***/
			if ((this->af.f & Z80_FLAG_H) || (tmpA & 0x0f) > 9) tmpI = 6;
			if (tmpC != 0 || tmpA > 0x99) tmpI |= 0x60;
			if (tmpA > 0x99) tmpC = Z80_FLAG_C;
			if (this->af.f & Z80_FLAG_N) Z80_SUB_A(tmpI); else Z80_ADD_A(tmpI);
			this->af.f = (this->af.f & ~(Z80_FLAG_C | Z80_FLAG_PV)) | tmpC | (SZ53PTAB(this->af.a) & Z80_FLAG_PV);
		}


		ZYMOSIS_INLINE void Z80_RRD_A() {
			uint8_t tmpB = Z80_PeekB3T(this->hl.w);
			/*IOP(4)*/
			this->memptr.w = (this->hl.w + 1) & 0xffff;
			Z80_ContentionBy1(this->hl.w, 4);
			Z80_PokeB3T(this->hl.w, (this->af.a << 4) | (tmpB >> 4));
			this->af.a = (this->af.a & 0xf0) | (tmpB & 0x0f);
			this->af.f = (this->af.f & Z80_FLAG_C) | SZ53PTAB(this->af.a);
		}

		ZYMOSIS_INLINE void Z80_RLD_A() {
			uint8_t tmpB = Z80_PeekB3T(this->hl.w);
			/*IOP(4)*/
			this->memptr.w = (this->hl.w + 1) & 0xffff;
			Z80_ContentionBy1(this->hl.w, 4);
			Z80_PokeB3T(this->hl.w, (tmpB << 4) | (this->af.a & 0x0f));
			this->af.a = (this->af.a & 0xf0) | (tmpB >> 4);
			this->af.f = (this->af.f & Z80_FLAG_C) | SZ53PTAB(this->af.a);
		}


		ZYMOSIS_INLINE void Z80_LD_A_IR(uint8_t ir) {
			this->af.a = ir;
			this->prev_was_EIDDR = -1;
			Z80_ContentionIRBy1(1);
			this->af.f = (SZ53PTAB(this->af.a) & ~Z80_FLAG_PV) | (this->af.f & Z80_FLAG_C) | (this->iff2 ? Z80_FLAG_PV : 0);
		}


	public:
		Z80Cpu()
		{
#if defined(ZYMOSIS_FLAGS_IN_ARRAY)
			Z80_InitTables();
#endif
		}

		void Z80_Reset()
		{
			this->reset();

			this->bc.w = this->de.w = this->hl.w = this->af.w = this->sp.w = this->ix.w = this->iy.w = 0;
			this->bcx.w = this->dex.w = this->hlx.w = this->afx.w = 0;
			this->pc = /* this->prev_pc =*/ this->org_pc = 0;
			this->memptr.w = 0;
			this->regI = this->regR = 0;
			this->iff1 = this->iff2 = false;
			this->im = 0;
			this->halted = false;
			this->prev_was_EIDDR = 0;
			this->tstates = 0;
			this->dd = &this->hl;

			this->evenM1 = false;
		}
		void Z80_Execute()
		{
			uint8_t opcode;
			bool gotDD, trueCC; /* booleans */
			int disp;
			uint8_t tmpB, tmpC, rsrc, rdst;
			uint16_t tmpW = 0; /* shut up the compiler; it's wrong but stubborn */
			/***/
			while (this->tstates < this->next_event_tstate) {
				this->pagerFn();
				if (this->checkBPFn()) return;
				//this->prev_pc = this->org_pc; 
				this->org_pc = this->pc;
				/* read opcode -- OCR(4) */
				GET_OPCODE(opcode);
				this->prev_was_EIDDR = 0;
				disp = gotDD = 0;
				this->dd = &this->hl;
				if (this->halted) { DEC_W(this->pc); continue; }
				/***/
				if (opcode == 0xdd || opcode == 0xfd) {
					static const uint32_t withIndexBmp[8] PROGMEM = { 0x00,0x700000,0x40404040,0x40bf4040,0x40404040,0x40404040,0x0800,0x00 };
					/* IX/IY prefix */
					this->dd = (opcode == 0xdd ? &this->ix : &this->iy);
					/* read opcode -- OCR(4) */
					GET_OPCODE_EXT(opcode);
					/* test if this instruction have (HL) */
					if (pgm_read_dword(&withIndexBmp[opcode >> 5]) & (1 << (opcode & 0x1f))) {
						/* 3rd byte is always DISP here */
						disp = Z80_PeekB3TA(this->pc); if (disp > 127) disp -= 256;
						INC_PC;
						this->memptr.w = ZADD_WX(this->dd->w, disp);
					}
					else if (opcode == 0xdd || opcode == 0xfd) {
						/* double prefix; restart main loop */
						this->prev_was_EIDDR = 1;
						continue;
					}
					gotDD = 1;
				}
				/* instructions */
				if (opcode == 0xed) {
					this->dd = &this->hl;
					/* read opcode -- OCR(4) */
					GET_OPCODE_EXT(opcode);
					switch (opcode) {
						/* LDI, LDIR, LDD, LDDR */
					case 0xa0: case 0xb0: case 0xa8: case 0xb8:
						tmpB = Z80_PeekB3T(this->hl.w);
						Z80_PokeB3T(this->de.w, tmpB);
						/*MWR(5)*/
						Z80_ContentionBy1(this->de.w, 2);
						DEC_W(this->bc.w);
						tmpB = (tmpB + this->af.a) & 0xff;
						/***/
						this->af.f =
							(tmpB & Z80_FLAG_3) | (this->af.f & (Z80_FLAG_C | Z80_FLAG_Z | Z80_FLAG_S)) |
							(this->bc.w != 0 ? Z80_FLAG_PV : 0) |
							(tmpB & 0x02 ? Z80_FLAG_5 : 0);
						/***/
						if (CBX_REPEATED) {
							if (this->bc.w != 0) {
								/*IOP(5)*/
								Z80_ContentionBy1(this->de.w, 5);
								/* do it again */
								XSUB_W(this->pc, 2);
								this->memptr.w = (this->pc + 1) & 0xffff;
							}
						}
						if (!CBX_BACKWARD) { INC_W(this->hl.w); INC_W(this->de.w); }
						else { DEC_W(this->hl.w); DEC_W(this->de.w); }
						break;
						/* CPI, CPIR, CPD, CPDR */
					case 0xa1: case 0xb1: case 0xa9: case 0xb9:
						/* MEMPTR */
						if (CBX_REPEATED && (!(this->bc.w == 1 || Z80_PeekBI(this->hl.w) == this->af.a))) {
							this->memptr.w = ZADD_WX(this->org_pc, 1);
						}
						else {
							this->memptr.w = ZADD_WX(this->memptr.w, (CBX_BACKWARD ? -1 : 1));
						}
						/***/
						tmpB = Z80_PeekB3T(this->hl.w);
						/*IOP(5)*/
						Z80_ContentionBy1(this->hl.w, 5);
						DEC_W(this->bc.w);
						/***/
						this->af.f =
							Z80_FLAG_N |
							(this->af.f & Z80_FLAG_C) |
							(this->bc.w != 0 ? Z80_FLAG_PV : 0) |
							((int32_t)(this->af.a & 0x0f) - (int32_t)(tmpB & 0x0f) < 0 ? Z80_FLAG_H : 0);
						/***/
						tmpB = ((int32_t)this->af.a - (int32_t)tmpB) & 0xff;
						/***/
						this->af.f |=
							(tmpB == 0 ? Z80_FLAG_Z : 0) |
							(tmpB & Z80_FLAG_S);
						/***/
						if (this->af.f & Z80_FLAG_H) tmpB = ((uint16_t)tmpB - 1) & 0xff;
						this->af.f |= (tmpB & Z80_FLAG_3) | (tmpB & 0x02 ? Z80_FLAG_5 : 0);
						/***/
						if (CBX_REPEATED) {
							/* repeated */
							if ((this->af.f & (Z80_FLAG_Z | Z80_FLAG_PV)) == Z80_FLAG_PV) {
								/*IOP(5)*/
								Z80_ContentionBy1(this->hl.w, 5);
								/* do it again */
								XSUB_W(this->pc, 2);
							}
						}
						if (CBX_BACKWARD) DEC_W(this->hl.w); else INC_W(this->hl.w);
						break;
						/* OUTI, OTIR, OUTD, OTDR */
					case 0xa3: case 0xb3: case 0xab: case 0xbb:
						DEC_B(this->bc.b);
						/* fallthru */
					  /* INI, INIR, IND, INDR */
					case 0xa2: case 0xb2: case 0xaa: case 0xba:
						this->memptr.w = ZADD_WX(this->bc.w, (CBX_BACKWARD ? -1 : 1));
						/*OCR(5)*/
						Z80_ContentionIRBy1(1);
						if (opcode & 0x01) {
							/* OUT* */
							tmpB = Z80_PeekB3T(this->hl.w);/*MRD(3)*/
							Z80_PortOut(this->bc.w, tmpB);
							tmpW = ZADD_WX(this->hl.w, (CBX_BACKWARD ? -1 : 1));
							tmpC = (tmpB + tmpW) & 0xff;
						}
						else {
							/* IN* */
							tmpB = Z80_PortIn(this->bc.w);
							Z80_PokeB3T(this->hl.w, tmpB);/*MWR(3)*/
							DEC_B(this->bc.b);
							if (CBX_BACKWARD) tmpC = ((int32_t)tmpB + (int32_t)this->bc.c - 1) & 0xff; else tmpC = (tmpB + this->bc.c + 1) & 0xff;
						}
						/***/
						this->af.f =
							(tmpB & 0x80 ? Z80_FLAG_N : 0) |
							(tmpC < tmpB ? Z80_FLAG_H | Z80_FLAG_C : 0) |
							(SZ53PTAB((tmpC & 0x07) ^ this->bc.b) & Z80_FLAG_PV) |
							(SZ53PTAB(this->bc.b) & ~Z80_FLAG_PV);
						/***/
						if (CBX_REPEATED) {
							/* repeating commands */
							if (this->bc.b != 0) {
								uint16_t a = (opcode & 0x01 ? this->bc.w : this->hl.w);
								/***/
								/*IOP(5)*/
								Z80_ContentionBy1(a, 5);
								/* do it again */
								XSUB_W(this->pc, 2);
							}
						}
						if (CBX_BACKWARD) DEC_W(this->hl.w); else INC_W(this->hl.w);
						break;
						/* not strings, but some good instructions anyway */
					default:
						if ((opcode & 0xc0) == 0x40) {
							/* 0x40...0x7f */
							switch (opcode & 0x07) {
								/* IN r8,(C) */
							case 0:
								this->memptr.w = ZADD_WX(this->bc.w, 1);
								tmpB = Z80_PortIn(this->bc.w);
								this->af.f = SZ53PTAB(tmpB) | (this->af.f & Z80_FLAG_C);
								switch ((opcode >> 3) & 0x07) {
								case 0: this->bc.b = tmpB; break;
								case 1: this->bc.c = tmpB; break;
								case 2: this->de.d = tmpB; break;
								case 3: this->de.e = tmpB; break;
								case 4: this->hl.h = tmpB; break;
								case 5: this->hl.l = tmpB; break;
								case 7: this->af.a = tmpB; break;
									/* 6 affects only flags */
								}
								break;
								/* OUT (C),r8 */
							case 1:
								this->memptr.w = ZADD_WX(this->bc.w, 1);
								switch ((opcode >> 3) & 0x07) {
								case 0: tmpB = this->bc.b; break;
								case 1: tmpB = this->bc.c; break;
								case 2: tmpB = this->de.d; break;
								case 3: tmpB = this->de.e; break;
								case 4: tmpB = this->hl.h; break;
								case 5: tmpB = this->hl.l; break;
								case 7: tmpB = this->af.a; break;
								default: tmpB = 0; break; /*6*/
								}
								Z80_PortOut(this->bc.w, tmpB);
								break;
								/* SBC HL,rr/ADC HL,rr */
							case 2:
								/*IOP(4),IOP(3)*/
								Z80_ContentionIRBy1(7);
								switch ((opcode >> 4) & 0x03) {
								case 0: tmpW = this->bc.w; break;
								case 1: tmpW = this->de.w; break;
								case 2: tmpW = this->hl.w; break;
								default: tmpW = this->sp.w; break;
								}
								this->hl.w = (opcode & 0x08 ? Z80_ADC_DD(tmpW, this->hl.w) : Z80_SBC_DD(tmpW, this->hl.w));
								break;
								/* LD (nn),rr/LD rr,(nn) */
							case 3:
								tmpW = Z80_GetWordPC(0);
								this->memptr.w = (tmpW + 1) & 0xffff;
								if (opcode & 0x08) {
									/* LD rr,(nn) */
									switch ((opcode >> 4) & 0x03) {
									case 0: this->bc.w = Z80_PeekW6T(tmpW); break;
									case 1: this->de.w = Z80_PeekW6T(tmpW); break;
									case 2: this->hl.w = Z80_PeekW6T(tmpW); break;
									case 3: this->sp.w = Z80_PeekW6T(tmpW); break;
									}
								}
								else {
									/* LD (nn),rr */
									switch ((opcode >> 4) & 0x03) {
									case 0: Z80_PokeW6T(tmpW, this->bc.w); break;
									case 1: Z80_PokeW6T(tmpW, this->de.w); break;
									case 2: Z80_PokeW6T(tmpW, this->hl.w); break;
									case 3: Z80_PokeW6T(tmpW, this->sp.w); break;
									}
								}
								break;
								/* NEG */
							case 4:
								tmpB = this->af.a;
								this->af.a = 0;
								Z80_SUB_A(tmpB);
								break;
								/* RETI/RETN */
							case 5:
								/*RETI: 0x4d, 0x5d, 0x6d, 0x7d*/
								/*RETN: 0x45, 0x55, 0x65, 0x75*/
								this->iff1 = this->iff2;
								this->memptr.w = this->pc = Z80_Pop6T();
								if (opcode & 0x08) {
									/* RETI */
									if (this->retiFn(opcode)) return;
								}
								else {
									/* RETN */
									if (this->retnFn(opcode)) return;
								}
								break;
								/* IM n */
							case 6:
								switch (opcode) {
								case 0x56: case 0x76: this->im = 1; break;
								case 0x5e: case 0x7e: this->im = 2; break;
								default: this->im = 0; break;
								}
								break;
								/* specials */
							case 7:
								switch (opcode) {
									/* LD I,A */
								case 0x47:
									/*OCR(5)*/
									Z80_ContentionIRBy1(1);
									this->regI = this->af.a;
									break;
									/* LD R,A */
								case 0x4f:
									/*OCR(5)*/
									Z80_ContentionIRBy1(1);
									this->regR = this->af.a;
									break;
									/* LD A,I */
								case 0x57: Z80_LD_A_IR(this->regI); break;
									/* LD A,R */
								case 0x5f: Z80_LD_A_IR(this->regR); break;
									/* RRD */
								case 0x67: Z80_RRD_A(); break;
									/* RLD */
								case 0x6F: Z80_RLD_A(); break;
								}
							}
						}
						else {
							/* slt and other traps */
							if (this->trapEDFn(opcode)) return;
						}
						break;
					}
					continue;
				} /* 0xed done */
				/***/
				if (opcode == 0xcb) {
					/* shifts and bit operations */
					/* read opcode -- OCR(4) */
					if (!gotDD) {
						GET_OPCODE_EXT(opcode);
					}
					else {
						Z80_Contention(this->pc, 3, Z80_MREQ_READ | Z80_MEMIO_OPCEXT);
						opcode = this->memReadFn(this->pc, Z80_MEMIO_OPCEXT);
						Z80_ContentionPCBy1(2);
						INC_PC;
					}
					if (gotDD) {
						tmpW = ZADD_WX(this->dd->w, disp);
						tmpB = Z80_PeekB3T(tmpW);
						Z80_ContentionBy1(tmpW, 1);
					}
					else {
						switch (opcode & 0x07) {
						case 0: tmpB = this->bc.b; break;
						case 1: tmpB = this->bc.c; break;
						case 2: tmpB = this->de.d; break;
						case 3: tmpB = this->de.e; break;
						case 4: tmpB = this->hl.h; break;
						case 5: tmpB = this->hl.l; break;
						case 6: tmpB = Z80_PeekB3T(this->hl.w); Z80_Contention(this->hl.w, 1, Z80_MREQ_READ | Z80_MEMIO_DATA); break;
						case 7: tmpB = this->af.a; break;
						}
					}
					switch ((opcode >> 3) & 0x1f) {
					case 0: tmpB = Z80_RLC(tmpB); break;
					case 1: tmpB = Z80_RRC(tmpB); break;
					case 2: tmpB = Z80_RL(tmpB); break;
					case 3: tmpB = Z80_RR(tmpB); break;
					case 4: tmpB = Z80_SLA(tmpB); break;
					case 5: tmpB = Z80_SRA(tmpB); break;
					case 6: tmpB = Z80_SLL(tmpB); break;
					case 7: tmpB = Z80_SLR(tmpB); break;
					default:
						switch ((opcode >> 6) & 0x03) {
						case 1: Z80_BIT((opcode >> 3) & 0x07, tmpB, (gotDD || (opcode & 0x07) == 6)); break;
						case 2: tmpB &= ~(1 << ((opcode >> 3) & 0x07)); break; /* RES */
						case 3: tmpB |= (1 << ((opcode >> 3) & 0x07)); break; /* SET */
						}
						break;
					}
					/***/
					if ((opcode & 0xc0) != 0x40) {
						/* BITs are not welcome here */
						if (gotDD) {
							/* tmpW was set earlier */
							if ((opcode & 0x07) != 6) Z80_PokeB3T(tmpW, tmpB);
						}
						switch (opcode & 0x07) {
						case 0: this->bc.b = tmpB; break;
						case 1: this->bc.c = tmpB; break;
						case 2: this->de.d = tmpB; break;
						case 3: this->de.e = tmpB; break;
						case 4: this->hl.h = tmpB; break;
						case 5: this->hl.l = tmpB; break;
						case 6: Z80_PokeB3T(ZADD_WX(this->dd->w, disp), tmpB); break;
						case 7: this->af.a = tmpB; break;
						}
					}
					continue;
				} /* 0xcb done */
				/* normal things */
				switch (opcode & 0xc0) {
					/* 0x00..0x3F */
				case 0x00:
					switch (opcode & 0x07) {
						/* misc,DJNZ,JR,JR cc */
					case 0:
						if (opcode & 0x30) {
							/* branches */
							if (opcode & 0x20) {
								/* JR cc */
								switch ((opcode >> 3) & 0x03) {
								case 0: trueCC = (this->af.f & Z80_FLAG_Z) == 0; break;
								case 1: trueCC = (this->af.f & Z80_FLAG_Z) != 0; break;
								case 2: trueCC = (this->af.f & Z80_FLAG_C) == 0; break;
								case 3: trueCC = (this->af.f & Z80_FLAG_C) != 0; break;
								default: trueCC = 0; break;
								}
							}
							else {
								/* DJNZ/JR */
								if ((opcode & 0x08) == 0) {
									/* DJNZ */
									/*OCR(5)*/
									Z80_ContentionIRBy1(1);
									DEC_B(this->bc.b);
									trueCC = (this->bc.b != 0);
								}
								else {
									/* JR */
									trueCC = 1;
								}
							}
							/***/
							disp = Z80_PeekB3TA(this->pc);
							if (trueCC) {
								/* execute branch (relative) */
								/*IOP(5)*/
								if (disp > 127) disp -= 256;
								Z80_ContentionPCBy1(5);
								INC_PC;
								ZADD_W(this->pc, disp);
								this->memptr.w = this->pc;
							}
							else {
								INC_PC;
							}
						}
						else {
							/* EX AF,AF' or NOP */
							if (opcode != 0) Z80_EXAFAF();
						}
						break;
						/* LD rr,nn/ADD HL,rr */
					case 1:
						if (opcode & 0x08) {
							/* ADD HL,rr */
							/*IOP(4),IOP(3)*/
							Z80_ContentionIRBy1(7);
							switch ((opcode >> 4) & 0x03) {
							case 0: this->dd->w = Z80_ADD_DD(this->bc.w, this->dd->w); break;
							case 1: this->dd->w = Z80_ADD_DD(this->de.w, this->dd->w); break;
							case 2: this->dd->w = Z80_ADD_DD(this->dd->w, this->dd->w); break;
							case 3: this->dd->w = Z80_ADD_DD(this->sp.w, this->dd->w); break;
							}
						}
						else {
							/* LD rr,nn */
							tmpW = Z80_GetWordPC(0);
							switch ((opcode >> 4) & 0x03) {
							case 0: this->bc.w = tmpW; break;
							case 1: this->de.w = tmpW; break;
							case 2: this->dd->w = tmpW; break;
							case 3: this->sp.w = tmpW; break;
							}
						}
						break;
						/* LD xxx,xxx */
					case 2:
						switch ((opcode >> 3) & 0x07) {
							/* LD (BC),A */
						case 0: Z80_PokeB3T(this->bc.w, this->af.a); this->memptr.l = (this->bc.c + 1) & 0xff; this->memptr.h = this->af.a; break;
							/* LD A,(BC) */
						case 1: this->af.a = Z80_PeekB3T(this->bc.w); this->memptr.w = (this->bc.w + 1) & 0xffff; break;
							/* LD (DE),A */
						case 2: Z80_PokeB3T(this->de.w, this->af.a); this->memptr.l = (this->de.e + 1) & 0xff; this->memptr.h = this->af.a; break;
							/* LD A,(DE) */
						case 3: this->af.a = Z80_PeekB3T(this->de.w); this->memptr.w = (this->de.w + 1) & 0xffff; break;
							/* LD (nn),HL */
						case 4:
							tmpW = Z80_GetWordPC(0);
							this->memptr.w = (tmpW + 1) & 0xffff;
							Z80_PokeW6T(tmpW, this->dd->w);
							break;
							/* LD HL,(nn) */
						case 5:
							tmpW = Z80_GetWordPC(0);
							this->memptr.w = (tmpW + 1) & 0xffff;
							this->dd->w = Z80_PeekW6T(tmpW);
							break;
							/* LD (nn),A */
						case 6:
							tmpW = Z80_GetWordPC(0);
							this->memptr.l = (tmpW + 1) & 0xff;
							this->memptr.h = this->af.a;
							Z80_PokeB3T(tmpW, this->af.a);
							break;
							/* LD A,(nn) */
						case 7:
							tmpW = Z80_GetWordPC(0);
							this->memptr.w = (tmpW + 1) & 0xffff;
							this->af.a = Z80_PeekB3T(tmpW);
							break;
						}
						break;
						/* INC rr/DEC rr */
					case 3:
						/*OCR(6)*/
						Z80_ContentionIRBy1(2);
						if (opcode & 0x08) {
							/*DEC*/
							switch ((opcode >> 4) & 0x03) {
							case 0: DEC_W(this->bc.w); break;
							case 1: DEC_W(this->de.w); break;
							case 2: DEC_W(this->dd->w); break;
							case 3: DEC_W(this->sp.w); break;
							}
						}
						else {
							/*INC*/
							switch ((opcode >> 4) & 0x03) {
							case 0: INC_W(this->bc.w); break;
							case 1: INC_W(this->de.w); break;
							case 2: INC_W(this->dd->w); break;
							case 3: INC_W(this->sp.w); break;
							}
						}
						break;
						/* INC r8 */
					case 4:
						switch ((opcode >> 3) & 0x07) {
						case 0: this->bc.b = Z80_INC8(this->bc.b); break;
						case 1: this->bc.c = Z80_INC8(this->bc.c); break;
						case 2: this->de.d = Z80_INC8(this->de.d); break;
						case 3: this->de.e = Z80_INC8(this->de.e); break;
						case 4: this->dd->h = Z80_INC8(this->dd->h); break;
						case 5: this->dd->l = Z80_INC8(this->dd->l); break;
						case 6:
							if (gotDD) { DEC_PC; Z80_ContentionPCBy1(5); INC_PC; }
							tmpW = ZADD_WX(this->dd->w, disp);
							tmpB = Z80_PeekB3T(tmpW);
							Z80_ContentionBy1(tmpW, 1);
							tmpB = Z80_INC8(tmpB);
							Z80_PokeB3T(tmpW, tmpB);
							break;
						case 7: this->af.a = Z80_INC8(this->af.a); break;
						}
						break;
						/* DEC r8 */
					case 5:
						switch ((opcode >> 3) & 0x07) {
						case 0: this->bc.b = Z80_DEC8(this->bc.b); break;
						case 1: this->bc.c = Z80_DEC8(this->bc.c); break;
						case 2: this->de.d = Z80_DEC8(this->de.d); break;
						case 3: this->de.e = Z80_DEC8(this->de.e); break;
						case 4: this->dd->h = Z80_DEC8(this->dd->h); break;
						case 5: this->dd->l = Z80_DEC8(this->dd->l); break;
						case 6:
							if (gotDD) { DEC_PC; Z80_ContentionPCBy1(5); INC_PC; }
							tmpW = ZADD_WX(this->dd->w, disp);
							tmpB = Z80_PeekB3T(tmpW);
							Z80_ContentionBy1(tmpW, 1);
							tmpB = Z80_DEC8(tmpB);
							Z80_PokeB3T(tmpW, tmpB);
							break;
						case 7: this->af.a = Z80_DEC8(this->af.a); break;
						}
						break;
						/* LD r8,n */
					case 6:
						tmpB = Z80_PeekB3TA(this->pc);
						INC_PC;
						switch ((opcode >> 3) & 0x07) {
						case 0: this->bc.b = tmpB; break;
						case 1: this->bc.c = tmpB; break;
						case 2: this->de.d = tmpB; break;
						case 3: this->de.e = tmpB; break;
						case 4: this->dd->h = tmpB; break;
						case 5: this->dd->l = tmpB; break;
						case 6:
							if (gotDD) { DEC_PC; Z80_ContentionPCBy1(2); INC_PC; }
							tmpW = ZADD_WX(this->dd->w, disp);
							Z80_PokeB3T(tmpW, tmpB);
							break;
						case 7: this->af.a = tmpB; break;
						}
						break;
						/* swim-swim-hungry */
					case 7:
						switch ((opcode >> 3) & 0x07) {
						case 0: Z80_RLCA(); break;
						case 1: Z80_RRCA(); break;
						case 2: Z80_RLA(); break;
						case 3: Z80_RRA(); break;
						case 4: Z80_DAA(); break;
						case 5: /* CPL */
							this->af.a ^= 0xff;
							this->af.f = (this->af.a & Z80_FLAG_35) | (Z80_FLAG_N | Z80_FLAG_H) | (this->af.f & (Z80_FLAG_C | Z80_FLAG_PV | Z80_FLAG_Z | Z80_FLAG_S));
							break;
						case 6: /* SCF */
							this->af.f = (this->af.f & (Z80_FLAG_PV | Z80_FLAG_Z | Z80_FLAG_S)) | (this->af.a & Z80_FLAG_35) | Z80_FLAG_C;
							break;
						case 7: /* CCF */
							tmpB = this->af.f & Z80_FLAG_C;
							this->af.f = (this->af.f & (Z80_FLAG_PV | Z80_FLAG_Z | Z80_FLAG_S)) | (this->af.a & Z80_FLAG_35);
							this->af.f |= tmpB ? Z80_FLAG_H : Z80_FLAG_C;
							break;
						}
						break;
					}
					break;
					/* 0x40..0x7F (LD r8,r8) */
				case 0x40:
					if (opcode == 0x76) { this->halted = true; DEC_W(this->pc); continue; } /* HALT */
					rsrc = (opcode & 0x07);
					rdst = ((opcode >> 3) & 0x07);
					switch (rsrc) {
					case 0: tmpB = this->bc.b; break;
					case 1: tmpB = this->bc.c; break;
					case 2: tmpB = this->de.d; break;
					case 3: tmpB = this->de.e; break;
					case 4: tmpB = (gotDD && rdst == 6 ? this->hl.h : this->dd->h); break;
					case 5: tmpB = (gotDD && rdst == 6 ? this->hl.l : this->dd->l); break;
					case 6:
						if (gotDD) { DEC_PC; Z80_ContentionPCBy1(5); INC_PC; }
						tmpW = ZADD_WX(this->dd->w, disp);
						tmpB = Z80_PeekB3T(tmpW);
						break;
					case 7: tmpB = this->af.a; break;
					}
					switch (rdst) {
					case 0: this->bc.b = tmpB; break;
					case 1: this->bc.c = tmpB; break;
					case 2: this->de.d = tmpB; break;
					case 3: this->de.e = tmpB; break;
					case 4: if (gotDD && rsrc == 6) this->hl.h = tmpB; else this->dd->h = tmpB; break;
					case 5: if (gotDD && rsrc == 6) this->hl.l = tmpB; else this->dd->l = tmpB; break;
					case 6:
						if (gotDD) { DEC_PC; Z80_ContentionPCBy1(5); INC_PC; }
						tmpW = ZADD_WX(this->dd->w, disp);
						Z80_PokeB3T(tmpW, tmpB);
						break;
					case 7: this->af.a = tmpB; break;
					}
					break;
					/* 0x80..0xBF (ALU A,r8) */
				case 0x80:
					switch (opcode & 0x07) {
					case 0: tmpB = this->bc.b; break;
					case 1: tmpB = this->bc.c; break;
					case 2: tmpB = this->de.d; break;
					case 3: tmpB = this->de.e; break;
					case 4: tmpB = this->dd->h; break;
					case 5: tmpB = this->dd->l; break;
					case 6:
						if (gotDD) { DEC_PC; Z80_ContentionPCBy1(5); INC_PC; }
						tmpW = ZADD_WX(this->dd->w, disp);
						tmpB = Z80_PeekB3T(tmpW);
						break;
					case 7: tmpB = this->af.a; break;
					}
					switch ((opcode >> 3) & 0x07) {
					case 0: Z80_ADD_A(tmpB); break;
					case 1: Z80_ADC_A(tmpB); break;
					case 2: Z80_SUB_A(tmpB); break;
					case 3: Z80_SBC_A(tmpB); break;
					case 4: Z80_AND_A(tmpB); break;
					case 5: Z80_XOR_A(tmpB); break;
					case 6: Z80_OR_A(tmpB); break;
					case 7: Z80_CP_A(tmpB); break;
					}
					break;
					/* 0xC0..0xFF */
				case 0xC0:
					switch (opcode & 0x07) {
						/* RET cc */
					case 0:
						Z80_ContentionIRBy1(1);
						trueCC = SET_TRUE_CC(opcode);
						if (trueCC) this->memptr.w = this->pc = Z80_Pop6T();
						break;
						/* POP rr/special0 */
					case 1:
						if (opcode & 0x08) {
							/* special 0 */
							switch ((opcode >> 4) & 0x03) {
								/* RET */
							case 0: this->memptr.w = this->pc = Z80_Pop6T(); break;
								/* EXX */
							case 1: Z80_EXX(); break;
								/* JP (HL) */
							case 2: this->pc = this->dd->w; break;
								/* LD SP,HL */
							case 3:
								/*OCR(6)*/
								Z80_ContentionIRBy1(2);
								this->sp.w = this->dd->w;
								break;
							}
						}
						else {
							/* POP rr */
							tmpW = Z80_Pop6T();
							switch ((opcode >> 4) & 0x03) {
							case 0: this->bc.w = tmpW; break;
							case 1: this->de.w = tmpW; break;
							case 2: this->dd->w = tmpW; break;
							case 3: this->af.w = tmpW; break;
							}
						}
						break;
						/* JP cc,nn */
					case 2:
						trueCC = SET_TRUE_CC(opcode);
						this->memptr.w = Z80_GetWordPC(0);
						if (trueCC) this->pc = this->memptr.w;
						break;
						/* special1/special3 */
					case 3:
						switch ((opcode >> 3) & 0x07) {
							/* JP nn */
						case 0: this->memptr.w = this->pc = Z80_GetWordPC(0); break;
							/* OUT (n),A */
						case 2:
							tmpW = Z80_PeekB3TA(this->pc);
							INC_PC;
							this->memptr.l = (tmpW + 1) & 0xff;
							this->memptr.h = this->af.a;
							tmpW |= (((uint16_t)(this->af.a)) << 8);
							Z80_PortOut(tmpW, this->af.a);
							break;
							/* IN A,(n) */
						case 3:
							tmpW = (((uint16_t)(this->af.a)) << 8) | Z80_PeekB3TA(this->pc);
							INC_PC;
							this->memptr.w = (tmpW + 1) & 0xffff;
							this->af.a = Z80_PortIn(tmpW);
							break;
							/* EX (SP),HL */
						case 4:
							/*SRL(3),SRH(4)*/
							tmpW = Z80_PeekW6T(this->sp.w);
							Z80_ContentionBy1((this->sp.w + 1) & 0xffff, 1);
							/*SWL(3),SWH(5)*/
							Z80_PokeW6TInv(this->sp.w, this->dd->w);
							Z80_ContentionBy1(this->sp.w, 2);
							this->memptr.w = this->dd->w = tmpW;
							break;
							/* EX DE,HL */
						case 5:
							tmpW = this->de.w;
							this->de.w = this->hl.w;
							this->hl.w = tmpW;
							break;
							/* DI */
						case 6: this->iff1 = this->iff2 = false; break;
							/* EI */
						case 7: this->iff1 = this->iff2 = true; this->prev_was_EIDDR = 1; break;
						}
						break;
						/* CALL cc,nn */
					case 4:
						trueCC = SET_TRUE_CC(opcode);
						this->memptr.w = Z80_GetWordPC(trueCC);
						if (trueCC) {
							Z80_Push6T(this->pc);
							this->pc = this->memptr.w;
						}
						break;
						/* PUSH rr/special2 */
					case 5:
						if (opcode & 0x08) {
							if (((opcode >> 4) & 0x03) == 0) {
								/* CALL */
								this->memptr.w = tmpW = Z80_GetWordPC(1);
								Z80_Push6T(this->pc);
								this->pc = tmpW;
							}
						}
						else {
							/* PUSH rr */
							/*OCR(5)*/
							Z80_ContentionIRBy1(1);
							switch ((opcode >> 4) & 0x03) {
							case 0: tmpW = this->bc.w; break;
							case 1: tmpW = this->de.w; break;
							case 2: tmpW = this->dd->w; break;
							default: tmpW = this->af.w; break;
							}
							Z80_Push6T(tmpW);
						}
						break;
						/* ALU A,n */
					case 6:
						tmpB = Z80_PeekB3TA(this->pc);
						INC_PC;
						switch ((opcode >> 3) & 0x07) {
						case 0: Z80_ADD_A(tmpB); break;
						case 1: Z80_ADC_A(tmpB); break;
						case 2: Z80_SUB_A(tmpB); break;
						case 3: Z80_SBC_A(tmpB); break;
						case 4: Z80_AND_A(tmpB); break;
						case 5: Z80_XOR_A(tmpB); break;
						case 6: Z80_OR_A(tmpB); break;
						case 7: Z80_CP_A(tmpB); break;
						}
						break;
						/* RST nnn */
					case 7:
						/*OCR(5)*/
						Z80_ContentionIRBy1(1);
						Z80_Push6T(this->pc);
						this->memptr.w = this->pc = opcode & 0x38;
						break;
					}
					break;
				} /* end switch */
			}
		}
		int32_t Z80_ExecuteStep() /* returns number of executed ticks */
		{
			int32_t one = this->next_event_tstate, ots = this->tstates;
			/***/
			this->next_event_tstate = ots + 1;
			Z80_Execute();
			this->next_event_tstate = one;
			return this->tstates - ots;
		}
		int Z80_Interrupt() /* !0: interrupt was accepted (returns # of t-states eaten); changes tstates if interrupt occurs */
		{
			uint16_t a;
			int ots = this->tstates;
			/***/
			if (this->prev_was_EIDDR < 0) { this->prev_was_EIDDR = 0; this->af.f &= ~Z80_FLAG_PV; } /* Z80 bug */
			if (this->prev_was_EIDDR || !this->iff1) return 0; /* not accepted */
			if (this->halted) { this->halted = false; INC_PC; }
			this->iff1 = this->iff2 = false; /* disable interrupts */
			/***/
			switch ((this->im &= 0x03)) {
			case 3: /* ??? */ this->im = 0;
			case 0: /* take instruction from the bus (for now we assume that reading from bus always returns 0xff) */
			  /* with a CALL nnnn on the data bus, it takes 19 cycles: */
			  /* M1 cycle: 7 T to acknowledge interrupt (where exactly data bus reading occures?) */
			  /* M2 cycle: 3 T to read low byte of 'nnnn' from data bus */
			  /* M3 cycle: 3 T to read high byte of 'nnnn' and decrement SP */
			  /* M4 cycle: 3 T to write high byte of PC to the stack and decrement SP */
			  /* M5 cycle: 3 T to write low byte of PC and jump to 'nnnn' */
				this->tstates += 6;
			case 1: /* just do RST #38 */
				INC_R;
				this->tstates += 7; /* M1 cycle: 7 T to acknowledge interrupt and decrement SP */
				/* M2 cycle: 3 T states write high byte of PC to the stack and decrement SP */
				/* M3 cycle: 3 T states write the low byte of PC and jump to #0038 */
				Z80_Push6T(this->pc);
				this->memptr.w = this->pc = 0x38;
				break;
			case 2:
				INC_R;
				this->tstates += 7; /* M1 cycle: 7 T to acknowledge interrupt and decrement SP */
				/* M2 cycle: 3 T states write high byte of PC to the stack and decrement SP */
				/* M3 cycle: 3 T states write the low byte of PC */
				Z80_Push6T(this->pc);
				/* M4 cycle: 3 T to read high byte from the interrupt vector */
				/* M5 cycle: 3 T to read low byte from bus and jump to interrupt routine */
				a = (((uint16_t)this->regI) << 8) | 0xff;
				this->memptr.w = this->pc = Z80_PeekW6T(a);
				break;
			}
			return this->tstates - ots; /* accepted */
		}
		int Z80_NMI() /* !0: interrupt was accepted (returns # of t-states eaten); changes tstates if interrupt occurs */
		{
			int ots = this->tstates;
			/***/
			/* emulate Z80 bug with interrupted LD A,I/R */
			/*if (this->prev_was_EIDDR < 0) { this->prev_was_EIDDR = 0; this->af.f &= ~Z80_FLAG_PV; }*/
			/*if (this->prev_was_EIDDR) return 0;*/
			this->prev_was_EIDDR = 0; /* don't care */
			if (this->halted) { this->halted = false; INC_PC; }
			INC_R;
			this->iff1 = true; /* IFF2 is not changed */
			this->tstates += 5; /* M1 cycle: 5 T states to do an opcode read and decrement SP */
			/* M2 cycle: 3 T states write high byte of PC to the stack and decrement SP */
			/* M3 cycle: 3 T states write the low byte of PC and jump to #0066 */
			Z80_Push6T(this->pc);
			this->memptr.w = this->pc = 0x66;
			return this->tstates - ots;
		}

		/* without contention, using Z80_MEMIO_OTHER */
		uint16_t Z80_Pop()
		{
			uint16_t res = Z80_PeekBI(this->sp.w);
			/***/
			this->sp.w = (this->sp.w + 1) & 0xffff;
			res |= ((uint16_t)Z80_PeekBI(this->sp.w)) << 8;
			this->sp.w = (this->sp.w + 1) & 0xffff;
			return res;
		}

		void Z80_Push(uint16_t value)
		{
			this->sp.w = (((int32_t)this->sp.w) - 1) & 0xffff;
			Z80_PokeBI(this->sp.w, (value >> 8) & 0xff);
			this->sp.w = (((int32_t)this->sp.w) - 1) & 0xffff;
			Z80_PokeBI(this->sp.w, value & 0xff);
		}

		/* execute at least 'tstates' t-states; return real number of executed t-states */
		/* WARNING: this function resets both tstates and next_event_tstate! */
		int32_t Z80_ExecuteTS(int32_t _tstates)
		{
			if (_tstates > 0) {
				this->tstates = 0;
				this->next_event_tstate = _tstates;
				Z80_Execute();
				return this->tstates;
			}
			return 0;
		}
	};

#if defined(ZYMOSIS_FLAGS_IN_ARRAY)
	template <class T>
	bool Z80Cpu<T>::tablesInitialized = false;

	template <class T>
	uint8_t Z80Cpu<T>::sz53pTable[256]; /* bits 3, 5 and 7 of result, Z and P flags */
#endif

}; // zymosis namespace


#endif/*__ZYMOSIS_HPP__*/
