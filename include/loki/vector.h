/*! \file vector.h
 * \brief Fast memory operations for Loki. */

#ifndef LOKI_VECTOR_H_
#define LOKI_VECTOR_H_

#include <stdint.h>
#include <loki/channel_io.h>
#include <loki/sendconfig.h>

// Vector types.

//! Vector of  2   int8_t values. Pointers must be  2 byte aligned.
typedef   int8_t __attribute__((__vector_size__( 2)))    v2int8_t;
//! Vector of  4   int8_t values. Pointers must be  4 byte aligned.
typedef   int8_t __attribute__((__vector_size__( 4)))    v4int8_t;
//! Vector of  8   int8_t values. Pointers must be  8 byte aligned.
typedef   int8_t __attribute__((__vector_size__( 8)))    v8int8_t;
//! Vector of 16   int8_t values. Pointers must be 16 byte aligned.
typedef   int8_t __attribute__((__vector_size__(16)))   v16int8_t;
//! Vector of 32   int8_t values. Pointers must be 32 byte aligned.
typedef   int8_t __attribute__((__vector_size__(32)))   v32int8_t;
//! Vector of  2  uint8_t values. Pointers must be  2 byte aligned.
typedef  uint8_t __attribute__((__vector_size__( 2)))   v2uint8_t;
//! Vector of  4  uint8_t values. Pointers must be  4 byte aligned.
typedef  uint8_t __attribute__((__vector_size__( 4)))   v4uint8_t;
//! Vector of  8  uint8_t values. Pointers must be  8 byte aligned.
typedef  uint8_t __attribute__((__vector_size__( 8)))   v8uint8_t;
//! Vector of 16  uint8_t values. Pointers must be 16 byte aligned.
typedef  uint8_t __attribute__((__vector_size__(16)))  v16uint8_t;
//! Vector of 32  uint8_t values. Pointers must be 32 byte aligned.
typedef  uint8_t __attribute__((__vector_size__(32)))  v32uint8_t;
//! Vector of  2  int16_t values. Pointers must be  4 byte aligned.
typedef  int16_t __attribute__((__vector_size__( 4)))   v2int16_t;
//! Vector of  4  int16_t values. Pointers must be  8 byte aligned.
typedef  int16_t __attribute__((__vector_size__( 8)))   v4int16_t;
//! Vector of  8  int16_t values. Pointers must be 16 byte aligned.
typedef  int16_t __attribute__((__vector_size__(16)))   v8int16_t;
//! Vector of 16  int16_t values. Pointers must be 32 byte aligned.
typedef  int16_t __attribute__((__vector_size__(32)))  v16int16_t;
//! Vector of  2 uint16_t values. Pointers must be  4 byte aligned.
typedef uint16_t __attribute__((__vector_size__( 4)))  v2uint16_t;
//! Vector of  4 uint16_t values. Pointers must be  8 byte aligned.
typedef uint16_t __attribute__((__vector_size__( 8)))  v4uint16_t;
//! Vector of  8 uint16_t values. Pointers must be 16 byte aligned.
typedef uint16_t __attribute__((__vector_size__(16)))  v8uint16_t;
//! Vector of 16 uint16_t values. Pointers must be 32 byte aligned.
typedef uint16_t __attribute__((__vector_size__(32))) v16uint16_t;
//! Vector of  2  int32_t values. Pointers must be  8 byte aligned.
typedef  int32_t __attribute__((__vector_size__( 8)))   v2int32_t;
//! Vector of  4  int32_t values. Pointers must be 16 byte aligned.
typedef  int32_t __attribute__((__vector_size__(16)))   v4int32_t;
//! Vector of  8  int32_t values. Pointers must be 32 byte aligned.
typedef  int32_t __attribute__((__vector_size__(32)))   v8int32_t;
//! Vector of  2 uint32_t values. Pointers must be  8 byte aligned.
typedef uint32_t __attribute__((__vector_size__( 8)))  v2uint32_t;
//! Vector of  4 uint32_t values. Pointers must be 16 byte aligned.
typedef uint32_t __attribute__((__vector_size__(16)))  v4uint32_t;
//! Vector of  8 uint32_t values. Pointers must be 32 byte aligned.
typedef uint32_t __attribute__((__vector_size__(32)))  v8uint32_t;

//! Performes a store of 2 int8_t values as efficiently as possible.
//!
//! \remark Current implementation is 5 cycles.
static inline void loki_store_2_int8_t(
	  v2int8_t * const address
	, int8_t const v0, int8_t const v1
) {
	*address = (v2int8_t){v0, v1};
}

//! Performes a store of 4 int8_t values as efficiently as possible.
//!
//! \remark Current implementation is 9 cycles.
static inline void loki_store_4_int8_t(
	  v4int8_t * const address
	, int8_t const v0, int8_t const v1
	, int8_t const v2, int8_t const v3
) {
	*address = (v4int8_t){v0, v1, v2, v3};
}

//! Performes a store of 8 int8_t values as efficiently as possible.
//!
//! \remark Current implementation is 17 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline void loki_store_8_int8_t(
	  v8int8_t * const address
	, int8_t const v0, int8_t const v1
	, int8_t const v2, int8_t const v3
	, int8_t const v4, int8_t const v5
	, int8_t const v6, int8_t const v7
) {
	*address = (v8int8_t){v0, v1, v2, v3, v4, v5, v6, v7};
}

