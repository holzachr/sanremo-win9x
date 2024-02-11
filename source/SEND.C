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

   send.c

Abstract:

    This file contains the code for putting a packet through the
    staged allocation for transmission.

    This is a process of

    1) Calculating the what would need to be done to the
    packet so that the packet can be transmitted on the hardware.

    2) Potentially allocating adapter buffers and copying user data
    to those buffers so that the packet data is transmitted under
    the hardware constraints.

   3) Relinquish thos ring entries to the hardware.

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

#ifdef NDIS_WIN
    #ifndef DEBUG
      #ifdef CHICAGO
        #pragma LCODE
      #endif // CHICAGO
    #endif //DEBUG
#endif //NDIS_WIN

STATIC
BOOLEAN
LanceProcessSendPacketImmediately(
   IN PLANCE_ADAPTER Adapter,
   IN PNDIS_PACKET Packet,
   IN PNDIS_BUFFER SourceBuffer
   );

BOOLEAN
SendSynchronizeWithInterrupt(
    IN PVOID Context
    );

VOID
LanceSendImmediate( 
    IN PLANCE_ADAPTER Adapter
    );

STATIC
VOID
DeterminePacketAddressing(
    IN NDIS_HANDLE MacBindingHandle,
    PLANCE_OPEN Open,
    IN PNDIS_PACKET Packet,
    OUT PUINT TotalPacketSize,
    OUT PNDIS_BUFFER *SourceBuffer
    );

STATIC
VOID
TransmitPacket(
   IN PLANCE_ADAPTER Adapter,
   IN PNDIS_PACKET Packet,
	IN USHORT CurrentDescriptorIndex,
	IN PVOID CurrentDescriptor
   );

NDIS_STATUS
LanceSend(
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_PACKET Packet
    )

/*++

Routine Description:

    The LanceSend request instructs a MAC to transmit a packet through
    the adapter onto the medium.

Arguments:

    MacBindingHandle - The context value returned by the MAC  when the
    adapter was opened.  In reality, it is a pointer to LANCE_OPEN.

    Packet - A pointer to a descriptor for the packet that is to be
    transmitted.

Return Value:

    The function value is the status of the operation.

--*/

{

    //
    // Holds the status that should be returned to the caller.
    //
    NDIS_STATUS StatusToReturn = NDIS_STATUS_PENDING;

    //
    // Will point to the current source buffer.
    //
    PNDIS_BUFFER SourceBuffer = NULL;

    //
    // Pointer to the adapter.
    //
    PLANCE_ADAPTER Adapter;

    USHORT Csr0Value;

    BOOLEAN Canceled;

    Adapter = PLANCE_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);
    NdisAcquireSpinLock(&Adapter->Lock);
    Adapter->References++;

    if (!Adapter->ResetInProgress) {

      PLANCE_OPEN Open;

      Open = PLANCE_OPEN_FROM_BINDING_HANDLE(MacBindingHandle);

      if (!Open->BindingShuttingDown) {

         UINT TotalPacketSize = 0; 

         PLANCE_PACKET_RESERVED Reserved;

         //
         // Increment the references on the open while we are
         // accessing it in the interface.
         //

         Open->References++;

         Reserved = PLANCE_RESERVED_FROM_PACKET(Packet);

         //
         // Check to see if the packet should even make it out to
         // the media.  The primary reason this shouldn't *actually*
         // be sent is if the destination is equal to the source
         // address.
         //
         // If it doesn't need to be placed on the wire then we can
         // simply put it onto the loopback queue.
         //

         Reserved->MacBindingHandle = MacBindingHandle;

         //
         // All packets, except DA=SA packets need to
         // be placed out on the wire.
         //

         DeterminePacketAddressing(
                    MacBindingHandle,
                    Open,
                    Packet,
                    &TotalPacketSize,
                    &SourceBuffer);

         //
         // Do a quick check and fail if the packet is larger
         // than the maximum an ethernet can handle.
         //
  			//

         if ((!TotalPacketSize) || (!SourceBuffer) ||
             (TotalPacketSize > LANCE_INDICATE_MAXIMUM)) {
         
           #if DBG
           if (LanceDbg) {
               _Debug_Printf_Service("Transmit: Packet too small or too big.\n");
           }
           #endif

           StatusToReturn = NDIS_STATUS_INVALID_PACKET;
           --Open->References;
           NdisReleaseSpinLock(&Adapter->Lock);

           return StatusToReturn;
         }

         Reserved->PacketLength = TotalPacketSize;

         if (Reserved->PacketType == LANCE_LOOPBACK) {

           Adapter->LoopBackQueue = TRUE;
           LancePutPacketOnLoopBack(
                    Adapter,
                    Packet
                    );
           Adapter->LoopBackQueue = FALSE;

            //
            // Tally statistics now; assume that loopback
            // always "succeeds". These packets are always
            // directed (to us), so add to those counts.
            //

            ++Adapter->GeneralMandatory[GM_TRANSMIT_GOOD];

            ++Adapter->GeneralOptionalFrameCount[GO_DIRECTED_TRANSMITS];

            LanceAddUlongToLargeInteger(
                    &Adapter->GeneralOptionalByteCount[GO_DIRECTED_TRANSMITS],
                    Reserved->PacketLength);

           --Open->References;

           #if DBG
           if (LanceTx) {
               _Debug_Printf_Service("LSLP:ProcessingDeferredOperations=%d\n",Adapter->ProcessingDeferredOperations);
               _Debug_Printf_Service("LSLP:References=%d\n",Adapter->References);
               _Debug_Printf_Service("LSLP:ResetInProgress=%d\n",Adapter->ResetInProgress);
               _Debug_Printf_Service("LSLP:ResetPending=%d\n",Adapter->ResetPending);
               _Debug_Printf_Service("LSLP:FirstSendPacket=%lx\n",Adapter->FirstSendPacket);
               _Debug_Printf_Service("LSLP:FirstLoopBack=%lx\n",Adapter->FirstLoopBack);
           }
           #endif

           //
           // This macro assumes it is called with the lock held,
           // and releases it.
           //

           LANCE_DO_SEND(Adapter)
           return StatusToReturn;

         } else {

           #if DBG
           if (LanceTx) {
               _Debug_Printf_Service("LSImm:ProcessingDeferredOperations=%d\n",Adapter->ProcessingDeferredOperations);
               _Debug_Printf_Service("LSImm:References=%d\n",Adapter->References);
               _Debug_Printf_Service("LSImm:ResetInProgress=%d\n",Adapter->ResetInProgress);
               _Debug_Printf_Service("LSLP:ResetPending=%d\n",Adapter->ResetPending);
               _Debug_Printf_Service("LSImm:FirstSendPacket=%lx\n",Adapter->FirstSendPacket);
               _Debug_Printf_Service("LSImm:FirstLoopBack=%lx\n",Adapter->FirstLoopBack);
           }
           #endif

           LANCE_SEND_NO_Q(Adapter, Packet, SourceBuffer)

           --Open->References;
           Adapter->SendQueue = TRUE;
           LancePutPacketOnSendPacketQueue(Adapter, Packet);
           Adapter->SendQueue = FALSE;

           //
           // This macro assumes it is called with the lock held,
           // and releases it.
           //

           // LANCE_DO_SEND(Adapter)
           Adapter->References--; \
           NdisReleaseSpinLock(&Adapter->Lock); \

           return StatusToReturn;
         }

      } else {

         StatusToReturn = NDIS_STATUS_CLOSING;

      }

    } else {

      StatusToReturn = NDIS_STATUS_RESET_IN_PROGRESS;
    #if DBG
    if (LanceDbg) {
        _Debug_Printf_Service("LSImm:Reset In progress.\n");
    }
    #endif


    }

    #if DBG
    if (LanceTx) {
        _Debug_Printf_Service("LSImm:DO_DEFERRED Called.\n");
    }
    #endif

    //
    // This macro assumes it is called with the lock held,
    // and releases it.
    //

    LANCE_DO_DEFERRED(Adapter);
    return StatusToReturn;
}

