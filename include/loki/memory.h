/*! \file memory.h
 * \brief Memory control operations for Loki. */

#ifndef LOKI_MEMORY_H_
#define LOKI_MEMORY_H_

#include <assert.h>
#include <loki/channel_io.h>
#include <loki/types.h>

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
} loki_memory_directory_entry_t;

//! Full contents of the MHL directory.
typedef struct loki_memory_directory_configuration {
	//! Next level table.
	loki_memory_directory_entry_t entries[LOKI_MEMORY_DIRECTORY_SIZE];
	//! Bits to use for mask. Between `0` and `32 - LOKI_MEMORY_DIRECTORY_SIZE_LOG2`.
	unsigned char mask_index;
} loki_memory_directory_configuration_t;

//! Converts a \ref loki_memory_directory_entry_t to and int.
static inline int loki_memory_directory_entry_to_int(
	  loki_memory_directory_entry_t const value
) {
	return
		((int)value.replacement_bits << TILE_ID_T_BITS) |
		(int)value.next_tile;
}

//! \brief Updates a single entry in the L1 directory on the default memory
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

//! \brief Updates the directory mask in the L1 directory on the default memory
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

//! \brief Reconfigures a tile's directory.
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
void loki_memory_directory_reconfigure(
	  loki_memory_directory_configuration_t const value
);

//! The memmory configuration of a Loki memory bank.
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

//! \brief Reconfigures a tile's memory system.
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

#endif
