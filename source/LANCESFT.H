/*++

Copyright (c) 1993 ADVANCED MICRO DEVICES, INC. All Rights Reserved.
This software is unpblished and contains the trade secrets and
confidential proprietary information of AMD. Unless otherwise provided
in the Software Agreement associated herewith, it is licensed in confidence
"AS IS" and is not to be reproduced in whole or part by any means except
for backup. Use, duplication, or disclosure by the Government is subject
to the restrictions in paragraph (b) (3) (B) of the Rights in Technical
Data and Computer Software clause in DFAR 52.227-7013 (a) (Oct 1988).
Software owned by Advanced Micro Devices, Inc., 901 Thompson Place,
Sunnyvale, CA 94088.

Module Name:

    lancesft.h

Abstract:

    The main header for a LANCE NDIS driver.

Environment:

    This driver is expected to work in DOS, OS2 and NT at the equivalent
    of kernel mode.

    Architecturally, there is an assumption in this driver that we are
    on a little endian machine.

Created:

    09/30/92

Revision History:

    09/30/92   modified lance adpter structure
--*/

#ifndef _LANCESFT_
#define _LANCESFT_

//
// We use STATIC to define procedures that will be static in the
// final build but which we now make extern to allow them to be
// debugged.
//

#if DEVL
#define STATIC
#else
#define STATIC static
#endif


//
// This variable is used to control debug output.
//

#if DBG
extern INT LanceDbg;
extern INT LanceIsr;
extern INT LanceTx;
extern INT LanceTxPacket;
extern INT LanceTxOwn;
extern INT LanceRx;
extern INT LanceErr;
extern INT LanceAddress;
#endif

#if DBG

#define LANCELOG 0

#else

#define LANCELOG 0

#endif

#if LANCELOG

#define LOG_SIZE 256

#define TIMER      '.'
#define IN_ISR     'i'
#define OUT_ISR    'I'
#define IN_DPC     'd'
#define OUT_DPC    'D'
#define RECEIVE    'R'
#define TRANSMIT   'x'
#define TRANSMIT_COMPLETE   'X'
#define PEND       'p'
#define UNPEND     'P'
#define INDICATE   'r'
#define IN_SEND    's'
#define OUT_SEND   'S'
#define START      'G'
#define RESET_STEP_1 '1'
#define RESET_STEP_2 '2'
#define RESET_SAVE_PACKET     'b'
#define RESET_RECOVER_PACKET  'B'
#define RESET_COMPLETE_PACKET 'c'
#define RESET_STEP_3 '3'
#define REMOVE 'V'
#define CLOSE 'C'
#define UNLOAD 'U'
#define MISSED 'M'
#define ERR 'E'
#define BABL '?'
#define HEART 'H'

extern UCHAR Log[LOG_SIZE];

extern UCHAR LogPlace;
extern UCHAR LogWrapped;

#define LOG(c)     { Log[LogPlace++] = (c); Log[LogPlace+3] ='\0'; }

#else

#define LOG(c)

#endif

//
// Used when registering ourselves with NDIS.
//

#define LANCE_NDIS_MAJOR_VERSION 3
#define LANCE_NDIS_MINOR_VERSION 0


//
// Used for parsing OIDs
//

#define OID_TYPE_MASK                       0xffff0000
#define OID_TYPE_GENERAL_OPERATIONAL        0x00010000
#define OID_TYPE_GENERAL_STATISTICS         0x00020000
#define OID_TYPE_802_3_OPERATIONAL          0x01010000
#define OID_TYPE_802_3_STATISTICS           0x01020000

#define OID_REQUIRED_MASK                   0x0000ff00
#define OID_REQUIRED_MANDATORY              0x00000100
#define OID_REQUIRED_OPTIONAL               0x00000200

#define OID_INDEX_MASK                      0x000000ff

//
// Indexes in the GeneralMandatory array.
//

#define GM_TRANSMIT_GOOD                  0x00
#define GM_RECEIVE_GOOD                   0x01
#define GM_TRANSMIT_BAD                   0x02
#define GM_RECEIVE_BAD                    0x03
#define GM_RECEIVE_NO_BUFFER              0x04
#define GM_ARRAY_SIZE                     0x05

//
// Indexes in the GeneralOptional array. There are
// two sections, the ones up to COUNT_ARRAY_SIZE
// have entries for number (4 bytes) and number of
// bytes (8 bytes), the rest are a normal array.
//

