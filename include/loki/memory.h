/*! \file memory.h
 * \brief Memory control operations for Loki.
 *
 * This file contains functions which allow control over the memory hierarchy
 * beyond the L1 cache. For configuration of the L1 cache itself, see
 * channels.h.
 *
 * Each tile contains a directory which is accessed on an L1 cache miss. The
 * directory tells which tile on the chip is now responsible for providing the
 * required data. This tile may be another compute tile, in which case the 8
 * memory banks there are treated as a single 8-way set-associative L2 cache, or
 * the tile may be a memory controller at the edge of the chip, in which case
 * main memory is accessed.
 *
 * A number of bits are taken from the missing memory address and used to select
 * an entry in the directory. The position of these bits is configurable (using
 * `mask_index`), and these bits can optionally be modified (using 
 * `replacement_bits`). The entry also points to another tile on the chip, to
 * which the request will be forwarded.
 *
 * In the 4x4 test chip:
 *  * Compute tiles occupy positions (1,1) to (4,4), inclusive
 *  * Memory controllers occupy positions (1,0), (4,0), (1,5) and (4,5)
 *
 * Further documentation is available
 * [here](https://svr-rdm34-issue.cl.cam.ac.uk/w/loki/architecture/memory/#directory).
 */

#ifndef LOKI_MEMORY_H_
#define LOKI_MEMORY_H_

#include <assert.h>
#include <loki/channel_io.h>
#include <loki/sendconfig.h>
#include <loki/types.h>
#include <stdbool.h>
#include <stdint.h>

//! Log base 2 of size of the MHL directory.
#define LOKI_MEMORY_DIRECTORY_SIZE_LOG2 4
//! Size of the MHL directory. Always a power of 2.
#define LOKI_MEMORY_DIRECTORY_SIZE      (1 << LOKI_MEMORY_DIRECTORY_SIZE_LOG2)

//! Contents of an entry in the MHL directory.
typedef struct loki_memory_directory_entry {
	//! Next level of memory hierarchy's location.
	tile_id_t next_tile;
	//! Segment replacement bits. \ref LOKI_MEMORY_DIRECTORY_SIZE_LOG2 bits.
	unsigned char replacement_bits;
	//! Whether or not the entry is a scratchpad (or else a cache/main memory).
	bool scratchpad;
} loki_memory_directory_entry_t;

//! Full contents of the MHL directory.
typedef struct loki_memory_directory_configuration {
	//! Next level table.
	loki_memory_directory_entry_t entries[LOKI_MEMORY_DIRECTORY_SIZE];
	//! Bits to use for mask. Between `0` and `32 - LOKI_MEMORY_DIRECTORY_SIZE_LOG2`.
	unsigned char mask_index;
} loki_memory_directory_configuration_t;

//! Convert a \ref loki_memory_directory_entry_t to an int.
static inline int loki_memory_directory_entry_to_int(
	  loki_memory_directory_entry_t const value
) {
	return
		((int)value.scratchpad << (TILE_ID_T_BITS + LOKI_MEMORY_DIRECTORY_SIZE_LOG2)) |
		((int)value.replacement_bits << TILE_ID_T_BITS) |
		(int)value.next_tile;
}

//! \brief Update a single entry in the L1 directory on the default memory
//! channel.
//!
//! \param address The address to access, which determines the entry to update.
//! \param value The new entry.
//!
//! Generates an update directory entry memory request and sends it to the
//! default memory channel.
//!
//! \warning Updating a directory entry could impact memory consistency and
//! should only be done with care. Use \ref loki_memory_directory_reconfigure
//! for a safe interface.
static inline void loki_memory_directory_l1_entry_update(
	  void * const address
	, loki_memory_directory_entry_t const value
) {
	assert(value.replacement_bits < LOKI_MEMORY_DIRECTORY_SIZE);
	loki_channel_update_directory_entry(
		  1
		, address
		, loki_memory_directory_entry_to_int(value)
	);
}

