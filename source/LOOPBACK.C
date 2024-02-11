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

    loopback.c

Abstract:

    The routines here indicate packets on the loopback queue and are
    responsible for inserting and removing packets from the loopback
   queue.

Environment:

    Operates at dpc level - or the equivalent on os2 and dos.

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

BOOLEAN
LanceProcessLoopback(
   IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This routine is responsible for indicating *one* packet on
    the loopback queue either completing it or moving on to the
    finish send queue.

    NOTE: This routine is entered with LOCK held.
Arguments:

    Adapter - The adapter whose loopback queue we are processing.

Return Value:

    None.

--*/

{

   //
   // Packet at the head of the loopback list.
   //
   PNDIS_PACKET PacketToMove;

   //
   // The reserved portion of the above packet.
   //
   PLANCE_PACKET_RESERVED Reserved;

   //
   // The first buffer in the ndis packet to be loopbacked.
   //
   PNDIS_BUFFER FirstBuffer;

   //
   // The total amount of user data in the packet to be
   // loopbacked.
   //
   UINT TotalPacketLength;

   //
   // Eventually the address of the data to be indicated
   // to the transport.
   //
   PVOID BufferAddress;

   //
   // The address of the data to be indicated
   // to the transport.
   //
   PVOID DataAddress;

   //
   // Eventually the length of the data to be indicated
   // to the transport.
   //
   UINT BufferLength;

   PLANCE_OPEN Open;

   UINT TotalPacketSize;

   PNDIS_BUFFER SourceBuffer = NULL;


   //
   // We want to serialize the LoopBacks to avoid out of order
   // transmission of packets.  So make sure that only one thread
   // is in LanceProcessLoopback routine.
   //
   if (Adapter->ProcessingLoopBacks) {

      return FALSE;

   }

   //
   // Set the flag for serializing LoopBacks.
   //

   Adapter->ProcessingLoopBacks = TRUE;

   
   PacketToMove = Adapter->FirstLoopBack;

   Reserved = PLANCE_RESERVED_FROM_PACKET(Adapter->FirstLoopBack);

   Open = PLANCE_OPEN_FROM_BINDING_HANDLE(Reserved->MacBindingHandle);

   if (Reserved->Next == NULL) {

      Adapter->LastLoopBack = NULL;

   }

   Adapter->FirstLoopBack = Reserved->Next;

   Adapter->CurrentLoopbackPacket = PacketToMove;

   NdisReleaseSpinLock(&Adapter->Lock);

   //
   // See if we need to copy the data from the packet
   // into the loopback buffer.
   //
   // We need to copy to the local loopback buffer if
   // the first buffer of the packet is less than the
   // minimum loopback size AND the first buffer isn't
   // the total packet.
   //

   NdisQueryPacket(
       PacketToMove,
       NULL,
       NULL,
       &FirstBuffer,
       &TotalPacketLength
       );

   NdisQueryBuffer(
       FirstBuffer,
       &BufferAddress,
       &BufferLength
       );


   if (BufferLength < MAC_HEADER_SIZE) {

      //
      // Must have at least the destination address
      //

#if DBG
      if (LanceDbg) {
          _Debug_Printf_Service("Loopback:received runt packet\n");
      }
#endif
      if (BufferLength >= ETH_LENGTH_OF_ADDRESS) {

         //
         // Runt packet
         //

         EthFilterIndicateReceive(
            Adapter->FilterDB,
            (NDIS_HANDLE)NULL,
            ((PCHAR)BufferAddress),
            BufferAddress,                  // header
            BufferLength,                   // header size
            NULL,                           // lookahead
            0,                              // lookahead size
            0                               // packet size
            );

      }

   } else {

#if DBG
      if (LanceDbg) {
          _Debug_Printf_Service("Loopback:received good packet\n");
      }
#endif

      //
      // Copy the data if the first buffer does not hold
      // the header plus the loopback amount required.
      //
      // NOTE: We could copy less if all the bindings had
      // a short lookahead length set.
      //

      if ((BufferLength < LANCE_LOOPBACK_MAXIMUM+MAC_HEADER_SIZE) &&
         (BufferLength != TotalPacketLength)) {

         LanceCopyFromPacketToBuffer(
            PacketToMove,
            MAC_HEADER_SIZE,
            LANCE_LOOPBACK_MAXIMUM,
            Adapter->Loopback,
            &BufferLength,
            &TotalPacketSize,
            &SourceBuffer
         );

         DataAddress = Adapter->Loopback;

      } else {

         DataAddress = (PUCHAR)BufferAddress + MAC_HEADER_SIZE;
         BufferLength -= MAC_HEADER_SIZE;

      }

      //
      // Indicate the packet to every open binding
      // that could want it. Since loopback indications
      // are seralized, we store the packet here
      // and use a NULL handle to indicate that it
      // is for a loopback packet.
      //

      EthFilterIndicateReceive(
         Adapter->FilterDB,
         (NDIS_HANDLE)NULL,
         ((PCHAR)BufferAddress),
         BufferAddress,                  // header
         MAC_HEADER_SIZE,                // header size
         DataAddress,                    // lookahead
         BufferLength,                   // lookahead size
         TotalPacketLength - MAC_HEADER_SIZE // packet size
         );

   }


   //
   // Remove the packet from the loopback queue and
   // either indicate that it is finished or put
   // it on the finishing up queue for the real transmits.
   //

   NdisCompleteSend(
      Open->NdisBindingContext,
      PacketToMove,
      NDIS_STATUS_SUCCESS
      );

   //
   // If there is nothing else on the loopback queue
   // then indicate that reception is "done".
   //

   if (!Adapter->FirstLoopBack) {

       //
       // We need to signal every open binding that the
       // "receives" are complete.
       //

       EthFilterIndicateReceiveComplete(Adapter->FilterDB);


   }

   NdisAcquireSpinLock(&Adapter->Lock);

   //
   // We can decrement the reference count on the open
   // since it is no longer being "referenced" by the
   // packet on the loopback queue.
   //

   Open->References--;

   //
   // Reset the flag for serializing transmits.
   //
   Adapter->ProcessingLoopBacks = FALSE;

#if DBG
      if (LanceDbg) {
          _Debug_Printf_Service("LanceProcessLoopback:End\n");
      }
#endif


}

VOID
LancePutPacketOnLoopBack(
   IN PLANCE_ADAPTER Adapter,
   IN PNDIS_PACKET Packet
   )

/*++

Routine Description:

    Put the packet on the adapter wide loop back list.

    NOTE: This routine assumes that the lock is held.

    NOTE: This routine absolutely must be called before the packet
    is relinquished to the hardware.

    NOTE: This routine also increments the reference count on the
    open binding.

Arguments:

    Adapter - The adapter that contains the loop back list.

    Packet - The packet to be put on loop back.

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

   if (!Adapter->FirstLoopBack) {

      Adapter->FirstLoopBack = Packet;

   } else {

      PLANCE_RESERVED_FROM_PACKET(Adapter->LastLoopBack)->Next = Packet;

   }

   Reserved->Next = NULL;

   Adapter->LastLoopBack = Packet;

}