#define GO_DIRECTED_TRANSMITS             0x00
#define GO_MULTICAST_TRANSMITS            0x01
#define GO_BROADCAST_TRANSMITS            0x02
#define GO_DIRECTED_RECEIVES              0x03
#define GO_MULTICAST_RECEIVES             0x04
#define GO_BROADCAST_RECEIVES             0x05
#define GO_COUNT_ARRAY_SIZE               0x06

#define GO_ARRAY_START                    0x0C
#define GO_RECEIVE_CRC                    0x0C
#define GO_TRANSMIT_QUEUE_LENGTH          0x0D
#define GO_ARRAY_SIZE                     0x0E

//
// Indexes in the MediaMandatory array.
//

#define MM_RECEIVE_ERROR_ALIGNMENT        0x00
#define MM_TRANSMIT_ONE_COLLISION         0x01
#define MM_TRANSMIT_MORE_COLLISIONS       0x02
#define MM_ARRAY_SIZE                     0x03

//
// Indexes in the MediaOptional array.
//

#define MO_TRANSMIT_DEFERRED              0x00
#define MO_TRANSMIT_MAX_COLLISIONS        0x01
#define MO_RECEIVE_OVERRUN                0x02
#define MO_TRANSMIT_UNDERRUN              0x03
#define MO_TRANSMIT_HEARTBEAT_FAILURE     0x04
#define MO_TRANSMIT_TIMES_CRS_LOST        0x05
#define MO_TRANSMIT_LATE_COLLISIONS       0x06
#define MO_ARRAY_SIZE                     0x07

//
//    Scan parameters for the board search across the buses
//
#define LED_DEFAULT     -1          /* Default for Led0,1 & 2          */
#define BURST_RXTX       0          /* Default burst mode: Rx+Tx */
#define BURST_TX         1          /* Burst mode: Tx only */
#define BUSTIMER_DEFAULT 60         /* Default for Csr82 (Bus Activity timer) */
#define MCA             7           /* Glorious Micro Channel Bus      */
#define LANCE_INIT_OK     0         /* LanceHardwareDetails return codes */
#define LANCE_INIT_WARNING_8   8
#define LANCE_INIT_ERROR_9    9
#define LANCE_INIT_ERROR_10     10
#define LANCE_INIT_ERROR_11     11
#define LANCE_INIT_ERROR_13     13
#define LANCE_INIT_ERROR_14     14
#define LANCE_INIT_ERROR_15     15
#define LANCE_INIT_ERROR_16     16
#define LANCE_INIT_ERROR_17     17
#define LANCE_INIT_ERROR_18     18
#define LANCE_INIT_ERROR_19     19
#define LANCE_INIT_ERROR_20     20
#define LANCE_INIT_ERROR_21     21
#define LANCE_INIT_ERROR_22     22
#define LANCE_INIT_ERROR_23     23
#define LANCE_INIT_ERROR_24     24

#define NO_DMA 0

//
// Macros used for memory allocation and deallocation.
//

#define LANCE_ALLOC_MEMORY(_Status, _Address, _Length) \
    { \
   NDIS_PHYSICAL_ADDRESS Temp = NDIS_PHYSICAL_ADDRESS_CONST(-1, -1); \
        *(_Status) = NdisAllocateMemory( \
                        (PVOID)(_Address), \
                        (_Length), \
                        0,        \
                        Temp      \
                        );        \
    }

#define LANCE_FREE_MEMORY(Address, Length) \
    NdisFreeMemory( \
        (PVOID)(Address), \
        (Length),  \
        0          \
        )

#define LANCE_ALLOC_CONTIGUOUS_MEMORY(_Status, _Address, _Length) \
    { \
        NDIS_PHYSICAL_ADDRESS Temp = NDIS_PHYSICAL_ADDRESS_CONST(-1, 0); \
        *(_Status) = NdisAllocateMemory( \
                        (PVOID)(_Address), \
                        (_Length),  \
                        NDIS_MEMORY_CONTIGUOUS,  \
                        Temp       \
                        );         \
    }

#define LANCE_FREE_CONTIGUOUS_MEMORY(Address, Length) \
    NdisFreeMemory( \
        (PVOID)(Address), \
        (Length),  \
        NDIS_MEMORY_CONTIGUOUS \
        )

