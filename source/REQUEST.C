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

    request.c

Abstract:

	This is the cose to handle NdisRequestss for the Advanced Micro Devices
   LANCE Ethernet controller.      This driver conforms to the NDIS 3.0
   interface.

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

extern const UCHAR VendorDescription[] = "AMD PCNET Family Ethernet Adapter";


NDIS_OID LanceGlobalSupportedOids[] = {
   OID_GEN_SUPPORTED_LIST,
   OID_GEN_HARDWARE_STATUS,
   OID_GEN_MEDIA_SUPPORTED,
   OID_GEN_MEDIA_IN_USE,
   OID_GEN_MAXIMUM_LOOKAHEAD,
   OID_GEN_MAXIMUM_FRAME_SIZE,
   OID_GEN_MAC_OPTIONS,
   OID_GEN_PROTOCOL_OPTIONS,
   OID_GEN_LINK_SPEED,
   OID_GEN_TRANSMIT_BUFFER_SPACE,
   OID_GEN_RECEIVE_BUFFER_SPACE,
   OID_GEN_TRANSMIT_BLOCK_SIZE,
   OID_GEN_RECEIVE_BLOCK_SIZE,
   OID_GEN_VENDOR_ID,
   OID_GEN_VENDOR_DESCRIPTION,
   OID_GEN_CURRENT_PACKET_FILTER,
   OID_GEN_CURRENT_LOOKAHEAD,
   OID_GEN_DRIVER_VERSION,
   OID_GEN_MAXIMUM_TOTAL_SIZE,

   OID_GEN_XMIT_OK,
   OID_GEN_RCV_OK,
   OID_GEN_XMIT_ERROR,
   OID_GEN_RCV_ERROR,
   OID_GEN_RCV_NO_BUFFER,

   OID_GEN_DIRECTED_BYTES_XMIT,
   OID_GEN_DIRECTED_FRAMES_XMIT,
   OID_GEN_MULTICAST_BYTES_XMIT,
   OID_GEN_MULTICAST_FRAMES_XMIT,
   OID_GEN_BROADCAST_BYTES_XMIT,
   OID_GEN_BROADCAST_FRAMES_XMIT,
   OID_GEN_DIRECTED_BYTES_RCV,
   OID_GEN_DIRECTED_FRAMES_RCV,
   OID_GEN_MULTICAST_BYTES_RCV,
   OID_GEN_MULTICAST_FRAMES_RCV,
   OID_GEN_BROADCAST_BYTES_RCV,
   OID_GEN_BROADCAST_FRAMES_RCV,

   OID_GEN_RCV_CRC_ERROR,
   OID_GEN_TRANSMIT_QUEUE_LENGTH,

   OID_802_3_PERMANENT_ADDRESS,
   OID_802_3_CURRENT_ADDRESS,
   OID_802_3_MULTICAST_LIST,
   OID_802_3_MAXIMUM_LIST_SIZE,

   OID_802_3_RCV_ERROR_ALIGNMENT,
   OID_802_3_XMIT_ONE_COLLISION,
   OID_802_3_XMIT_MORE_COLLISIONS,

   OID_802_3_XMIT_DEFERRED,
   OID_802_3_XMIT_MAX_COLLISIONS,
   OID_802_3_RCV_OVERRUN,
   OID_802_3_XMIT_UNDERRUN,
   OID_802_3_XMIT_HEARTBEAT_FAILURE,
   OID_802_3_XMIT_TIMES_CRS_LOST,
   OID_802_3_XMIT_LATE_COLLISIONS
   };

NDIS_OID LanceProtocolSupportedOids[] = {
   OID_GEN_SUPPORTED_LIST,
   OID_GEN_HARDWARE_STATUS,
   OID_GEN_MEDIA_SUPPORTED,
   OID_GEN_MEDIA_IN_USE,
   OID_GEN_MAXIMUM_LOOKAHEAD,
   OID_GEN_MAXIMUM_FRAME_SIZE,
   OID_GEN_MAC_OPTIONS,
   OID_GEN_PROTOCOL_OPTIONS,
   OID_GEN_LINK_SPEED,
   OID_GEN_TRANSMIT_BUFFER_SPACE,
   OID_GEN_RECEIVE_BUFFER_SPACE,
   OID_GEN_TRANSMIT_BLOCK_SIZE,
   OID_GEN_RECEIVE_BLOCK_SIZE,
   OID_GEN_VENDOR_ID,
   OID_GEN_VENDOR_DESCRIPTION,
   OID_GEN_CURRENT_PACKET_FILTER,
   OID_GEN_CURRENT_LOOKAHEAD,
   OID_GEN_DRIVER_VERSION,
   OID_GEN_MAXIMUM_TOTAL_SIZE,

   OID_802_3_PERMANENT_ADDRESS,
   OID_802_3_CURRENT_ADDRESS,
   OID_802_3_MULTICAST_LIST,
   OID_802_3_MAXIMUM_LIST_SIZE
    };


STATIC
UINT
CalculateCRC(
    IN UINT NumberOfBytes,
    IN PCHAR Input
    );


STATIC
NDIS_STATUS
LanceQueryInformation(
    IN PLANCE_ADAPTER Adapter,
    IN PLANCE_OPEN Open,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN INT InformationBufferLength,
    IN PUINT BytesWritten,
    IN PUINT BytesNeeded,
    IN BOOLEAN Global
    );

STATIC
NDIS_STATUS
LanceSetInformation(
   IN PLANCE_ADAPTER Adapter,
   IN PLANCE_OPEN Open,
   IN NDIS_OID Oid,
   IN PVOID InformationBuffer,
   IN INT InformationBufferLength,
   IN PUINT BytesRead,
   IN PUINT BytesNeeded
    );

STATIC
NDIS_STATUS
ChangeClassDispatch(
   IN PLANCE_ADAPTER Adapter,
   IN UINT OldFilterClasses,
   IN UINT NewFilterClasses
   );

