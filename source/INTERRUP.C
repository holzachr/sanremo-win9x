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

   interrup.c

Abstract:

   This is a part of the driver for the Advanced Micro Devices LANCE
   Ethernet controller.  It contains the interrupt-handling routines.
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

#ifdef NDIS_WIN
    #ifndef DEBUG
      #ifdef CHICAGO
        #pragma LCODE
      #endif // CHICAGO
    #endif //DEBUG
#endif //NDIS_WIN

STATIC
VOID
LanceTimerProcessNoLock(
    IN PVOID Context
    );

STATIC
VOID
ProcessReceiveInterrupts(
    IN PLANCE_ADAPTER Adapter
    );

BOOLEAN
LanceInterruptService(
    IN PVOID Context
    )

/*++

Routine Description:

    Interrupt service routine for the Lance.  It's main job is
    to get the value of CSR0 and record the changes in the
    adapters own list of interrupt reasons.

Arguments:

    Context - Really a pointer to the adapter.

Return Value:

    Returns true if the card CSR0 is non-zero.

--*/

{
   USHORT io18, io02;
   
   //
   // Holds the pointer to the adapter.
   //
   PLANCE_ADAPTER Adapter = Context;

   //
   // Local value of CSR0.
   // Bug Fix. Spurious interrupts change the value of this
   // field in the Adapter structure. If the interrupt returns
   // false the field is not updated, and if true, field is updated.

   ULONG Csr0Value;

   LOG(IN_ISR)

   #if DBG
   if (LanceIsr)
       _Debug_Printf_Service("LanceISR\n");
   #endif

   /* Read ASIC interrupt flags?
      ASIC is not documented. */  	
   NdisReadPortUshort(Adapter->NdisAdapterHandle, Adapter->LanceBaseAddress + 0x18, (PUSHORT)&io18);
   NdisReadPortUshort(Adapter->NdisAdapterHandle, Adapter->LanceBaseAddress + 0x02, (PUSHORT)&io02);

   LANCE_READ_PORT(Adapter, LANCE_CSR0, &Csr0Value);

   //Check if STOP bit was set by Power Management
   if(Csr0Value & LANCE_CSR0_STOP){

     //
     // Disable device interrupts before reading.
     //
                               
     LANCE_WRITE_PORT(Adapter, LANCE_CSR0, 0);

     // 
     // Indicate that the DPC will be called with 
     // Interrupts disabled.
     //

     Adapter->InterruptFlag = TRUE ;

     // Mask the IENA bit before writting back to the chip.
     Csr0Value &= 0xFFBF;

     LANCE_WRITE_PORT(Adapter, LANCE_CSR0, Csr0Value);

     Adapter->stop_set = 1;

     Adapter->Csr0Value = Csr0Value;

     /* Acknowledge ASIC interrupt flags and re-prepare?
        ASIC is not documented. */
     NdisWritePortUshort(Adapter->NdisAdapterHandle, Adapter->LanceBaseAddress + 0x18, (USHORT)io18);
     NdisWritePortUshort(Adapter->NdisAdapterHandle, Adapter->LanceBaseAddress + 0x02, (USHORT)io02);
     NdisWritePortUshort(Adapter->NdisAdapterHandle, Adapter->LanceBaseAddress + 0x1A, (USHORT)0x0FFF);

     LOG(OUT_ISR)
     return TRUE;
   }
   
   //
   // It's our interrupt. Clear only those bits that we got
   // in this read of CSR0.  We do it this way in case any new
   // reasons for interrupts occur between the time that we
   // read CSR0 and the time that we clear the bits.  So we
   // write the value of CSR0 we read back to CSR0 and clear
   // the bits were set when we read.
   //

   if ((Csr0Value & 0x0080) && 
       (Csr0Value & LANCE_CSR0_INEA)) {

      //
      // Disable device interrupts before reading.
      //

      LANCE_WRITE_PORT(Adapter, LANCE_CSR0, 0);

      // 
      // Indicate that the DPC will be called with 
      // Interrupts disabled.
      //

      Adapter->InterruptFlag = TRUE ;

      // Mask the IENA bit before writting back to the chip.
      Csr0Value &= 0xFFBF;
      
      LANCE_WRITE_PORT(Adapter, LANCE_CSR0, Csr0Value);                  

      /* Acknowledge ASIC interrupt flags and re-prepare?
         ASIC is not documented. */
      NdisWritePortUshort(Adapter->NdisAdapterHandle, Adapter->LanceBaseAddress + 0x18, (USHORT)io18);
      NdisWritePortUshort(Adapter->NdisAdapterHandle, Adapter->LanceBaseAddress + 0x02, (USHORT)io02);
      NdisWritePortUshort(Adapter->NdisAdapterHandle, Adapter->LanceBaseAddress + 0x1A, (USHORT)0x0FFF);
      
      LOG(OUT_ISR)

      Adapter->Csr0Value = Csr0Value;

      return TRUE;

   } else {

      /* Acknowledge ASIC interrupt flags and re-prepare?
         ASIC is not documented. */
      NdisWritePortUshort(Adapter->NdisAdapterHandle, Adapter->LanceBaseAddress + 0x18, (USHORT)io18);
      NdisWritePortUshort(Adapter->NdisAdapterHandle, Adapter->LanceBaseAddress + 0x02, (USHORT)io02);
      NdisWritePortUshort(Adapter->NdisAdapterHandle, Adapter->LanceBaseAddress + 0x1A, (USHORT)0x0FFF);
    
      LOG(OUT_ISR)
      return FALSE;

   }

}