#define LANCE_ALLOC_NONCACHED_MEMORY(_Status, _Address, _Length) \
    { \
      NDIS_PHYSICAL_ADDRESS Temp = NDIS_PHYSICAL_ADDRESS_CONST(-1, 0); \
        *(_Status) = NdisAllocateMemory( \
                        (PVOID)(_Address), \
                        (_Length),  \
                        NDIS_MEMORY_CONTIGUOUS | NDIS_MEMORY_NONCACHED, \
                        Temp       \
                        );         \
    }

#define LANCE_FREE_NONCACHED_MEMORY(Address, Length) \
    NdisFreeMemory( \
        (PVOID)(Address), \
        (Length),  \
        NDIS_MEMORY_CONTIGUOUS | NDIS_MEMORY_NONCACHED \
        )



//
// Macros to move and zero memory.
//

#define LANCE_MOVE_MEMORY(Destination,Source,Length) NdisMoveMemory(Destination,Source,Length)
#define LANCE_ZERO_MEMORY(Destination,Length) NdisZeroMemory(Destination,Length)

#ifndef NDIS_NT

//
// This useful macro is not defined outside of NT.
//
// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure.
//

#define CONTAINING_RECORD(address, type, field) ((type *)( \
                                                  (PCHAR)(address) - \
                                                  (PCHAR)(&((type *)0)->field)))

#endif  // #ifndef NDIS_NT


//
// Used to record the 8-byte counters.
//

typedef struct _LANCE_LARGE_INTEGER {
    ULONG LowPart;
    ULONG HighPart;
} LANCE_LARGE_INTEGER, *PLANCE_LARGE_INTEGER;

//
// This initializes an 8-byte counter.
//

#define LanceZeroLargeInteger(LargeInteger) \
{ \
    LargeInteger.LowPart = 0L;\
    LargeInteger.HighPart = 0L; \
}

//
// This adds a longword to an 8-byte counter.
//

#define LanceAddUlongToLargeInteger(LargeInteger, Ulong) \
{ \
    PLANCE_LARGE_INTEGER TmpLarge = (LargeInteger); \
    TmpLarge->LowPart += (ULONG)Ulong; \
    if (TmpLarge->LowPart < (ULONG)Ulong) { \
        ++TmpLarge->HighPart; \
    } \
}

//
// This flushes a buffer for write.
//

#define LANCE_FLUSH_WRITE_BUFFER(Buffer) \
    NdisFlushBuffer( \
        Buffer, \
        TRUE    \
        )

//
// This identifies the type of the packet for quick reference
// in the LANCE_PACKET_RESERVED.PacketType field.
//

#define LANCE_DIRECTED     0x0001
#define LANCE_MULTICAST    0x0002
#define LANCE_BROADCAST    0x0004
#define LANCE_LOOPBACK     0x0008


//
// This record type is used to store lance global data.
// It is used as the MacMacContext in the call to
// NdisRegisterMac.
//

typedef struct _LANCE_MAC {

    //
    // The handle returned by NdisInitializeWrapper.
    //

    NDIS_HANDLE WrapperHandle;

    //
    // The handle returned by NdisRegisterMac.
    //

    NDIS_HANDLE MacHandle;

} LANCE_MAC, *PLANCE_MAC;

///
// This is the basic structure that defines the state of an
// adapter. There is one of these allocate per adapter that
// the Lance driver supports.
//