STATIC
VOID
ChangeAddressDispatch(
   IN PLANCE_ADAPTER Adapter,
   IN UINT AddressCount,
   IN CHAR Addresses[][ETH_LENGTH_OF_ADDRESS]
   );



NDIS_STATUS
LanceRequest(
   IN NDIS_HANDLE MacBindingHandle,
   IN PNDIS_REQUEST NdisRequest
   )

/*++

Routine Description:

   The LanceRequest function handles general requests from the
   protocol. Currently these include SetInformation and
   QueryInformation, more may be added in the future.

Arguments:

   MacBindingHandle - The context value returned by the MAC  when the
   adapter was opened.  In reality, it is a pointer to LANCE_OPEN.

   NdisRequest - A structure describing the request. In the case
   of asynchronous completion, this pointer will be used to
   identify the request that is completing.

Return Value:

   The function value is the status of the operation.


--*/

{
   //
   // This holds the status we will return.
   //
   NDIS_STATUS StatusOfRequest;

   //
   // Points to the adapter that this request is coming through.
   //
   PLANCE_ADAPTER Adapter;

   //
   // Points to the MacReserved section of the request.
   //
   PLANCE_REQUEST_RESERVED Reserved = PLANCE_RESERVED_FROM_REQUEST(NdisRequest);


   Adapter = PLANCE_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);

   NdisAcquireSpinLock(&Adapter->Lock);
   Adapter->References++;

   if (!Adapter->ResetInProgress) {

      PLANCE_OPEN Open;

      Open = PLANCE_OPEN_FROM_BINDING_HANDLE(MacBindingHandle);

      if (!Open->BindingShuttingDown) {

	 switch (NdisRequest->RequestType) {

	    case NdisRequestSetInformation:
	       //
	       // This is a valid request, queue it.
	       //

	       Open->References++;

	       Reserved->OpenBlock = Open;
	       Reserved->Next = (PNDIS_REQUEST)NULL;

	       LanceQueueRequest(Adapter, NdisRequest);

	       StatusOfRequest = NDIS_STATUS_PENDING;
	       break;

	    case NdisRequestQueryInformation:

	       //
	       // This is a valid request, queue it.
	       //

	       Open->References++;

	       Reserved->OpenBlock = Open;
	       Reserved->Next = (PNDIS_REQUEST)NULL;
               //Duke's Change
               //LanceQueueRequest(Adapter, NdisRequest);
#if DBG
               if (LanceDbg)
                  _Debug_Printf_Service("LanceProcessRequest: QueryInfo request\n");
#endif
               StatusOfRequest = LanceQueryInformation(
   	        		     Adapter,
		    		     Reserved->OpenBlock,
		     		     NdisRequest->DATA.QUERY_INFORMATION.Oid,
		     		     NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
		     		     NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
		     		     &(NdisRequest->DATA.QUERY_INFORMATION.BytesWritten),
		     		     &(NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded),
		     		     FALSE);

               //Duke's Change
	       //StatusOfRequest = NDIS_STATUS_PENDING;   //original
	       --Open->References;
	       //StatusOfRequest = NDIS_STATUS_SUCCESS;     //change
	       break;

	    default:

	       //
	       // Unknown request
	       //

	       StatusOfRequest = NDIS_STATUS_NOT_SUPPORTED;
	    break;

	 }

      } else {

	 StatusOfRequest = NDIS_STATUS_CLOSING;

      }

              } else {

      StatusOfRequest = NDIS_STATUS_RESET_IN_PROGRESS;

   }


#if DBG
	    if (LanceDbg)
	       _Debug_Printf_Service("LanceRequest: DO_SEND called\n");
#endif

   //
   // This macro assumes it is called with the lock held,
   // and releases it.
   //

   LANCE_DO_SEND(Adapter);
   return StatusOfRequest;
}

VOID
LanceQueueRequest(
   IN PLANCE_ADAPTER Adapter,
   IN PNDIS_REQUEST NdisRequest
   )

/*++

Routine Description:

   LanceQueueRequest takes an NDIS_REQUEST and ensures that it
   gets processed and completed. It processes the
   request immediately if nothing else is in progress, otherwise
   it queues it for later processing.

   THIS ROUTINE IS CALLED WITH THE LOCK HELD.

Arguments:

   Adapter - The adapter that the request is for.

   NdisRequest - The NDIS_REQUEST structure describing the request.
   The LanceReserved section is partially filled in, except
   for the queueing and current offset fields.

Return Value:

   None.

--*/

{

   PNDIS_REQUEST Request;

   //
   // Queue the request.
   //

   if (Adapter->FirstRequest != (PNDIS_REQUEST)NULL){

     //
     // Something else on the queue, just queue it.
     //

     PLANCE_RESERVED_FROM_REQUEST(Adapter->LastRequest)->Next = NdisRequest;
     Adapter->LastRequest = NdisRequest;

   } 
/*
   else if (((Adapter->ProcessingLoopBacks) ||
               (Adapter->ProcessingTransmits) ||
               (Adapter->ResetInProgress) ||
               (Adapter->ProcessingReceiveInterrupt)) && 
              (NdisRequest-RequestType == NdisRequestClose)){

     // Just Queue it.
     PLANCE_RESERVED_FROM_REQUEST(Adapter->LastRequest)->Next = NdisRequest;
     Adapter->LastRequest = NdisRequest;

   }
*/
   else {

      //
      // The queue if empty; if nothing is in progress, if we
      // are not resetting, then process this request; if
      // we are resetting, then after the reset the queue
      // will be restarted.
      //

      Adapter->FirstRequest = NdisRequest;
      Adapter->LastRequest = NdisRequest;

      if (!Adapter->RequestInProgress) {

	     LanceProcessRequestQueue(Adapter);

      }

   }
}

VOID
LanceProcessRequestQueue(
   IN PLANCE_ADAPTER Adapter
   )