VOID
DeterminePacketAddressing(
    IN NDIS_HANDLE MacBindingHandle,
    IN PLANCE_OPEN Open,
    IN PNDIS_PACKET Packet,
    OUT PUINT TotalPacketSize,
    OUT PNDIS_BUFFER *SourceBuffer
    )

/*++

Routine Description:

   Finds the packet type for this packet. It also determines
   if this packet should go out on the wire.

Arguments:

    MacBindingHandle - Is a pointer to the open binding.

    Packet - Packet whose source and destination addresses are tested.

Return Value:

    None.
--*/

{

    //
    // Holds the source address from the packet.
    //
    //    UCHAR Source[ETH_LENGTH_OF_ADDRESS];
    PUCHAR Source;

    //
    // Holds the destination address from the packet.
    //
    //    UCHAR Destination[ETH_LENGTH_OF_ADDRESS];
    PUCHAR Destination;

    UCHAR ArrayOfAddresses[ETH_LENGTH_OF_ADDRESS*2];

    //
    // Junk variable to hold the length of the source address.
    //
    UINT AddressLength;

    //
    // Will hold the result of the comparison of the two addresses.
    //
    INT Result = 0;
    INT i = 0;

    //
    // The MacReserved field in the packet.
    //
    PLANCE_PACKET_RESERVED Reserved = PLANCE_RESERVED_FROM_PACKET(Packet);

    PLANCE_ADAPTER Adapter = PLANCE_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);

    Reserved->PacketType = 0;

    LanceCopyFromPacketToBuffer(
        Packet,
        0,
        ETH_LENGTH_OF_ADDRESS*2,
        ArrayOfAddresses,
        &AddressLength,
        TotalPacketSize,
        SourceBuffer
        );

    ASSERT(AddressLength == ETH_LENGTH_OF_ADDRESS*2);

    Destination = (PUCHAR)ArrayOfAddresses;
    Source = &ArrayOfAddresses[6];

    // for(;Result<ETH_LENGTH_OF_ADDRESS;Result++){
    //    Destination[Result] = ArrayOfAddresses[Result];
    // }
	 // 
    // for(i=0;Result<ETH_LENGTH_OF_ADDRESS*2;Result++,i++)
    //   Source[i] = ArrayOfAddresses[Result];      

#ifdef NDIS_WIN

    if (!(Open->ProtOptionFlags & NDIS_PROT_OPTION_NO_LOOPBACK) &&
        (EthShouldAddressLoopBack(Adapter->FilterDB, Destination))) {

      Reserved->PacketType |= LANCE_LOOPBACK;

    }
#else

    if (EthShouldAddressLoopBack(Adapter->FilterDB, Destination)) {

      Reserved->PacketType |= LANCE_LOOPBACK;

    }

#endif

    if (ETH_IS_MULTICAST(Destination)) {

        if (ETH_IS_BROADCAST(Destination)) {

            Reserved->PacketType |= LANCE_BROADCAST;

        } else {

            Reserved->PacketType |= LANCE_MULTICAST;

        }

    } else {

      Reserved->PacketType |= LANCE_DIRECTED;

      ETH_COMPARE_NETWORK_ADDRESSES(
            Source,
            Destination,
            &Result
            );

      if (!Result) {

         Reserved->PacketType = LANCE_LOOPBACK;

        }

    }

#if DBG
    if (LanceAddress) {
       _Debug_Printf_Service("Destination:[ %.2x-%.2x-%.2x-%.2x-%.2x-%.2x ] ",
            Destination[0],
            Destination[1],
            Destination[2],
            Destination[3],
            Destination[4],
            Destination[5]);
       _Debug_Printf_Service("\n");
    }