typedef struct _LANCE_ADAPTER {

   //
   // Device Type, i.e., PCNET-ISA or LANCE
   //
   UCHAR Type;

    //
    // TRUE if RemoveAdapter has been called.
    //
    BOOLEAN Removed;

    //
    // The burned-in network address from the hardware.
    //
    CHAR PermanentNetworkAddress[ETH_LENGTH_OF_ADDRESS];

    //
    // The current network address from the hardware.
    //
    CHAR CurrentNetworkAddress[ETH_LENGTH_OF_ADDRESS];

    //
    // These are boolean, used as a gate to ensure that only one thread
    // of execution is actually processing the specified source of
    // deferred processing.
    //

    BOOLEAN ProcessingReceiveInterrupt;
    BOOLEAN ProcessingTransmits;
    BOOLEAN ProcessingDeferredOperations;
    BOOLEAN ProcessingLoopBacks;
    BOOLEAN SendQueue;
    BOOLEAN LoopBackQueue;

    //
    // Use to queue deferred operations.
    //
    NDIS_TIMER DeferredTimer;

    //
    // Use to detect Lance lockups.
    //
    NDIS_TIMER DetectTimer;
    BOOLEAN LockUpTimeout;

    //
    // Holds the interrupt object for this adapter.
    //
    NDIS_INTERRUPT Interrupt;

    //
    // Holds a value of the CSR0 from one or more interrupts.
    // This value is only set by the interrupt service routine
    // and cleared by the interrupt synchronization routine.
    //
    USHORT Csr0Value;

    //
    // Address of the beginning of the LANCE ports.
    //
    ULONG LanceBaseAddress;

    //  IRQ, DMA and keywords for the adapter.
    CHAR LanceInterruptVector;
    CHAR LanceDmaChannel;
    LONG led0;
    LONG led1;
    LONG led2;
    LONG led3;
    LONG burst;

    //
    //Dma Timer value in Csr 82.
    SHORT BusTimer;

    //
    //Board Type found. ISA+, ISA, LOCAL, PCI
    ULONG board_found;

    //
    //
    ULONG stop_set;

    //
    // Keeps a reference count on the current number of uses of
    // this adapter block.  Uses is defined to be the number of
    // routines currently within the "external" interface.
    //
    INT References;

    //
    // List head for all open bindings for this adapter.
    //
    LIST_ENTRY OpenBindings;

    //
    // Number of open bindings.
    //
    UINT OpenCount;

    //
    // List head for all opens that had outstanding references
    // when an attempt was made to close them.
    //
    LIST_ENTRY CloseList;

    //
    // Spinlock to protect fields in this structure..
    //
    NDIS_SPIN_LOCK Lock;

    //
    // Holds the adapter in the global MAC list.
    //
    LIST_ENTRY AdapterList;

    //
    // Handle given by NDIS when the MAC registered itself.
    //
    NDIS_HANDLE NdisMacHandle;

    //
    // Handle given by NDIS when the adapter was registered.
    //
    NDIS_HANDLE NdisAdapterHandle;

    //
    // Pointer to the filter database for the MAC.
    //
    PETH_FILTER FilterDB;

    //
    // The head of the request queue for this adapter.
    //
    PNDIS_REQUEST FirstRequest;

    //
    // The tail of the request queue for this adapter.
    //
    PNDIS_REQUEST LastRequest;

    //
    // The current packet filter.
    //
    UINT CurrentPacketFilter;

    //
    // Physical address of shared memory.
    //
    NDIS_PHYSICAL_ADDRESS SharedMemoryPa;

    //
    // Virtual address of shared memory.
    //
    PVOID SharedMemoryVa;

    //
    // Number of Transmit Descriptors
    //
    UINT NumberOfTransmitDescriptors;

    //
    // The pointer to transmit buffers.
    //
    PUCHAR TransmitBufferPointer;

    //
    // The physical address of the TransmitBufferPointer
    //
    NDIS_PHYSICAL_ADDRESS TransmitBufferPointerPhysical;

    //
    // Pointer to the transmit descriptor ring (this is
    // allocated to be of size NumberOfTransmitDescriptors).
    //
    PVOID TransmitDescriptorRing;

    //
    // The physical address of the TransmitDescriptorRing.
    //
    NDIS_PHYSICAL_ADDRESS TransmitDescriptorRingPhysical;

    //
    // Index to the next available transmit descriptor
    // in the descriptor ring.
    //
    UCHAR NextTransmitDescriptorIndex;

    //
    // Flags to indicate whether the descriptor pointed by
    // NextTransmitDescriptorIndex is available or not.
    //
    BOOLEAN TransmitDescriptorAvailable[TRANSMIT_BUFFERS];

    //
    // The number of receive descriptors
    //
    UINT NumberOfReceiveDescriptors;

    //
    // The pointer to receive buffers
    //
    PUCHAR ReceiveBufferPointer;

    //
    // The physical address of the ReceiveBufferPointer
    //
    NDIS_PHYSICAL_ADDRESS ReceiveBufferPointerPhysical;

    //
    // Pointer to the receive descriptor ring
    // (this is allocated to be of size NumberOfReceiveDescriptors).
    //
    PVOID ReceiveDescriptorRing;

    //
    // The physical address of ReceiveDescriptorRing
    //
    NDIS_PHYSICAL_ADDRESS ReceiveDescriptorRingPhysical;

    //
    // Index to the next available receive   descriptor
    // in the descriptor ring.
    //
    USHORT NextReceiveDescriptorIndex;

    //
    // The intialization block for the adapter
    //
    PVOID InitializationBlock;

    //
    // The physical address of the Initialization block
    //
    NDIS_PHYSICAL_ADDRESS InitializationBlockPhysical;

    //
    // Pointer to the first packet on the loopback list.
    //
    // Can only be accessed when the adapter lock
    // is held.
    //
    PNDIS_PACKET FirstLoopBack;

    //
    // Pointer to the last packet on the loopback list.
    //
    // Can only be accessed when the adapter lock
    // is held.
    //
    PNDIS_PACKET LastLoopBack;

    //
    // Pointer to the current packet on loopback list.
    //
    PNDIS_PACKET CurrentLoopbackPacket;
    //

    // Pointer to the first packet on the send packet list.
    //
    // Can only be accessed when the adapter lock
    // is held.
    //
    PNDIS_PACKET FirstSendPacket;

    //
    // Pointer to the last packet on the send packet list.
    //
    // Can only be accessed when the adapter lock
    // is held.
    //
    PNDIS_PACKET LastSendPacket;

    //
    // Holds the open that queued a reset.
    //
    struct _LANCE_OPEN * ResettingOpen;

    //
    // This Flag is set to TRUE by the ISR and reset to FALSE
    // by the DPC. This flag will be checked using
    // NdisSynchronizeWithInterrupt.
    BOOLEAN InterruptFlag;

    //
    // Flag that when enabled lets routines know that a reset
    // is in progress, for the purposes of blocking other
    // requests (except other resets).
    //
    BOOLEAN ResetInProgress;

    //
    // Flag that when enabled lets routines know that a reset
    // is needed, for the purposes of blocking other
    // requests.
    //
    BOOLEAN ResetPending;

    //
    // Flag the lets us know that we are indicating RESET_START
    // to all the protocols. During this time we can reject
    // NdisReset requests with the status RESET_IN_PROGRESS.
    //
    BOOLEAN IndicatingResetStart;

    //
    // Flag that says that we are indicating RESET_END to all
    // the protocols. If this is set when a reset comes in, then
    // we cannot indicating RESET_START right away; the thread
    // indicating RESET_END will indicate the RESET_START if
    // it fines that BlockResets has come on while it was
    // indicating RESET_END.
    //
    BOOLEAN IndicatingResetEnd;

    //
    // TRUE if a request is being processed.
    //
    BOOLEAN RequestInProgress;

    // BOOLEAN ProcessPendingLoopBacks;

    //
    // These hold adapter statistics.
    //
    ULONG GeneralMandatory[GM_ARRAY_SIZE];
    LANCE_LARGE_INTEGER GeneralOptionalByteCount[GO_COUNT_ARRAY_SIZE];
    ULONG GeneralOptionalFrameCount[GO_COUNT_ARRAY_SIZE];
    ULONG GeneralOptional[GO_ARRAY_SIZE - GO_ARRAY_START];
    ULONG MediaMandatory[MM_ARRAY_SIZE];
    ULONG MediaOptional[MO_ARRAY_SIZE];

    //
    // Total number of packets to check lock up conditions.
    //

    ULONG TxPacketNumber;

    //
    // For indicating loopback packets.
    //

    UCHAR Loopback[LANCE_LOOPBACK_MAXIMUM];

} LANCE_ADAPTER,*PLANCE_ADAPTER;