//! Performes a store of 16 int8_t values as efficiently as possible.
//!
//! \remark Current implementation is 33 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline void loki_store_16_int8_t(
	  v16int8_t * const address
	, int8_t const v0 , int8_t const v1
	, int8_t const v2 , int8_t const v3
	, int8_t const v4 , int8_t const v5
	, int8_t const v6 , int8_t const v7
	, int8_t const v8 , int8_t const v9
	, int8_t const v10, int8_t const v11
	, int8_t const v12, int8_t const v13
	, int8_t const v14, int8_t const v15
) {
	*address = (v16int8_t){
		  v0 , v1 , v2 , v3 , v4 , v5 , v6 , v7
		, v8 , v9 , v10, v11, v12, v13, v14, v15
	};
}

//! Performes a store of 32 int8_t values as efficiently as possible.
//!
//! \remark Poor performance unless several values are set from the same
//! register due to size of register file.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline void loki_store_32_int8_t(
	  v32int8_t * const address
	, int8_t const v0 , int8_t const v1
	, int8_t const v2 , int8_t const v3
	, int8_t const v4 , int8_t const v5
	, int8_t const v6 , int8_t const v7
	, int8_t const v8 , int8_t const v9
	, int8_t const v10, int8_t const v11
	, int8_t const v12, int8_t const v13
	, int8_t const v14, int8_t const v15
	, int8_t const v16, int8_t const v17
	, int8_t const v18, int8_t const v19
	, int8_t const v20, int8_t const v21
	, int8_t const v22, int8_t const v23
	, int8_t const v24, int8_t const v25
	, int8_t const v26, int8_t const v27
	, int8_t const v28, int8_t const v29
	, int8_t const v30, int8_t const v31
) {
	loki_channel_validate_cache_line(1, address);
	*address = (v32int8_t){
		  v0 , v1 , v2 , v3 , v4 , v5 , v6 , v7
		, v8 , v9 , v10, v11, v12, v13, v14, v15
		, v16, v17, v18, v19, v20, v21, v22, v23
		, v24, v25, v26, v27, v28, v29, v30, v31
	};
}

//! Performes a store of 2 uint8_t values as efficiently as possible.
//!
//! \remark Current implementation is 5 cycles.
static inline void loki_store_2_uint8_t(
	  v2uint8_t * const address
	, uint8_t const v0, uint8_t const v1
) {
	*(uint16_t*)address = (uint16_t)((uint32_t)v0 << 0 | (uint32_t)v1 << 8);
}

//! Performes a store of 4 uint8_t values as efficiently as possible.
//!
//! \remark Current implementation is 9 cycles.
static inline void loki_store_4_uint8_t(
	  v4uint8_t * const address
	, uint8_t const v0, uint8_t const v1
	, uint8_t const v2, uint8_t const v3
) {
	*(uint32_t*)address =
		  (uint32_t)v0 << 0  | (uint32_t)v1 << 8
		| (uint32_t)v2 << 16 | (uint32_t)v3 << 24;
}

//! Performes a store of 8 uint8_t values as efficiently as possible.
//!
//! \remark Current implementation is 17 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline void loki_store_8_uint8_t(
	  v8uint8_t * const address
	, uint8_t const v0, uint8_t const v1
	, uint8_t const v2, uint8_t const v3
	, uint8_t const v4, uint8_t const v5
	, uint8_t const v6, uint8_t const v7
) {
	*((uint32_t*)address + 0) =
		  (uint32_t)v0 << 0  | (uint32_t)v1 << 8
		| (uint32_t)v2 << 16 | (uint32_t)v3 << 24;
	*((uint32_t*)address + 1) =
		  (uint32_t)v4 << 0  | (uint32_t)v5 << 8
		| (uint32_t)v6 << 16 | (uint32_t)v7 << 24;
}

//! Performes a store of 16 uint8_t values as efficiently as possible.
//!
//! \remark Current implementation is 33 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline void loki_store_16_uint8_t(
	  v16uint8_t * const address
	, uint8_t const v0 , uint8_t const v1
	, uint8_t const v2 , uint8_t const v3
	, uint8_t const v4 , uint8_t const v5
	, uint8_t const v6 , uint8_t const v7
	, uint8_t const v8 , uint8_t const v9
	, uint8_t const v10, uint8_t const v11
	, uint8_t const v12, uint8_t const v13
	, uint8_t const v14, uint8_t const v15
) {
	*((uint32_t*)address + 0) =
		  (uint32_t)v0  << 0  | (uint32_t)v1  << 8
		| (uint32_t)v2  << 16 | (uint32_t)v3  << 24;
	*((uint32_t*)address + 1) =
		  (uint32_t)v4  << 0  | (uint32_t)v5  << 8
		| (uint32_t)v6  << 16 | (uint32_t)v7  << 24;
	*((uint32_t*)address + 2) =
		  (uint32_t)v8  << 0  | (uint32_t)v9  << 8
		| (uint32_t)v10 << 16 | (uint32_t)v11 << 24;
	*((uint32_t*)address + 3) =
		  (uint32_t)v12 << 0  | (uint32_t)v13 << 8
		| (uint32_t)v14 << 16 | (uint32_t)v15 << 24;
}