VOID
LanceDeferredProcessing(
        IN PVOID SystemSpecific1,
        IN PVOID Context,
        IN PVOID SystemSpecific2,
        IN PVOID SystemSpecific3
        )

/*++

Routine Description:

        This DPC routine is queued by the interrupt service routine
        and other routines within the driver that notice that
        some deferred processing needs to be done.  It's main
        job is to call the interrupt processing code.

Arguments:

        Context - Really a pointer to the adapter.

        SystemSpecific123 - None of these arguments is used.

Return Value:

        None.

--*/

{

   //
   // Local value of CSR0.

   USHORT Csr0Value;

   BOOLEAN Canceled;

   UINT Data = 0;
   USHORT Time = 0;      
                                                                           
   //
   // A pointer to the adapter object.
   //
   
   PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)Context;

   #if DBG
   if (LanceIsr)
       _Debug_Printf_Service("LanceDPC\n");
   #endif

   LOG(IN_DPC)
   UNREFERENCED_PARAMETER(SystemSpecific1);
   UNREFERENCED_PARAMETER(SystemSpecific2);
   UNREFERENCED_PARAMETER(SystemSpecific3);

   #if DBG
   if(LanceRx){
     _Debug_Printf_Service("DPC Begin\n");
     _Debug_Printf_Service("RX CSR0: %x\n", Adapter->Csr0Value);
   }
   #endif

   // bug fix for 3888  
   if (Adapter->Removed) return;

   NdisDprAcquireSpinLock(&Adapter->Lock);

   if (Adapter->ProcessingReceiveInterrupt) {

     #if DBG
     if (LanceDbg)
       _Debug_Printf_Service("Adapter processing interrupts. Wait!! \n");

     #endif


     NdisDprReleaseSpinLock(&Adapter->Lock);
     
     return;

   }

   Csr0Value=Adapter->Csr0Value;
   
   Adapter->ProcessingReceiveInterrupt = TRUE;

   //
   // Initialize LockUpTimeout flag to FALSE for Lance chip.
   //
   
   // If STOP bit is set, reset the chip
   if(Adapter->stop_set) {

     Adapter->References++;

     #if DBG
       if (LanceDbg)
         _Debug_Printf_Service("STOP bit set on Rx: reset initiated \n");
     #endif

     if((!Adapter->ResetInProgress) &&
        (!Adapter->ProcessingTransmits)){

       Adapter->ResetInProgress = TRUE;

       //
       // First we make sure that the device is stopped.
       //

       LanceStopChip(Adapter);
       
       SetupRegistersAndInit(Adapter);
       
       //
       // Start Lance, but do not enable interrupts as
       // interrupts will be enabled at the end of DPC.
       //
       
       LANCE_WRITE_PORT(Adapter, LANCE_CSR0, LANCE_CSR0_START|LANCE_CSR0_INIT);

       Adapter->ResetInProgress = FALSE;

       // After the reset return and ignore the incoming packet (if any).

       Adapter->References--;

       Adapter->ProcessingReceiveInterrupt = FALSE;
                                          
       SANREMO_ENABLE_INTERRUPTS(Adapter);              
       LANCE_WRITE_PORT(Adapter, LANCE_CSR0, LANCE_CSR0_INEA);

       // 
       // Indicate that the DPC will reenable interrupts 
       // before a return.
       //

       Adapter->InterruptFlag = FALSE ;

       NdisDprReleaseSpinLock(&Adapter->Lock);

       LOG(OUT_DPC)

       return ;

     }

     Adapter->References--;
     
   }

   //
   // Check for receive interrupts.
   //

   if (((Csr0Value & LANCE_CSR0_RINT)||(LANCE_CSR0_MISS)) &&  
        (!Adapter->ResetInProgress)) {

       Adapter->References++;

       ProcessReceiveInterrupts(Adapter);

       //
       // We need to signal every open binding that the
       // receives are complete.
       //
       
       Adapter->ProcessingReceiveInterrupt = FALSE;

       NdisDprReleaseSpinLock(&Adapter->Lock);

       EthFilterIndicateReceiveComplete(Adapter->FilterDB);

       NdisDprAcquireSpinLock(&Adapter->Lock);

       Adapter->References--;

       //
       // Check the interrupt source and other reasons
       // for processing.  If there are no reasons to
       // process then exit this loop.  Here we have only
       // one more checking to do for the error interrupt as
       // we do not nothing for transmit interrupt.
       //

   }

   if(Csr0Value & LANCE_CSR0_ERR) {

     //
     // Note: We hold the lock here.
     //
     Adapter->References++;

     //
     // Check for different errors and take appropriate steps
     // to recover from it and update the statastics
     //

     if (Csr0Value & LANCE_CSR0_BABL) {
         LOG(BABL)
		 
		 #if DBG
         if (LanceErr) 
             _Debug_Printf_Service("DPC: BABL\n");
         #endif
     }

     if (Csr0Value & LANCE_CSR0_CERR) {
             ++Adapter->MediaOptional[MO_TRANSMIT_HEARTBEAT_FAILURE];
             LOG(HEART)
			 
			 #if DBG
             if (LanceErr) 
                 _Debug_Printf_Service("DPC: CERR\n");
             #endif
     }

     if (Csr0Value & LANCE_CSR0_MISS) {

       ++Adapter->GeneralMandatory[GM_RECEIVE_NO_BUFFER];
       LOG(MISSED)
       
	   #if DBG
       if (LanceErr) 
           _Debug_Printf_Service("DPC: MISS\n");
       #endif
     }
       
     if (Csr0Value & LANCE_CSR0_MERR) {
       LOG(ERR)
	   
	   #if DBG
       if (LanceErr) 
           _Debug_Printf_Service("DPC: MERR\n");
       #endif
     }
     Adapter->References--;
   }   

   //
   // Note: We hold the lock here.
   //

   #if DBG
   if (LanceTx) {
       _Debug_Printf_Service("DPC:ProcessingDeferredOperations=%d\n",Adapter->ProcessingDeferredOperations);
       _Debug_Printf_Service("DPC:References=%d\n",Adapter->References);
       _Debug_Printf_Service("DPC:ResetInProgress=%d\n",Adapter->ResetInProgress);
       _Debug_Printf_Service("LSLP:ResetPending=%d\n",Adapter->ResetPending);
       _Debug_Printf_Service("DPC:FirstSendPacket=%lx\n",Adapter->FirstSendPacket);
       _Debug_Printf_Service("DPC:FirstLoopBack=%lx\n",Adapter->FirstLoopBack);
       _Debug_Printf_Service("DPC:SendQueue=%lx\n",Adapter->SendQueue);
   }
   #endif

   //
   // NOTE: We have the spinlock here.
   //

   if ((!Adapter->ProcessingDeferredOperations) &&
           (Adapter->FirstLoopBack ||
           (Adapter->ResetInProgress && (Adapter->References == 1)) ||
           (!IsListEmpty(&Adapter->CloseList)))) {

     Adapter->ProcessingDeferredOperations = TRUE;
     Adapter->References++;

     // The chip interrupts have to be enabled before
     // releasing the Spin Lock. Doing otherwise, allows 
     // the TransmitPacket routine to disable interrupts
     // for ever. This problem is seen on a multi processor NT
     // system.

     LanceTimerProcessNoLock(Adapter);

   }

   Adapter->References++;

   while (Adapter->FirstSendPacket && 
         (!Adapter->ResetInProgress) &&
         (!Adapter->SendQueue)) {

     //
     // Process the pending send packet queue.
     //

     if(!LanceProcessSendPacketQueue(Adapter))

        break;

   }

   Adapter->References--;

   //
   // NOTE: We have the spinlock here.
   //

   Adapter->ProcessingReceiveInterrupt = FALSE;

   // Indicate that the DPC will reenable interrupts 
   // before a return.
   //
   Adapter->InterruptFlag = FALSE ;

   //
   // NOTE: This code assumes that the above code left
   // the spinlock released.
   //
                     
   LANCE_WRITE_PORT(Adapter, LANCE_CSR0, (USHORT)(LANCE_CSR0_INEA));

   NdisDprReleaseSpinLock(&Adapter->Lock);

   #if DBG
   if(LanceRx){
     _Debug_Printf_Service("DPC Completed\n");
   }
   #endif

   LOG(OUT_DPC)

}