//
// Given a MacBindingHandle this macro returns a pointer to the
// LANCE_ADAPTER.
//

#define PLANCE_ADAPTER_FROM_BINDING_HANDLE(Handle) \
    (((PLANCE_OPEN)((PVOID)(Handle)))->OwningLance)


//
// Given a MacContextHandle return the PLANCE_ADAPTER
// it represents.
//

#define PLANCE_ADAPTER_FROM_CONTEXT_HANDLE(Handle) \
    ((PLANCE_ADAPTER)((PVOID)(Handle)))


//
// This record type is inserted into the MacReserved portion
// of the packet header when the packet is going through the
// staged allocation of buffer space prior to the actual send.
//
typedef struct _LANCE_PACKET_RESERVED {

    //
    // Points to the next packet in the chain of queued packets.
    //
    // We always keep the packet on a list so that in case the
    // the adapter is closing down or resetting, all the packets
    // can easily be located and "canceled".
    //
    PNDIS_PACKET Next;

   //
    // This field holds the binding handle of the open binding
    // that submitted this packet for send.
    //
    NDIS_HANDLE MacBindingHandle;

    //
    // The length of the packet.
    //
    USHORT PacketLength;

    //
    // This identifies the type of the packet.
    //
    UCHAR PacketType;

    //
    // Under the protection of the transmit queue lock
    // this value will be examined by both the loopback
    // completion code and the hardware send completion
    // code.  If either of them find the value to be true
    // they will send the transmit complete.
    //
    // Note that if the packet didn't have to be loopbacked
    // or if the packet didn't need to go out on the wire
    // the this value will be initialized to true.  Otherwise
    // this value will be set to false just before it is
    // relinquished to the hardware and to the loopback queue.
    //
    BOOLEAN ReadyToComplete;

} LANCE_PACKET_RESERVED,*PLANCE_PACKET_RESERVED;