/*++

Routine Description:

   LanceProcessRequestQueue takes the requests on the queue
   and processes them as much as possible. It will complete
   any requests that it fully processes. It will stop when
   the queue is empty or it finds a request that has to pend.

   THIS ROUTINE IS CALLED WITH THE LOCK HELD.

Arguments:

   Adapter - The adapter that the request is for.

Return Value:

   NDIS_STATUS_PENDING (probably should be VOID...)


--*/
{
   PNDIS_REQUEST Request;
   PLANCE_REQUEST_RESERVED Reserved;
   NDIS_STATUS Status;
   PLANCE_OPEN Open;


   Request = Adapter->FirstRequest;

   for (;;) {

      //
      // Loop until we exit, which happens when a
      // request pends, or we empty the queue.
      //

      if (Request == (PNDIS_REQUEST)NULL) {
	 Adapter->RequestInProgress = FALSE;
	 break;
      }

      if (Adapter->ResetInProgress) {
	 Adapter->RequestInProgress = FALSE;

         //Duke's addition
         Adapter->FirstRequest = (PNDIS_REQUEST)NULL;
         //

	 break;
      }

      Adapter->RequestInProgress = TRUE;

      Reserved = PLANCE_RESERVED_FROM_REQUEST(Request);

      switch (Request->RequestType) {

	 case NdisRequestClose:

#if DBG
	    if (LanceDbg)
	       _Debug_Printf_Service("LanceProcessRequest: Close request\n");
#endif
	    Open = Reserved->OpenBlock;

	    Status = EthDeleteFilterOpenAdapter(
	       Adapter->FilterDB,
	       Open->NdisFilterHandle,
	       NULL
	       );


	    //
	    // If the status is successful that merely implies that
	    // we were able to delete the reference to the open binding
	    // from the filtering code.
	    //
	    // The delete filter routine can return a "special" status
	    // that indicates that there is a current NdisIndicateReceive
	    // on this binding.  See below.
	    //

	    if (Status == NDIS_STATUS_SUCCESS) {

	       //
	       // Account for the filter's reference to this open.
	       //

	       Open->References--;

	    } else if (Status == NDIS_STATUS_PENDING) {

	       //
	       // Theoretically, this will never happen as
	       // LanceChangeClass and LanceChangeAddresses routines
	       // never return this status.
	       //

	    } else if (Status == NDIS_STATUS_CLOSING_INDICATING) {

	       //
	       // When we have this status it indicates that the filtering
	       // code was currently doing an NdisIndicateReceive. Our
	       // close action routine will get called when the filter
	       // is done with us, we remove the reference there.
	       //

	       Status = NDIS_STATUS_PENDING;

	    } else {

	       ASSERT(0);

	    }
		
	 //
	 // This flag prevents further requests on this binding.
	 //

	 Open->BindingShuttingDown = TRUE;

	 //
	 // Remove the reference kept for the fact that we
	 // had something queued.
	 //

	 Open->References--;

	 //
	 // Remove the open from the open list and put it on
	 // the closing list. This list is checked after every
	 // request, and when the reference count goes to zero
	 // the close is completed.
	 //

	 RemoveEntryList(&Open->OpenList);
	 InsertTailList(&Adapter->CloseList,&Open->OpenList);
	
	 break;

      case NdisRequestOpen:

#if DBG
	 if (LanceDbg)
	    _Debug_Printf_Service("LanceProcessRequest: Open request\n");
#endif

	 Open = Reserved->OpenBlock;

	 if (!EthNoteFilterOpenAdapter(
	       Open->OwningLance->FilterDB,
	       Open,
	       Open->NdisBindingContext,
	       &Open->NdisFilterHandle
	       )) {

	    NdisReleaseSpinLock(&Adapter->Lock);

	    NdisCompleteOpenAdapter(
	       Open->NdisBindingContext,
	       NDIS_STATUS_FAILURE,
	       0);

	   #ifdef NDIS_NT

	    NdisWriteErrorLogEntry(
	       Adapter->NdisAdapterHandle,
	       NDIS_ERROR_CODE_OUT_OF_RESOURCES,
	       2,
	       openAdapter,
	       LANCE_ERRMSG_OPEN_DB
	       );

	    #endif

	    LANCE_FREE_MEMORY(Open, sizeof(LANCE_OPEN));

	    NdisAcquireSpinLock(&Adapter->Lock);

	 } else {

	 //
	 // Everything has been filled in.  Synchronize access to the
	 // adapter block and link the new open adapter in and increment
	 // the opens reference count to account for the fact that the
	 // filter routines have a "reference" to the open.
	 //

	    InsertTailList(&Adapter->OpenBindings,&Open->OpenList);
	    Adapter->OpenCount++;
	    Open->References++;

	    NdisReleaseSpinLock(&Adapter->Lock);

	    NdisCompleteOpenAdapter(
	       Open->NdisBindingContext,
	       NDIS_STATUS_SUCCESS,
	       0);

	    NdisAcquireSpinLock(&Adapter->Lock);

	 }

	 //
	 // Set this, since we want to continue processing
	 // the queue.
	 //

	 Status = NDIS_STATUS_SUCCESS;

	 break;

      case NdisRequestQueryInformation:

#if DBG
	 if (LanceDbg)
	    _Debug_Printf_Service("LanceProcessRequest: QueryInfo request\n");
#endif
	 Status = LanceQueryInformation(
		     Adapter,
		     Reserved->OpenBlock,
		     Request->DATA.QUERY_INFORMATION.Oid,
		     Request->DATA.QUERY_INFORMATION.InformationBuffer,
		     Request->DATA.QUERY_INFORMATION.InformationBufferLength,
		     &(Request->DATA.QUERY_INFORMATION.BytesWritten),
		     &(Request->DATA.QUERY_INFORMATION.BytesNeeded),
		     FALSE);

	 break;

      case NdisRequestQueryStatistics:

#if DBG
	 if (LanceDbg)
	    _Debug_Printf_Service("LanceProcessRequest: QueryStat request\n");
#endif
	 Status = LanceQueryInformation(
		     Adapter,
		     Reserved->OpenBlock,
		     Request->DATA.QUERY_INFORMATION.Oid,
		     Request->DATA.QUERY_INFORMATION.InformationBuffer,
		     Request->DATA.QUERY_INFORMATION.InformationBufferLength,
		     &(Request->DATA.QUERY_INFORMATION.BytesWritten),
		     &(Request->DATA.QUERY_INFORMATION.BytesNeeded),
		     TRUE);

	 break;

      case NdisRequestSetInformation:

#if DBG
	 if (LanceDbg)
	    _Debug_Printf_Service("LanceProcessRequest: SetInfo request\n");
#endif
	 Status = LanceSetInformation(
		     Adapter,
		     Reserved->OpenBlock,
		     Request->DATA.SET_INFORMATION.Oid,
		     Request->DATA.SET_INFORMATION.InformationBuffer,
		     Request->DATA.SET_INFORMATION.InformationBufferLength,
		     &(Request->DATA.SET_INFORMATION.BytesRead),
		     &(Request->DATA.SET_INFORMATION.BytesNeeded));

	 break;

   }


   //
   // If the operation pended, then stop processing the queue.
   //

   if (Status == NDIS_STATUS_PENDING) {

       return;

   }


   //
   // If we fall through here, we are done with this request.
   //

   Adapter->FirstRequest = Reserved->Next;

   if ((Request->RequestType == NdisRequestQueryInformation) ||
       (Request->RequestType == NdisRequestSetInformation)) {

       Open = Reserved->OpenBlock;

       NdisReleaseSpinLock(&Adapter->Lock);

       NdisCompleteRequest(
      Open->NdisBindingContext,
      Request,
      Status);

       NdisAcquireSpinLock(&Adapter->Lock);

       --Open->References;

   } else if (Request->RequestType == NdisRequestQueryStatistics) {

       NdisReleaseSpinLock(&Adapter->Lock);

       NdisCompleteQueryStatistics(
      Adapter->NdisAdapterHandle,
      Request,
      Status);

       NdisAcquireSpinLock(&Adapter->Lock);

       --Adapter->References;

   }

   Request = Adapter->FirstRequest;

   //
   // Now loop and continue on with the next request.
   //

    }

}