VOID
LanceTimerProcess(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )

/*++

Routine Description:

    Process the operations that are deferred by LanceDeferredProcessing.

Arguments:

    Context - A pointer to the adapter.

Return Value:

    None.

--*/

{

   //
   // Local Value to cancel the Deferred Timer.
   //

   BOOLEAN Canceled;

   PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)Context;

   UNREFERENCED_PARAMETER(SystemSpecific1);
   UNREFERENCED_PARAMETER(SystemSpecific2);
   UNREFERENCED_PARAMETER(SystemSpecific3);

   NdisDprAcquireSpinLock(&Adapter->Lock);

   #if DBG
   if (LanceDbg){
     _Debug_Printf_Service("LanceTimer:TRef: %d\n", Adapter->References);
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
       _Debug_Printf_Service("\n Unwanted TReset \n");
     }

     #endif

     Adapter->ProcessingDeferredOperations = FALSE;
     Adapter->References--;
     NdisDprReleaseSpinLock(&Adapter->Lock);
     StartAdapterReset(Adapter);
     return;

   }

   while (Adapter->FirstLoopBack &&
         (!Adapter->LoopBackQueue)) {

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

        NdisDprReleaseSpinLock(&Adapter->Lock);

        NdisCompleteCloseAdapter(
                OpenBindingContext,
                NDIS_STATUS_SUCCESS
                );

        NdisDprAcquireSpinLock(&Adapter->Lock);

     }

   }

   //
   // NOTE: We hold the spinlock here.
   //

   if ((Adapter->FirstLoopBack || 
       (!IsListEmpty(&Adapter->CloseList)))) {

   //                                                  
   // Fire off another call to LanceTimerProcess after 10 milli secs.
   //
      
       NdisSetTimer(&Adapter->DeferredTimer,0);
    
   } else {

       Adapter->ProcessingDeferredOperations = FALSE ;

       Adapter->References--;
        
   }
    
   NdisDprReleaseSpinLock(&Adapter->Lock);
   
}