//! Performes a store of 32 uint8_t values as efficiently as possible.
//!
//! \remark Poor performance unless several values are set from the same
//! register due to size of register file.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline void loki_store_32_uint8_t(
	  v32uint8_t * const address
	, uint8_t const v0 , uint8_t const v1
	, uint8_t const v2 , uint8_t const v3
	, uint8_t const v4 , uint8_t const v5
	, uint8_t const v6 , uint8_t const v7
	, uint8_t const v8 , uint8_t const v9
	, uint8_t const v10, uint8_t const v11
	, uint8_t const v12, uint8_t const v13
	, uint8_t const v14, uint8_t const v15
	, uint8_t const v16, uint8_t const v17
	, uint8_t const v18, uint8_t const v19
	, uint8_t const v20, uint8_t const v21
	, uint8_t const v22, uint8_t const v23
	, uint8_t const v24, uint8_t const v25
	, uint8_t const v26, uint8_t const v27
	, uint8_t const v28, uint8_t const v29
	, uint8_t const v30, uint8_t const v31
) {
	loki_channel_store_cache_line(
		  1
		, address
		, (int)(
			(uint32_t)v0  << 0  | (uint32_t)v1  << 8  |
			(uint32_t)v2  << 16 | (uint32_t)v3  << 24
		  )
		, (int)(
			(uint32_t)v4  << 0  | (uint32_t)v5  << 8  |
			(uint32_t)v6  << 16 | (uint32_t)v7  << 24
		  )
		, (int)(
			(uint32_t)v8  << 0  | (uint32_t)v9  << 8  |
			(uint32_t)v10 << 16 | (uint32_t)v11 << 24
		  )
		, (int)(
			(uint32_t)v12 << 0  | (uint32_t)v13 << 8  |
			(uint32_t)v14 << 16 | (uint32_t)v15 << 24
		  )
		, (int)(
			(uint32_t)v16 << 0  | (uint32_t)v17 << 8  |
			(uint32_t)v18 << 16 | (uint32_t)v19 << 24
		  )
		, (int)(
			(uint32_t)v20 << 0  | (uint32_t)v21 << 8  |
			(uint32_t)v22 << 16 | (uint32_t)v23 << 24
		  )
		, (int)(
			(uint32_t)v24 << 0  | (uint32_t)v25 << 8  |
			(uint32_t)v26 << 16 | (uint32_t)v27 << 24
		  )
		, (int)(
			(uint32_t)v28 << 0  | (uint32_t)v29 << 8  |
			(uint32_t)v30 << 16 | (uint32_t)v31 << 24
		  )
	);
}

//! Performes a store of 2 int16_t values as efficiently as possible.
//!
//! \remark Current implementation is 5 cycles.
static inline void loki_store_2_int16_t(
	  v2int16_t * const address
	, int16_t const v0, int16_t const v1
) {
	*address = (v2int16_t){v0, v1};
}

//! Performes a store of 4 int16_t values as efficiently as possible.
//!
//! \remark Current implementation is 9 cycles.
static inline void loki_store_4_int16_t(
	  v4int16_t * const address
	, int16_t const v0, int16_t const v1
	, int16_t const v2, int16_t const v3
) {
	*address = (v4int16_t){v0, v1, v2, v3};
}

//! Performes a store of 8 int16_t values as efficiently as possible.
//!
//! \remark Current implementation is 17 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline void loki_store_8_int16_t(
	  v8int16_t * const address
	, int16_t const v0, int16_t const v1
	, int16_t const v2, int16_t const v3
	, int16_t const v4, int16_t const v5
	, int16_t const v6, int16_t const v7
) {
	*address = (v8int16_t){v0, v1, v2, v3, v4, v5, v6, v7};
}

//! Performes a store of 16 int16_t values as efficiently as possible.
//!
//! \remark Current implementation is 35 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline void loki_store_16_int16_t(
	  v16int16_t * const address
	, int16_t const v0 , int16_t const v1
	, int16_t const v2 , int16_t const v3
	, int16_t const v4 , int16_t const v5
	, int16_t const v6 , int16_t const v7
	, int16_t const v8 , int16_t const v9
	, int16_t const v10, int16_t const v11
	, int16_t const v12, int16_t const v13
	, int16_t const v14, int16_t const v15
) {
	loki_channel_validate_cache_line(1, address);
	*address = (v16int16_t){
		  v0 , v1 , v2 , v3 , v4 , v5 , v6 , v7
		, v8 , v9 , v10, v11, v12, v13, v14, v15
	};
}

//! Performes a store of 2 uint16_t values as efficiently as possible.
//!
//! \remark Current implementation is 5 cycles.
static inline void loki_store_2_uint16_t(
	  v2uint16_t * const address
	, uint16_t const v0, uint16_t const v1
) {
	*(uint32_t*)address = ((uint32_t)v0 << 0 | (uint32_t)v1 << 16);
}

//! Performes a store of 4 uint16_t values as efficiently as possible.
//!
//! \remark Current implementation is 9 cycles.
static inline void loki_store_4_uint16_t(
	  v4uint16_t * const address
	, uint16_t const v0, uint16_t const v1
	, uint16_t const v2, uint16_t const v3
) {
	*((uint32_t*)address + 0) = ((uint32_t)v0 << 0 | (uint32_t)v1 << 16);
	*((uint32_t*)address + 1) = ((uint32_t)v2 << 0 | (uint32_t)v3 << 16);
}

//! Performes a store of 8 uint16_t values as efficiently as possible.
//!
//! \remark Current implementation is 17 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline void loki_store_8_uint16_t(
	  v8uint16_t * const address
	, uint16_t const v0, uint16_t const v1
	, uint16_t const v2, uint16_t const v3
	, uint16_t const v4, uint16_t const v5
	, uint16_t const v6, uint16_t const v7
) {
	*((uint32_t*)address + 0) = ((uint32_t)v0 << 0 | (uint32_t)v1 << 16);
	*((uint32_t*)address + 1) = ((uint32_t)v2 << 0 | (uint32_t)v3 << 16);
	*((uint32_t*)address + 2) = ((uint32_t)v4 << 0 | (uint32_t)v5 << 16);
	*((uint32_t*)address + 3) = ((uint32_t)v6 << 0 | (uint32_t)v7 << 16);
}