//
// This macro will return a pointer to the lance reserved portion
// of a packet given a pointer to a packet.
//
#define PLANCE_RESERVED_FROM_PACKET(Packet) \
   ((PLANCE_PACKET_RESERVED)((PVOID)((Packet)->MacReserved)))


//
// This structure is used in the MacReserved field of
// an NDIS_REQUEST_BLOCK, passed in during multicast
// address/packet filter operations.
//

typedef struct _LANCE_REQUEST_RESERVED {
    PNDIS_REQUEST Next;
   struct _LANCE_OPEN * OpenBlock;
} _LANCE_REQUEST_RESERVED, * PLANCE_REQUEST_RESERVED;

//
// This macro will return a pointer to the lance reserved portion
// of a request given a pointer to the request.
//
#define PLANCE_RESERVED_FROM_REQUEST(Request) \
   ((PLANCE_REQUEST_RESERVED)((PVOID)((Request)->MacReserved)))


//
// One of these structures is created on each MacOpenAdapter.
//

typedef struct _LANCE_OPEN {

    //
    // Linking structure for all of the open bindings of a particular
    // adapter.
    //
    LIST_ENTRY OpenList;

    //
    // The Adapter that requested this open binding.
    //
    PLANCE_ADAPTER OwningLance;

    //
    // Handle of this adapter in the filter database.
    //
    NDIS_HANDLE NdisFilterHandle;

    //
    // Given by NDIS when the adapter was opened.
    //
    NDIS_HANDLE NdisBindingContext;

    //
    // Counter of all the different reasons that a open binding
    // couldn't be closed.  This would be incremented each time
    // for:
    //
    // While a particular interface routine is accessing this open
    //
    // During an indication.
    //
    // When the open causes a reset.
    //
    // A packet currently being sent.
    //
    // (Basically the above two mean any time the open has left
    //  some processing around to be accomplished later.)
    //
    // This field should only be accessed when the adapter lock is held.
    //
    UINT References;

    //
    // A request that we use to queue the open or close request on
    // the adapter's queue.
    //
    NDIS_REQUEST OpenCloseRequest;

    //
    // A flag indicating that this binding is in the process of closing.
    //
    BOOLEAN BindingShuttingDown;

    UINT ProtOptionFlags;

} LANCE_OPEN,*PLANCE_OPEN;


//
// This macro returns a pointer to a PLANCE_OPEN given a MacBindingHandle.
//

#define PLANCE_OPEN_FROM_BINDING_HANDLE(Handle) \
    ((PLANCE_OPEN)((PVOID)Handle))


//
// This macro returns a NDIS_HANDLE from a PLANCE_OPEN
//

#define BINDING_HANDLE_FROM_PLANCE_OPEN(Open) \
    ((NDIS_HANDLE)((PVOID)Open))


//
// procedures which do error logging
//

typedef enum _LANCE_PROC_ID{
    registerAdapter,
    openAdapter,
    hardwareDetails,
    handleDeferred,
    processReceiveInterrupts
} LANCE_PROC_ID;


//
// Error log values
//