NDIS_STATUS
LanceQueryGlobalStatistics(
    IN NDIS_HANDLE MacAdapterContext,
    IN PNDIS_REQUEST NdisRequest
    )

/*++

Routine Description:

   LanceQueryGlobalStatistics handles a per-adapter query
   for statistics. It is similar to LanceQueryInformation,
    which is per-binding.

Arguments:

    MacAdapterContext - The context value that the MAC passed
   to NdisRegisterAdapter; actually as pointer to a
      LANCE_ADAPTER.

    NdisRequest - Describes the query request.

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING

--*/

{
    //
    // This holds the status we will return.                    
    //

    NDIS_STATUS StatusOfRequest;

    //
    // Points to the adapter that this request is coming through.
    //
   PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)MacAdapterContext;

   PLANCE_REQUEST_RESERVED Reserved = PLANCE_RESERVED_FROM_REQUEST(NdisRequest);

    NdisAcquireSpinLock(&Adapter->Lock);
    Adapter->References++;

    if (!Adapter->ResetInProgress) {

   switch (NdisRequest->RequestType) {

   case NdisRequestQueryStatistics:

       //
       // Valid request.
       //

	 Reserved->OpenBlock = (PLANCE_OPEN)NULL;
       Reserved->Next = (PNDIS_REQUEST)NULL;

       Adapter->References++;

	 LanceQueueRequest (Adapter, NdisRequest);

       StatusOfRequest = NDIS_STATUS_PENDING;
       break;

   default:

       //
       // Unknown request
       //

       StatusOfRequest = NDIS_STATUS_NOT_SUPPORTED;
       break;

   }

    } else {

   StatusOfRequest = NDIS_STATUS_RESET_IN_PROGRESS;

    }

#if DBG
	    if (LanceDbg)
	       _Debug_Printf_Service("LanceQueryGlobalStats: DO_SEND called\n");
#endif

    //
    // This macro assumes it is called with the lock held,
    // and releases it.
    //

    LANCE_DO_DEFERRED(Adapter);
    return StatusOfRequest;
}

STATIC
NDIS_STATUS
LanceQueryInformation(
   IN PLANCE_ADAPTER Adapter,
   IN PLANCE_OPEN Open,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN INT InformationBufferLength,
    IN PUINT BytesWritten,
    IN PUINT BytesNeeded,
    IN BOOLEAN Global
    )

/*++

Routine Description:

   LanceQueryInformation handles a query operation for a
    single OID.

    THIS ROUTINE IS CALLED WITH THE LOCK HELD.

Arguments:

    Adapter - The adapter that the query is for.

    Open - The binding that the query is for.

    Oid - The OID of the query.

    InformationBuffer - Holds the result of the query.

    InformationBufferLength - The length of InformationBuffer.

    BytesWritten - If the call is successful, returns the number
   of bytes written to InformationBuffer.

    BytesNeeded - If there is not enough room in InformationBuffer
   to satisfy the OID, returns the amount of storage needed.

    Global - TRUE if this is for a QueryGlobalInformation, FALSE for
   a protocol QueryInformation.

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    NDIS_STATUS_INVALID_LENGTH
    NDIS_STATUS_INVALID_OID

--*/