//! Performes a store of 16 uint16_t values as efficiently as possible.
//!
//! \remark Current implementation is 28 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline void loki_store_16_uint16_t(
	  v16uint16_t * const address
	, uint16_t const v0 , uint16_t const v1
	, uint16_t const v2 , uint16_t const v3
	, uint16_t const v4 , uint16_t const v5
	, uint16_t const v6 , uint16_t const v7
	, uint16_t const v8 , uint16_t const v9
	, uint16_t const v10, uint16_t const v11
	, uint16_t const v12, uint16_t const v13
	, uint16_t const v14, uint16_t const v15
) {
	loki_channel_store_cache_line(
		  1
		, address
		, (int)((uint32_t)v0  << 0 | (uint32_t)v1  << 16)
		, (int)((uint32_t)v2  << 0 | (uint32_t)v3  << 16)
		, (int)((uint32_t)v4  << 0 | (uint32_t)v5  << 16)
		, (int)((uint32_t)v6  << 0 | (uint32_t)v7  << 16)
		, (int)((uint32_t)v8  << 0 | (uint32_t)v9  << 16)
		, (int)((uint32_t)v10 << 0 | (uint32_t)v11 << 16)
		, (int)((uint32_t)v12 << 0 | (uint32_t)v13 << 16)
		, (int)((uint32_t)v14 << 0 | (uint32_t)v15 << 16)
	);
}

//! Performes a store of 2 int32_t values as efficiently as possible.
//!
//! \remark Current implementation is 5 cycles.
static inline void loki_store_2_int32_t(
	  v2int32_t * const address
	, int32_t const v0, int32_t const v1
) {
	*address = (v2int32_t){v0, v1};
}

//! Performes a store of 4 int32_t values as efficiently as possible.
//!
//! \remark Current implementation is 9 cycles.
static inline void loki_store_4_int32_t(
	  v4int32_t * const address
	, int32_t const v0, int32_t const v1
	, int32_t const v2, int32_t const v3
) {
	*address = (v4int32_t){v0, v1, v2, v3};
}

//! Performes a store of 8 int32_t values as efficiently as possible.
//!
//! \remark Current implementation is 12 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline void loki_store_8_int32_t(
	  v8int32_t * const address
	, int32_t const v0, int32_t const v1
	, int32_t const v2, int32_t const v3
	, int32_t const v4, int32_t const v5
	, int32_t const v6, int32_t const v7
) {
	loki_channel_store_cache_line(1, address, v0, v1, v2, v3, v4, v5, v6, v7);
}

//! Performes a store of 2 uint32_t values as efficiently as possible.
//!
//! \remark Current implementation is 5 cycles.
static inline void loki_store_2_uint32_t(
	  v2uint32_t * const address
	, uint32_t const v0, uint32_t const v1
) {
	*address = (v2uint32_t){v0, v1};
}

//! Performes a store of 4 uint32_t values as efficiently as possible.
//!
//! \remark Current implementation is 9 cycles.
static inline void loki_store_4_uint32_t(
	  v4uint32_t * const address
	, uint32_t const v0, uint32_t const v1
	, uint32_t const v2, uint32_t const v3
) {
	*address = (v4uint32_t){v0, v1, v2, v3};
}

//! Performes a store of 8 uint32_t values as efficiently as possible.
//!
//! \remark Current implementation is 12 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline void loki_store_8_uint32_t(
	  v8uint32_t * const address
	, uint32_t const v0, uint32_t const v1
	, uint32_t const v2, uint32_t const v3
	, uint32_t const v4, uint32_t const v5
	, uint32_t const v6, uint32_t const v7
) {
	loki_channel_store_cache_line(
		  1, address
		, (int)v0, (int)v1, (int)v2, (int)v3
		, (int)v4, (int)v5, (int)v6, (int)v7
	);
}