STATIC
VOID
ProcessReceiveInterrupts(
   IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    Process the packets that have finished receiving.

    NOTE: This routine assumes that no other thread of execution
    is processing receives!

Arguments:

    Adapter - The adapter to indicate to.

Return Value:

    None.

--*/

{

   //
   // We don't get here unless there was a receive.  Loop through
   // the receive descriptors starting at the last known descriptor
   // owned by the hardware that begins a packet.
   //
   // Examine each receive ring descriptor for errors.
   //

   //
   // Pointer to the receive descriptor being examined.
   //
   PLANCE_RECEIVE_DESCRIPTOR CurrentDescriptor;

   //
   // Pointer to the receive descriptor being examined.
   //
   PLANCE_RECEIVE_DESCRIPTOR_HI CurrentDescriptorHi;


   //
   // Index to descriptor to be processed.
   //
   USHORT CurrentDescriptorIndex;	

   //
   // Receive status of the packet.
   //
   UCHAR ReceiveStatus;

   //
   // The virtual address of the packet.
   //
   PVOID PacketVa;

   //
   // Look adead packet size for indication
   //
   UINT LookAheadSize;

   //
   // The size of the packet.
   //
   UINT PacketSize;

   //
   // Receive packet type.
   //
   USHORT ReceivePacketType;

   //
   // Storage of Csr0 Values.
   //

   USHORT Csr0Value;

   //
   // NdisSetTimer Loop Value set with this variable.
   //

   USHORT Time = 0;

   LOG(RECEIVE)

   while (TRUE) {

      CurrentDescriptorIndex = Adapter->NextReceiveDescriptorIndex;

      CurrentDescriptorHi = (PLANCE_RECEIVE_DESCRIPTOR_HI)Adapter->ReceiveDescriptorRing +
                                                                 CurrentDescriptorIndex;
      ReceiveStatus = CurrentDescriptorHi->LanceRMDFlags;

      #if DBG
        if (LanceRx) {
            _Debug_Printf_Service("RxCurrentDescriptor = %x Status = %x Index = %d\n", CurrentDescriptorHi,
		    	ReceiveStatus,CurrentDescriptorIndex);
	     }
      #endif

      //
      // Check to see whether we own the packet.  If
      // we don't then simply return to the caller.
      //
      if (ReceiveStatus & OWN) {
         #if DBG
         if(LanceRx)
             _Debug_Printf_Service("Descriptors not driver's \n");
         #endif
         return;
      }

      //
      // Advance our pointers to the next descriptor.
      //

      if ((++Adapter->NextReceiveDescriptorIndex) >=
                    Adapter->NumberOfReceiveDescriptors) {

              Adapter->NextReceiveDescriptorIndex = 0;

      }

      NdisDprReleaseSpinLock(&Adapter->Lock);

      //
      // Check that the packet was received correctly.
      // As we have the buffer which can receive maximum size packet,
      // ENP and STP should be set.
      //

      if ((ReceiveStatus & DERR) ||
         !((ReceiveStatus & ENP) && (ReceiveStatus & STP))) {

        #if DBG
        if (LanceRx)
           _Debug_Printf_Service("RX: Skipping %lx\n", ReceiveStatus);
        #endif

        goto SkipIndication;

      }

      //
      // Packet is good.
      // Prepare to indicate the packet.
      //

      PacketSize = CurrentDescriptorHi->ByteCount - 4;
      
      if (PacketSize > 1514) {

        #if DBG
        if (LanceRx)
          _Debug_Printf_Service("DPC: Skipping packet, length %d\n", PacketSize);
        #endif

        goto SkipIndication;

      }

      #if DBG
      if (LanceRx)
        _Debug_Printf_Service("Receive: Packet size = %d\n ", PacketSize);
      #endif

      if (PacketSize < LANCE_INDICATE_MAXIMUM) {

              LookAheadSize = PacketSize;

      } else {

              LookAheadSize = LANCE_INDICATE_MAXIMUM;

      }

      PacketVa = (PVOID)(Adapter->ReceiveBufferPointer +
                     (CurrentDescriptorIndex * RECEIVE_BUFFER_SIZE));


      //
      // Check just before we do indications that we aren't
      // resetting.
      //

      if (Adapter->ResetInProgress) {
              #if DBG
              if (LanceRx) {
                 _Debug_Printf_Service("Receive: Reset in progress.\n");
              }
              #endif
              NdisDprAcquireSpinLock (&Adapter->Lock);

              return;

      }

      //
      // Is it broadcast/multicast?
      //

      if (ETH_IS_MULTICAST(PacketVa)){

              //
              // Is it broadcast?
              //

              if (ETH_IS_BROADCAST(PacketVa)) {

                 ReceivePacketType = LANCE_BROADCAST;

                 #if DBG
                 if (LanceRx)
                   _Debug_Printf_Service("This is a broadcast packet\n");
                 #endif

              } else {

              //
              // This is multicast.
              //

                 ReceivePacketType = LANCE_MULTICAST;


                 #if DBG
                 if (LanceRx)
                   _Debug_Printf_Service("This is a multicast packet\n");
                 #endif
 
             }

      } else {

         //
         // This is a directed packet.
         //


               #if DBG
               if (LanceRx)
                 _Debug_Printf_Service("This is a directed packet\n");
               #endif
 
               ReceivePacketType = LANCE_DIRECTED;

      }

      if (PacketSize < MAC_HEADER_SIZE) {

              //
              // Must have at least the destination address
              //

              if (PacketSize >= ETH_LENGTH_OF_ADDRESS) {

                      //
                      // Runt packet
                      //

                      #if DBG
                      if (LanceRx)
                        _Debug_Printf_Service("Send runt packet to wrapper \n");
                      #endif

                      EthFilterIndicateReceive(
                         Adapter->FilterDB,
                         (NDIS_HANDLE)((PUCHAR)PacketVa + MAC_HEADER_SIZE),  // context
                         PacketVa,                              // destination address
                         PacketVa,                              // header buffer
                         PacketSize,                            // header buffer size
                         NULL,                                  // lookahead buffer
                         0,                                     // lookahead buffer size
                         0                             
                         );

              }

      } else {

                   #if DBG
                   if (LanceRx)
                     _Debug_Printf_Service("Send good packet to wrapper \n");
                   #endif

                   EthFilterIndicateReceive(
                           Adapter->FilterDB,
                           (NDIS_HANDLE)((PUCHAR)PacketVa + MAC_HEADER_SIZE), // context                   // context
                           (PCHAR)PacketVa,                        // destination address
                           PacketVa,                               // Header buffer
                           MAC_HEADER_SIZE,                        // header buffer size
                           (PUCHAR)PacketVa + MAC_HEADER_SIZE,     // lookahead buffer
                           LookAheadSize - MAC_HEADER_SIZE,        // lookahead buffer size
                           PacketSize - MAC_HEADER_SIZE            // packet size
                           );

      }

SkipIndication:;

      NdisDprAcquireSpinLock (&Adapter->Lock);

      //
      //  Restore the bcnt field, it is overwriten by HILANCE!
      //
      CurrentDescriptorHi->BufferSize = -RECEIVE_BUFFER_SIZE;

      // Reset the Runt/.. Counts.
      //
      CurrentDescriptorHi->LanceRMDReserved1 = 0;

      //
      // Reset the error bits.
      //
      CurrentDescriptorHi->LanceRMDFlags &= 0x0;
		
      //
      // Give the descriptor back to the hardware.
      //
      CurrentDescriptorHi->LanceRMDFlags |= OWN;

      //
      // Update statistics now based on the receive status.
      //

      if (!(ReceiveStatus & DERR)) {

              ++Adapter->GeneralMandatory[GM_RECEIVE_GOOD];

              if (ReceivePacketType == LANCE_BROADCAST) {

                 ++Adapter->GeneralOptionalFrameCount[GO_BROADCAST_RECEIVES];

                 LanceAddUlongToLargeInteger(
                         &Adapter->GeneralOptionalByteCount[GO_BROADCAST_RECEIVES],
                            PacketSize);

              } else if (ReceivePacketType == LANCE_MULTICAST) {

                 ++Adapter->GeneralOptionalFrameCount[GO_MULTICAST_RECEIVES];
                 LanceAddUlongToLargeInteger(
                         &Adapter->GeneralOptionalByteCount[GO_MULTICAST_RECEIVES],
                            PacketSize);

              } else {

                 ++Adapter->GeneralOptionalFrameCount[GO_DIRECTED_RECEIVES];
                 LanceAddUlongToLargeInteger(
                         &Adapter->GeneralOptionalByteCount[GO_DIRECTED_RECEIVES],
                            PacketSize);

              }

      } else {

              ++Adapter->GeneralMandatory[GM_RECEIVE_BAD];

              if (ReceiveStatus & LANCE_RECEIVE_CRC_ERROR) {

               //Bug!! Bug!!
               //GO_RECEIVE_CRC and GO_TRANSMIT_QUEUE_LENGTH have to be 
               //used along with GO_ARRAY_START, whenever they are referrenced.

                ++Adapter->GeneralOptional[GO_RECEIVE_CRC-GO_ARRAY_START];

              } else if (ReceiveStatus & LANCE_RECEIVE_FRAME_ERROR) {

                 ++Adapter->MediaMandatory[MM_RECEIVE_ERROR_ALIGNMENT];

              } else if (ReceiveStatus & LANCE_RECEIVE_BUFFER_ERROR) {

                 ++Adapter->MediaOptional[MO_RECEIVE_OVERRUN];
              }

      }

   }

}

extern
VOID
LanceLockUpDetectProcess(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )

/*++

Routine Description:

    This DPC routine is queued every 5 seconds to check on interrupts
    occurance in Lance device. This is to solve problems where
    Lance receiver locks up under heavy traffic and RXON bit in CSR0
    is still set.  But no interrupts are generated.  If this happens then
    we reset the chip.  This is not applicable to PCNET-ISA device.

Arguments:

    Context - Really a pointer to the adapter.

Return Value:

    None.

--*/
{
    ULONG TxPacketNumber;

    PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)Context;


    UNREFERENCED_PARAMETER(SystemSpecific1);
    UNREFERENCED_PARAMETER(SystemSpecific2);
    UNREFERENCED_PARAMETER(SystemSpecific3);

    NdisDprAcquireSpinLock(&Adapter->Lock);

    // This code is incomplete. Need to check lock up conditions here.
    // Normally for Tx only, but can add both Rx and Tx lock checks here.
    //

    NdisDprReleaseSpinLock(&Adapter->Lock);


    //
    // Fire off another Dpc to execute after 5 second.
    //

    NdisSetTimer(&Adapter->DetectTimer, 5000);

}