{

    INT i;
    PNDIS_OID SupportedOidArray;
    INT SupportedOids;
    PVOID SourceBuffer;
    ULONG SourceBufferLength;
    ULONG GenericUlong;
    USHORT GenericUshort;
    UINT MulticastAddresses;
    NDIS_STATUS Status;
   UCHAR VendorId[4];


    //
    // Check that the OID is valid.
    //

    if (Global) {

      SupportedOidArray = (PNDIS_OID)LanceGlobalSupportedOids;
   SupportedOids = sizeof(LanceGlobalSupportedOids)/sizeof(ULONG);

   } else {

      SupportedOidArray = (PNDIS_OID)LanceProtocolSupportedOids;
   SupportedOids = sizeof(LanceProtocolSupportedOids)/sizeof(ULONG);

   }

    for (i=0; i<SupportedOids; i++) {
   if (Oid == SupportedOidArray[i]) {
       break;
   }
    }

    if (i == SupportedOids) {
   *BytesWritten = 0;
   return NDIS_STATUS_NOT_SUPPORTED;
    }

    //
    // Initialize these once, since this is the majority
    // of cases.
    //

    SourceBuffer = &GenericUlong;
    SourceBufferLength = sizeof(ULONG);

    switch (Oid & OID_TYPE_MASK) {

      case OID_TYPE_GENERAL_OPERATIONAL:

	 switch (Oid) {

	       case OID_GEN_MAC_OPTIONS:

#ifdef NDIS_WIN
	       GenericUlong = (ULONG)(NDIS_MAC_OPTION_TRANSFERS_NOT_PEND   |
		  NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA  |
		  NDIS_MAC_OPTION_RECEIVE_SERIALIZED |
		  NDIS_MAC_OPTION_NO_LOOPBACK
		  );
#else
	       GenericUlong = (ULONG)(NDIS_MAC_OPTION_TRANSFERS_NOT_PEND   |
		  NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA  |
		  NDIS_MAC_OPTION_RECEIVE_SERIALIZED
		  );

#endif

	       break;

	    case OID_GEN_SUPPORTED_LIST:

	       SourceBuffer =  SupportedOidArray;
	       SourceBufferLength = SupportedOids * sizeof(ULONG);
	       break;

	    case OID_GEN_HARDWARE_STATUS:

	       GenericUlong = NdisHardwareStatusReady;
	       break;

	    case OID_GEN_MEDIA_SUPPORTED:
	    case OID_GEN_MEDIA_IN_USE:

	       GenericUlong = NdisMedium802_3;
	       if (Global) {
		  if (Adapter->OpenCount == 0) {
		     SourceBufferLength = 0;
		  }
	       }
	       break;

	    case OID_GEN_MAXIMUM_LOOKAHEAD:

	       GenericUlong = LANCE_INDICATE_MAXIMUM-MAC_HEADER_SIZE ;
	       break;

	    case OID_GEN_MAXIMUM_FRAME_SIZE:

	       GenericUlong = 1500;
	       break;

	    case OID_GEN_MAXIMUM_TOTAL_SIZE:

	       GenericUlong = 1514;
	       break;

	    case OID_GEN_LINK_SPEED:

	       GenericUlong = 100000;    // 10 Mbps in 100 bps units
	       break;

	    case OID_GEN_TRANSMIT_BUFFER_SPACE:

	       GenericUlong = TRANSMIT_BUFFER_SIZE * TRANSMIT_BUFFERS;
	       break;

	    case OID_GEN_RECEIVE_BUFFER_SPACE:

	       GenericUlong = RECEIVE_BUFFER_SIZE * RECEIVE_BUFFERS;
	       break;

	    case OID_GEN_TRANSMIT_BLOCK_SIZE:

	       GenericUlong = TRANSMIT_BUFFER_SIZE;
	       break;

	    case OID_GEN_RECEIVE_BLOCK_SIZE:

	       GenericUlong = RECEIVE_BUFFER_SIZE;
	       break;

	    case OID_GEN_VENDOR_ID:

	       LANCE_MOVE_MEMORY(VendorId, Adapter->PermanentNetworkAddress, 3);
	       VendorId[3] = 0x0;
	       SourceBuffer = VendorId;
	       SourceBufferLength = sizeof(VendorId);
	       break;

	    case OID_GEN_VENDOR_DESCRIPTION:

	       SourceBuffer = (PVOID)VendorDescription;
	       SourceBufferLength = sizeof(VendorDescription);
	       break;

	    case OID_GEN_DRIVER_VERSION:

	       GenericUshort = (LANCE_NDIS_MAJOR_VERSION << 8) + LANCE_NDIS_MINOR_VERSION;
	       SourceBuffer = &GenericUshort;
	       SourceBufferLength = sizeof(USHORT);
	       break;

	    case OID_GEN_CURRENT_PACKET_FILTER:

	       if (Global) {
		  GenericUlong = Adapter->CurrentPacketFilter;
	       } else {
		  GenericUlong = ETH_QUERY_PACKET_FILTER (Adapter->FilterDB, Open->NdisFilterHandle);
	       }
	       break;

	    case OID_GEN_CURRENT_LOOKAHEAD:

	       GenericUlong = LANCE_INDICATE_MAXIMUM-MAC_HEADER_SIZE ;
	       break;

	    default:

	       ASSERT(FALSE);
	       break;

	 }

	 break;

      case OID_TYPE_GENERAL_STATISTICS:

	 if (Global) {

	    NDIS_OID MaskOid = (Oid & OID_INDEX_MASK) - 1;

	    switch (Oid & OID_REQUIRED_MASK) {

	       case OID_REQUIRED_MANDATORY:

		  ASSERT (MaskOid < GM_ARRAY_SIZE);

		  GenericUlong = Adapter->GeneralMandatory[MaskOid];
		  break;

	       case OID_REQUIRED_OPTIONAL:

		  ASSERT (MaskOid < GO_ARRAY_SIZE);

		  if ((MaskOid / 2) < GO_COUNT_ARRAY_SIZE) {

		     if (MaskOid & 0x01) {
			// Frame count
			GenericUlong = Adapter->GeneralOptionalFrameCount[MaskOid / 2];
		     } else {
			// Byte count
			SourceBuffer = &Adapter->GeneralOptionalByteCount[MaskOid / 2];
			SourceBufferLength = sizeof(LARGE_INTEGER);
		     }

		  } else {

		     GenericUlong = Adapter->GeneralOptional[MaskOid - GO_ARRAY_START];

		  }

		  break;

	       default:

		  ASSERT(FALSE);
		  break;

	    }

	 } else {

	    //
	    // We don't support the optional per-open stats yet.
	    //
	    ASSERT(FALSE);

	 }

	 break;

      case OID_TYPE_802_3_OPERATIONAL:

	 switch (Oid) {

	    case OID_802_3_PERMANENT_ADDRESS:

	       SourceBuffer = Adapter->PermanentNetworkAddress;
	       SourceBufferLength = 6;
	       break;

	    case OID_802_3_CURRENT_ADDRESS:

	       SourceBuffer = Adapter->CurrentNetworkAddress;
	       SourceBufferLength = 6;
	       break;

	    case OID_802_3_MULTICAST_LIST:

	       if (Global) {

		  EthQueryGlobalFilterAddresses(
		     &Status,
		     Adapter->FilterDB,
		     InformationBufferLength,
		     &MulticastAddresses,
		     (PVOID)InformationBuffer);

		  SourceBuffer = (PVOID)InformationBuffer;
		  SourceBufferLength = MulticastAddresses * ETH_LENGTH_OF_ADDRESS;

	       } else {

		  EthQueryOpenFilterAddresses(
		     &Status,
		     Adapter->FilterDB,
		     Open->NdisFilterHandle,
		     InformationBufferLength,
		     &MulticastAddresses,
		     (PVOID)InformationBuffer);

		  if (Status == NDIS_STATUS_SUCCESS) {
		     SourceBuffer = (PVOID)InformationBuffer;
		     SourceBufferLength = MulticastAddresses * ETH_LENGTH_OF_ADDRESS;
		  } else {
		     SourceBuffer = (PVOID)InformationBuffer;
		     SourceBufferLength = ETH_LENGTH_OF_ADDRESS *
		     EthNumberOfOpenFilterAddresses(
		     Adapter->FilterDB,
		     Open->NdisFilterHandle);
		  }

	       }

	       break;

	    case OID_802_3_MAXIMUM_LIST_SIZE:

	       GenericUlong = LANCE_MAX_MULTICAST;
	       break;

	    default:

	       ASSERT(FALSE);
	       break;

	 }

	 break;

      case OID_TYPE_802_3_STATISTICS:

	 if (Global) {

	    NDIS_OID MaskOid = (Oid & OID_INDEX_MASK) - 1;

	    switch (Oid & OID_REQUIRED_MASK) {

	       case OID_REQUIRED_MANDATORY:

		  ASSERT (MaskOid < MM_ARRAY_SIZE);

		  GenericUlong = Adapter->MediaMandatory[MaskOid];
		  break;

	       case OID_REQUIRED_OPTIONAL:

		  ASSERT (MaskOid < MO_ARRAY_SIZE);

		  GenericUlong = Adapter->MediaOptional[MaskOid];
		  break;

	       default:

		  ASSERT(FALSE);
		  break;

	    }

	 } else {

	    //
	    // We don't support the optional per-open stats yet.
	    //

	    ASSERT(FALSE);

	 }

	 break;
   }

   if (SourceBufferLength > (UINT)InformationBufferLength) {
   *BytesNeeded = SourceBufferLength;
   return NDIS_STATUS_BUFFER_TOO_SHORT;
    }

   LANCE_MOVE_MEMORY (InformationBuffer, SourceBuffer, SourceBufferLength);
    *BytesWritten = SourceBufferLength;

    return NDIS_STATUS_SUCCESS;

}