#define LANCE_ERRMSG_INIT_INTERRUPT      (ULONG)0x01
#define LANCE_ERRMSG_CREATE_FILTER       (ULONG)0x02
#define LANCE_ERRMSG_ALLOC_MEMORY        (ULONG)0x03
#define LANCE_ERRMSG_REGISTER_ADAPTER    (ULONG)0x04
#define LANCE_ERRMSG_ALLOC_DEVICE_NAME   (ULONG)0x05
#define LANCE_ERRMSG_ALLOC_ADAPTER       (ULONG)0x06
#define LANCE_ERRMSG_INITIAL_INIT        (ULONG)0x07
#define LANCE_ERRMSG_OPEN_DB             (ULONG)0x08
#define LANCE_ERRMSG_ALLOC_OPEN          (ULONG)0x09
#define LANCE_ERRMSG_HARDWARE_ADDRESS    (ULONG)0x0A
#define LANCE_ERRMSG_BOARD_NOT_FOUND    (ULONG)0x0B
#define LANCE_ERRMSG_HARDWARE_ALLOC_BUFFER   (ULONG)0x0C

//
// This macro will act a "epilogue" to every routine in the
// *interface*.  It will check whether there any requests needed
// to defer there processing.  It will also decrement the reference
// count on the adapter.  If the reference count is zero and there
// is deferred work to do it will insert the interrupt processing
// routine in the DPC queue.
//
// Note that we don't need to include checking for blocked receives
// since blocked receives imply that there will eventually be an
// interrupt.
//
// NOTE: This macro assumes that it is called with the lock acquired.
//

#define LANCE_DO_DEFERRED(Adapter) \
{ \
    PLANCE_ADAPTER _A = (Adapter); \
    if ((!_A->ProcessingDeferredOperations) && \
        (((_A->References == 2) && \
         _A->ResetInProgress) || \
         _A->FirstSendPacket || \
         _A->FirstLoopBack || \
         (!IsListEmpty(&_A->CloseList)))) { \
        _A->ProcessingDeferredOperations = TRUE; \
        NdisReleaseSpinLock(&_A->Lock); \
        NdisSetTimer(&_A->DeferredTimer, 0); \
    } else { \
        _A->References--; \
        NdisReleaseSpinLock(&_A->Lock); \
    } \
}

#define LANCE_DO_SEND(Adapter) \
{ \
    PLANCE_ADAPTER _A = (Adapter); \
    if ((!_A->ProcessingDeferredOperations) && \
        (((_A->References == 2) && \
         _A->ResetInProgress) || \
         _A->FirstLoopBack || \
         (!IsListEmpty(&_A->CloseList)))) { \
        _A->ProcessingDeferredOperations = TRUE; \
        NdisReleaseSpinLock(&_A->Lock); \
        LanceSendImmediate(_A);\
    } else { \
        _A->References--; \
        NdisReleaseSpinLock(&_A->Lock); \
    } \
}

#define LANCE_SEND_NO_Q(Adapter, Packet, SourceBuffer) \
{ \
    PLANCE_ADAPTER _A = (Adapter); \
    PNDIS_PACKET _P = (Packet);\
    PNDIS_BUFFER _S = (SourceBuffer);\
    if((!_A->FirstSendPacket) && (!_A->ProcessingTransmits)){ \
      if(LanceProcessSendPacketImmediately(_A, _P, _S)){ \
        _A->References--; \
        NdisReleaseSpinLock(&_A->Lock); \
        return NDIS_STATUS_PENDING; \
      } \
    } \
}


//
// These are global structures for the MAC.
//

extern NDIS_HANDLE LanceMacHandle;
extern LIST_ENTRY LanceAdapterList;
extern BOOLEAN LanceUnloading;
extern NDIS_SPIN_LOCK LanceAdapterListLock;


//
// Definitions of lance functions which are used by multiple
// source files.
//


//
// alloc.c
//

extern
BOOLEAN
AllocateAdapterMemory(
    IN PLANCE_ADAPTER Adapter
    );

extern
VOID
DeleteAdapterMemory(
    IN PLANCE_ADAPTER Adapter
    );


//
// interrup.c
//

extern
VOID
LanceDeferredProcessing(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    );

extern
BOOLEAN
LanceInterruptService(
    IN PVOID Context
    );

extern
VOID
LanceTimerProcess(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    );

extern
VOID
LanceLockUpDetectProcess(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    );

VOID
LanceWakeUpDpc(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    );

//
// loopback.c
//

extern
BOOLEAN
LanceProcessLoopback(
   IN PLANCE_ADAPTER Adapter
   );

