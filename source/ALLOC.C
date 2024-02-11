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

    alloc.c

Abstract:

    This file contains the code for allocating and freeing adapter
    resources for the AMD Lance Ethernet controller.
    This driver conforms to the NDIS 3.0 interface.

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

#if DBG
// For _Debug_Printf_Service()
#define  WANTVXDWRAPS
#include <basedef.h>
#include <vmm.h>
#include <vxdwraps.h>
#include <configmg.h>
#endif

#include <ndis.h>


#include <efilter.h>
#include <lancehrd.h>
#include <lancesft.h>

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//

#ifdef NDIS_WIN
    #ifndef CHICAGO // SNOWBALL
      #ifdef ALLOC_PRAGMA
        #pragma alloc_text(INIT,AllocateAdapterMemory)
      #endif
    #endif // SNOWBALL
#endif //NDIS_WIN


#ifdef NDIS_WIN
    #ifndef DEBUG
      #ifdef CHICAGO
        #pragma LCODE
      #else // SNOWBALL
        #pragma code_seg ("_ITEXT","ICODE")
      #endif // CHICAGO
    #endif //DEBUG
#endif //NDIS_WIN

BOOLEAN
AllocateAdapterMemory(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This routine allocates memory for:

    - Transmit ring entries

    - Receive ring entries

    - Transmit buffers

    - Receive buffers

    - Initialization block

Arguments:

    Adapter - The adapter to allocate memory for.

Return Value:

    Returns FALSE if some memory needed for the adapter could not
    be allocated. It does NOT call DeleteAdapterMemory in this
    case.

--*/

{
    UINT MemNeeded, Length;
    UINT pTempVa, pTempPa;
    
    //
    // allocate shared memory in one big chunk
    //

    // Memory Allocation needed for the 32 Bit devices.
    //

    MemNeeded = 
        sizeof(LANCE_TRANSMIT_DESCRIPTOR_HI)*Adapter->NumberOfTransmitDescriptors+
        sizeof(LANCE_RECEIVE_DESCRIPTOR_HI)*Adapter->NumberOfReceiveDescriptors+
        TRANSMIT_BUFFER_SIZE*Adapter->NumberOfTransmitDescriptors+
        RECEIVE_BUFFER_SIZE*Adapter->NumberOfReceiveDescriptors+
        sizeof(LANCE_INIT_BLOCK_HI)+ 0x20 ;
    
    NdisAllocateSharedMemory(
        Adapter->NdisAdapterHandle,
        MemNeeded,
        FALSE,         // non-cached
        (PVOID *)&Adapter->SharedMemoryVa,
        &Adapter->SharedMemoryPa
        );

    if (Adapter->SharedMemoryVa == NULL){

		//
		// Did not get it. Try it with a little less.
		// 

		Adapter->NumberOfTransmitDescriptors = TRANSMIT_BUFFERS/2;
		Adapter->NumberOfReceiveDescriptors = RECEIVE_BUFFERS/2;

    //
    // Memory Allocation needed for the 32 Bit devices.
    //

    MemNeeded = 
    	  sizeof(LANCE_TRANSMIT_DESCRIPTOR_HI)*Adapter->NumberOfTransmitDescriptors+
    	  sizeof(LANCE_RECEIVE_DESCRIPTOR_HI)*Adapter->NumberOfReceiveDescriptors+
    	  TRANSMIT_BUFFER_SIZE*Adapter->NumberOfTransmitDescriptors+
    	  RECEIVE_BUFFER_SIZE*Adapter->NumberOfReceiveDescriptors+
    	  sizeof(LANCE_INIT_BLOCK_HI)+ 0x20 ;

		NdisAllocateSharedMemory(
			Adapter->NdisAdapterHandle,
			MemNeeded,
			FALSE,         // non-cached
			(PVOID *)&Adapter->SharedMemoryVa,
			&Adapter->SharedMemoryPa
			);

		if (Adapter->SharedMemoryVa == NULL){
			return FALSE;
		}

    }    
       
    NdisZeroMemory(
        Adapter->SharedMemoryVa,
        MemNeeded
        );

    pTempVa = ((ULONG)Adapter->SharedMemoryVa + 0xf) & 0xfffffff0;
    pTempPa = (NdisGetPhysicalAddressLow(Adapter->SharedMemoryPa) + 0xf ) & 0xfffffff0;
    
    //
    // Allocate the initialization block.
    //

    Adapter->InitializationBlock = (PLANCE_INIT_BLOCK_HI) pTempVa;
    NdisSetPhysicalAddressLow(Adapter->InitializationBlockPhysical, pTempPa);
    
#if DBG
   if (LanceDbg)
       _Debug_Printf_Service("Initialization block: V = %lx, P = %lx\n", pTempVa, pTempPa);
#endif    
    Length = 0x20;
    pTempVa += Length;
    pTempPa += Length;


    //
    // Allocate the transmit ring descriptors.
    //

    Adapter->TransmitDescriptorRing = (PLANCE_TRANSMIT_DESCRIPTOR_HI)pTempVa;  
    NdisSetPhysicalAddressLow(Adapter->TransmitDescriptorRingPhysical, pTempPa);

#if DBG
   if (LanceDbg)
       _Debug_Printf_Service("Transmit descriptors ring: V = %lx, P = %lx\n", pTempVa, pTempPa);
#endif    

    Length = sizeof(LANCE_TRANSMIT_DESCRIPTOR_HI)*Adapter->NumberOfTransmitDescriptors;
    pTempVa += Length;
    pTempPa += Length;
    

    //
    // Allocate the receive ring descriptors.
    //
    
    Adapter->ReceiveDescriptorRing = (PLANCE_RECEIVE_DESCRIPTOR_HI) pTempVa;
    NdisSetPhysicalAddressLow(Adapter->ReceiveDescriptorRingPhysical, pTempPa);

#if DBG
   if (LanceDbg)
       _Debug_Printf_Service("Receive descriptor ring: V = %lx, P = %lx\n", pTempVa, pTempPa);
#endif    

    Length = sizeof(LANCE_RECEIVE_DESCRIPTOR_HI)*Adapter->NumberOfReceiveDescriptors;
    pTempVa += Length;
    pTempPa += Length;

    //
    // We have the transmit buffer pointers. Allocate the buffer
    // for each entry and zero them.
    //
    
    Adapter->TransmitBufferPointer = (PCHAR) pTempVa;
    NdisSetPhysicalAddressLow(Adapter->TransmitBufferPointerPhysical, pTempPa);

#if DBG
   if (LanceDbg)
      _Debug_Printf_Service("Transmit buffer: V = %lx, P = %lx\n", pTempVa, pTempPa);
#endif    
    
    Length = TRANSMIT_BUFFER_SIZE*Adapter->NumberOfTransmitDescriptors;
    pTempVa += Length;
    pTempPa += Length;

    
    //
    // We have the receive buffer pointers. Allocate the buffer
    // for each entry and zero them.
    //

    Adapter->ReceiveBufferPointer = (PCHAR) pTempVa;
    NdisSetPhysicalAddressLow(Adapter->ReceiveBufferPointerPhysical, pTempPa);

#if DBG
   if (LanceDbg)    
      _Debug_Printf_Service("Receive buffer: V = %lx, P = %lx\n", pTempVa, pTempPa);
#endif    
    
    Length = RECEIVE_BUFFER_SIZE*Adapter->NumberOfReceiveDescriptors;
    pTempVa += Length;
    pTempPa += Length;


    return TRUE;

}

#ifdef NDIS_WIN
    #ifndef DEBUG
      #ifndef CHICAGO // SNOWBALL
        #pragma code_seg ()
      #endif // SNOWBALL
    #endif //DEBUG
#endif //NDIS_WIN

VOID
DeleteAdapterMemory(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This routine deallocates memory for:

    - Transmit ring entries

    - Receive ring entries

    - Transmit buffers

   - Receive buffers

   - Initialization block

Arguments:

    Adapter - The adapter to deallocate memory for.

Return Value:

    None.

--*/

{
   UINT MemNeeded;

   MemNeeded = 
        sizeof(LANCE_TRANSMIT_DESCRIPTOR)*Adapter->NumberOfTransmitDescriptors+
        sizeof(LANCE_RECEIVE_DESCRIPTOR)*Adapter->NumberOfReceiveDescriptors+
        TRANSMIT_BUFFER_SIZE*Adapter->NumberOfTransmitDescriptors+
        RECEIVE_BUFFER_SIZE*Adapter->NumberOfReceiveDescriptors+
        sizeof(LANCE_INIT_BLOCK)+ 7 ;
    
   if (Adapter->SharedMemoryVa) {

      NdisFreeSharedMemory(
        Adapter->NdisAdapterHandle,
        MemNeeded,
        FALSE,         // non-cached
        Adapter->SharedMemoryVa,
        Adapter->SharedMemoryPa
        );


    }


}

