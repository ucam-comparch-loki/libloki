/*!
 *! \file sendconfig.h
 *! \brief Useful constants which can be ORed together to create sendconfig commands.
 *
 * All values have been pre-shifted to their correct positions, and can be 
 * combined arbitrarily. More detailed documentation is available at 
 * https://svr-rdm34-issue.cl.cam.ac.uk/w/loki/configuration/sendconfig/
 */
 
/* Positions of different fields. Users shouldn't need to touch these. */
#define SC_POS_EOP_BIT            0

#define SC_POS_ALLOC_BIT          1
#define SC_POS_ACQUIRED_BIT       2

#define SC_POS_OPCODE             0
#define SC_POS_L1_SKIP_BIT        5
#define SC_POS_L2_SKIP_BIT        6
#define SC_POS_L1_SCRATCHPAD_BIT  7
#define SC_POS_RETURN_CHANNEL     8

/* Constants for all packet types. */
#define SC_END_OF_PACKET          (0x1 << SC_POS_EOP_BIT)
#define SC_EOP                    SC_END_OF_PACKET

/* Constants for core-to-core messages on the global network. */
#define SC_ALLOCATE               (0x1 << SC_POS_ALLOC_BIT)

#define SC_UNACQUIRED             (0x0 << SC_POS_ACQUIRED_BIT)
#define SC_ACQUIRED               (0x1 << SC_POS_ACQUIRED_BIT)

/* Constants for memory requests. */
#define SC_STORE_WORD             (0x0 << SC_POS_OPCODE)
#define SC_LOAD_WORD              (0x1 << SC_POS_OPCODE)
#define SC_STORE_CONDITIONAL      (0x2 << SC_POS_OPCODE)
#define SC_LOAD_LINKED            (0x3 << SC_POS_OPCODE)
#define SC_STORE_HALFWORD         (0x4 << SC_POS_OPCODE)
#define SC_LOAD_HALFWORD          (0x5 << SC_POS_OPCODE)
#define SC_STORE_BYTE             (0x6 << SC_POS_OPCODE)
#define SC_LOAD_BYTE              (0x7 << SC_POS_OPCODE)
#define SC_STORE_LINE             (0x8 << SC_POS_OPCODE)
#define SC_FETCH_LINE             (0x9 << SC_POS_OPCODE)
#define SC_MEMSET_LINE            (0xa << SC_POS_OPCODE)
#define SC_IPK_READ               (0xb << SC_POS_OPCODE)
#define SC_PUSH_LINE              (0xc << SC_POS_OPCODE)
#define SC_VALIDATE_LINE          (0xd << SC_POS_OPCODE)
#define SC_PREFETCH_LINE          (0xf << SC_POS_OPCODE)
#define SC_LOAD_AND_ADD           (0x10 << SC_POS_OPCODE)
#define SC_FLUSH_LINE             (0x11 << SC_POS_OPCODE)
#define SC_LOAD_AND_OR            (0x12 << SC_POS_OPCODE)
#define SC_INVALIDATE_LINE        (0x13 << SC_POS_OPCODE)
#define SC_LOAD_AND_AND           (0x14 << SC_POS_OPCODE)
#define SC_FLUSH_ALL_LINES        (0x15 << SC_POS_OPCODE)
#define SC_LOAD_AND_XOR           (0x16 << SC_POS_OPCODE)
#define SC_INVALIDATE_ALL_LINES   (0x17 << SC_POS_OPCODE)
#define SC_EXCHANGE               (0x18 << SC_POS_OPCODE)
#define SC_UPDATE_DIRECTORY_ENTRY (0x1a << SC_POS_OPCODE)
#define SC_UPDATE_DIRECTORY_MASK  (0x1c << SC_POS_OPCODE)
#define SC_PAYLOAD                (0x1e << SC_POS_OPCODE)
#define SC_PAYLOAD_EOP            (0x1f << SC_POS_OPCODE)

#define SC_SKIP_L1                (0x1 << SC_POS_L1_SKIP_BIT)
#define SC_SKIP_L2                (0x1 << SC_POS_L2_SKIP_BIT)
#define SC_L1_SCRATCHPAD          (0x1 << SC_POS_L1_SCRATCHPAD_BIT)

#define SC_RETURN_TO_IPK_FIFO     (0 << SC_POS_RETURN_CHANNEL)
#define SC_RETURN_TO_IPK_CACHE    (1 << SC_POS_RETURN_CHANNEL)
#define SC_RETURN_TO_R2           (2 << SC_POS_RETURN_CHANNEL)
#define SC_RETURN_TO_R3           (3 << SC_POS_RETURN_CHANNEL)
#define SC_RETURN_TO_R4           (4 << SC_POS_RETURN_CHANNEL)
#define SC_RETURN_TO_R5           (5 << SC_POS_RETURN_CHANNEL)
#define SC_RETURN_TO_R6           (6 << SC_POS_RETURN_CHANNEL)
#define SC_RETURN_TO_R7           (7 << SC_POS_RETURN_CHANNEL)