STATIC
NDIS_STATUS
LanceSetInformation(
   IN PLANCE_ADAPTER Adapter,
   IN PLANCE_OPEN Open,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN INT InformationBufferLength,
    IN PUINT BytesRead,
    IN PUINT BytesNeeded
    )

/*++

Routine Description:

   LanceQueryInformation handles a set operation for a
    single OID.

    THIS ROUTINE IS CALLED WITH THE LOCK HELD.

Arguments:

    Adapter - The adapter that the set is for.

    Open - The binding that the set is for.

    Oid - The OID of the set.

    InformationBuffer - Holds the data to be set.

    InformationBufferLength - The length of InformationBuffer.

    BytesRead - If the call is successful, returns the number
   of bytes read from OvbBuffer.

    BytesNeeded - If there is not enough data in OvbBuffer
   to satisfy the OID, returns the amount of storage needed.

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    NDIS_STATUS_INVALID_LENGTH
    NDIS_STATUS_INVALID_OID

--*/

{

    NDIS_STATUS Status;
    ULONG PacketFilter;

    //
    // Now check for the most common OIDs
    //

    switch (Oid) {

      case OID_802_3_MULTICAST_LIST:

	 if (InformationBufferLength % ETH_LENGTH_OF_ADDRESS != 0) {

	 //
	 // The data must be a multiple of the Ethernet
	 // address size.
	 //

	    return NDIS_STATUS_INVALID_DATA;

	 }

	 //
	 // Now call the filter package to set up the addresses.
	 //

	 Status = EthChangeFilterAddresses(
		     Adapter->FilterDB,
		     Open->NdisFilterHandle,
		     (PNDIS_REQUEST)NULL,
		     InformationBufferLength / ETH_LENGTH_OF_ADDRESS,
		     InformationBuffer,
		     TRUE
		     );

	 *BytesRead = InformationBufferLength;

	 return Status;

	 break;

      case OID_GEN_CURRENT_PACKET_FILTER:

	 if (InformationBufferLength != 4) {

	    return NDIS_STATUS_INVALID_DATA;

	 }

	 //
	 // Now call the filter package to set the packet filter.
	 //

	 LANCE_MOVE_MEMORY ((PVOID)&PacketFilter, InformationBuffer, sizeof(ULONG));

	 //
	 // Verify bits
	 //

	 if (PacketFilter & (NDIS_PACKET_TYPE_SOURCE_ROUTING |
			    NDIS_PACKET_TYPE_SMT |
			    NDIS_PACKET_TYPE_MAC_FRAME |
			    NDIS_PACKET_TYPE_FUNCTIONAL |
			    NDIS_PACKET_TYPE_ALL_FUNCTIONAL |
			    NDIS_PACKET_TYPE_GROUP
			   )) {

	    *BytesRead = 4;
	    *BytesNeeded = 0;

	    return NDIS_STATUS_NOT_SUPPORTED;

	 }
	
	 Status = EthFilterAdjust(
			Adapter->FilterDB,
			Open->NdisFilterHandle,
			(PNDIS_REQUEST)NULL,
			PacketFilter,
			TRUE
			);

	 *BytesRead = InformationBufferLength;

	 return Status;

	 break;

      case OID_GEN_CURRENT_LOOKAHEAD:

    // If the requested LookAhead Buffer is greater than the max allowed
    // by the adapter then fail the request.

	 if (*(PULONG)InformationBuffer > (LANCE_INDICATE_MAXIMUM-MAC_HEADER_SIZE ))
	   return NDIS_STATUS_INVALID_DATA;

	 *BytesRead = 4;
	 return NDIS_STATUS_SUCCESS;
	 break;

      case OID_GEN_PROTOCOL_OPTIONS:
	if (InformationBufferLength != 4) {
	
	    *BytesNeeded = 4;
	    return NDIS_STATUS_INVALID_LENGTH; 
	    break;
	}

	LANCE_MOVE_MEMORY(&Open->ProtOptionFlags, InformationBuffer, 4);

	*BytesRead = 4;
	return NDIS_STATUS_SUCCESS;
	break;

      default:

	 return NDIS_STATUS_INVALID_OID;
	 break;

    }

}