extern
VOID
LancePutPacketOnLoopBack(
   IN PLANCE_ADAPTER Adapter,
   IN PNDIS_PACKET Packet
   );


//
// packet.c
//

extern
VOID
LanceCopyFromPacketToBuffer(
    IN PNDIS_PACKET Packet,
    IN UINT Offset,
    IN UINT BytesToCopy,
    OUT PCHAR Buffer,
    OUT PUINT BytesCopied,
    OUT PUINT TotalPacketSize,
    OUT PNDIS_BUFFER *SourceBuffer
    );

extern
VOID
LanceCopyFromBufferToPacket(
    IN PCHAR Buffer,
    IN UINT BytesToCopy,
    IN PNDIS_PACKET Packet,
    IN UINT Offset,
    OUT PUINT BytesCopied
    );

//
// request.c
//

extern
VOID
LanceQueueRequest(
   IN PLANCE_ADAPTER Adapter,
    IN PNDIS_REQUEST NdisRequest
    );

extern
NDIS_STATUS
LanceRequest(
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_REQUEST NdisRequest
    );

extern
NDIS_STATUS
LanceQueryGlobalStatistics(
    IN NDIS_HANDLE MacAdapterContext,
    IN PNDIS_REQUEST NdisRequest
    );

extern
VOID
LanceProcessRequestQueue(
   IN PLANCE_ADAPTER Adapter
    );

extern
NDIS_STATUS
LanceChangeClass(
    IN UINT OldFilterClasses,
    IN UINT NewFilterClasses,
    IN NDIS_HANDLE NdisBindingContext,
    IN PNDIS_REQUEST NdisRequest,
    IN BOOLEAN Set
    );

extern
NDIS_STATUS
LanceChangeAddresses(
    IN UINT OldAddressCount,
    IN CHAR OldAddresses[][ETH_LENGTH_OF_ADDRESS],
    IN UINT NewAddressCount,
    IN CHAR NewAddresses[][ETH_LENGTH_OF_ADDRESS],
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_REQUEST NdisRequest,
    IN BOOLEAN Set
    );

extern
VOID
LanceCloseAction(
    IN NDIS_HANDLE MacBindingHandle
    );


//
// send.c
//

extern
NDIS_STATUS
LanceSend(
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_PACKET Packet
    );

extern
BOOLEAN
LanceProcessSendPacketQueue(
   IN PLANCE_ADAPTER Adapter
   );

extern
VOID
LancePutPacketOnSendPacketQueue(
    IN PLANCE_ADAPTER Adapter,
    IN PNDIS_PACKET Packet
    );


//
// lance.c
//

extern
BOOLEAN
LanceInit(
   IN PLANCE_ADAPTER Adapter
    );

extern
BOOLEAN
LanceInitializeChip(
   IN PLANCE_ADAPTER Adapter
    );

extern
VOID
LanceStartChip(
   IN PLANCE_ADAPTER Adapter
    );

extern
VOID
StartAdapterReset(
   IN PLANCE_ADAPTER Adapter
    );

STATIC
ULONG
LanceHardwareDetails(
   IN PLANCE_ADAPTER Adapter,
   IN NDIS_HANDLE ConfigurationHandle
    );

extern
VOID
LanceStopChip(
    IN PLANCE_ADAPTER Adapter
    );

STATIC
VOID
mca_enable_disable_dma(
    IN PLANCE_ADAPTER Adapter,
    BOOLEAN EnableDma
    );

extern
BOOLEAN
SetupRegistersAndInit(
    IN PLANCE_ADAPTER Adapter
    );

STATIC
VOID
scan_mca (
    IN PLANCE_ADAPTER Adapter,
    IN NDIS_HANDLE ConfigurationHandle
    );

STATIC
VOID
sanremo_init (
    IN NDIS_HANDLE ConfigurationHandle,
    IN ULONG IoAddr
    );

extern
BOOLEAN
check_conflict(
    IN ULONG LanceBaseAddress,
    IN CHAR LanceInterruptVector,
    IN CHAR LanceDmaChannel
    );

//
//
// transfer.c
//

extern
NDIS_STATUS
LanceTransferData(
    IN NDIS_HANDLE MacBindingHandle,
    IN NDIS_HANDLE MacReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer,
    OUT PNDIS_PACKET Packet,
    OUT PUINT BytesTransferred
    );

#endif // _LANCESFT_

