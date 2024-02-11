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

    transfer.c

Abstract:

    This file contains the code to implement the MacTransferData
    API for the NDIS 3.0 interface.

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

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

NDIS_STATUS
LanceTransferData(
    IN NDIS_HANDLE MacBindingHandle,
    IN NDIS_HANDLE MacReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer,
    OUT PNDIS_PACKET Packet,
    OUT PUINT BytesTransferred
    )

/*++

Routine Description:

   A protocol calls the LanceTransferData request (indirectly via
    NdisTransferData) from within its Receive event handler
    to instruct the MAC to copy the contents of the received packet
    a specified paqcket buffer.

Arguments:

    MacBindingHandle - The context value returned by the MAC when the
    adapter was opened.  In reality this is a pointer to SONIC_OPEN.

    MacReceiveContext - The context value passed by the MAC on its call
    to NdisIndicateReceive.  The MAC can use this value to determine
    which packet, on which adapter, is being received.

    ByteOffset - An unsigned integer specifying the offset within the
    received packet at which the copy is to begin.  If the entire packet
    is to be copied, ByteOffset must be zero.

    BytesToTransfer - An unsigned integer specifying the number of bytes
    to copy.  It is legal to transfer zero bytes; this has no effect.  If
    the sum of ByteOffset and BytesToTransfer is greater than the size
    of the received packet, then the remainder of the packet (starting from
    ByteOffset) is transferred, and the trailing portion of the receive
    buffer is not modified.

    Packet - A pointer to a descriptor for the packet storage into which
    the MAC is to copy the received packet.

    BytesTransfered - A pointer to an unsigned integer.  The MAC writes
    the actual number of bytes transferred into this location.  This value
    is not valid if the return status is STATUS_PENDING.

Return Value:

    The function value is the status of the operation.


--*/

{

   PLANCE_ADAPTER Adapter;

    NDIS_STATUS StatusToReturn;

   Adapter = PLANCE_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);

    NdisAcquireSpinLock(&Adapter->Lock);
    Adapter->References++;

    if (!Adapter->ResetInProgress) {

      PLANCE_OPEN Open = PLANCE_OPEN_FROM_BINDING_HANDLE(MacBindingHandle);

        if (!Open->BindingShuttingDown) {

            Open->References++;

            NdisReleaseSpinLock(&Adapter->Lock);

            //
            // Determine if this was a loopback packet.
            //

            if (MacReceiveContext == (NDIS_HANDLE)NULL) {

            NdisCopyFromPacketToPacket(
                    Packet,
                    0,
                    BytesToTransfer,
                    Adapter->CurrentLoopbackPacket,
                    ByteOffset + MAC_HEADER_SIZE,
                    BytesTransferred
                    );

            } else {

            LanceCopyFromBufferToPacket(
                    (PCHAR)MacReceiveContext + ByteOffset,
                    BytesToTransfer,
                    Packet,
                    0,
                    BytesTransferred
                    );

            }

            NdisAcquireSpinLock(&Adapter->Lock);
            Open->References--;
/*
#if DBG
   if (LanceDbg)
      _Debug_Printf_Service("LanceTransferData called.\n");
#endif
*/
            StatusToReturn = NDIS_STATUS_SUCCESS;

        } else {

            StatusToReturn = NDIS_STATUS_REQUEST_ABORTED;

        }

    } else {

        StatusToReturn = NDIS_STATUS_RESET_IN_PROGRESS;

    }

   LANCE_DO_DEFERRED(Adapter);
    return StatusToReturn;
}