//! Performes a load of 2 int8_t values as efficiently as possible.
//!
//! \remark Current implementation is 8 cycles.
static inline v2int8_t loki_load_v2int8_t(
	v2int8_t const * const address
) {
	v2int8_t register result;
	asm (
		"fetchr 0f\n"
		"ldhwu 0(%[address]) -> 1\n"
		"srli %1, r2, 16\n"
		"srli %0, %1, 8\n"
		"srai %0, %0, 24\n"
		"srai.eop %1, %1, 24\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 4 int8_t values as efficiently as possible.
//!
//! \remark Current implementation is 13 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v4int8_t loki_load_v4int8_t(
	v4int8_t const * const address
) {
	v4int8_t register result;
	asm (
		"fetchr 0f\n"
		"ldw 0(%[address]) -> 1\n"
		"or %3, r2, r0\n"
		"srli %0, %3, 24\n"
		"srli %1, %3, 16\n"
		"srli %2, %3, 8\n"
		"srai %0, %0, 24\n"
		"srai %1, %1, 24\n"
		"srai %2, %2, 24\n"
		"srai.eop %3, %3, 24\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1]), "=r"(result[2]), "=r"(result[3])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 8 int8_t values as efficiently as possible.
//!
//! \remark Current implementation is 24 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v8int8_t loki_load_v8int8_t(
	v8int8_t const * const address
) {
	v8int8_t register result;
	asm (
		"fetchr 0f\n"
		"ldw 0(%[address]) -> 1\n"
		"ldw 4(%[address]) -> 1\n"
		"or %3, r2, r0\n"
		"srli %0, %3, 24\n"
		"srli %1, %3, 16\n"
		"srli %2, %3, 8\n"
		"srai %0, %0, 24\n"
		"srai %1, %1, 24\n"
		"srai %2, %2, 24\n"
		"srai %3, %3, 24\n"
		"or %7, r2, r0\n"
		"srli %4, %7, 24\n"
		"srli %5, %7, 16\n"
		"srli %6, %7, 8\n"
		"srai %4, %4, 24\n"
		"srai %5, %5, 24\n"
		"srai %6, %6, 24\n"
		"srai.eop %7, %7, 24\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1]), "=r"(result[2]), "=r"(result[3])
	, "=r"(result[4]), "=r"(result[5]), "=r"(result[6]), "=r"(result[7])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 16 int8_t values as efficiently as possible.
//!
//! \remark Current implementation is 38 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v16int8_t loki_load_v16int8_t(
	v16int8_t const * const address
) {
	v16int8_t register result;
	asm (
		"fetchr 0f\n"
		"ldw  0(%[address]) -> 1\n"
		"ldw  4(%[address]) -> 1\n"
		"ldw  8(%[address]) -> 1\n"
		"ldw 12(%[address]) -> 1\n"
		"or %3, r2, r0\n"
		"srli %0, %3, 24\n"
		"srli %1, %3, 16\n"
		"srli %2, %3, 8\n"
		"srai %0, %0, 24\n"
		"srai %1, %1, 24\n"
		"srai %2, %2, 24\n"
		"srai %3, %3, 24\n"
		"or %7, r2, r0\n"
		"srli %4, %7, 24\n"
		"srli %5, %7, 16\n"
		"srli %6, %7, 8\n"
		"srai %4, %4, 24\n"
		"srai %5, %5, 24\n"
		"srai %6, %6, 24\n"
		"srai %7, %7, 24\n"
		"or %11, r2, r0\n"
		"srli %8, %11, 24\n"
		"srli %9, %11, 16\n"
		"srli %10, %11, 8\n"
		"srai %8, %8, 24\n"
		"srai %9, %9, 24\n"
		"srai %10, %10, 24\n"
		"srai %11, %11, 24\n"
		"or %15, r2, r0\n"
		"srli %12, %15, 24\n"
		"srli %13, %15, 16\n"
		"srli %14, %15, 8\n"
		"srai %12, %12, 24\n"
		"srai %13, %13, 24\n"
		"srai %14, %14, 24\n"
		"srai.eop %15, %15, 24\n"
		"0:\n"
	: "=r"(result[ 0]), "=r"(result[ 1]), "=r"(result[ 2]), "=r"(result[ 3])
	, "=r"(result[ 4]), "=r"(result[ 5]), "=r"(result[ 6]), "=r"(result[ 7])
	, "=r"(result[ 8]), "=r"(result[ 9]), "=r"(result[10]), "=r"(result[11])
	, "=r"(result[12]), "=r"(result[13]), "=r"(result[14]), "=r"(result[15])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 2 uint8_t values as efficiently as possible.
//!
//! \remark Current implementation is 6 cycles.
static inline v2uint8_t loki_load_v2uint8_t(
	v2uint8_t const * const address
) {
	v2uint8_t register result;
	asm (
		"fetchr 0f\n"
		"ldbu 0(%[address]) -> 1\n"
		"ldbu 1(%[address]) -> 1\n"
		"or %0, r2, r0\n"
		"or.eop %1, r2, r0\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 4 uint8_t values as efficiently as possible.
//!
//! \remark Current implementation is 9 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v4uint8_t loki_load_v4uint8_t(
	v4uint8_t const * const address
) {
	v4uint8_t register result;
	asm (
		"fetchr 0f\n"
		"ldbu 0(%[address]) -> 1\n"
		"ldbu 1(%[address]) -> 1\n"
		"ldbu 2(%[address]) -> 1\n"
		"ldbu 3(%[address]) -> 1\n"
		"or %0, r2, r0\n"
		"or %1, r2, r0\n"
		"or %2, r2, r0\n"
		"or.eop %3, r2, r0\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1]), "=r"(result[2]), "=r"(result[3])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 8 uint8_t values as efficiently as possible.
//!
//! \remark Current implementation is 17 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v8uint8_t loki_load_v8uint8_t(
	v8uint8_t const * const address
) {
	v8uint8_t register result;
	asm (
		"fetchr 0f\n"
		"ldbu 0(%[address]) -> 1\n"
		"ldbu 1(%[address]) -> 1\n"
		"ldbu 2(%[address]) -> 1\n"
		"ldbu 3(%[address]) -> 1\n"
		"or %0, r2, r0\n"
		"or %1, r2, r0\n"
		"or %2, r2, r0\n"
		"or %3, r2, r0\n"
		"ldbu 4(%[address]) -> 1\n"
		"ldbu 5(%[address]) -> 1\n"
		"ldbu 6(%[address]) -> 1\n"
		"ldbu 7(%[address]) -> 1\n"
		"or %4, r2, r0\n"
		"or %5, r2, r0\n"
		"or %6, r2, r0\n"
		"or.eop %7, r2, r0\n"
		"0:\n"
	: "=&r"(result[0]), "=&r"(result[1]), "=&r"(result[2]), "=&r"(result[3])
	, "=r" (result[4]), "=r" (result[5]), "=r" (result[6]), "=r" (result[7])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 16 uint8_t values as efficiently as possible.
//!
//! \remark Current implementation is 33 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v16uint8_t loki_load_v16uint8_t(
	v16uint8_t const * const address
) {
	v16uint8_t register result;
	asm (
		"fetchr 0f\n"
		"ldbu 0(%[address]) -> 1\n"
		"ldbu 1(%[address]) -> 1\n"
		"ldbu 2(%[address]) -> 1\n"
		"ldbu 3(%[address]) -> 1\n"
		"or %0, r2, r0\n"
		"or %1, r2, r0\n"
		"or %2, r2, r0\n"
		"or %3, r2, r0\n"
		"ldbu 4(%[address]) -> 1\n"
		"ldbu 5(%[address]) -> 1\n"
		"ldbu 6(%[address]) -> 1\n"
		"ldbu 7(%[address]) -> 1\n"
		"or %4, r2, r0\n"
		"or %5, r2, r0\n"
		"or %6, r2, r0\n"
		"or %7, r2, r0\n"
		"ldbu 8(%[address]) -> 1\n"
		"ldbu 9(%[address]) -> 1\n"
		"ldbu 10(%[address]) -> 1\n"
		"ldbu 11(%[address]) -> 1\n"
		"or %8, r2, r0\n"
		"or %9, r2, r0\n"
		"or %10, r2, r0\n"
		"or %11, r2, r0\n"
		"ldbu 12(%[address]) -> 1\n"
		"ldbu 13(%[address]) -> 1\n"
		"ldbu 14(%[address]) -> 1\n"
		"ldbu 15(%[address]) -> 1\n"
		"or %12, r2, r0\n"
		"or %13, r2, r0\n"
		"or %14, r2, r0\n"
		"or.eop %15, r2, r0\n"
		"0:\n"
	: "=&r"(result[ 0]), "=&r"(result[ 1])
	, "=&r"(result[ 2]), "=&r"(result[ 3])
	, "=&r"(result[ 4]), "=&r"(result[ 5])
	, "=&r"(result[ 6]), "=&r"(result[ 7])
	, "=&r"(result[ 8]), "=&r"(result[ 9])
	, "=&r"(result[10]), "=&r"(result[11])
	, "=r" (result[12]), "=r" (result[13])
	, "=r" (result[14]), "=r" (result[15])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 2 int16_t values as efficiently as possible.
//!
//! \remark Current implementation is 8 cycles.
static inline v2int16_t loki_load_v2int16_t(
	v2int16_t const * const address
) {
	v2int16_t register result;
	asm (
		"fetchr 0f\n"
		"ldw 0(%[address]) -> 1\n"
		"or %1, r2, r0\n"
		"srli %0, %1, 16\n"
		"srai %0, %0, 16\n"
		"srai.eop %1, %1, 16\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 4 int16_t values as efficiently as possible.
//!
//! \remark Current implementation is 13 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v4int16_t loki_load_v4int16_t(
	v4int16_t const * const address
) {
	v4int16_t register result;
	asm (
		"fetchr 0f\n"
		"ldw 0(%[address]) -> 1\n"
		"ldw 4(%[address]) -> 1\n"
		"or %1, r2, r0\n"
		"srli %0, %1, 16\n"
		"or %3, r2, r0\n"
		"srli %2, %3, 16\n"
		"srai %0, %0, 16\n"
		"srai %1, %1, 16\n"
		"srai %2, %2, 16\n"
		"srai.eop %3, %3, 16\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1]), "=r"(result[2]), "=r"(result[3])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 8 int16_t values as efficiently as possible.
//!
//! \remark Current implementation is 21 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v8int16_t loki_load_v8int16_t(
	v8int16_t const * const address
) {
	v8int16_t register result;
	asm (
		"fetchr 0f\n"
		"ldw 0(%[address]) -> 1\n"
		"ldw 4(%[address]) -> 1\n"
		"ldw 8(%[address]) -> 1\n"
		"ldw 16(%[address]) -> 1\n"
		"or %1, r2, r0\n"
		"srli %0, %1, 16\n"
		"or %3, r2, r0\n"
		"srli %2, %3, 16\n"
		"or %5, r2, r0\n"
		"srli %4, %5, 16\n"
		"or %7, r2, r0\n"
		"srli %6, %7, 16\n"
		"srai %0, %0, 16\n"
		"srai %1, %1, 16\n"
		"srai %2, %2, 16\n"
		"srai %3, %3, 16\n"
		"srai %4, %4, 16\n"
		"srai %5, %5, 16\n"
		"srai %6, %6, 16\n"
		"srai.eop %7, %7, 16\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1]), "=r"(result[2]), "=r"(result[3])
	, "=r"(result[4]), "=r"(result[5]), "=r"(result[6]), "=r"(result[7])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 16 int16_t values as efficiently as possible.
//!
//! \remark Current implementation is 37 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v16int16_t loki_load_v16int16_t(
	v16int16_t const * const address
) {
	v16int16_t register result;
	asm (
		"fetchr.eop 0f\n"
		".p2align 5\n"
		"0:\n"
		"sendconfig %[address], %17 -> 1\n"
		"fetchr 0f\n"
		"or %1, r2, r0\n"
		"or %3, r2, r0\n"
		"or %5, r2, r0\n"
		"or %7, r2, r0\n"
		"or %9, r2, r0\n"
		"or %11, r2, r0\n"
		"or %13, r2, r0\n"
		"or %15, r2, r0\n"
		"srli %0, %1, 16\n"
		"srli %2, %3, 16\n"
		"srli %4, %5, 16\n"
		"srli %6, %7, 16\n"
		"srli %8, %9, 16\n"
		"srli %10, %11, 16\n"
		"srli %12, %13, 16\n"
		"srli %14, %15, 16\n"
		"srai %0, %0, 16\n"
		"srai %1, %1, 16\n"
		"srai %2, %2, 16\n"
		"srai %3, %3, 16\n"
		"srai %4, %4, 16\n"
		"srai %5, %5, 16\n"
		"srai %6, %6, 16\n"
		"srai %7, %7, 16\n"
		"srai %8, %8, 16\n"
		"srai %9, %9, 16\n"
		"srai %10, %10, 16\n"
		"srai %11, %11, 16\n"
		"srai %12, %12, 16\n"
		"srai %13, %13, 16\n"
		"srai %14, %14, 16\n"
		"srai.eop %15, %15, 16\n"
		"0:\n"
	: "=r"(result[ 0]), "=r"(result[ 1]), "=r"(result[ 2]), "=r"(result[ 3])
	, "=r"(result[ 4]), "=r"(result[ 5]), "=r"(result[ 6]), "=r"(result[ 7])
	, "=r"(result[ 8]), "=r"(result[ 9]), "=r"(result[10]), "=r"(result[11])
	, "=r"(result[12]), "=r"(result[13]), "=r"(result[14]), "=r"(result[15])
	: [address] "r"(address), "n" (SC_FETCH_LINE)
	: "memory"
	);
	return result;
}

//! Performes a load of 2 uint16_t values as efficiently as possible.
//!
//! \remark Current implementation is 6 cycles.
static inline v2uint16_t loki_load_v2uint16_t(
	v2uint16_t const * const address
) {
	v2uint16_t register result;
	asm (
		"fetchr 0f\n"
		"ldhwu 0(%[address]) -> 1\n"
		"ldhwu 2(%[address]) -> 1\n"
		"or %0, r2, r0\n"
		"or.eop %1, r2, r0\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 4 uint16_t values as efficiently as possible.
//!
//! \remark Current implementation is 9 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v4uint16_t loki_load_v4uint16_t(
	v4uint16_t const * const address
) {
	v4uint16_t register result;
	asm (
		"fetchr 0f\n"
		"ldhwu 0(%[address]) -> 1\n"
		"ldhwu 2(%[address]) -> 1\n"
		"ldhwu 4(%[address]) -> 1\n"
		"ldhwu 6(%[address]) -> 1\n"
		"or %0, r2, r0\n"
		"or %1, r2, r0\n"
		"or %2, r2, r0\n"
		"or.eop %3, r2, r0\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1]), "=r"(result[2]), "=r"(result[3])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 8 uint16_t values as efficiently as possible.
//!
//! \remark Current implementation is 17 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v8uint16_t loki_load_v8uint16_t(
	v8uint16_t const * const address
) {
	v8uint16_t register result;
	asm (
		"fetchr 0f\n"
		"ldhwu 0(%[address]) -> 1\n"
		"ldhwu 2(%[address]) -> 1\n"
		"ldhwu 4(%[address]) -> 1\n"
		"ldhwu 6(%[address]) -> 1\n"
		"or %0, r2, r0\n"
		"or %1, r2, r0\n"
		"or %2, r2, r0\n"
		"or %3, r2, r0\n"
		"ldhwu 8(%[address]) -> 1\n"
		"ldhwu 10(%[address]) -> 1\n"
		"ldhwu 12(%[address]) -> 1\n"
		"ldhwu 14(%[address]) -> 1\n"
		"or %4, r2, r0\n"
		"or %5, r2, r0\n"
		"or %6, r2, r0\n"
		"or.eop %7, r2, r0\n"
		"0:\n"
	: "=&r"(result[0]), "=&r"(result[1]), "=&r"(result[2]), "=&r"(result[3])
	, "=r" (result[4]), "=r" (result[5]), "=r" (result[6]), "=r" (result[7])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 16 uint16_t values as efficiently as possible.
//!
//! \remark Current implementation is 33 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v16uint16_t loki_load_v16uint16_t(
	v16uint16_t const * const address
) {
	v16uint16_t register result;
	asm (
		"fetchr 0f\n"
		"ldhwu 0(%[address]) -> 1\n"
		"ldhwu 2(%[address]) -> 1\n"
		"ldhwu 4(%[address]) -> 1\n"
		"ldhwu 6(%[address]) -> 1\n"
		"or %0, r2, r0\n"
		"or %1, r2, r0\n"
		"or %2, r2, r0\n"
		"or %3, r2, r0\n"
		"ldhwu 8(%[address]) -> 1\n"
		"ldhwu 10(%[address]) -> 1\n"
		"ldhwu 12(%[address]) -> 1\n"
		"ldhwu 14(%[address]) -> 1\n"
		"or %4, r2, r0\n"
		"or %5, r2, r0\n"
		"or %6, r2, r0\n"
		"or %7, r2, r0\n"
		"ldhwu 16(%[address]) -> 1\n"
		"ldhwu 18(%[address]) -> 1\n"
		"ldhwu 20(%[address]) -> 1\n"
		"ldhwu 22(%[address]) -> 1\n"
		"or %8, r2, r0\n"
		"or %9, r2, r0\n"
		"or %10, r2, r0\n"
		"or %11, r2, r0\n"
		"ldhwu 24(%[address]) -> 1\n"
		"ldhwu 26(%[address]) -> 1\n"
		"ldhwu 28(%[address]) -> 1\n"
		"ldhwu 30(%[address]) -> 1\n"
		"or %12, r2, r0\n"
		"or %13, r2, r0\n"
		"or %14, r2, r0\n"
		"or.eop %15, r2, r0\n"
		"0:\n"
	: "=&r"(result[ 0]), "=&r"(result[ 1])
	, "=&r"(result[ 2]), "=&r"(result[ 3])
	, "=&r"(result[ 4]), "=&r"(result[ 5])
	, "=&r"(result[ 6]), "=&r"(result[ 7])
	, "=&r"(result[ 8]), "=&r"(result[ 9])
	, "=&r"(result[10]), "=&r"(result[11])
	, "=r" (result[12]), "=r" (result[13])
	, "=r" (result[14]), "=r" (result[15])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 2 int32_t values as efficiently as possible.
//!
//! \remark Current implementation is 6 cycles.
static inline v2int32_t loki_load_v2int32_t(
	v2int32_t const * const address
) {
	v2int32_t register result;
	asm (
		"fetchr 0f\n"
		"ldw 0(%[address]) -> 1\n"
		"ldw 4(%[address]) -> 1\n"
		"or %0, r2, r0\n"
		"or.eop %1, r2, r0\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 4 int32_t values as efficiently as possible.
//!
//! \remark Current implementation is 9 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v4int32_t loki_load_v4int32_t(
	v4int32_t const * const address
) {
	v4int32_t register result;
	asm (
		"fetchr 0f\n"
		"ldw 0(%[address]) -> 1\n"
		"ldw 4(%[address]) -> 1\n"
		"ldw 8(%[address]) -> 1\n"
		"ldw 12(%[address]) -> 1\n"
		"or %0, r2, r0\n"
		"or %1, r2, r0\n"
		"or %2, r2, r0\n"
		"or.eop %3, r2, r0\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1]), "=r"(result[2]), "=r"(result[3])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 8 int32_t values as efficiently as possible.
//!
//! \remark Current implementation is 12 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v8int32_t loki_load_v8int32_t(
	v8int32_t const * const address
) {
	v8int32_t register result;
	asm (
		"fetchr.eop 0f\n"
		".p2align 5\n"
		"0:\n"
		"sendconfig %[address], %9 -> 1\n"
		"fetchr 0f\n"
		"or %0, r2, r0\n"
		"or %1, r2, r0\n"
		"or %2, r2, r0\n"
		"or %3, r2, r0\n"
		"or %4, r2, r0\n"
		"or %5, r2, r0\n"
		"or %6, r2, r0\n"
		"or.eop %7, r2, r0\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1]), "=r"(result[2]), "=r"(result[3])
	, "=r"(result[4]), "=r"(result[5]), "=r"(result[6]), "=r"(result[7])
	: [address] "r"(address), "n" (SC_FETCH_LINE)
	: "memory"
	);
	return result;
}

//! Performes a load of 2 uint32_t values as efficiently as possible.
//!
//! \remark Current implementation is 6 cycles.
static inline v2uint32_t loki_load_v2uint32_t(
	v2uint32_t const * const address
) {
	v2uint32_t register result;
	asm (
		"fetchr 0f\n"
		"ldw 0(%[address]) -> 1\n"
		"ldw 2(%[address]) -> 1\n"
		"or %0, r2, r0\n"
		"or.eop %1, r2, r0\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 4 uint32_t values as efficiently as possible.
//!
//! \remark Current implementation is 9 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v4uint32_t loki_load_v4uint32_t(
	v4uint32_t const * const address
) {
	v4uint32_t register result;
	asm (
		"fetchr 0f\n"
		"ldw 0(%[address]) -> 1\n"
		"ldw 2(%[address]) -> 1\n"
		"ldw 4(%[address]) -> 1\n"
		"ldw 6(%[address]) -> 1\n"
		"or %0, r2, r0\n"
		"or %1, r2, r0\n"
		"or %2, r2, r0\n"
		"or.eop %3, r2, r0\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1]), "=r"(result[2]), "=r"(result[3])
	: [address] "r"(address)
	: "memory"
	);
	return result;
}

//! Performes a load of 8 uint32_t values as efficiently as possible.
//!
//! \remark Current implementation is 12 cycles.
//!
//! \remark Poor performance if inlining is not enabled in the compiler due to
//! ABI.
static inline v8uint32_t loki_load_v8uint32_t(
	v8uint32_t const * const address
) {
	v8uint32_t register result;
	asm (
		"fetchr.eop 0f\n"
		".p2align 5\n"
		"0:\n"
		"sendconfig %[address], %9 -> 1\n"
		"fetchr 0f\n"
		"or %0, r2, r0\n"
		"or %1, r2, r0\n"
		"or %2, r2, r0\n"
		"or %3, r2, r0\n"
		"or %4, r2, r0\n"
		"or %5, r2, r0\n"
		"or %6, r2, r0\n"
		"or.eop %7, r2, r0\n"
		"0:\n"
	: "=r"(result[0]), "=r"(result[1]), "=r"(result[2]), "=r"(result[3])
	, "=r"(result[4]), "=r"(result[5]), "=r"(result[6]), "=r"(result[7])
	: [address] "r"(address), "n" (SC_FETCH_LINE)
	: "memory"
	);
	return result;
}

#endif