#endif
#if DBG
    if (LanceAddress) {
       _Debug_Printf_Service("Source:[ %.2x-%.2x-%.2x-%.2x-%.2x-%.2x ] ",
            Source[0],
            Source[1],
            Source[2],
            Source[3],
            Source[4],
            Source[5]);
       _Debug_Printf_Service("\n");
    }
#endif


    //
    // If the two addresses are equal then the
    // packet shouldn't go out on the wire.
    //

    return;

}

VOID
TransmitPacket(
   IN PLANCE_ADAPTER Adapter,
   IN PNDIS_PACKET Packet,
	IN USHORT CurrentDescriptorIndex,
	IN PVOID CurrentDescriptor
   )

/*++

Routine Description:

   Given a packet attempt to acquire adapter buffer resources
   so that the packet can be copied in its entirety into an
   adapter buffer.

   Note:  This routine enters with the lock held and returns without
   releasing the lock.

Arguments:

    Adapter - The adapter the packet is coming through.

    Packet - The packet whose buffers are to be constrained.
             The packet reserved section is filled with information
             detailing how the packet needs to be adjusted.

Return Value:

    Returns TRUE if the packet is suitable for the hardware.

--*/

{

   //
   // Pointer to the reserved section of the packet.
   //
   PLANCE_PACKET_RESERVED Reserved = PLANCE_RESERVED_FROM_PACKET(Packet);

   //
   // Will point into the virtual address space addressed
   // by the adapter buffer.
   //
   PCHAR CurrentDestination;

   //
   // Transmit status of the packet.
   //
   UCHAR TransmitStatus;

   //
   // Transmit error of the packet.
   //
   USHORT TransmitError;

   //
   // Will hold the total amount of data copied to the
   // adapter buffer.
   //
   INT TotalDataMoved = 0;

   //
   // Will point to the current source buffer.
   //
   PNDIS_BUFFER SourceBuffer;

   //
   // Points to the virtual address of the source buffers data.
   //
   PVOID SourceData;

   //
   // Will point to the number of bytes of data in the source
   // buffer.
   //
   UINT SourceLength;

   //
   // Local copy of PacketType
   //
   UCHAR PacketType;

   #ifdef NDIS_WIN
   //
   // Local value of Csr0Value
   //
   ULONG Csr0Value;
   #endif

   // 
   BOOLEAN Canceled;

   //
   // Pointer to the transmit descriptor to be used.
   //
   PLANCE_TRANSMIT_DESCRIPTOR CurrentDescriptorLocal;

   //
   // Pointer to the transmit descriptor to be used.
   //
   PLANCE_TRANSMIT_DESCRIPTOR_HI CurrentDescriptorHiLocal;
   

   CurrentDestination = Adapter->TransmitBufferPointer +
                  (CurrentDescriptorIndex * TRANSMIT_BUFFER_SIZE);

   CurrentDescriptorHiLocal = (PLANCE_TRANSMIT_DESCRIPTOR_HI)CurrentDescriptor;
   TransmitStatus =  CurrentDescriptorHiLocal->LanceTMDFlags;
   TransmitError =  CurrentDescriptorHiLocal->TransmitError;

   #if DBG
     if (LanceTx) {
         _Debug_Printf_Service("TxCurrentDescriptor = %x Status = %x Index = %d\n", CurrentDescriptorHiLocal,
			TransmitStatus,CurrentDescriptorIndex);
	  }
   #endif

   //
   // Increment the next available descriptor index.
   //
   if (++Adapter->NextTransmitDescriptorIndex >=
            Adapter->NumberOfTransmitDescriptors) {

      Adapter->NextTransmitDescriptorIndex = 0;
   }

   //
   // Make this descriptor unavailable to other threads.
   //
   Adapter->TransmitDescriptorAvailable[CurrentDescriptorIndex] = FALSE;

   //
   // Release the lock.
   //
   NdisReleaseSpinLock(&Adapter->Lock);

   //
   // Fill in the adapter buffer with the data from the users
   // buffers.
   //

   NdisQueryPacket(
      Packet,
      NULL,
      NULL,
      &SourceBuffer,
      NULL
      );

    while (SourceBuffer) {

      NdisQueryBuffer(
         SourceBuffer,
         &SourceData,
         &SourceLength
         );

      LANCE_MOVE_MEMORY(
         CurrentDestination,
         SourceData,
         SourceLength
         );

      CurrentDestination = (PCHAR)CurrentDestination + SourceLength;

      TotalDataMoved += SourceLength;

      NdisGetNextBuffer(
         SourceBuffer,
         &SourceBuffer
         );

    }

   // Enabled Automatic Padding for Transmit Packets.
   //

   CurrentDescriptorHiLocal->ByteCount = -TotalDataMoved;

   NdisAcquireSpinLock(&Adapter->Lock);

   //
   // If reset ocurred between we released the lock and acquired again,
   // then we do nto want to do anything.
   //
   if (Adapter->TransmitDescriptorAvailable[CurrentDescriptorIndex]) {

      //
      // Reset the flag for serializing transmits.
      //
      // Adapter->ProcessingTransmits = FALSE;

      return;

   }

   //
   // Now change the ownership of the packet to Lance.
   // STP and ENP bits are permanently set as all packets should
   // fit in one buffer only. Also clear the error bits.
   //

   CurrentDescriptorHiLocal->LanceTMDFlags &=
       ~(LANCE_TRANSMIT_MORE_COLLISION |
       LANCE_TRANSMIT_ONE_COLLISION |
       LANCE_TRANSMIT_DEF_ERROR |
       DERR);
   CurrentDescriptorHiLocal->TransmitError = 0;
   CurrentDescriptorHiLocal->LanceTMDFlags |= OWN;

   //
   // Make this descriptor available to other threads.
   //
   Adapter->TransmitDescriptorAvailable[CurrentDescriptorIndex] = TRUE;

   //
   // Now instruct Lance to transmit by setting TDMD bit in CSR0.
   //

   // Read the Csr0 register and mask the INEA bit. Otherwise interrupts get
   // disabled !!
   // Note!! We're doing a Read - Mask and then a write back to Register Csr0.
   // 

   #ifdef NDIS_WIN
   _asm pushfd
   _asm cli
   
   LANCE_READ_PORT(Adapter, LANCE_CSR0, &Csr0Value);

   Csr0Value &= LANCE_CSR0_INEA;
   
   LANCE_WRITE_PORT(Adapter, LANCE_CSR0, (USHORT)(Csr0Value | LANCE_CSR0_TDMD));

   _asm popfd
   #endif


   #ifdef NDIS_NT

   NdisSynchronizeWithInterrupt(
       &Adapter->Interrupt,
       SendSynchronizeWithInterrupt,
       (PVOID)Adapter);

   #endif

#if DBG
   if (LanceDbg)
      _Debug_Printf_Service("Transmit: Packet transmitted.\n");
#endif

   //
   // As we do not update the statistics in the ISR, we need to
   // update them here before we use this descriptor again.
   // Check if the packet completed OK, and update statistics.
   //

   if (TransmitStatus & (DERR | LANCE_TRANSMIT_DEF_ERROR)) {

     #if DBG
     if (LanceDbg)
       _Debug_Printf_Service("Transmit failed: STATUS=%lx\n", TransmitStatus);
     #endif

     ++Adapter->GeneralMandatory[GM_TRANSMIT_BAD];

     if (TransmitStatus & LANCE_TRANSMIT_DEF_ERROR ) {
        ++Adapter->MediaOptional[MO_TRANSMIT_DEFERRED];
        }

     if (TransmitStatus & DERR) {

        if (TransmitError & LANCE_TRANSMIT_UFLO_ERROR) {
           ++Adapter->MediaOptional[MO_TRANSMIT_UNDERRUN];

            #if DBG
            if (LanceDbg)
               _Debug_Printf_Service("Transmit: UFLO error detected\n");
            #endif

        }
        if (TransmitError & LANCE_TRANSMIT_LCAR_ERROR) {
           ++Adapter->MediaOptional[MO_TRANSMIT_TIMES_CRS_LOST];
        }
        if (TransmitError & LANCE_TRANSMIT_LCOL_ERROR) {
           ++Adapter->MediaOptional[MO_TRANSMIT_LATE_COLLISIONS];
        }
        if (TransmitError & LANCE_TRANSMIT_RTRY_ERROR) {
           ++Adapter->MediaOptional[MO_TRANSMIT_MAX_COLLISIONS];
        }
        if (TransmitError & LANCE_TRANSMIT_BUF_ERROR) {
        #if DBG
        if (LanceDbg)
           _Debug_Printf_Service("Transmit: BUF error detected\n");
        #endif
        }
     }

   } else {

     ++Adapter->GeneralMandatory[GM_TRANSMIT_GOOD];

     if (TransmitStatus & LANCE_TRANSMIT_ONE_COLLISION) {
        ++Adapter->MediaMandatory[MM_TRANSMIT_ONE_COLLISION];
     }
     if   (TransmitStatus & LANCE_TRANSMIT_MORE_COLLISION) {
        ++Adapter->MediaMandatory[MM_TRANSMIT_MORE_COLLISIONS];
     }

     //
     // Turn off loopback flag from the type, if it was set.
     //
     PacketType = Reserved->PacketType;
     PacketType &= ~LANCE_LOOPBACK;

     switch (PacketType) {

        case LANCE_DIRECTED:

           ++Adapter->GeneralOptionalFrameCount[GO_DIRECTED_TRANSMITS];
           LanceAddUlongToLargeInteger(
                   &Adapter->GeneralOptionalByteCount[GO_DIRECTED_TRANSMITS],
                   Reserved->PacketLength);
           break;

        case LANCE_MULTICAST:

           ++Adapter->GeneralOptionalFrameCount[GO_MULTICAST_TRANSMITS];
           LanceAddUlongToLargeInteger(
                   &Adapter->GeneralOptionalByteCount[GO_MULTICAST_TRANSMITS],
                   Reserved->PacketLength);
           break;

        case LANCE_BROADCAST:

           ++Adapter->GeneralOptionalFrameCount[GO_BROADCAST_TRANSMITS];
           LanceAddUlongToLargeInteger(
                   &Adapter->GeneralOptionalByteCount[GO_BROADCAST_TRANSMITS],
                   Reserved->PacketLength);
           break;
     }

   }
   return;
}