VOID
LanceTimerProcessNoLock(
    IN PVOID Context
    )

/*++

Routine Description:

    Process the operations that are deferred by LanceDeferredProcessing.

Arguments:

    Context - A pointer to the adapter.

Return Value:

    None.

--*/

{
   //
   // Local Value to cancel the Deferred Timer.
   //

   BOOLEAN Canceled;

   PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)Context;

   #if DBG
   if (LanceDbg){
       _Debug_Printf_Service("LanceTimerNoLock:Ref: %d\n", Adapter->References);
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
       _Debug_Printf_Service("\n Unwanted TReset \n");
     }

     #endif

     Adapter->ProcessingDeferredOperations = FALSE;
     Adapter->References--;
     NdisDprReleaseSpinLock(&Adapter->Lock);
     StartAdapterReset(Adapter);
     NdisDprAcquireSpinLock(&Adapter->Lock);
     return ;

   }

   while (Adapter->FirstLoopBack &&
         (!Adapter->LoopBackQueue)) {


     //
     // Process the loopback queue.
     //
     // NOTE: Incase anyone ever figures out how to make this
     // loop more reentriant, special care needs to be taken that
     // loopback packets and regular receive packets are NOT being
     // indicated at the same time.  While the filter indication
     // routines can handle this, I doubt that the transport can.
     //

     if(!LanceProcessLoopback(Adapter))
       break;

   }

   //
   // If there are any opens on the closing list and their
   // reference counts are zero then complete the close and
   // delete them from the list.
   //

   while (!IsListEmpty(&Adapter->CloseList)) {

     PLANCE_OPEN Open;

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

        NdisDprReleaseSpinLock(&Adapter->Lock);

        NdisCompleteCloseAdapter(
                OpenBindingContext,
                NDIS_STATUS_SUCCESS
                );

        NdisDprAcquireSpinLock(&Adapter->Lock);

     }

   }

   //
   // NOTE: We hold the spinlock here.
   //

   if ((Adapter->FirstLoopBack ||
       (!IsListEmpty(&Adapter->CloseList)))) {

   //                                                  
   // Fire off another call to LanceTimerProcess after 10 milli secs.
   //
      
           NdisSetTimer(&Adapter->DeferredTimer, 0);
    
   } else {
      Adapter->ProcessingDeferredOperations = FALSE ;

      Adapter->References--;
        
   }
    
   
}