//! \brief Update the directory mask in the L1 directory on the default memory
//! channel.
//!
//! \param value New value of the directory mask. Must be between 0 and
//! 32 - \ref LOKI_MEMORY_DIRETORY_SIZE_LOG2
//!
//! Generates an update directory mask memory request and sends it to the
//! default memory channel.
//!
//! \warning Updating a directory entry could impact memory consistency and
//! should only be done with care. Use \ref loki_memory_directory_reconfigure
//! for a safe interface.
static inline void loki_memory_directory_l1_mask_update(
	  unsigned char const value
) {
	assert(value < 32 - LOKI_MEMORY_DIRECTORY_SIZE_LOG2);
	loki_channel_update_directory_mask(
		  1
		, (void *)0
		, (int)value
	);
}

//! \brief Reconfigure a tile's directory.
//!
//! \param value The new configuration of the directory. If the configuration is
//! impossible, the behaviour of this method is undefined.
//!
//! This method updates the MHL directory mask along with all entries in the MHL
//! directory. The update is performed safely that all data currently in the
//! cache banks will be flushed and properly invalidated as necessary. The
//! update is performed atomically in the sense that no memory requests are made
//! in a configuration partially between old and new.
//!
//! \warning All other cores on the tile must be idle when this method is run.
//!
//! \warning This method does not take L2 caches into consideration.
//!
//! \warning Overwrites channel map table entries 2, 3 and 4, and uses `CH_REGISTER_3`,
//! `CH_REGISTER_4`, `CH_REGISTER_5` and `CH_REGISTER_6`.
void loki_memory_directory_reconfigure(
	  loki_memory_directory_configuration_t const value
);

//! The configuration of a Loki memory bank.
//!
//! \remark If a bank is neither icache nor dcache, it is assumed it must be
//! acting as a scratchpad or special memory.
typedef struct loki_memory_bank_configuration {
	//! Set of cores that this bank is instruction cache for.
	enum MulticastDestinations icache;
	//! Set of cores that this bank is data cache for.
	enum MulticastDestinations dcache;
} loki_memory_bank_configuration_t;

//! The cache configuration of a Loki tile.
//!
//! \remark Some configurations are impossible, for example having
//! a non-contiguous group of banks acting as a particular core's icache.
typedef struct loki_memory_cache_configuration {
	//! Configuration of each memory bank.
	loki_memory_bank_configuration_t banks[BANKS_PER_TILE];
	//! Set of cores that skip the L1 icache.
	enum MulticastDestinations icache_skip_l1;
	//! Set of cores that skip the L1 dcache.
	enum MulticastDestinations dcache_skip_l1;
	//! Set of cores that skip the L2 icache.
	enum MulticastDestinations icache_skip_l2;
	//! Set of cores that skip the L2 dcache.
	enum MulticastDestinations dcache_skip_l2;
} loki_memory_cache_configuration_t;

//! \brief Reconfigures a tile's caches.
//!
//! \param value The new configuration of the memory system. If the
//! configuration is impossible, the behaviour of this method is undefined.
//!
//! This method updates the memory configuration of all cores on a tile. The
//! update is performed safely that all data currently in the cache banks will
//! be flushed and properly invalidated as necessary. The update is performed
//! atomically in the sense that no memory requests are made in a configuration
//! partially between old and new.
//!
//! \warning All other cores on the tile must be idle when this method is run.
//! \warning Overwrites channel map table entries 2, 3 and 4, and uses `CH_REGISTER_3`,
//! `CH_REGISTER_4`, `CH_REGISTER_5` and `CH_REGISTER_6`.
void loki_memory_cache_reconfigure(
	  loki_memory_cache_configuration_t const value
);