NDIS_STATUS
LanceChangeClass(
    IN UINT OldFilterClasses,
    IN UINT NewFilterClasses,
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_REQUEST NdisRequest,
    IN BOOLEAN Set
    )

/*++

Routine Description:

    Action routine that will get called when a particular filter
    class is first used or last cleared.

    NOTE: This routine assumes that it is called with the lock
    acquired.

Arguments:

    OldFilterClasses - The values of the class filter before it
    was changed.

    NewFilterClasses - The current value of the class filter

    MacBindingHandle - The context value returned by the MAC  when the
   adapter was opened.  In reality, it is a pointer to LANCE_OPEN.

    RequestHandle - A value supplied by the NDIS interface that the MAC
    must use when completing this request with the NdisCompleteRequest
    service, if the MAC completes this request asynchronously.

    Set - If true the change resulted from a set, otherwise the
    change resulted from a open closing.

Return Value:

    None.


--*/

{


   PLANCE_ADAPTER Adapter = PLANCE_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);

   //
   // Holds the change that should be returned to the filtering package.
   //
   NDIS_STATUS StatusOfChange;

   if (Adapter->ResetInProgress ||
       Adapter->ProcessingReceiveInterrupt ||
       Adapter->ProcessingTransmits) {

     StatusOfChange = NDIS_STATUS_RESET_IN_PROGRESS;

    } else {

   //
   // The whole purpose of this routine is to determine whether
   // the filtering changes need to result in the hardware being
   // reset.
   //

   ASSERT(OldFilterClasses != NewFilterClasses);

      StatusOfChange = ChangeClassDispatch(Adapter,
	       OldFilterClasses,
	       NewFilterClasses
	       );

   }

    return StatusOfChange;

}

STATIC
NDIS_STATUS
ChangeClassDispatch(
   IN PLANCE_ADAPTER Adapter,
    IN UINT OldFilterClasses,
   IN UINT NewFilterClasses
   )

/*++

Routine Description:

   Modifies the mode register and logical address filter of Lance.

Arguments:

    Adapter - The adapter.

   OldFilterClasses - The values of the class filter before it
    was changed.

    NewFilterClasses - The current value of the class filter

Return Value:

   None.
--*/

{
   //
   // Local Pointer to the Initialization Block.
   //
   PLANCE_INIT_BLOCK InitializationBlock;
   PLANCE_INIT_BLOCK_HI InitializationBlockHi;

   //
   // Status to return.
   //
   NDIS_STATUS     StatusToReturn = NDIS_STATUS_SUCCESS;

   InitializationBlockHi = (PLANCE_INIT_BLOCK_HI)Adapter->InitializationBlock;

   if (NewFilterClasses & NDIS_PACKET_TYPE_PROMISCUOUS) {

     #if DBG
     if (LanceDbg)
       _Debug_Printf_Service("ChangeClass: Go promiscious.\n");
     #endif

     InitializationBlockHi->Mode = LANCE_PROMISCIOUS_MODE;
  
     } else {

     USHORT i;

     InitializationBlockHi->Mode = LANCE_NORMAL_MODE;
  
     if (NewFilterClasses & NDIS_PACKET_TYPE_ALL_MULTICAST) {

       #if DBG
       if (LanceDbg)
       _Debug_Printf_Service("ChangeClass: Receive all multicast.\n");
       #endif

       for (i=0; i<8; i++)
	      InitializationBlockHi->LogicalAddressFilter[i] = 0xFF;

     } else if (NewFilterClasses & NDIS_PACKET_TYPE_MULTICAST) {

	    NDIS_STATUS Status;
	    UINT AddressCount;
	    CHAR AddressBuffer[LANCE_MAX_MULTICAST][ETH_LENGTH_OF_ADDRESS];

       #if DBG
       if (LanceDbg)
       _Debug_Printf_Service("ChangeClass: Receive selected multicast.\n");
       #endif

	    EthQueryGlobalFilterAddresses(
	     &Status,
	     Adapter->FilterDB,
	     LANCE_MAX_MULTICAST * ETH_LENGTH_OF_ADDRESS,
	     &AddressCount,
	     AddressBuffer);
	    
	    ChangeAddressDispatch(
	     Adapter,
	     AddressCount,
	     AddressBuffer);
	    
     }

   }

   Adapter->ResetInProgress = TRUE;

   //
   // Initialize the lance.
   //

   LanceInit(Adapter);

   Adapter->ResetInProgress = FALSE;

   Adapter->CurrentPacketFilter = NewFilterClasses;

   #if DBG
   if (LanceDbg)
    _Debug_Printf_Service("ChangeClass: CurrentPacketFilter = %x\n",
	 Adapter->CurrentPacketFilter);
   #endif

   return StatusToReturn;
}