BOOLEAN
SendSynchronizeWithInterrupt(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used during a Transmit on Demand. It read
    modify writes the csr0 to ensure the IENA bit in csr0 is 
    not modified.
    

Arguments:

    Context - A pointer to a LANCE_ADAPTER structure.

Return Value:

    Always returns true.

--*/

{
   ULONG Csr0Value;

   PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)Context;

   // Read the Csr0 register and mask the INEA bit. Otherwise interrupts get
   // disabled !!
   // Note!! We're doing a Read - Mask and then a write back to Register Csr0.
   // 

   LANCE_READ_PORT(Adapter, LANCE_CSR0, &Csr0Value);

   Csr0Value &= LANCE_CSR0_INEA;
   
   LANCE_WRITE_PORT(Adapter, LANCE_CSR0, (USHORT)(Csr0Value | LANCE_CSR0_TDMD));

   return TRUE;

}


BOOLEAN
LanceProcessSendPacketQueue(
   IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This routine is responsible for indicating *one* packet on
    the pending packet queue either completing it or moving on to the
    finish send queue.

    NOTE: This routine is entered with LOCK held returns without
    releasing the lock.

Arguments:

    Adapter - The adapter whose send packet queue we are processing.

Return Value:

    TRUE if it can transmit, else false.

--*/

{

   //
   // Packet at the head of the list.
   //
   PNDIS_PACKET Packet;

   //
   // The reserved portion of the above packet.
   //
   PLANCE_PACKET_RESERVED Reserved;

   //
   // Open handle for the current packet.
   //
   PLANCE_OPEN Open;

   //
   // Pointer to the transmit descriptor to be used.
   //
   PLANCE_TRANSMIT_DESCRIPTOR CurrentDescriptor;

   //
   // Pointer to the transmit descriptor to be used.
   //
   PLANCE_TRANSMIT_DESCRIPTOR_HI CurrentDescriptorHi;

   //
   // Index to next descriptor available.
   //
   USHORT CurrentDescriptorIndex;

   //
   // Transmit status of the packet.
   //
   UCHAR TransmitStatus;

   //
   // Local Value of the Csr0 value read.
   //

   USHORT Csr0Value;

   //
   // Local Value to cancel the Deferred Timer.
   //

   BOOLEAN Canceled;
   #if DBG
   if(LanceTx){
     _Debug_Printf_Service("LancePSPQ Begin\n");
   }
   #endif


   //
   // We want to serialize the transmit to avoid out of order
   // transmission of packets.  So make sure that only one thread
   // is in TransmitPacket routine.
   //
   if (Adapter->ProcessingTransmits) {
   #if DBG
   if(LanceTx){
     _Debug_Printf_Service("LancePSPQ: processingTx= True. Return\n");
   }
   #endif


      return FALSE;

   }


   //
   // Set the flag for serializing transmits.
   //
   Adapter->ProcessingTransmits = TRUE;

   //
   // Get the current descriptor.
   //

   CurrentDescriptorIndex = Adapter->NextTransmitDescriptorIndex;

   CurrentDescriptorHi = (PLANCE_TRANSMIT_DESCRIPTOR_HI)Adapter->TransmitDescriptorRing +
                          CurrentDescriptorIndex;
   TransmitStatus =  CurrentDescriptorHi->LanceTMDFlags;

   //
   // Check if current discriptor is owned by LANCE or not.
   //
   if ((TransmitStatus & OWN) ||
        (!Adapter->TransmitDescriptorAvailable[CurrentDescriptorIndex])) {
 	   
      #if DBG
      if (LanceTx) {
         _Debug_Printf_Service("ProcessSendPacketQueue: No descriptor available. ");
         _Debug_Printf_Service("Status = %x Index = %d\n", TransmitStatus,
               CurrentDescriptorIndex);
      }
      #endif

      //
      // Reset the flag for serializing transmits.
      //
      Adapter->ProcessingTransmits = FALSE;

      return FALSE;
   }

   //
   //
   //
   // Now we know that we have a Tx resource available, remove a
   // packet from the queue and transmit it.
   //

   Packet = Adapter->FirstSendPacket;

   Reserved = PLANCE_RESERVED_FROM_PACKET(Adapter->FirstSendPacket);

   if (Reserved->Next == NULL) {

      Adapter->LastSendPacket = NULL;

   }

   Adapter->FirstSendPacket = Reserved->Next;

   Open = PLANCE_OPEN_FROM_BINDING_HANDLE(Reserved->MacBindingHandle);

   TransmitPacket(Adapter, Packet, CurrentDescriptorIndex, (PVOID)CurrentDescriptorHi);

   if (Reserved->PacketType & LANCE_LOOPBACK) {

      LancePutPacketOnLoopBack(
            Adapter,
            Packet
            );

   } else {

      NdisReleaseSpinLock(&Adapter->Lock);

      //
      // Remove the packet from the pending send packet queue and
      // indicate that it is finished.
      //
      NdisCompleteSend(
         Open->NdisBindingContext,
         Packet,
         NDIS_STATUS_SUCCESS
         );

      NdisAcquireSpinLock(&Adapter->Lock);
   }

   //
   // We can decrement the reference count on the open
   // since it is no longer being "referenced" by the
   // packet on the send packet queue.
   //

   Open->References--;
   #if DBG
   if(LanceTx){
     _Debug_Printf_Service("LancePSPQ: End\n");
   }
   #endif

   //
   // Reset the flag for serializing transmits.
   //
   Adapter->ProcessingTransmits = FALSE;

   return TRUE;

}

VOID
LancePutPacketOnSendPacketQueue(
   IN PLANCE_ADAPTER Adapter,
   IN PNDIS_PACKET Packet
   )

/*++

Routine Description:

    Put the packet on the adapter wide pending send packet list.

    NOTE: This routine assumes that the lock is held.

    NOTE: This routine absolutely must be called before the packet
    is relinquished to the hardware.

Arguments:

    Adapter - The adapter that contains the pending send packet list.

    Packet - The packet to be transmitted.

Return Value:

    None.

--*/

{

   PLANCE_PACKET_RESERVED Reserved = PLANCE_RESERVED_FROM_PACKET(Packet);

   PLANCE_OPEN Open = PLANCE_OPEN_FROM_BINDING_HANDLE(Reserved->MacBindingHandle);

   //
   // Increment the references on the open while we are
   // accessing it in the interface.
   //

   Open->References++;

   if (!Adapter->FirstSendPacket) {

        Adapter->FirstSendPacket = Packet;

    } else {

        PLANCE_RESERVED_FROM_PACKET(Adapter->LastSendPacket)->Next = Packet;

    }

    Reserved->Next = NULL;

    Adapter->LastSendPacket = Packet;

}

VOID
LanceSendImmediate( 
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    Process the operations that are deferred by LanceDeferredProcessing.

Arguments:

    Adapter - A pointer to the adapter.

Return Value:

    None.

--*/

{

   //
   // Local Value to cancel the Deferred Timer.
   //

   BOOLEAN Canceled;

   NdisAcquireSpinLock(&Adapter->Lock);
   
   #if DBG
   if (LanceDbg){
     _Debug_Printf_Service("LanceSendImmediate:Ref: %d\n", Adapter->References);
   }
   #endif

   //
   // If we have a reset in progress and the adapters reference
   // count is 1 (meaning no routine is in the interface and
   // we are the only "active" interrupt processing routine) then
   // it is safe to start the reset.
   //

   if (Adapter->ResetInProgress && (Adapter->References == 2)) {

     #if DBG
     if (LanceDbg){
       _Debug_Printf_Service("\n Unwanted Reset \n");
     }
     #endif


           Adapter->ProcessingDeferredOperations = FALSE;
           Adapter->References--;
           NdisReleaseSpinLock(&Adapter->Lock);
           StartAdapterReset(Adapter);
           return;

   }
/*
   while (Adapter->FirstSendPacket && 
         (!Adapter->ResetInProgress) &&
         (!Adapter->SendQueue)) {

     //
     // Process the pending send packet queue.
     //

     if(!LanceProcessSendPacketQueue(Adapter)) 

        break;

   }
*/
   while (Adapter->FirstLoopBack &&
         (!Adapter->LoopBackQueue)) {


     #if DBG
     if (LanceDbg){
       _Debug_Printf_Service("SendImmediate:Processing LoopBacks \n");
     }
     #endif

     //
     // Process the loopback queue.
     //
     // NOTE: Incase anyone ever figures out how to make this
     // loop more reentriant, special care needs to be taken that
     // loopback packets and regular receive packets are NOT being
     // indicated at the same time.  While the filter indication
     // routines can handle this, I doubt that the transport can.
     //

     if(!LanceProcessLoopback(Adapter)){
       break;
     }

   }

   //
   // If there are any opens on the closing list and their
   // reference counts are zero then complete the close and
   // delete them from the list.
   //

   while (!IsListEmpty(&Adapter->CloseList)) {

     PLANCE_OPEN Open;

     #if DBG
     if (LanceDbg){
       _Debug_Printf_Service("SendImmediate:Processing Close List.\n");
     }
     #endif

     Open = CONTAINING_RECORD(
             Adapter->CloseList.Flink,
             LANCE_OPEN,
             OpenList
             );

     if (!Open->References) {

        NDIS_HANDLE OpenBindingContext = Open->NdisBindingContext;

        RemoveEntryList(&Open->OpenList);

        LANCE_FREE_MEMORY(Open, sizeof(LANCE_OPEN));

        --Adapter->OpenCount;

        NdisReleaseSpinLock(&Adapter->Lock);

        NdisCompleteCloseAdapter(
                OpenBindingContext,
                NDIS_STATUS_SUCCESS
                );

        NdisAcquireSpinLock(&Adapter->Lock);

     }

   }

   //
   // NOTE: We hold the spinlock here.
   //

   if ((Adapter->FirstLoopBack ||
       (!IsListEmpty(&Adapter->CloseList)))) {
     #if DBG
     if (LanceDbg){
       _Debug_Printf_Service("SendImmediate:Timer Set.\n");
     }
     #endif

        
     //
     // Fire off another call to LanceTimerProcess after 10 milli secs.
     //

     NdisSetTimer(&Adapter->DeferredTimer, 0);
    
   } else {

          // NdisCancelTimer(&Adapter->DeferredTimer, &Canceled);

          // if(Canceled)
            Adapter->ProcessingDeferredOperations = FALSE ;

           Adapter->References--;
    
   }

     #if DBG
     if (LanceDbg){
       _Debug_Printf_Service("SendImmediate:End.\n");
     }
     #endif

   NdisReleaseSpinLock(&Adapter->Lock);
   
}

STATIC
BOOLEAN
LanceProcessSendPacketImmediately(
   IN PLANCE_ADAPTER Adapter,
   IN PNDIS_PACKET Packet,
   IN PNDIS_BUFFER SourceBuffer
   )

/*++

Routine Description:

    This routine is responsible for indicating *one* packet on
    the pending packet queue either completing it or moving on to the
    finish send queue.

    NOTE: This routine is entered with LOCK held returns without
    releasing the lock.

Arguments:

    Adapter - The adapter whose send packet queue we are processing.

    Packet - A pointer to a descriptor for the packet that is to be
    transmitted.

    SourceBuffer - Will point to the current source buffer.
   
Return Value:

    TRUE if it can transmit, else false.

--*/

{
   //
   // Open handle for the current packet.
   //
   PLANCE_OPEN Open;

   //
   // Pointer to the transmit descriptor to be used.
   //
   PLANCE_TRANSMIT_DESCRIPTOR CurrentDescriptor;

   //
   // Pointer to the transmit descriptor to be used.
   //
   PLANCE_TRANSMIT_DESCRIPTOR_HI CurrentDescriptorHi;

   //
   // Index to next descriptor available.
   //
   USHORT CurrentDescriptorIndex;

   //
   // Will point into the virtual address space addressed
   // by the adapter buffer.
   //
   PUCHAR CurrentDestination;

   //
   // Transmit status of the packet.
   //
   UCHAR TransmitStatus;

   //
   // Transmit error of the packet.
   //
   USHORT TransmitError;

   //
   // Will hold the total amount of data copied to the
   // adapter buffer.
   //
   INT TotalDataMoved = 0;

   //
   // Points to the virtual address of the source buffers data.
   //
   PVOID SourceData;

   //
   // Will point to the number of bytes of data in the source
   // buffer.
   //
   UINT SourceLength;

   //
   // Local copy of PacketType
   //
   UCHAR PacketType;

   // Local Copy of the Packet Length.
   //
   UINT PacketLength;

   #ifdef NDIS_WIN
   //
   // Local value of Csr0Value
   //
   USHORT Csr0Value;
   #endif

   // 
   BOOLEAN Canceled;

   //
   // Pointer to the reserved section of the packet.
   //
   PLANCE_PACKET_RESERVED Reserved = PLANCE_RESERVED_FROM_PACKET(Packet);

   //
   // We want to serialize the transmit to avoid out of order
   // transmission of packets.  So make sure that only one thread
   // is in TransmitPacket routine.
   //
   if (Adapter->ProcessingTransmits) {
     #if DBG
     if (LanceTxOwn) {
           _Debug_Printf_Service("Imm: ProcesssingTransmits=TRUE \n");
     }
     #endif

     return FALSE;
   }

   //
   // Set the flag for serializing transmits.
   //
   Adapter->ProcessingTransmits = TRUE;

   //
   // Get the current descriptor.
   //

   CurrentDescriptorIndex = Adapter->NextTransmitDescriptorIndex;

   CurrentDescriptorHi = (PLANCE_TRANSMIT_DESCRIPTOR_HI)Adapter->TransmitDescriptorRing +
                       CurrentDescriptorIndex;
   TransmitStatus =  CurrentDescriptorHi->LanceTMDFlags;
   TransmitError =  CurrentDescriptorHi->TransmitError;
   CurrentDestination = Adapter->TransmitBufferPointer +
                        (CurrentDescriptorIndex * TRANSMIT_BUFFER_SIZE);

   #if DBG
     if (LanceTx) {
         _Debug_Printf_Service("Imm:TxCurrentDescriptor = %x Status = %x Index = %d\n", CurrentDescriptorHi,
			TransmitStatus,CurrentDescriptorIndex);
	  }
   #endif

   //
   // Check if current discriptor is owned by LANCE or not.
   //
   if ((TransmitStatus & OWN) ||
        (!Adapter->TransmitDescriptorAvailable[CurrentDescriptorIndex])) {

     #if DBG
     if (LanceTxOwn) {
           _Debug_Printf_Service("Imm: No descriptors!! ");
           _Debug_Printf_Service("Imm: CurrentDescriptor = %x Status = %x Index = %d\n", CurrentDescriptorHi,TransmitStatus,
                     CurrentDescriptorIndex);
     }
     #endif

     //
     // Reset the flag for serializing transmits.
     //
     Adapter->ProcessingTransmits = FALSE;

     return FALSE;
   }

   //
   // Make this descriptor unavailable to other threads.
   //
   Adapter->TransmitDescriptorAvailable[CurrentDescriptorIndex] = FALSE;

   //
   // Increment the next available descriptor index.
   //

   if (++Adapter->NextTransmitDescriptorIndex >=
            Adapter->NumberOfTransmitDescriptors) {

      Adapter->NextTransmitDescriptorIndex = 0;
   }

   //
   //
   //
   // Now we know that we have a Tx resource available, remove a
   // packet from the queue and transmit it.
   //

   Open = PLANCE_OPEN_FROM_BINDING_HANDLE(Reserved->MacBindingHandle);

   //
   // Release the lock.
   //
   NdisReleaseSpinLock(&Adapter->Lock);

   //
   // Fill in the adapter buffer with the data from the users
   // buffers.
   //

   while (SourceBuffer) {

     NdisQueryBuffer(
        SourceBuffer,
        &SourceData,
        &SourceLength
        );

     LANCE_MOVE_MEMORY(
        CurrentDestination,
        SourceData,
        SourceLength
        );

     CurrentDestination = (PCHAR)CurrentDestination + SourceLength;

     TotalDataMoved += SourceLength;

     NdisGetNextBuffer(
        SourceBuffer,
        &SourceBuffer
        );

   }

   // Enabled Automatic Padding for Transmit Packets.
   //

   CurrentDescriptorHi->ByteCount = -TotalDataMoved;

   NdisAcquireSpinLock(&Adapter->Lock);

   //
   // If reset ocurred between we released the lock and acquired again,
   // then we don't want to do anything.
   //
   if (Adapter->TransmitDescriptorAvailable[CurrentDescriptorIndex]) {

      //
      // Reset the flag for serializing transmits.
      //
      Adapter->ProcessingTransmits = FALSE;

      return FALSE;

   }

   //
   // Now change the ownership of the packet to Lance.
   // STP and ENP bits are permanently set as all packets should
   // fit in one buffer only. Also clear the error bits.
   //

   CurrentDescriptorHi->LanceTMDFlags &=
       ~(LANCE_TRANSMIT_MORE_COLLISION |
       LANCE_TRANSMIT_ONE_COLLISION |
       LANCE_TRANSMIT_DEF_ERROR |
       DERR);
   CurrentDescriptorHi->TransmitError = 0;
   CurrentDescriptorHi->LanceTMDFlags |= OWN;

   //
   // Make this descriptor available to other threads.
   //
   Adapter->TransmitDescriptorAvailable[CurrentDescriptorIndex] = TRUE;

   //
   // Now instruct Lance to transmit by setting TDMD bit in CSR0.
   //

   // Read the Csr0 register and mask the INEA bit. Otherwise interrupts get
   // disabled !!
   // Note!! We're doing a Read - Mask and then a write back to Register Csr0.
   // 

   #ifdef NDIS_WIN
   _asm pushfd
   _asm cli
   
   LANCE_READ_PORT(Adapter, LANCE_CSR0, &Csr0Value);

   Csr0Value &= LANCE_CSR0_INEA;
   
   LANCE_WRITE_PORT(Adapter, LANCE_CSR0, (USHORT)(Csr0Value | LANCE_CSR0_TDMD));

   _asm popfd
   #endif


   #ifdef NDIS_NT

   NdisSynchronizeWithInterrupt(
       &Adapter->Interrupt,
       SendSynchronizeWithInterrupt,
       (PVOID)Adapter);

   #endif

   #if DBG
   if (LanceTxPacket) {
     _Debug_Printf_Service("ImmTx: Packet.\n");
   }
   #endif

   //
   // Turn off loopback flag from the type, if it was set.
   //
   PacketType = Reserved->PacketType;
   PacketType &= ~LANCE_LOOPBACK;
   PacketLength = Reserved->PacketLength;

   if (Reserved->PacketType & LANCE_LOOPBACK) {

     Adapter->LoopBackQueue = TRUE;
     LancePutPacketOnLoopBack(
           Adapter,
           Packet
           );
     Adapter->LoopBackQueue = FALSE;

     if (!Adapter->ProcessingDeferredOperations){
       Adapter->ProcessingDeferredOperations = TRUE;

       while ((Adapter->FirstLoopBack) &&
              (!Adapter->LoopBackQueue)) {

         if(!LanceProcessLoopback(Adapter))
		     break;
       }

       if (Adapter->FirstLoopBack || 
           (!IsListEmpty(&Adapter->CloseList))) {

          //                                                  
          // Fire off another call to LanceTimerProcess
          // after 10 milli secs.
          //

          // Tell the Send Routine not to call NdisCompleteSend till
          // the loopback is processed.
          //

          Adapter->ProcessingDeferredOperations = TRUE;
          Adapter->References++;
      
          NdisSetTimer(&Adapter->DeferredTimer,0);
    
       } else {

         Adapter->ProcessingDeferredOperations = FALSE ;
       
       }
     }

   } else {

      NdisReleaseSpinLock(&Adapter->Lock);

      //
      // Remove the packet from the pending send packet queue and
      // indicate that it is finished.
      //
      NdisCompleteSend(
         Open->NdisBindingContext,
         Packet,
         NDIS_STATUS_SUCCESS
         );

      NdisAcquireSpinLock(&Adapter->Lock);
   }

   //
   // As we do not update the statistics in the ISR, we need to
   // update them here before we use this descriptor again.
   // Check if the packet completed OK, and update statistics.
   //

   if (TransmitStatus & (DERR | LANCE_TRANSMIT_DEF_ERROR)) {

     #if DBG
     if (LanceTx){
       _Debug_Printf_Service("Imm:Tx failed: STATUS=%lx\n", TransmitStatus);
     }
     #endif

     ++Adapter->GeneralMandatory[GM_TRANSMIT_BAD];

     if (TransmitStatus & LANCE_TRANSMIT_DEF_ERROR ) {
        ++Adapter->MediaOptional[MO_TRANSMIT_DEFERRED];
        }

     if (TransmitStatus & DERR) {

        if (TransmitError & LANCE_TRANSMIT_UFLO_ERROR) {
           ++Adapter->MediaOptional[MO_TRANSMIT_UNDERRUN];

            #if DBG
            if (LanceDbg){
               _Debug_Printf_Service("ImmTx: UFLO error detected\n");
            }
            #endif

        }
        if (TransmitError & LANCE_TRANSMIT_LCAR_ERROR) {
           ++Adapter->MediaOptional[MO_TRANSMIT_TIMES_CRS_LOST];
        }
        if (TransmitError & LANCE_TRANSMIT_LCOL_ERROR) {
           ++Adapter->MediaOptional[MO_TRANSMIT_LATE_COLLISIONS];
        }
        if (TransmitError & LANCE_TRANSMIT_RTRY_ERROR) {
           ++Adapter->MediaOptional[MO_TRANSMIT_MAX_COLLISIONS];
        }
        if (TransmitError & LANCE_TRANSMIT_BUF_ERROR) {
        #if DBG
        if (LanceDbg)
           _Debug_Printf_Service("Transmit: BUF error detected\n");
        #endif
        }
     }

   } else {

     ++Adapter->GeneralMandatory[GM_TRANSMIT_GOOD];

     if (TransmitStatus & LANCE_TRANSMIT_ONE_COLLISION) {
        ++Adapter->MediaMandatory[MM_TRANSMIT_ONE_COLLISION];
     }
     if   (TransmitStatus & LANCE_TRANSMIT_MORE_COLLISION) {
        ++Adapter->MediaMandatory[MM_TRANSMIT_MORE_COLLISIONS];
     }

     switch (PacketType) {

        case LANCE_DIRECTED:

           ++Adapter->GeneralOptionalFrameCount[GO_DIRECTED_TRANSMITS];
           LanceAddUlongToLargeInteger(
                   &Adapter->GeneralOptionalByteCount[GO_DIRECTED_TRANSMITS],
                   PacketLength);
           break;

        case LANCE_MULTICAST:

           ++Adapter->GeneralOptionalFrameCount[GO_MULTICAST_TRANSMITS];
           LanceAddUlongToLargeInteger(
                   &Adapter->GeneralOptionalByteCount[GO_MULTICAST_TRANSMITS],
                   PacketLength);
           break;

        case LANCE_BROADCAST:

           ++Adapter->GeneralOptionalFrameCount[GO_BROADCAST_TRANSMITS];
           LanceAddUlongToLargeInteger(
                   &Adapter->GeneralOptionalByteCount[GO_BROADCAST_TRANSMITS],
                   PacketLength);
           break;
     }

   }

   //
   // We can decrement the reference count on the open
   // since it is no longer being "referenced" by the
   // packet on the send packet queue.
   //

   Open->References--;

   //
   // Reset the flag for serializing transmits.
   //
   Adapter->ProcessingTransmits = FALSE;


   return TRUE;

}