//! Memory configuration for a shared L1 icache and dcache of 8 banks.
static loki_memory_cache_configuration_t const loki_memory_cache_configuration_id8 = {
	  .banks = {
		  { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
	  }
	, .icache_skip_l1 = MULTICAST_CORE_NONE
	, .dcache_skip_l1 = MULTICAST_CORE_NONE
	, .icache_skip_l2 = MULTICAST_CORE_NONE
	, .dcache_skip_l2 = MULTICAST_CORE_NONE
};

//! Memory configuration for a shared L1 icache and dcache of 4 banks.
static loki_memory_cache_configuration_t const loki_memory_cache_configuration_id4 = {
	  .banks = {
		  { .icache = MULTICAST_CORE_ALL , .dcache = MULTICAST_CORE_ALL  }
		, { .icache = MULTICAST_CORE_ALL , .dcache = MULTICAST_CORE_ALL  }
		, { .icache = MULTICAST_CORE_ALL , .dcache = MULTICAST_CORE_ALL  }
		, { .icache = MULTICAST_CORE_ALL , .dcache = MULTICAST_CORE_ALL  }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
	  }
	, .icache_skip_l1 = MULTICAST_CORE_NONE
	, .dcache_skip_l1 = MULTICAST_CORE_NONE
	, .icache_skip_l2 = MULTICAST_CORE_NONE
	, .dcache_skip_l2 = MULTICAST_CORE_NONE
};

//! Memory configuration for a shared L1 icache and dcache of 2 banks.
static loki_memory_cache_configuration_t const loki_memory_cache_configuration_id2 = {
	  .banks = {
		  { .icache = MULTICAST_CORE_ALL , .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL , .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
	  }
	, .icache_skip_l1 = MULTICAST_CORE_NONE
	, .dcache_skip_l1 = MULTICAST_CORE_NONE
	, .icache_skip_l2 = MULTICAST_CORE_NONE
	, .dcache_skip_l2 = MULTICAST_CORE_NONE
};

//! Memory configuration for a shared L1 icache and dcache of 1 banks.
static loki_memory_cache_configuration_t const loki_memory_cache_configuration_id1 = {
	  .banks = {
		  { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_NONE }
	  }
	, .icache_skip_l1 = MULTICAST_CORE_NONE
	, .dcache_skip_l1 = MULTICAST_CORE_NONE
	, .icache_skip_l2 = MULTICAST_CORE_NONE
	, .dcache_skip_l2 = MULTICAST_CORE_NONE
};

//! Memory configuration for a split L1 icache and dcache of 4 banks each.
static loki_memory_cache_configuration_t const loki_memory_cache_configuration_i4d4 = {
	  .banks = {
		  { .icache = MULTICAST_CORE_ALL , .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_ALL , .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_ALL , .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_ALL , .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_ALL  }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_ALL  }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_ALL  }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_ALL  }
	  }
	, .icache_skip_l1 = MULTICAST_CORE_NONE
	, .dcache_skip_l1 = MULTICAST_CORE_NONE
	, .icache_skip_l2 = MULTICAST_CORE_NONE
	, .dcache_skip_l2 = MULTICAST_CORE_NONE
};

//! Memory configuration for a private L1 cache for each core.
static loki_memory_cache_configuration_t const loki_memory_cache_configuration_pid1 = {
	  .banks = {
		  { .icache = MULTICAST_CORE_0, .dcache = MULTICAST_CORE_0 }
		, { .icache = MULTICAST_CORE_1, .dcache = MULTICAST_CORE_1 }
		, { .icache = MULTICAST_CORE_2, .dcache = MULTICAST_CORE_2 }
		, { .icache = MULTICAST_CORE_3, .dcache = MULTICAST_CORE_3 }
		, { .icache = MULTICAST_CORE_4, .dcache = MULTICAST_CORE_4 }
		, { .icache = MULTICAST_CORE_5, .dcache = MULTICAST_CORE_5 }
		, { .icache = MULTICAST_CORE_6, .dcache = MULTICAST_CORE_6 }
		, { .icache = MULTICAST_CORE_7, .dcache = MULTICAST_CORE_7 }
	  }
	, .icache_skip_l1 = MULTICAST_CORE_NONE
	, .dcache_skip_l1 = MULTICAST_CORE_NONE
	, .icache_skip_l2 = MULTICAST_CORE_NONE
	, .dcache_skip_l2 = MULTICAST_CORE_NONE
};

//! Memory configuration for private L1 instruction cache for pairs of cores,
//! with a shared L1 data cache of 4 banks.
static loki_memory_cache_configuration_t const loki_memory_cache_configuration_p2i1d4 = {
	  .banks = {
		  { .icache = MULTICAST_CORE_01, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_23, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_45, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_67, .dcache = MULTICAST_CORE_NONE }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_NONE, .dcache = MULTICAST_CORE_ALL }
	  }
	, .icache_skip_l1 = MULTICAST_CORE_NONE
	, .dcache_skip_l1 = MULTICAST_CORE_NONE
	, .icache_skip_l2 = MULTICAST_CORE_NONE
	, .dcache_skip_l2 = MULTICAST_CORE_NONE
};