NDIS_STATUS
LanceChangeAddresses(
    IN UINT OldAddressCount,
    IN CHAR OldAddresses[][ETH_LENGTH_OF_ADDRESS],
    IN UINT NewAddressCount,
    IN CHAR NewAddresses[][ETH_LENGTH_OF_ADDRESS],
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_REQUEST NdisRequest,
    IN BOOLEAN Set
    )

/*++

Routine Description:

    Action routine that will get called when the multicast address
    list has changed.

    NOTE: This routine assumes that it is called with the lock
    acquired.

Arguments:

    OldAddressCount - The number of addresses in OldAddresses.

    OldAddresses - The old multicast address list.

    NewAddressCount - The number of addresses in NewAddresses.

    NewAddresses - The new multicast address list.

    MacBindingHandle - The context value returned by the MAC  when the
    adapter was opened.  In reality, it is a pointer to SONIC_OPEN.

    RequestHandle - A value supplied by the NDIS interface that the MAC
    must use when completing this request with the NdisCompleteRequest
    service, if the MAC completes this request asynchronously.

    Set - If true the change resulted from a set, otherwise the
    change resulted from a open closing.

Return Value:

    None.


--*/

{

   PLANCE_ADAPTER Adapter = PLANCE_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);

   //
   // Holds the change that should be returned to the filtering package.
   //
   NDIS_STATUS StatusOfChange;

   ULONG GenericUlong;

   PLANCE_OPEN Open;

   Open = PLANCE_OPEN_FROM_BINDING_HANDLE(MacBindingHandle);

   if (Adapter->ResetInProgress ||
       Adapter->ProcessingReceiveInterrupt ||
       Adapter->ProcessingTransmits) {

   StatusOfChange = NDIS_STATUS_RESET_IN_PROGRESS;

    } else if (NewAddressCount > LANCE_MAX_MULTICAST) {

   StatusOfChange = NDIS_STATUS_MULTICAST_FULL;

    } else {

   GenericUlong = ETH_QUERY_PACKET_FILTER (Adapter->FilterDB, 
					   Open->NdisFilterHandle);

   if (GenericUlong & NDIS_PACKET_TYPE_ALL_MULTICAST)
     return NDIS_STATUS_SUCCESS;

   ChangeAddressDispatch(Adapter,
      NewAddressCount,
      NewAddresses
      );

   Adapter->ResetInProgress = TRUE;

   //
   // Initialize the lance.
   //

   LanceInit(Adapter);

   Adapter->ResetInProgress = FALSE;

   StatusOfChange = NDIS_STATUS_SUCCESS;

   }

    return StatusOfChange;

}

STATIC
VOID
ChangeAddressDispatch(
   IN PLANCE_ADAPTER Adapter,
   IN UINT AddressCount,
   IN CHAR Addresses[][ETH_LENGTH_OF_ADDRESS]
   )

/*++

Routine Description:

   Modifies the logical address filter.

Arguments:

    Adapter - The adapter.

    AddressCount - The number of addresses in Addresses

   Addresses - The new multicast address list.

Return Value:

   None.

--*/

{

   UINT i, j;

   //
   // Local Pointer to the Initialization Block.
   //
   PLANCE_INIT_BLOCK InitializationBlock;
   PLANCE_INIT_BLOCK_HI InitializationBlockHi;

   InitializationBlockHi = (PLANCE_INIT_BLOCK_HI)Adapter->InitializationBlock;

   //
   // Loop through, copying the addresses into the CAM.
   //

   for (i=0; i<8; i++)
      InitializationBlockHi->LogicalAddressFilter[i] = 0;

   for (i=0; i<AddressCount; i++) {

     UCHAR HashCode;
     UCHAR FilterByte;
     UINT CRCValue;

     HashCode = 0;

     CRCValue = CalculateCRC(ETH_LENGTH_OF_ADDRESS,
		        Addresses[i]);

     for (j=0; j<6; j++)
	    HashCode = (HashCode << 1) + (((UCHAR)CRCValue >> j) & 0x01);

     //
     // Bits 3-5 of HashCode point to byte in address filter.
     // Bits 0-2 point to bit within that byte.
     //
     FilterByte = HashCode >> 3;
     InitializationBlockHi->LogicalAddressFilter[FilterByte] |=
	      (1 << (HashCode & 0x07));

   }          
}


STATIC
UINT
CalculateCRC(
    IN UINT NumberOfBytes,
    IN PCHAR Input
    )

/*++

Routine Description:

    Calculates a 32 bit crc value over the input number of bytes.

    NOTE: ZZZ This routine assumes UINTs are 32 bits.

Arguments:

    NumberOfBytes - The number of bytes in the input.

    Input - An input "string" to calculate a CRC over.

Return Value:

    A 32 bit crc value.


--*/

{

    const UINT POLY = 0x04c11db6;
    UINT CRCValue = 0xffffffff;

    ASSERT(sizeof(UINT) == 4);

    for (
   ;
   NumberOfBytes;
   NumberOfBytes--
   ) {

   UINT CurrentBit;
   UCHAR CurrentByte = *Input;
   Input++;

   for (
       CurrentBit = 8;
       CurrentBit;
       CurrentBit--
       ) {

       UINT CurrentCRCHigh = CRCValue >> 31;

       CRCValue <<= 1;

       if (CurrentCRCHigh ^ (CurrentByte & 0x01)) {

      CRCValue ^= POLY;
      CRCValue |= 0x00000001;

       }

       CurrentByte >>= 1;

   }

    }

    return CRCValue;

}

VOID
LanceCloseAction(
    IN NDIS_HANDLE MacBindingHandle
    )

/*++

Routine Description:

    Action routine that will get called when a particular binding
    was closed while it was indicating through NdisIndicateReceive

    All this routine needs to do is to decrement the reference count
    of the binding.

    NOTE: This routine assumes that it is called with the lock acquired.

Arguments:

    MacBindingHandle - The context value returned by the MAC  when the
    adapter was opened.  In reality, it is a pointer to SONIC_OPEN.

Return Value:

    None.


--*/

{

   PLANCE_OPEN_FROM_BINDING_HANDLE(MacBindingHandle)->References--;

}
