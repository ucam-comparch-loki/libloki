#ifndef LOKI_CONSTANTS
#define LOKI_CONSTANTS

#define CORES_PER_TILE 8

/*
 * Input channels.
 */

enum Channels {
  CH_IPK_FIFO,
  CH_IPK_CACHE,
  CH_REGISTER_2,
  CH_REGISTER_3,
  CH_REGISTER_4,
  CH_REGISTER_5,
  CH_REGISTER_6,
  CH_REGISTER_7
};

/*
 * System calls.
 */

enum SystemCalls {
  SYS_EXIT            = 0x01,   // Terminate the program
  SYS_OPEN            = 0x02,   // Open a file
  SYS_CLOSE           = 0x03,   // Close a file
  SYS_READ            = 0x04,   // Read from a file
  SYS_WRITE           = 0x05,   // Write to a file
  SYS_SEEK            = 0x06,   // Seek within a file
  
  SYS_TILE_ID         = 0x10,   // Unique ID of this tile
  SYS_POSITION        = 0x11,   // Position within this tile
  
  SYS_ENERGY_LOG_ON   = 0x20,   // Start recording energy-consuming events
  SYS_ENERGY_LOG_OFF  = 0x21,   // Stop recording energy-consuming events
  SYS_DEBUG_ON        = 0x22,   // Print lots of information to stdout
  SYS_DEBUG_OFF       = 0x23,   // Stop printing debug information
  SYS_INST_TRACE_ON   = 0x24,   // Print address of every instruction executed
  SYS_INST_TRACE_OFF  = 0x25,   // Stop printing instruction addresses
  
  SYS_CURRENT_CYCLE   = 0x30
};

/*
 * Memory configurations.
 */

enum MemConfig {
  SCRATCHPAD          = 0,
  CACHE               = 1,
  
  LINESIZE_1          = 0,
  LINESIZE_2          = 1,
  LINESIZE_4          = 2,
  LINESIZE_8          = 3,
  LINESIZE_16         = 4,
  LINESIZE_32         = 5,
  LINESIZE_64         = 6,
  
  GROUPSIZE_1         = 0,
  GROUPSIZE_2         = 1,
  GROUPSIZE_4         = 2,
  GROUPSIZE_8         = 3,
  GROUPSIZE_16        = 4,
  GROUPSIZE_32        = 5,
  GROUPSIZE_64        = 6,
  
  ASSOCIATIVITY_1     = 0,
  ASSOCIATIVITY_2     = 1,
  ASSOCIATIVITY_4     = 2,
  ASSOCIATIVITY_8     = 3
};

#endif /* LOKI_CONSTANTS */