//! Memory configuration for private L1 instruction cache for each core,
//! overlapping a shared L1 data cache.
static loki_memory_cache_configuration_t const loki_memory_cache_configuration_pi1od8 = {
	  .banks = {
		  { .icache = MULTICAST_CORE_0, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_1, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_2, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_3, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_4, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_5, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_6, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_7, .dcache = MULTICAST_CORE_ALL }
	  }
	, .icache_skip_l1 = MULTICAST_CORE_NONE
	, .dcache_skip_l1 = MULTICAST_CORE_NONE
	, .icache_skip_l2 = MULTICAST_CORE_NONE
	, .dcache_skip_l2 = MULTICAST_CORE_NONE
};

//! Memory configuration for an L2 tile.
static loki_memory_cache_configuration_t const loki_memory_cache_configuration_l2 = {
	  .banks = {
		  { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
	  }
	, .icache_skip_l1 = MULTICAST_CORE_ALL
	, .dcache_skip_l1 = MULTICAST_CORE_ALL
	, .icache_skip_l2 = MULTICAST_CORE_NONE
	, .dcache_skip_l2 = MULTICAST_CORE_NONE
};

//! Memory configuration for uncached instructions and data.
static loki_memory_cache_configuration_t const loki_memory_cache_configuration_none = {
	  .banks = {
		  { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
		, { .icache = MULTICAST_CORE_ALL, .dcache = MULTICAST_CORE_ALL }
	  }
	, .icache_skip_l1 = MULTICAST_CORE_ALL
	, .dcache_skip_l1 = MULTICAST_CORE_ALL
	, .icache_skip_l2 = MULTICAST_CORE_ALL
	, .dcache_skip_l2 = MULTICAST_CORE_ALL
};

//! \brief Reconfigure a tile's memory system.
//!
//! \param cache The new configuration of the core's caches. If the
//! configuration is impossible, the behaviour of this method is undefined.
//! \param directory The new configuration of the MHL directory. If the
//! configuration is impossible, the behaviour of this method is undefined.
//!
//! This method updates the memory configuration of all cores on a tile, along
//! with all entries in the MHL directory. The update is performed safely that
//! all data currently in the cache banks will be flushed and properly
//! invalidated as necessary. The update is performed atomically in the sense
//! that no memory requests are made in a configuration partially between old
//! and new.
//!
//! \warning All other cores on the tile must be idle when this method is run.
//!
//! \warning This method does not take L2 caches into consideration.
void loki_memory_tile_reconfigure(
	  loki_memory_cache_configuration_t const cache
	, loki_memory_directory_configuration_t const directory
);


//! \brief Push a cache line to a remote tile's L1.
//!
//! \param channel The memory channel to send the command on. Typically 1 will
//! work fine, though any channel can be used if desired.
//! \param address The address of the cache line created in the remote bank.
//! This should be 32 byte aligned.
//! \param directory_mask_index The mask_index value from the directory
//! configuration. (See \ref loki_memory_directory_configuration_t).
//! \param directory_index The entry in the directory to push to. (See \ref
//! loki_memory_directory_configuration_t).
//! \param groupStart The memory configuration of the REMOTE tile.
//! \param groupSize The memory configuration of the REMOTE tile.
//! \param value0 The value stored at address + 0.
//! \param value1 The value stored at address + 4.
//! \param value2 The value stored at address + 8.
//! \param value3 The value stored at address + 12.
//! \param value4 The value stored at address + 16.
//! \param value5 The value stored at address + 20.
//! \param value6 The value stored at address + 24.
//! \param value7 The value stored at address + 28.
//!
//! This method updates a remote L1 bank (on a different tile) such that the
//! cache line at given address has given values. The line will also be a hit
//! until it is evicted by normal cache replacement policy. The line will be
//! marked as dirty, so the remote cache will perform a writeback if it evicts
//! the line.
//!
//! In order to address the remote tile, the directory on the current core must
//! first be configured to map a particular directory entry to the target tile.
//! This can be done with \ref loki_memory_directory_reconfigure. This method
//! must also know the remote tile's L1 configuration. In particular, the target
//! group start and size must be known. This therefore allows pushing to any
//! group of memory, including for example to a specific private cache.
//!
//! Data can also be pushed to remote scratchpads though this is not safe in
//! general, and could cause a arbitrary writebacks to occur. In particular, no
//! two push operations can push to the same line of a scratchpad, otherwise a
//! writeback will be triggered. It would however be safe to push exactly once
//! to each cache line, then have the remote core run an invalidate all lines
//! command on the scratchpad to prevent the writeback from ever ocurring.
//!
//! This method is not blocking, and will return immediately once the request
//! has been sent. There is no way for the sending core to know the push has
//! completed, so the receiver must poll the cache line to determine when
//! the data has arrived.
static inline void loki_memory_push_cache_line(
	  int const channel, void * const address
	, unsigned char directory_mask_index, unsigned char directory_index
	, enum Memories const groupStart
	, enum MemConfigGroupSize const groupSize
	, int value0, int value1, int value2, int value3
	, int value4, int value5, int value6, int value7
) {
	uint_fast32_t head = (uint_fast32_t)address;
	assert((head & 0x1f) == 0);
	head &= ~(0xf << directory_mask_index);
	head |= (directory_index << directory_mask_index);
	switch (groupSize) {
		case GROUPSIZE_1: break;
		case GROUPSIZE_2: head |= (head >> 5) & 0x01; break;
		case GROUPSIZE_4: head |= (head >> 5) & 0x03; break;
		case GROUPSIZE_8: head |= (head >> 5) & 0x07; break;
		case GROUPSIZE_16: head |= (head >> 5) & 0x0f; break;
		case GROUPSIZE_32: head |= (head >> 5) & 0x1f; break;
		default: assert(0); __builtin_unreachable();
	}
	loki_channel_push_cache_line(
		  channel, (void *)head
		, value0, value1, value2, value3
		, value4, value5, value6, value7
	);
}

//! \brief Flush all lines from a given cache group.
//!
//! \param channel The memory channel to send the command on. Typically 1 will
//! work fine, though any channel can be used if desired.
//! \param groupSize The memory configuration of the cache group.
//!
//! Will not return until all data has left the tile (though not necessarily
//! after it arrives at its destination).
static inline void loki_cache_flush_all_lines_ex(
	  int const channel
	, enum MemConfigGroupSize const groupSize
) {
	int const iterations = 1 << (int)groupSize;
	for (int i = 0; i < iterations; i++) {
		loki_channel_flush_all_lines(channel, (void *)(i * 0x2020));
	}
	for (int i = 0; i < iterations; i++) {
		asm volatile (
			/* sendconfig to ensure ret channel is 2 and spad mode
			 * to avoid actually fetching the address. */
			/* Not fully general, since it doesn't assure return
			 * core is this core, but that's not the common case! */
			"sendconfig %0, %1 -> 1\n"
			"fetchr 0f\n"
			"or.eop r0, r2, r0\n"
			"0:\n"
			:
			: "r" (i*0x20), "n" (SC_RETURN_TO_R2 | SC_L1_SCRATCHPAD | SC_LOAD_WORD)
			: "memory"
		);
	}
}

//! \brief Flush all lines from the channel 1 cache group.
//!
//! Will not return until all data has left the tile (though not necessarily
//! after it arrives at its destination).
//!
//! \remark Reads the channel map table to determine cache configuration.
static inline void loki_cache_flush_all_lines(void) {
	loki_cache_flush_all_lines_ex(1, loki_channel_memory_get_group_size(get_channel_map(1)));
}


//! \brief Invalidate all lines from a given cache group.
//!
//! \param channel The memory channel to send the command on.
//! \param groupSize The memory configuration of the cache group.
//!
//! \warning Doesn't flush the lines first. Make sure they're not dirty if you
//! need coherency. It is almost never safe to call this method on channel 1 as
//! you would need to prevent the compiler generating any memory operations.
static inline void loki_cache_invalidate_all_lines_ex(
	  int const channel
	, enum MemConfigGroupSize const groupSize
) {
	int const iterations = 1 << (int)groupSize;
	for (int i = 0; i < iterations; i++) {
		loki_channel_invalidate_all_lines(channel, (void *)(i * 0x2020));
	}
}

//! \brief Flush all lines from the channel 0 cache group.
//!
//! Will not return until all data has left the tile (though not necessarily
//! after it arrives at its destination).
//!
//! \remark Reads the channel map table to determine cache configuration.
//!
//! \warning Doesn't flush the lines first. It's almost never safe to call this
//! method if channel 1 and channel 0 share banks as you would need to prevent
//! the compiler generating memory operations.
static inline void loki_cache_invalidate_all_lines_icache(void) {
	loki_cache_invalidate_all_lines_ex(0, loki_channel_memory_get_group_size(get_channel_map(0)));
}

//! \brief Flush and invalidate all lines from a given cache group.
//!
//! \param channel The memory channel to send the command on. Typically 1 will
//! work fine, though any channel can be used if desired.
//! \param groupSize The memory configuration of the cache group.
//!
//! Will not return until all data has left the tile (though not necessarily
//! after it arrives at its destination).
//!
//! Use \ref loki_cache_flush_and_invalidate_all_lines_ex instead where possible.
#define LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(channel, groupSize) do {\
	asm volatile (\
		"fetchr 0f\n"\
		"mov.eop %1, %0\n"\
		".p2align 5\n"\
		"0:\n"\
		"addui.p %1, %1, -1\n"\
		"psel.fetchr 0b, 0f\n"\
		"sendconfig %2, %5 -> %4\n"\
		"sendconfig %2, %6 -> %4\n"\
		"sendconfig %2, %7 -> %4\n"\
		"addu.eop %2, %2, %3\n"\
		"0:\n"\
		"fetchr 0f\n"\
		"mov.eop %1, %0\n"\
		"0:\n"\
		"addui.p %1, %1, -1\n"\
		"psel.fetchr 0b, 0f\n"\
		"mov.eop r0, r2\n"\
		"0:\n"\
		:\
		: "r" ((1<<(groupSize))-1), "r"(groupSize), "r"(0), "r"(0x2020), "i"(channel),\
		  "n" (SC_FLUSH_ALL_LINES), "n" (SC_INVALIDATE_ALL_LINES),\
		  "n" (SC_RETURN_TO_R2 | SC_L1_SCRATCHPAD | SC_LOAD_WORD)\
		: "memory"\
	);\
} while (0)

//! \brief Flush and invalidate all lines from a given cache group.
//!
//! \param channel The memory channel to send the command on. Typically 1 will
//! work fine, though any channel can be used if desired.
//! \param groupSize The memory configuration of the cache group.
//!
//! Will not return until all data has left the tile (though not necessarily
//! after it arrives at its destination).
static inline void loki_cache_flush_and_invalidate_all_lines_ex(
	  int const channel
	, enum MemConfigGroupSize const groupSize
) {
	switch (channel) {
	case 0: LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(0, groupSize); return;
	case 1: LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(1, groupSize); return;
	case 2: LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(2, groupSize); return;
	case 3: LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(3, groupSize); return;
	case 4: LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(4, groupSize); return;
	case 5: LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(5, groupSize); return;
	case 6: LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(6, groupSize); return;
	case 7: LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(7, groupSize); return;
	case 8: LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(8, groupSize); return;
	case 9: LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(9, groupSize); return;
	case 10: LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(10, groupSize); return;
	case 11: LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(11, groupSize); return;
	case 12: LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(12, groupSize); return;
	case 13: LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(13, groupSize); return;
	case 14: LOKI_CACHE_FLUSH_AND_INVALIDATE_ALL_LINES_EX(14, groupSize); return;
	default: assert(0); __builtin_unreachable();
	}
}

//! \brief Flush and invalidate all lines from the channel 1 cache group.
//!
//! Will not return until all data has left the tile (though not necessarily
//! after it arrives at its destination).
//!
//! \remark Reads the channel map table to determine cache configuration.
static inline void loki_cache_flush_and_invalidate_all_lines(void) {
	loki_cache_flush_and_invalidate_all_lines_ex(1, loki_channel_memory_get_group_size(get_channel_map(1)));
}

#endif
