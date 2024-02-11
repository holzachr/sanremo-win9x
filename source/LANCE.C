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

    lance.c

Abstract:

    This is the main file for the Advanced Micro Devices LANCE
    Ethernet controller.  This driver conforms to the NDIS 3.0 interface.

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

#include "lance.inc"

//
// This variable is used to control debug output.
//
// INT LanceDbg = 1;
// UCHAR Log[LOG_SIZE];
// UCHAR LogPlace = 0;

#if DBG
INT LanceDbg = 0;
INT LanceIsr = 0;
INT LanceTx = 0;
INT LanceTxPacket = 0;
INT LanceTxOwn = 0;
INT LanceRx = 0;
INT LanceErr = 0;
INT LanceAddress = 0;
#if LANCELOG


NDIS_TIMER LogTimer;
BOOLEAN LogTimerRunning = FALSE;

UCHAR Log[LOG_SIZE];

UCHAR LogPlace = 0;
UCHAR LogWrapped = 0;

UCHAR LancePrintLog = 0;

#endif // LANCELOG
#endif // DBG

ULONG LanceHardwareConfig = LANCE_INIT_OK;
ULONG LanceBaseAddress = 0;
UCHAR LanceInterruptVector = 0;
UCHAR LanceDmaChannel = 0;
ULONG BusScan = 0;
SHORT BusTimer = BUSTIMER_DEFAULT;
LONG led0 = LED_DEFAULT;
LONG led1 = LED_DEFAULT;
LONG led2 = LED_DEFAULT;
LONG led3 = LED_DEFAULT;
LONG burst = BURST_RXTX;
ULONG board_found;
ULONG adapter_io[MAXIMUM_NUMBER_OF_ADAPTERS] = {0};
CHAR adapter_irq[MAXIMUM_NUMBER_OF_ADAPTERS] = {0};
CHAR adapter_dma[MAXIMUM_NUMBER_OF_ADAPTERS] = {0};
CHAR * copyright = "Copyright(c) 1996 ADVANCED MICRO DEVICES, INC.";

STATIC
NDIS_STATUS
LanceOpenAdapter(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT NDIS_HANDLE *MacBindingHandle,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_HANDLE MacAdapterContext,
    IN UINT OpenOptions,
    IN PSTRING AddressingInformation OPTIONAL
    );

STATIC
NDIS_STATUS
LanceCloseAdapter(
    IN NDIS_HANDLE MacBindingHandle
    );


STATIC
VOID
LanceUnload(
    IN NDIS_HANDLE MacMacContext
    );

STATIC
NDIS_STATUS
LanceAddAdapter(
    IN NDIS_HANDLE MacMacContext,
    IN NDIS_HANDLE ConfigurationHandle,
    IN PNDIS_STRING AdapterName
    );

STATIC
VOID
LanceRemoveAdapter(
    IN NDIS_HANDLE MacAdapterContext
    );

STATIC
VOID
LanceShutdown(
    IN PVOID ShutdownContext
    );

BOOLEAN
LanceSynchInterruptWithStart(
    IN PVOID Context
    );

STATIC
NDIS_STATUS
LanceReset(
    IN NDIS_HANDLE MacBindingHandle
    );


STATIC
VOID
SetupForReset(
   IN PLANCE_ADAPTER Adapter,
   IN PLANCE_OPEN Open
    );

STATIC
NDIS_STATUS
LanceRegisterAdapter(
    IN NDIS_HANDLE NdisMacHandle,
    IN NDIS_HANDLE ConfigurationHandle,
    IN PNDIS_STRING DeviceName,
    IN PUCHAR NetworkAddress,
    IN UINT MaximumOpenAdapters
    );

//
// These are global structures for the MAC.
//

NDIS_HANDLE LanceMacHandle;
NDIS_HANDLE LanceWrapperHandle;


//
// ZZZ Non portable interface.
//

#ifdef NDIS_WIN
    #ifndef DEBUG
      #ifndef CHICAGO // SNOWBALL
        #pragma code_seg ("_ITEXT","ICODE")
      #endif          // SNOWBALL
    #endif //DEBUG
#endif //NDIS_WIN


NTSTATUS
NDIS_API
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the primary initialization routine for the Lance driver.
    It is simply responsible for the intializing the wrapper and registering
    the MAC.  It then calls a system and architecture specific routine that
    will initialize and register each adapter.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The status of the operation.

--*/

{


    //
    // Receives the status of the NdisRegisterMac operation.
    //
    NDIS_STATUS Status;

    NDIS_MAC_CHARACTERISTICS LanceChar;

    PLANCE_MAC LanceMac;

    //
    // Initialize the wrapper.
    //

    NdisInitializeWrapper(
      &LanceWrapperHandle,
      DriverObject,
      RegistryPath,
      NULL
      );

    //
    // Now allocate memory for our global structure.
    //

    LANCE_ALLOC_MEMORY(&Status, &LanceMac, sizeof(LANCE_MAC));

    if (Status != NDIS_STATUS_SUCCESS) {
        return NDIS_STATUS_RESOURCES;
    }

    LanceMac->WrapperHandle = LanceWrapperHandle;

    //
    // Initialize the MAC characteristics for the call to
    // NdisRegisterMac.
    //

    LanceChar.MajorNdisVersion = LANCE_NDIS_MAJOR_VERSION;
    LanceChar.MinorNdisVersion = LANCE_NDIS_MINOR_VERSION;
    LanceChar.Reserved = 0;
    LanceChar.OpenAdapterHandler = (OPEN_ADAPTER_HANDLER)LanceOpenAdapter;
    LanceChar.CloseAdapterHandler = (CLOSE_ADAPTER_HANDLER)LanceCloseAdapter;
    LanceChar.SendHandler = (SEND_HANDLER)LanceSend;
    LanceChar.TransferDataHandler = (TRANSFER_DATA_HANDLER)LanceTransferData;
    LanceChar.ResetHandler = (RESET_HANDLER)LanceReset;
    LanceChar.RequestHandler = (REQUEST_HANDLER)LanceRequest;
    LanceChar.QueryGlobalStatisticsHandler = (REQUEST_HANDLER)LanceQueryGlobalStatistics;
    LanceChar.UnloadMacHandler = (UNLOAD_MAC_HANDLER)LanceUnload;
    LanceChar.AddAdapterHandler = (ADD_ADAPTER_HANDLER)LanceAddAdapter;
    LanceChar.RemoveAdapterHandler = (REMOVE_ADAPTER_HANDLER)LanceRemoveAdapter;
    LanceChar.Name = MacName;


    NdisRegisterMac(
   &Status,
   &LanceMacHandle,          // this is a global
   LanceWrapperHandle,
   (NDIS_HANDLE)LanceMac,
   &LanceChar,
   sizeof(LanceChar)
   );

    LanceMac->MacHandle = LanceMacHandle;

    return STATUS_SUCCESS;
}


#ifdef NDIS_WIN
    #ifndef DEBUG
      #ifndef CHICAGO // SNOWBALL
        #pragma code_seg ()
        #pragma code_seg ("_ITEXT","ICODE")
      #endif // SNOWBALL
    #endif //DEBUG
#endif //NDIS_WIN

#ifdef NDIS_WIN
    #ifndef DEBUG
      #ifndef CHICAGO // SNOWBALL
        #pragma code_seg ()
        #pragma code_seg ("_ITEXT","ICODE")
      #endif // SNOWBALL
    #endif //DEBUG
#endif //NDIS_WIN

STATIC
NDIS_STATUS
LanceAddAdapter(
    IN NDIS_HANDLE MacMacContext,
    IN NDIS_HANDLE ConfigurationHandle,
    IN PNDIS_STRING AdapterName
    )

/*++

Routine Description:

    LanceAddAdapter adds an adapter to the list supported
    by this MAC.

Arguments:

    MacMacContext - The context passed to NdisRegisterMac (will be NULL).

    ConfigurationHandle - A handle to pass to NdisOpenConfiguration.

    AdapterName - The name to register with via NdisRegisterAdapter.


Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING

--*/

{

    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    NDIS_HANDLE ConfigHandle;
    PNDIS_CONFIGURATION_PARAMETER ReturnedValue;
    NDIS_STRING NetworkAddressString = NDIS_STRING_CONST("NETADDRESS");
    #ifdef NDIS_NT    
    NDIS_STRING BaseAddressString = NDIS_STRING_CONST("IOBaseAddress");
    NDIS_STRING InterruptVectorString = NDIS_STRING_CONST("InterruptNumber");
    #endif
    NDIS_STRING DmaChannelString = NDIS_STRING_CONST("DmaChannel");
    NDIS_STRING LED0String = NDIS_STRING_CONST("LED0");
    NDIS_STRING LED1String = NDIS_STRING_CONST("LED1");
    NDIS_STRING LED2String = NDIS_STRING_CONST("LED2");
    NDIS_STRING LED3String = NDIS_STRING_CONST("LED3");
    NDIS_STRING BURSTString = NDIS_STRING_CONST("BURST");
    NDIS_STRING BusTimerString = NDIS_STRING_CONST("BUSTIMER");

    #ifdef NDIS_WIN
    NDIS_STRING BaseAddressString = NDIS_STRING_CONST("IOAddress");
    NDIS_STRING InterruptVectorString = NDIS_STRING_CONST("Interrupt");
    #endif

    // Local Array to save the Network Address when read from the registry.
    UCHAR NetAddress[6];

    // Local count variable
    UCHAR count;

    PUCHAR NetworkAddress;
    UINT NetworkAddressLength;
    ULONG anchorid = 0;
    ULONG anchorio = 0;

    #if DBG
    if (LanceDbg)
    {
       _Debug_Printf_Service("LanceAdapter Routine:\n");
       //DbgBreakPoint();
     }
    #endif


    if (ConfigurationHandle != NULL)
    {

    //
    // There is configuration info, read it.
    //

    NdisOpenConfiguration(
         &Status,
         &ConfigHandle,
         ConfigurationHandle
         );

    if (Status != NDIS_STATUS_SUCCESS){

       return Status;
    }


    //
    // Get the interrupt vector number
    //

    NdisReadConfiguration(
          &Status,
          &ReturnedValue,
          ConfigHandle,
          &InterruptVectorString,
          NdisParameterInteger
          );

    if (Status == NDIS_STATUS_SUCCESS) {
        LanceInterruptVector = (UCHAR)ReturnedValue->ParameterData.IntegerData;
    }
    //
    // Get the DMA channel number
    //

    NdisReadConfiguration(
          &Status,
          &ReturnedValue,
          ConfigHandle,
          &DmaChannelString,
          NdisParameterInteger
          );

    if (Status == NDIS_STATUS_SUCCESS) {
          LanceDmaChannel = (CCHAR)ReturnedValue->ParameterData.IntegerData;
    }

    //
    // Get the IO base address
    //

    NdisReadConfiguration(
          &Status,
          &ReturnedValue,
          ConfigHandle,
          &BaseAddressString,
          NdisParameterHexInteger
          );

    if (Status == NDIS_STATUS_SUCCESS) {
          LanceBaseAddress = ReturnedValue->ParameterData.IntegerData;
    }

    #if DBG
    if (LanceDbg)
    {
       _Debug_Printf_Service("Registry information:\n");
       _Debug_Printf_Service("%s %x\n",msg14, LanceBaseAddress);
       _Debug_Printf_Service("%s %d\n",msg15, LanceInterruptVector);
       _Debug_Printf_Service("%s %d\n",msg16, LanceDmaChannel);
     }
    #endif

    //
    // Get the Bus Timer keyword
    //
    NdisReadConfiguration(
          &Status,
          &ReturnedValue,
          ConfigHandle,
          &BusTimerString ,
          NdisParameterInteger
          );

    if(Status == NDIS_STATUS_SUCCESS) {

      BusTimer = (SHORT)ReturnedValue->ParameterData.IntegerData;

      switch (BusTimer){

        case 0:
        BusTimer = BUSTIMER_DEFAULT;
        break;

        case 5:
        BusTimer = 50;
        break;

        case 6:
        BusTimer = 60;
        break;

        case 7:
        BusTimer = 70;
        break;

        case 8:
        BusTimer = 80;
        break;

        case 9:
        BusTimer = 90;
        break;

        case 10:
        BusTimer = 100;
        break;

        case 11:
        BusTimer = 110;
        break;

        case 12:
        BusTimer = 120;
        break;

        case 13:
        BusTimer = 130;
        break;

      }

    }

    else

      BusTimer = BUSTIMER_DEFAULT;

    #if DBG
    if (LanceDbg)
       _Debug_Printf_Service("BUS TIMER = %d\n",BusTimer);
    #endif

    //
    // Get the LED0 keyword
    //

      NdisReadConfiguration(
          &Status,
          &ReturnedValue,
          ConfigHandle,
          &LED0String,
          NdisParameterHexInteger
          );

      if(Status == NDIS_STATUS_SUCCESS){

        led0 = ReturnedValue->ParameterData.IntegerData;

      if(led0 == 65536)

        led0 = LED_DEFAULT;

      }

      else

        led0 = LED_DEFAULT;


    #if DBG
    if (LanceDbg)
       _Debug_Printf_Service("%s  %x\n",msg3, led0);
    #endif

    //
    // Get the LED1 keyword
    //


      NdisReadConfiguration(
          &Status,
          &ReturnedValue,
          ConfigHandle,
          &LED1String,
          NdisParameterHexInteger
          );

      if(Status == NDIS_STATUS_SUCCESS) {

        led1 = ReturnedValue->ParameterData.IntegerData;


      if(led1 == 65536)

        led1 = LED_DEFAULT;

      }

      else

        led1 = LED_DEFAULT;



    #if DBG
    if (LanceDbg)
       _Debug_Printf_Service("%s  %x\n",msg4, led1);
    #endif

    //
    // Get the LED2 keyword
    //


      NdisReadConfiguration(
          &Status,
          &ReturnedValue,
          ConfigHandle,
          &LED2String,
          NdisParameterHexInteger
          );

      if(Status == NDIS_STATUS_SUCCESS){

        led2 = ReturnedValue->ParameterData.IntegerData;

      if(led2 == 65536)

        led2 = LED_DEFAULT;

      }

      else

        led2 = LED_DEFAULT;


    #if DBG
    if (LanceDbg)
       _Debug_Printf_Service("%s  %x\n",msg5, led2);
    #endif

    //
    // Get the LED3 keyword
    //


      NdisReadConfiguration(
          &Status,
          &ReturnedValue,
          ConfigHandle,
          &LED3String,
          NdisParameterHexInteger
          );

      if(Status == NDIS_STATUS_SUCCESS){

        led3 = ReturnedValue->ParameterData.IntegerData;

      if(led3 == 65536)

        led3 = LED_DEFAULT;

      }


      else

        led3 = LED_DEFAULT;


    #if DBG
    if (LanceDbg)
       _Debug_Printf_Service("%s  %x\n",msg6, led3);
    #endif

    //
    // Get the BURST keyword
    //     

      NdisReadConfiguration(
          &Status,
          &ReturnedValue,
          ConfigHandle,
          &BURSTString,
          NdisParameterHexInteger
          );

      if(Status == NDIS_STATUS_SUCCESS)
      {
        burst = ReturnedValue->ParameterData.IntegerData;

        if(burst == 65536)   
          burst = BURST_RXTX; 
      }  
      else    
        burst = BURST_RXTX;        

    #if DBG
    if (LanceDbg)
       _Debug_Printf_Service("BURST %x\n", burst);
    #endif
                   
    //
    // Read network address
    //

    NdisReadNetworkAddress(
          &Status,
          (PVOID *)&NetworkAddress,
          &NetworkAddressLength,
          ConfigHandle);

    //
    // Make sure that the address is the right length asnd
    // at least one of the bytes is non-zero.
    //

    if ((Status == NDIS_STATUS_SUCCESS) &&
       (NetworkAddressLength == ETH_LENGTH_OF_ADDRESS) &&
       ((NetworkAddress[0] |
       NetworkAddress[1] |
       NetworkAddress[2] |
       NetworkAddress[3] |
       NetworkAddress[4] |
       NetworkAddress[5]) != 0)) {

       for(count=0;count<ETH_LENGTH_OF_ADDRESS;count++)
         NetAddress[count]= (UCHAR)NetworkAddress[count];

       NetworkAddress = NetAddress;

       #if DBG
       if(LanceDbg) {
          _Debug_Printf_Service("%s %.x-%.x-%.x-%.x-%.x-%.x\n", msg8,
                 (UCHAR)NetAddress[0],
                 (UCHAR)NetAddress[1],
                 (UCHAR)NetAddress[2],
                 (UCHAR)NetAddress[3],
                 (UCHAR)NetAddress[4],
                 (UCHAR)NetAddress[5]);
       }
       #endif
    }

    else
    {

       //
       // Tells LanceRegisterAdapter to use the
       // burned-in address.
       //

       NetworkAddress = NULL;

    }

    //
    // Used passed-in adapter name to register.
    //

    NdisCloseConfiguration(ConfigHandle);

    }        // end read of configuration info



    Status = LanceRegisterAdapter(
            LanceMacHandle,
            ConfigurationHandle,
            AdapterName,
            NetworkAddress,
      32);

    LanceHardwareConfig = LANCE_INIT_OK;
    LanceBaseAddress = 0;
    LanceInterruptVector = 0;
    LanceDmaChannel = 0;

    return Status;           // should be NDIS_STATUS_SUCCESS

}          

STATIC
VOID
scan_mca (
    IN PLANCE_ADAPTER Adapter,
    IN NDIS_HANDLE ConfigurationHandle
    )

/*++

Routine Description:

This routine detects MCA device. If device is found, the device
parameters such as io address and interrupt number are saved.

Arguments:

Adapter - Driver data storage pointer

ConfigurationHandle - Configuration handle

Return Value:

None.

--*/

{
    NDIS_MCA_POS_DATA McaData;
    NDIS_STATUS Status;
    UINT slot;
    
    #if DBG
    if (LanceDbg)
      _Debug_Printf_Service("==>LanceScanMca\n");      
    //if (LanceBreak)
    //  _asm int 3;    
    #endif
    
    //
    // Read MCA POS codes and find out the resources
    //
    
    NdisReadMcaPosInformation(
        &Status,
        ConfigurationHandle,
        &slot,
        &McaData
        );              
    
    if (Status == NDIS_STATUS_SUCCESS)
    {
        if (McaData.AdapterId == SANREMO_ADAPTER_ID)
        {
            // Upper 6 bits of POS[2] contain the IO base * 0x100) 
		        LanceBaseAddress = ((McaData.PosData1 & 0xFC)) << 8;
		
		        // Lower 2 bits of POS[5] encode the IRQ 
		        switch (McaData.PosData4 & 0x03)
		        {
			        case 0x00: LanceInterruptVector = 15; break;
			        case 0x01: LanceInterruptVector = 12; break;
			        case 0x02: LanceInterruptVector = 11; break;
			        case 0x03: LanceInterruptVector = 10; break;
		        }
	
		        // Upper 4 bits of POS[3] contain the DMA arbitration level.
		        // Unused from CPU side, but reserved by POS for the adapter's 
		        // busmaster to use.
		        LanceDmaChannel = McaData.PosData2 >> 4;

            board_found = MCA;

            #if DBG
            if (LanceDbg)
              _Debug_Printf_Service("LanceScanMca found board in slot %d\n", slot);
              _Debug_Printf_Service("LanceBaseAddress = %x\n", LanceBaseAddress);  
              _Debug_Printf_Service("LanceInterruptVector = %d\n", LanceInterruptVector);
              _Debug_Printf_Service("LanceDmaChannel = %d\n", LanceDmaChannel);       
            #endif  
        }
        else
        {
            #if DBG
            if (LanceDbg)
              _Debug_Printf_Service("LanceScanMca no board found\n");      
            #endif
        }        
    }
    else
    {
        #if DBG
        if (LanceDbg)
          _Debug_Printf_Service("NdisReadMcaPosInformation failed\n");      
        #endif
    }    
        
    #if DBG
    if (LanceDbg)
      _Debug_Printf_Service("<==LanceScanMca\n");
    #endif
} // scan_mca()

STATIC
VOID
sanremo_init (
    IN NDIS_HANDLE ConfigurationHandle,
    IN ULONG IoAddr
    )

/*++

Routine Description:

Do all the San Remo specific ASIC and PCnet initialization

Arguments:

ConfigurationHandle - Configuration handle

IoAddr - I/O base address

Return Value:

None.

--*/

{
    ULONG temp, eepromPresent;

    #if DBG
    if(LanceDbg){
      _Debug_Printf_Service("==>sanremo_init()\n");
    }
    #endif
	
    /* This sequence of writes goes out to the San Remo ASIC
       and is required to start it up. 
       IBM only knows what they mean. */
    NdisImmediateWritePortUchar (ConfigurationHandle, IoAddr + 0x1D, 0x00);
    NdisImmediateWritePortUchar (ConfigurationHandle, IoAddr + 0x1E, 0x4F);
    NdisImmediateWritePortUchar (ConfigurationHandle, IoAddr + 0x1F, 0x04);
    NdisImmediateWritePortUlong (ConfigurationHandle, IoAddr + 0x28, 0x00000000);
    NdisImmediateWritePortUshort(ConfigurationHandle, IoAddr + 0x00, 0x0006);
    NdisImmediateWritePortUlong (ConfigurationHandle, IoAddr + 0x10, 0x00000000);
    NdisImmediateWritePortUlong (ConfigurationHandle, IoAddr + 0x14, 0x00000000);
    NdisImmediateWritePortUshort(ConfigurationHandle, IoAddr + 0x1A, 0x0FFF);
    NdisImmediateWritePortUchar (ConfigurationHandle, IoAddr + 0x22, 0x7F);
    NdisImmediateWritePortUshort(ConfigurationHandle, IoAddr + 0x20, 0x03FF);

    /* Set up the PCnet's PCI Configuration Space through the ASIC */

    // Read Latency and Header Type
    NdisImmediateWritePortUchar(ConfigurationHandle, (IoAddr + ASIC_PCI_CONFIG_CMD_REGISTER), ASIC_PCI_CONFIG_CMD);
    NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_ADDRESS_REGISTER), 0x0C);
    NdisImmediateReadPortUlong (ConfigurationHandle, (IoAddr + ASIC_IO_DATA_REGISTER), &temp);

    // Write Latency and Header Type
    NdisImmediateWritePortUchar(ConfigurationHandle, (IoAddr + ASIC_PCI_CONFIG_CMD_REGISTER), ASIC_PCI_CONFIG_CMD);
    NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_ADDRESS_REGISTER), 0x0C);
    NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_DATA_REGISTER), 0x0000FF00);

    // Write I/O Base Address
    NdisImmediateWritePortUchar(ConfigurationHandle, (IoAddr + ASIC_PCI_CONFIG_CMD_REGISTER), ASIC_PCI_CONFIG_CMD);
    NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_ADDRESS_REGISTER), 0x10);
    NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_DATA_REGISTER), ASIC_IO_OFFSET + IoAddr);

    // Write Control: SERREN, PERREN, IOEN
    // BMEN will be set later by mca_enable_disable_dma()
    NdisImmediateWritePortUchar(ConfigurationHandle, (IoAddr + ASIC_PCI_CONFIG_CMD_REGISTER), ASIC_PCI_CONFIG_CMD);
    NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_ADDRESS_REGISTER), 0x04);
    NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_DATA_REGISTER), 0x00000141);

    // Read PCI Revision ID
    NdisImmediateWritePortUchar(ConfigurationHandle, (IoAddr + ASIC_PCI_CONFIG_CMD_REGISTER), ASIC_PCI_CONFIG_CMD);
    NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_ADDRESS_REGISTER), 0x08);
    NdisImmediateReadPortUlong (ConfigurationHandle, (IoAddr + ASIC_IO_DATA_REGISTER), &temp);

    #if DBG
    if(LanceDbg){
        _Debug_Printf_Service("PCnet PCI Revision ID: %x\n", temp & 0xFF);
    }
    #endif
		
    /* The following 32-bit accesses will switch the PCnet from 16-bit WIO address mode to the 
       32-bit DWIO mode. Maybe DWIO is the only one supported by the ASIC, I have never tested WIO.
       The original AIX driver shifts gears into DWIO mode as first action between the driver and 
       the PCnet, and in Linux we do the same. Guaranteed to work fine. 
       From the Am79C971 datasheet:                    
          "The Software can invoke the DWIO mode by performing a DWord write 
           access to the I/O location at offset 10h (RDP)"                     */
    NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_ADDRESS_REGISTER), IoAddr + ASIC_IO_OFFSET + PCNET32_DWIO_RDP);
    NdisImmediateReadPortUlong (ConfigurationHandle, (IoAddr + ASIC_IO_DATA_REGISTER), &temp);
    NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_ADDRESS_REGISTER), IoAddr + ASIC_IO_OFFSET + PCNET32_DWIO_RDP);
    NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_DATA_REGISTER), temp);

    #if DBG
    if (LanceDbg)
    {
        UINT chipVersion = 0;
        NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_ADDRESS_REGISTER), IoAddr + ASIC_IO_OFFSET + PCNET32_DWIO_RAP);
        NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_DATA_REGISTER), 88);
        NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_ADDRESS_REGISTER), IoAddr + ASIC_IO_OFFSET + PCNET32_DWIO_RDP);
        NdisImmediateReadPortUlong (ConfigurationHandle, (IoAddr + ASIC_IO_DATA_REGISTER), &temp);
        chipVersion = temp & 0xFFFF;
        NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_ADDRESS_REGISTER), IoAddr + ASIC_IO_OFFSET + PCNET32_DWIO_RAP);
        NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_DATA_REGISTER), 89);
        NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_ADDRESS_REGISTER), IoAddr + ASIC_IO_OFFSET + PCNET32_DWIO_RDP);
        NdisImmediateReadPortUlong (ConfigurationHandle, (IoAddr + ASIC_IO_DATA_REGISTER), &temp);
        chipVersion |= (temp & 0xFFFF) << 16;

        _Debug_Printf_Service("PCnet chip version is: %x\n", chipVersion);
    }
    #endif
  
	/* Check BDP19 = EECAS = EEPROM Control and Status for bit 0x8000 = PVALID set,
	   that indicates an EEPROM has been read and found valid */
    temp = 0;
    LANCE_READ_BCR_BEFORE_REGISTRATION(IoAddr, 19, &temp, ConfigurationHandle);
    #if DBG
    if(LanceDbg){
       _Debug_Printf_Service("BCR19: %x\n", temp); 
    }
    #endif  
    if (!(temp & 0x8000))
    {
        UINT eepromValid, time;

        #if DBG
        if(LanceDbg){
          _Debug_Printf_Service("EEPROM not read\n");
        }
        #endif
		
        // Start EEPROM read.
        // This will trigger the PCnet to read the EEPROM 
        // and initialize some registers from the data. 
        LANCE_WRITE_BCR_BEFORE_REGISTRATION(IoAddr, 19, 0x4000, ConfigurationHandle)
        
        // Delay until EEPROM is read
        for(time = 0; time < 1000; time++)
            NdisStallExecution(1);

        // Check BDP19 = EECAS = EEPROM Control and Status for bit 0x8000 = PVALID set,
        // that indicates the EEPROM has been read and is checksum-correct
        temp = 0;
        LANCE_READ_BCR_BEFORE_REGISTRATION(IoAddr, 19, &temp, ConfigurationHandle);
        eepromValid = temp & 0x8000;
 		
        if (eepromValid)
        {
            #if DBG
            if(LanceDbg){
              _Debug_Printf_Service("EEPROM valid\n");
            }
            #endif
        }
        else
        {
            #if DBG
            if(LanceDbg){
              _Debug_Printf_Service("EEPROM not valid!\n");
            }
            #endif

//		        return EIO;
        }
    }
          
    // Do a soft reset of the PCnet
    NdisImmediateWritePortUlong(ConfigurationHandle, (IoAddr + ASIC_IO_ADDRESS_REGISTER), IoAddr + ASIC_IO_OFFSET + PCNET32_DWIO_RESET);
    NdisImmediateReadPortUlong (ConfigurationHandle, (IoAddr + ASIC_IO_DATA_REGISTER), &temp);

    #if DBG
    if(LanceDbg){
      _Debug_Printf_Service("<==sanremo_init()\n");
    }
    #endif

} // sanremo_init()

STATIC
NDIS_STATUS
LanceRegisterAdapter(
    IN NDIS_HANDLE NdisMacHandle,
    IN NDIS_HANDLE ConfigurationHandle,
    IN PNDIS_STRING DeviceName,
    IN PUCHAR NetworkAddress,
    IN UINT MaximumOpenAdapters
    )

/*++

Routine Description:

    This routine (and its interface) are not portable.  They are
    defined by the OS, the architecture, and the particular LANCE
    implementation.

    This routine is responsible for the allocation of the data structures
    for the driver as well as any hardware specific details necessary
    to talk with the device.

Arguments:

    NdisMacHandle - The handle given back to the mac from ndis when
    the mac registered itself.

    ConfigurationHandle - Config handle passed to MacAddAdapter.

    DeviceName - The string containing the name to give to the
    device adapter.

    NetworkAddress - The network address, or NULL if the default
    should be used.

    LanceBaseAddress - The IO base address to be used for the adapter.

    LanceInterruptVector - The interrupt vector to be used for the adapter.

    LanceDmaChannel - The DMA channel to be used for this adapter

    MaximumOpenAdapters - The maximum number of opens at any one time.

Return Value:

    Returns a failure status if anything occurred that prevents the
    initialization of the adapter.

--*/

{
    //
    // Iteration index
    //
    UCHAR i;

    //
    // Store number of buffers.
    //
    UINT  j;

    //
    // Pointer for the adapter root.
    //
    PLANCE_ADAPTER Adapter;

    //
    // Status of various NDIS calls.
    //
    NDIS_STATUS Status;

    //
    // Holds information needed when registering the adapter.
    //
    NDIS_ADAPTER_INFORMATION AdapterInformation;

    //
    // Holds dummy data read from the RESET port.
    //
    USHORT Data;

    //
    // Returned from LanceHardwareDetails; if it failed,
    // we log an error and exit.
    //
    ULONG HardwareDetailsStatus;

    //
    // Local Pointer to the Initialization Block.
    //
    PLANCE_INIT_BLOCK InitializationBlock;
    PLANCE_INIT_BLOCK_HI InitializationBlockHi;

    //
    // We put in this assertion to make sure that ushort are 2 bytes.
    // if they aren't then the initialization block definition needs
    // to be changed.
    //
    // Also all of the logic that deals with status registers assumes
    // that control registers are only 2 bytes.
    //

    ASSERT(sizeof(USHORT) == 2);

    //
    // Allocate the Adapter block.
    //

    LANCE_ALLOC_MEMORY(&Status, &Adapter, sizeof(LANCE_ADAPTER));

    if(Status == NDIS_STATUS_SUCCESS) {

      NdisZeroMemory(Adapter, sizeof(LANCE_ADAPTER));

      Adapter->NdisMacHandle = NdisMacHandle;

      //
      // Set up the AdapterInformation structure.
      //

      NdisZeroMemory (&AdapterInformation, sizeof(NDIS_ADAPTER_INFORMATION));

      AdapterInformation.DmaChannel = LanceDmaChannel;
      AdapterInformation.Master = TRUE;
      AdapterInformation.PhysicalMapRegistersNeeded = 1;
      AdapterInformation.MaximumPhysicalMapping = 1536;
      AdapterInformation.AdapterType = NdisInterfaceMca;
      AdapterInformation.Dma32BitAddresses = TRUE;

      //
      // This may modify resources used by adapter
      // as well as modiying some fields in Adapter.
      //

      if(LanceHardwareConfig == LANCE_INIT_OK)
        HardwareDetailsStatus = LanceHardwareDetails(Adapter,
                                                     ConfigurationHandle);

      else
        HardwareDetailsStatus = LanceHardwareConfig;

      // When Bus_To_Scan is chosen to be All then the board_found
      // variable should be used.
      //

      if(HardwareDetailsStatus >= LANCE_INIT_ERROR_9){

        //
        // If it fails, we call NdisRegisterAdapter anyway
        // (so we can log an error using the NdisAdapterHandle),
        // then return failure.
        //

        AdapterInformation.NumberOfPortDescriptors = 0;

      }

      else {

        // The Actual value of Dma Channel is needed for registration.
        // This is the reason the pnp and isa cards failed to register
        // when all the resources were equal to zero.
        //
        AdapterInformation.DmaChannel = LanceDmaChannel;

        AdapterInformation.NumberOfPortDescriptors = 1;
        AdapterInformation.PortDescriptors[0].InitialPort = LanceBaseAddress;
        AdapterInformation.PortDescriptors[0].NumberOfPorts = 32;

      }

      //
      // Register the adapter with NDIS.
      //
      if ((Status = NdisRegisterAdapter(
         &Adapter->NdisAdapterHandle,
         Adapter->NdisMacHandle,
         Adapter,
         ConfigurationHandle,
         DeviceName,
          &AdapterInformation
         )) == NDIS_STATUS_SUCCESS) {

        if (HardwareDetailsStatus != LANCE_INIT_OK) {

          //
          // Log an error or a warning.
          //

          #ifdef NDIS_NT

          NdisWriteErrorLogEntry(
              Adapter->NdisAdapterHandle,
              NDIS_ERROR_CODE_MISSING_CONFIGURATION_PARAMETER,
              4,
              hardwareDetails,
              LANCE_ERRMSG_INITIAL_INIT,
              NDIS_STATUS_FAILURE,
              HardwareDetailsStatus
              );

          #endif

        }

        if (HardwareDetailsStatus >= LANCE_INIT_ERROR_9) {

          //
          // If an error, exit.
          //
          NdisDeregisterAdapter(Adapter->NdisAdapterHandle);
          LANCE_FREE_MEMORY(Adapter, sizeof(LANCE_ADAPTER));
          return NDIS_STATUS_FAILURE;

        }

        //
        // ZZZ These values should be calculated via information from
        // the configuration manager.
        //
        Adapter->NumberOfTransmitDescriptors = TRANSMIT_BUFFERS;
        Adapter->NumberOfReceiveDescriptors = RECEIVE_BUFFERS; 

        //
        // Allocate memory for all of the adapter structures.
        //
        if (AllocateAdapterMemory(Adapter)) {

          //
          // Initialize the current hardware address.
          //
          NdisMoveMemory(
             Adapter->CurrentNetworkAddress,
             (NetworkAddress != NULL) ?
             NetworkAddress :
             Adapter->PermanentNetworkAddress,
             ETH_LENGTH_OF_ADDRESS);

          // Reset Power Management flag

          Adapter->stop_set = 0;

          InitializationBlockHi = (PLANCE_INIT_BLOCK_HI)Adapter->InitializationBlock;

          //
          // Initialize the init-block structure for the adapter
          //
          InitializationBlockHi->Mode = LANCE_NORMAL_MODE;

          for (i = 0; i < 6; i++)
             InitializationBlockHi->PhysicalNetworkAddress[i]
                = Adapter->CurrentNetworkAddress[i];
          for (i = 0; i < 8; i++)
             InitializationBlockHi->LogicalAddressFilter[i] = 0;

          InitializationBlockHi->ReceiveDescriptorRingPhysicalLow
                = LANCE_GET_LOW_PART_ADDRESS(
                   NdisGetPhysicalAddressLow(
                    Adapter->ReceiveDescriptorRingPhysical));

          j = Adapter->NumberOfReceiveDescriptors;
          while (j>>=1)
             InitializationBlockHi->RLen += BUFFER_LENGTH_EXPONENT_H;
              

          InitializationBlockHi->ReceiveDescriptorRingPhysicalHighL
                |= LANCE_GET_HIGH_PART_ADDRESS(
                    NdisGetPhysicalAddressLow(
                       Adapter->ReceiveDescriptorRingPhysical));

          InitializationBlockHi->ReceiveDescriptorRingPhysicalHighH
                |= LANCE_GET_HIGH_PART_ADDRESS_H(
                    NdisGetPhysicalAddressLow(
                       Adapter->ReceiveDescriptorRingPhysical));

          InitializationBlockHi->TransmitDescriptorRingPhysicalLow
                = LANCE_GET_LOW_PART_ADDRESS(
                   NdisGetPhysicalAddressLow(Adapter->TransmitDescriptorRingPhysical));

          j = Adapter->NumberOfTransmitDescriptors;
          while (j>>=1)
             InitializationBlockHi->TLen
                += BUFFER_LENGTH_EXPONENT_H;

          InitializationBlockHi->TransmitDescriptorRingPhysicalHighL
                |= LANCE_GET_HIGH_PART_ADDRESS(
                   NdisGetPhysicalAddressLow(
                      Adapter->TransmitDescriptorRingPhysical));

          InitializationBlockHi->TransmitDescriptorRingPhysicalHighH
                |= LANCE_GET_HIGH_PART_ADDRESS_H(
                   NdisGetPhysicalAddressLow(
                      Adapter->TransmitDescriptorRingPhysical));

          InitializeListHead(&Adapter->OpenBindings);
          InitializeListHead(&Adapter->CloseList);

          Adapter->FirstRequest = NULL;
          Adapter->LastRequest = NULL;

          Adapter->CurrentPacketFilter = 0;

          Adapter->OpenCount = 0;
          Adapter->Removed = FALSE;

          NdisAllocateSpinLock(&Adapter->Lock);

          Adapter->ProcessingReceiveInterrupt = FALSE;
          Adapter->ProcessingTransmits = FALSE;
          Adapter->ProcessingLoopBacks = FALSE;
          Adapter->ProcessingDeferredOperations = FALSE;
          Adapter->SendQueue = FALSE ;
          Adapter->LoopBackQueue = FALSE ;
          Adapter->FirstLoopBack = NULL;
          Adapter->LastLoopBack = NULL;
          Adapter->CurrentLoopbackPacket = NULL;
          Adapter->FirstSendPacket = NULL;
          Adapter->LastSendPacket = NULL;
          Adapter->InterruptFlag = FALSE ;

          Adapter->References = 1;

          Adapter->ResetInProgress = FALSE;
          Adapter->IndicatingResetStart = FALSE;
          Adapter->IndicatingResetEnd = FALSE;
          Adapter->ResettingOpen = NULL;
          Adapter->RequestInProgress = FALSE;

          NdisZeroMemory (Adapter->GeneralMandatory, GM_ARRAY_SIZE * sizeof(ULONG));
          NdisZeroMemory (Adapter->GeneralOptionalByteCount, GO_COUNT_ARRAY_SIZE * sizeof(LANCE_LARGE_INTEGER));
          NdisZeroMemory (Adapter->GeneralOptionalFrameCount, GO_COUNT_ARRAY_SIZE * sizeof(ULONG));
          NdisZeroMemory (Adapter->GeneralOptional, (GO_ARRAY_SIZE - GO_ARRAY_START) * sizeof(ULONG));
          NdisZeroMemory (Adapter->MediaMandatory, MM_ARRAY_SIZE * sizeof(ULONG));
          NdisZeroMemory (Adapter->MediaOptional, MO_ARRAY_SIZE * sizeof(ULONG));

          //
          // Initialize the filter and associated things.
          // At the beginning nothing is enabled since
          // our filter is 0, although we do store
          // our network address in the first slot.
          //

          if (!EthCreateFilter(
             LANCE_MAX_MULTICAST,    // maximum MC addresses
             LanceChangeAddresses,
             LanceChangeClass,
             LanceCloseAction,
             (PUCHAR)Adapter->CurrentNetworkAddress,
             &Adapter->Lock,
             &Adapter->FilterDB
             )) {

          #ifdef NDIS_NT

               NdisWriteErrorLogEntry(
                  Adapter->NdisAdapterHandle,
                  NDIS_ERROR_CODE_OUT_OF_RESOURCES,
                  2,
                  registerAdapter,
                  LANCE_ERRMSG_CREATE_FILTER
                  );

          #endif

          DeleteAdapterMemory(Adapter);
          NdisDeregisterAdapter(Adapter->NdisAdapterHandle);
          NdisFreeSpinLock(&Adapter->Lock);
          LANCE_FREE_MEMORY(Adapter, sizeof(LANCE_ADAPTER));
          return NDIS_STATUS_RESOURCES;

          }

          else {
            //
            // Initialize the interrupt.
            //
            NdisZeroMemory (&Adapter->Interrupt, sizeof(NDIS_INTERRUPT) );
            
            //
            // Add support for shared interrupts in PCI systems here.
            //

            NdisInitializeInterrupt(
               &Status,
               &Adapter->Interrupt,
               Adapter->NdisAdapterHandle,
               (PNDIS_INTERRUPT_SERVICE)LanceInterruptService,
               Adapter,
               (PNDIS_DEFERRED_PROCESSING)LanceDeferredProcessing,
               (UINT)LanceInterruptVector,
               (UINT)LanceInterruptVector,
               TRUE,
               NdisInterruptLevelSensitive
               );

            if (Status != NDIS_STATUS_SUCCESS) {

              #ifdef NDIS_NT

              NdisWriteErrorLogEntry(
              Adapter->NdisAdapterHandle,
              NDIS_ERROR_CODE_INTERRUPT_CONNECT,
              2,
              registerAdapter,
              LANCE_ERRMSG_INIT_INTERRUPT
              );

              #endif

              EthDeleteFilter(Adapter->FilterDB);
              DeleteAdapterMemory(Adapter);
              NdisDeregisterAdapter(Adapter->NdisAdapterHandle);
              NdisFreeSpinLock(&Adapter->Lock);
              LANCE_FREE_MEMORY(Adapter, sizeof(LANCE_ADAPTER));
              return Status;
            }

            NdisInitializeTimer(
               &Adapter->DeferredTimer,
               LanceTimerProcess,
               (PVOID)Adapter);

            //
            // Initialize the lock up detect timer to detect
            // lockups in Lance.  It fires continuously
            // every 5 seconds, and we check if there were any
            // interrupts in previous 5 second period.
            //

            // Commenting out this code. Not needed to support Lance
            // anymore.
            //

              NdisInitializeTimer(
                 &Adapter->DetectTimer,
                 LanceLockUpDetectProcess,
                 (PVOID)Adapter);

              NdisSetTimer(
                 &Adapter->DetectTimer,
                 1000
                 );

            //
            // Start the card up. This writes an error
            // log entry if it fails.
            //

            if (!LanceInitializeChip(Adapter)) {
               NdisRemoveInterrupt(&Adapter->Interrupt);
               EthDeleteFilter(Adapter->FilterDB);
               DeleteAdapterMemory(Adapter);
               NdisDeregisterAdapter(Adapter->NdisAdapterHandle);
               NdisFreeSpinLock(&Adapter->Lock);
               LANCE_FREE_MEMORY(Adapter, sizeof(LANCE_ADAPTER));
               return NDIS_STATUS_FAILURE;

            }

            else {

              NdisRegisterAdapterShutdownHandler(
                 Adapter->NdisAdapterHandle,
                 (PVOID)Adapter,
                 (ADAPTER_SHUTDOWN_HANDLER)LanceShutdown
                 );

              return NDIS_STATUS_SUCCESS; 
            }                  
          }               
        }           
        else {

          //
          // Call to AllocateAdapterMemory failed.
          //

          #ifdef NDIS_NT

          NdisWriteErrorLogEntry(
           Adapter->NdisAdapterHandle,
           NDIS_ERROR_CODE_OUT_OF_RESOURCES,
           2,
           registerAdapter,
           LANCE_ERRMSG_ALLOC_MEMORY
           );

          #endif

          DeleteAdapterMemory(Adapter);
          NdisDeregisterAdapter(Adapter->NdisAdapterHandle);
          LANCE_FREE_MEMORY(Adapter, sizeof(LANCE_ADAPTER));

          return NDIS_STATUS_RESOURCES;

        }

      }

      else {

        //
        // Call to NdisRegisterAdapter failed.
        //

        #ifdef NDIS_NT

        NdisWriteErrorLogEntry(
         Adapter->NdisAdapterHandle,
         NDIS_ERROR_CODE_OUT_OF_RESOURCES,
         2,
         registerAdapter,
         LANCE_ERRMSG_REGISTER_ADAPTER
         );

        #endif

        LANCE_FREE_MEMORY(Adapter, sizeof(LANCE_ADAPTER));

        return Status;
      }


    }

    else {

      //
      // Couldn't allocate adapter object.
      //

      #ifdef NDIS_NT

      NdisWriteErrorLogEntry(
       Adapter->NdisAdapterHandle,
       NDIS_ERROR_CODE_OUT_OF_RESOURCES,
       4,
       registerAdapter,
       LANCE_ERRMSG_ALLOC_ADAPTER,
       Status,
       sizeof(LANCE_ADAPTER)
       );

      #endif

      return Status;

    }

}


BOOLEAN
LanceInitializeChip(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This routine sets up the initial init of the driver
    and is called during initialization only.

Arguments:

    Adapter - The adapter for the hardware.

Return Value:

    None.

--*/

{

    //
    // ID for device.
    //

    ULONG DeviceId;

    USHORT Id;

    #if DBG
    //
    // _Debug_Printf_Service("LanceInitializeChip routine:\n");
    //
    #endif

    SetupRegistersAndInit(Adapter);

    //
    // First we make sure that the device is stopped.
    //

    LanceStopChip(Adapter);

    // We can start the chip.  We may not
    // have any bindings to indicate to but this
    // is unimportant.
    //

    mca_enable_disable_dma(Adapter, TRUE);

    LanceStartChip(Adapter);


    return TRUE;
}

#ifdef NDIS_WIN
    #ifndef DEBUG
      #ifndef CHICAGO // SNOWBALL
        #pragma code_seg ()
      #endif // SNOWBALL
    #endif //DEBUG
#endif //NDIS_WIN

BOOLEAN
LanceSynchInterruptWithStart(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used during a reset. It ensures that when
    the chip is started, the IENA bit in csr0 is not modified.

Arguments:

    Context - A pointer to a LANCE_ADAPTER structure.

Return Value:

    Always returns true.

--*/

{

   //
   // Storage of Csr0 Values.
   //

   ULONG Csr0Value;

   //
   // NdisSetTimer Loop Value set with this variable.
   //

   USHORT Time = 0;

   PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)Context;

   LANCE_WRITE_PORT(Adapter, LANCE_CSR0,LANCE_CSR0_START|LANCE_CSR0_INIT);
   // Wait for the IDON bit to be set.

   // Wait for 10 millisecond.
   while(Time < 10000){

     LANCE_READ_PORT(Adapter, LANCE_CSR0, &Csr0Value) ;
	
     if(Csr0Value & LANCE_CSR0_IDON){
       break;
     }
	
     NdisStallExecution(1) ;
     Time++ ;
   }

   if(!Adapter->InterruptFlag)
   {
      #if DBG
      if (LanceDbg)
           _Debug_Printf_Service("LanceSynchIn..(): Enable interrupts\n");      
      #endif

      SANREMO_ENABLE_INTERRUPTS(Adapter);
      LANCE_WRITE_PORT(Adapter, LANCE_CSR0, LANCE_CSR0_INEA);
   }

   return TRUE;

}

BOOLEAN
LanceInit(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This routine sets up the initial init of the driver.

Arguments:

    Adapter - The adapter for the hardware.

Return Value:

    None.

--*/

{

    //
    // First we make sure that the device is stopped.
    //

    LanceStopChip(Adapter);

    SetupRegistersAndInit(Adapter);

    mca_enable_disable_dma(Adapter, TRUE);

    //
    // Once the chip is stopped we can't get any more interrupts.
    // This call ensures that any ISR which is just about to run
    // will find the IENA bit in Csr0 preserved when the chip is
    // started.
    //

    #ifdef NDIS_NT

    NdisSynchronizeWithInterrupt(
        &Adapter->Interrupt,
        LanceSynchInterruptWithStart,
        (PVOID)Adapter);

    #endif

    #ifdef NDIS_WIN
    _asm pushfd
    _asm cli

    LanceSynchInterruptWithStart(Adapter);

    _asm popfd
    #endif


    return TRUE;

}

VOID
LanceStartChip(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This routine is used to start an already initialized Lance.

Arguments:

    Adapter - The adapter for the LANCE to start.

Return Value:

    None.

--*/

{

    //
    // Storage of Csr0 Values.
    //

    ULONG Csr0Value;

    //
    // NdisSetTimer Loop Value set with this variable.
    //

    USHORT Time = 0;

    //
    // Init and Start the Chip.
    //

    LANCE_WRITE_PORT(Adapter, LANCE_CSR0, LANCE_CSR0_START|LANCE_CSR0_INIT);

    // Wait for the IDON bit to be set.

    Time = 0;

    // Wait for 10 millisecond.
    while(Time < 10000){

      LANCE_READ_PORT(Adapter, LANCE_CSR0, &Csr0Value) ;

      if(Csr0Value & LANCE_CSR0_IDON){
        break;
      }

      NdisStallExecution(1) ;
      Time++ ;

    }

    //
    // Enable interrupts.
    //

    #if DBG
    if (LanceDbg)
         _Debug_Printf_Service("LanceStartChip(): Enable interrupts\n");      
    #endif

    SANREMO_ENABLE_INTERRUPTS(Adapter);
    LANCE_WRITE_PORT(Adapter, LANCE_CSR0, LANCE_CSR0_INEA);
}

STATIC
VOID
mca_enable_disable_dma(
    IN PLANCE_ADAPTER Adapter,
    BOOLEAN EnableDma
    )
 
/*++

Routine Description:

This routine set/clean pci command register DMA bit

Arguments:

Adapter - The adapter for the hardware.

EnableDma - Flag for operation.

Return Value:

None.

--*/

{
    ULONG Buffer;
    ULONG PatchData = 0;
	
    #if DBG
    if (LanceDbg)
        _Debug_Printf_Service("==>LanceSetPciDma %d\n", EnableDma);
    #endif
    
    NdisWritePortUchar(Adapter->NdisAdapterHandle, 
        (Adapter->LanceBaseAddress + ASIC_PCI_CONFIG_CMD_REGISTER), 
        ASIC_PCI_CONFIG_CMD);
    NdisWritePortUlong(Adapter->NdisAdapterHandle, 
        (Adapter->LanceBaseAddress + ASIC_IO_ADDRESS_REGISTER), 
        0x0004);
    NdisReadPortUlong(Adapter->NdisAdapterHandle, 
        (Adapter->LanceBaseAddress + ASIC_IO_DATA_REGISTER), 
        &Buffer);
    
	  if (EnableDma) 
	  {
        // Enable DMA
        Buffer = 0x00000145;
    } 
	  else 
	  {
        // Disable DMA
        Buffer &= 0x0000fffb;
    }
	
    NdisWritePortUchar(Adapter->NdisAdapterHandle, 
        (Adapter->LanceBaseAddress + ASIC_PCI_CONFIG_CMD_REGISTER), 
        ASIC_PCI_CONFIG_CMD);
    NdisWritePortUlong(Adapter->NdisAdapterHandle, 
        (Adapter->LanceBaseAddress + ASIC_IO_ADDRESS_REGISTER), 
        0x0004);
    NdisWritePortUlong(Adapter->NdisAdapterHandle, 
        (Adapter->LanceBaseAddress + ASIC_IO_DATA_REGISTER), 
        Buffer);
    
    #if DBG
    if (LanceDbg)
        _Debug_Printf_Service("<==LanceSetPciDma\n");
    #endif
}

VOID
LanceStopChip(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This routine is used to stop a Lance.

Arguments:

    Adapter - The adapter for the LANCE to stop.

Return Value:

    None.

--*/

{
  USHORT Time = 0; 

   /* stop normally */

   mca_enable_disable_dma(Adapter, FALSE);

   LANCE_WRITE_PORT(Adapter, LANCE_CSR0, LANCE_CSR0_STOP);

   #if DBG
   if (LanceDbg)
       _Debug_Printf_Service("LanceStopChip(): Disable interrupts\n");
   #endif
   
   SANREMO_DISABLE_INTERRUPTS(Adapter);

   // Ensure that the chip stops completely with interrupts disabled.
   //
   for(Time=0;Time<5;Time++)
     NdisStallExecution(1) ;

}
// LanceStopChip

#ifdef NDIS_WIN
    #ifndef DEBUG
      #ifndef CHICAGO // SNOWBALL
        #pragma code_seg ("_ITEXT","ICODE")
      #endif // SNOWBALL
    #endif //DEBUG
#endif //NDIS_WIN

STATIC
NDIS_STATUS
LanceOpenAdapter(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT NDIS_HANDLE *MacBindingHandle,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_HANDLE MacAdapterContext,
    IN UINT OpenOptions,
    IN PSTRING AddressingInformation OPTIONAL
    )

/*++

Routine Description:

    This routine is used to create an open instance of an adapter, in effect
    creating a binding between an upper-level module and the MAC module over
    the adapter.

Arguments:

    OpenErrorStatus - Returns more information in some cases.

    MacBindingHandle - A pointer to a location in which the MAC stores
    a context value that it uses to represent this binding.

    SelectedMediumIndex - A pointer to a location in which the MAC stores
    the medium selected out of MediumArray.

    MediumArray - An array of media that the protocol can support.

    MediumArraySize - The number of elements in MediumArray.

    NdisBindingContext - A value to be recorded by the MAC and passed as
    context whenever an indication is delivered by the MAC for this binding.

    MacAdapterContext - The value associated with the adapter that is being
    opened when the MAC registered the adapter with NdisRegisterAdapter.

    OpenOptions - A bit mask containing flags with information about this
    binding.

    AddressingInformation - An optional pointer to a variable length string
    containing hardware-specific information that can be used to program the
    device.  (This is not used by this MAC.)

Return Value:

    The function value is the status of the operation.  If the MAC does not
    complete this request synchronously, the value would be
    NDIS_STATUS_PENDING.


--*/

{

    //
    // The LANCE_ADAPTER that this open binding should belong too.
    //
    PLANCE_ADAPTER Adapter;

    //
    // Holds the status that should be returned to the caller.
    //
    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;

   //
   // Holds the status for memory allocation.
   //
   NDIS_STATUS Status;

    //
    // Simple iteration variable, for scanning the medium array.
    //
    UINT i;

    //
    // Pointer to the space allocated for the binding.
    //
    PLANCE_OPEN NewOpen;

    //
    // Points to the MacReserved section of NewOpen->OpenCloseRequest.
    //
   PLANCE_REQUEST_RESERVED Reserved;

    //
    // If we are being removed, don't allow new opens.
    //
    Adapter = PLANCE_ADAPTER_FROM_CONTEXT_HANDLE(MacAdapterContext);

    if (Adapter->Removed) {
      return NDIS_STATUS_FAILURE;
    }

    //
    // Search for the 802.3 media type
    //

    for (i=0; i<MediumArraySize; i++) {

   if (MediumArray[i] == NdisMedium802_3) {
       break;
   }

    }

    if (i == MediumArraySize) {

      return NDIS_STATUS_UNSUPPORTED_MEDIA;

    }

    *SelectedMediumIndex = i;

    NdisAcquireSpinLock(&Adapter->Lock);
    Adapter->References++;

    NdisReleaseSpinLock(&Adapter->Lock);

    //
    // Allocate the space for the open binding.  Fill in the fields.
    //

   LANCE_ALLOC_MEMORY(&Status, &NewOpen, sizeof(LANCE_OPEN));
   if (Status == NDIS_STATUS_SUCCESS) {

     *MacBindingHandle = BINDING_HANDLE_FROM_PLANCE_OPEN(NewOpen);
     InitializeListHead(&NewOpen->OpenList);
     NewOpen->NdisBindingContext = NdisBindingContext;
     NewOpen->References = 1;
     NewOpen->BindingShuttingDown = FALSE;
     NewOpen->OwningLance = Adapter;
     NewOpen->ProtOptionFlags = 0;

     NewOpen->OpenCloseRequest.RequestType = NdisRequestOpen;
     Reserved = PLANCE_RESERVED_FROM_REQUEST(&NewOpen->OpenCloseRequest);
     Reserved->OpenBlock = NewOpen;
     Reserved->Next = (PNDIS_REQUEST)NULL;

     NdisAcquireSpinLock(&Adapter->Lock);

#if DBG
 	   if (LanceDbg)
  	 	   _Debug_Printf_Service("Open request\n");
#endif

	 	if (!EthNoteFilterOpenAdapter(
	 	 	         NewOpen->OwningLance->FilterDB,
	       	 	   NewOpen,
	       	 	   NewOpen->NdisBindingContext,
	       	 	   &NewOpen->NdisFilterHandle
    	)) {

	 	    NdisReleaseSpinLock(&Adapter->Lock);

#ifdef NDIS_NT

 	   	 NdisWriteErrorLogEntry(
	       	 	   Adapter->NdisAdapterHandle,
	       	 	   NDIS_ERROR_CODE_OUT_OF_RESOURCES,
	       	 	   2,
	       	 	   openAdapter,
	       	 	   LANCE_ERRMSG_OPEN_DB
	       	 	   );

#endif
 	   	 LANCE_FREE_MEMORY(NewOpen, sizeof(LANCE_OPEN));

	    	 NdisAcquireSpinLock(&Adapter->Lock);

	   } else {

		  //
		 	// Everything has been filled in.  Synchronize access to the
		 	// adapter block and link the new open adapter in and increment
		 	// the opens reference count to account for the fact that the
		 	// filter routines have a "reference" to the open.
		 	//

		 	Adapter->OpenCount++;
	   }

      StatusToReturn = NDIS_STATUS_SUCCESS;

    } else {

#if DBG
    if (LanceDbg) {
      _Debug_Printf_Service("%s\n",msg19);
    }
#endif
   StatusToReturn = NDIS_STATUS_RESOURCES;

#ifdef NDIS_NT
   NdisWriteErrorLogEntry(
       Adapter->NdisAdapterHandle,
       NDIS_ERROR_CODE_OUT_OF_RESOURCES,
       2,
       openAdapter,
       LANCE_ERRMSG_ALLOC_OPEN
       );

#endif

   NdisAcquireSpinLock(&Adapter->Lock);

    }


    //
    // This macro assumes it is called with the lock held,
    // and releases it.
    //
   LANCE_DO_DEFERRED(Adapter);

    #if DBG
    //
    // _Debug_Printf_Service("Check the Csr0 here:\n");
    //
    #endif

    return StatusToReturn;
}

#ifdef NDIS_WIN
    #ifndef DEBUG
      #ifndef CHICAGO // SNOWBALL
        #pragma code_seg ()
      #endif // SNOWBALL
    #endif //DEBUG
#endif //NDIS_WIN


STATIC
NDIS_STATUS
LanceCloseAdapter(
    IN NDIS_HANDLE MacBindingHandle
    )

/*++

Routine Description:

    This routine causes the MAC to close an open handle (binding).

Arguments:

    MacBindingHandle - The context value returned by the MAC when the
    adapter was opened.  In reality it is a PLANCE_OPEN.

Return Value:

    The function value is the status of the operation.


--*/

{

    //
    // The LANCE_ADAPTER that this open binding should belong too.
    //
    PLANCE_ADAPTER Adapter;

    //
    // Holds the status that should be returned to the caller.
    //
    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;

    //
    // Pointer to the space allocated for the binding.
    //
    PLANCE_OPEN Open;


    Adapter = PLANCE_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);

    //
    // Hold the lock while we update the reference counts for the
    // adapter and the open.
    //

    NdisAcquireSpinLock(&Adapter->Lock);
    Adapter->References++;

    Open = PLANCE_OPEN_FROM_BINDING_HANDLE(MacBindingHandle);

    //
    // Don't do anything if this binding is already closing.
    //

    if (!Open->BindingShuttingDown) {

      PLANCE_REQUEST_RESERVED Reserved = PLANCE_RESERVED_FROM_REQUEST(&Open->OpenCloseRequest);

   Open->OpenCloseRequest.RequestType = NdisRequestClose;
   Reserved->OpenBlock = Open;
   Reserved->Next = (PNDIS_REQUEST)NULL;

   ++Open->References;

      //Duke's Change start
      //LanceQueueRequest(Adapter, &Open->OpenCloseRequest);
#if DBG
      if (LanceDbg)
         _Debug_Printf_Service("LanceProcessRequest: Close request\n");
#endif
      Open = Reserved->OpenBlock;

      StatusToReturn = EthDeleteFilterOpenAdapter(
         Adapter->FilterDB,
         Open->NdisFilterHandle,
         NULL
         );

      //
      // This flag prevents further requests on this binding.
      //

		Open->BindingShuttingDown = TRUE;


   //
   // Remove the creation reference.
   //

   --Open->References;

   //StatusToReturn = NDIS_STATUS_PENDING;
   StatusToReturn = NDIS_STATUS_SUCCESS;
   //Duke's change end
   //

    } else {

   StatusToReturn = NDIS_STATUS_CLOSING;

    }

    //
    // This macro assumes it is called with the lock held,
    // and releases it.
    //

    LANCE_DO_DEFERRED(Adapter);
    return StatusToReturn;

}

STATIC
VOID
LanceUnload(
    IN NDIS_HANDLE MacMacContext
    )
/*++

Routine Description:

    LanceUnload is called when the MAC is to unload itself.

Arguments:

    None.

Return Value:

    None.

--*/

{

    NDIS_STATUS Status;

    UNREFERENCED_PARAMETER(MacMacContext);


    NdisDeregisterMac(
            &Status,
            LanceMacHandle
            );

    NdisTerminateWrapper(
            LanceWrapperHandle,
            NULL
            );

    return;

}


STATIC
VOID
LanceRemoveAdapter(
    IN NDIS_HANDLE MacAdapterContext
    )

/*++

Routine Description:

    LanceRemoveAdapter removes an adapter previously registered
    with NdisRegisterAdapter.

Arguments:

    MacAdapterContext - The context value that the MAC passed
   to NdisRegisterAdapter; actually as pointer to a
   LANCE_ADAPTER.

Return Value:

    None.

--*/

{
    PLANCE_ADAPTER Adapter;

    //
    // Holds dummy data read from the RESET port.
    //

    ULONG Data;

    ULONG i;

    BOOLEAN Canceled;

    Adapter = PLANCE_ADAPTER_FROM_CONTEXT_HANDLE(MacAdapterContext);

    Adapter->Removed = TRUE;

    //
    // Cancel the Deferred Timer.
    //

    NdisCancelTimer(&Adapter->DeferredTimer, &Canceled);

    if(Canceled)
      Adapter->ProcessingDeferredOperations = FALSE ;

    //
    // Stop the deadman timer
    //

    NdisCancelTimer(&Adapter->DetectTimer, &Canceled);

    NdisStallExecution(2500);

    //
    // Stop the chip.
    //

    LanceStopChip (Adapter);

    //
    // Reset the controller to avoid DMA during Removal.
    //

    NdisWritePortUlong(ConfigurationHandle,
        LanceBaseAddress + ASIC_IO_OFFSET + PCNET32_DWIO_RESET,
        LanceBaseAddress + ASIC_IO_ADDRESS_REGISTER); 
    NdisReadPortUlong(ConfigurationHandle,
        LanceBaseAddress + ASIC_IO_DATA_REGISTER,
        &Data);    

    for(i=0;i<160000;i++)
      NdisStallExecution(1);

    NdisDeregisterAdapterShutdownHandler(Adapter->NdisAdapterHandle);

    ASSERT (Adapter->OpenCount == 0);

    //
    // There are no opens left, so remove ourselves.
    //

    NdisRemoveInterrupt(&Adapter->Interrupt);

    EthDeleteFilter(Adapter->FilterDB);

    DeleteAdapterMemory(Adapter);

    NdisDeregisterAdapter(Adapter->NdisAdapterHandle);

    #if DBG
    //
    _Debug_Printf_Service("Check Dynamic Loading & Unloading Here.\n");
    //
    #endif

    // Restore the Global IO base array contents to 0.
    // This allows for dynamic loading and unloading of
    // the adapter.

    for (i = 0; i <= MAXIMUM_NUMBER_OF_ADAPTERS; i++)
    {
      if(adapter_io[i] == Adapter->LanceBaseAddress){

        adapter_io[i] = 0;

        if(adapter_irq[i] == Adapter->LanceInterruptVector)
          adapter_irq[i] = 0;

        if(adapter_dma[i] == Adapter->LanceDmaChannel)
          adapter_dma[i] = 0;

        break;

      }

    }

    NdisFreeSpinLock(&Adapter->Lock);

    LANCE_FREE_MEMORY(Adapter, sizeof(LANCE_ADAPTER));

}


STATIC
NDIS_STATUS
LanceReset(
    IN NDIS_HANDLE MacBindingHandle
    )

/*++

Routine Description:

    The LanceReset request instructs the MAC to issue a hardware reset
    to the network adapter.  The MAC also resets its software state.  See
    the description of NdisReset for a detailed description of this request.

Arguments:

    MacBindingHandle - The context value returned by the MAC  when the
    adapter was opened.  In reality, it is a pointer to LANCE_OPEN.

Return Value:

    The function value is the status of the operation.


--*/

{

    //
    // Holds the status that should be returned to the caller.
    //
   NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;

    PLANCE_ADAPTER Adapter =
   PLANCE_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);

    if (Adapter->Removed) {

   return(NDIS_STATUS_FAILURE);

    }

   NdisAcquireSpinLock(&Adapter->Lock);
   Adapter->References++;

    //
    // Hold the locks while we update the reference counts on the
    // adapter and the open.
    //

   if (!Adapter->ResetInProgress && !Adapter->IndicatingResetStart) {

      PLANCE_OPEN ResettingOpen;

      ResettingOpen = PLANCE_OPEN_FROM_BINDING_HANDLE(MacBindingHandle);

      if (!ResettingOpen->BindingShuttingDown) {

         if (!Adapter->IndicatingResetEnd) {

            PLIST_ENTRY CurrentLink;

            PLANCE_OPEN Open;

            Adapter->IndicatingResetStart = TRUE;

            ResettingOpen->References++;

            //
            // We need to signal every open binding that the
            // reset is starting.   We increment the reference
      // count on the open binding while we're doing indications
            // so that the open can't be deleted out from under
      // us while we're indicating (recall that we can't own
      // the lock during the indication).
      //

            CurrentLink = Adapter->OpenBindings.Flink;

      while (CurrentLink != &Adapter->OpenBindings) {

          Open = CONTAINING_RECORD(
              CurrentLink,
              LANCE_OPEN,
              OpenList
              );

          Open->References++;
          NdisReleaseSpinLock(&Adapter->Lock);

          NdisIndicateStatus(
                Open->NdisBindingContext,
                NDIS_STATUS_RESET_START,
                NULL,
                0
                );

          NdisIndicateStatusComplete(Open->NdisBindingContext);

          NdisAcquireSpinLock(&Adapter->Lock);
          Open->References--;

          CurrentLink = CurrentLink->Flink;

      }

      Adapter->IndicatingResetStart = FALSE;

      SetupForReset(
          Adapter,
          PLANCE_OPEN_FROM_BINDING_HANDLE(MacBindingHandle)
          );

      NdisReleaseSpinLock(&Adapter->Lock);

      StartAdapterReset(Adapter);

      NdisAcquireSpinLock(&Adapter->Lock);

      // This routine enables dma in the pci config space and then 
      // starts the chip up (Init+Start+Inea bits). This is necessary as
      // the dma bit in the pci config space is disabled in the 
      // LanceRemoveAdapter/Shutdownhandler combination. 
      //

      LanceInit(Adapter);

      // LanceStartChip(Adapter);

      Adapter->ResetInProgress = FALSE;

      Adapter->IndicatingResetEnd = TRUE;

      //
      // If anything queued up while we were resetting
      // (in practice that could only be close requests)
      // then restart them now.
      //

      if (Adapter->FirstRequest) {
               LanceProcessRequestQueue(Adapter);
      }

      //
      // We need to signal every open binding that the
      // reset is complete.  We increment the reference
      // count on the open binding while we're doing indications
      // so that the open can't be deleted out from under
      // us while we're indicating (recall that we can't own
      // the lock during the indication).
      //

      CurrentLink = Adapter->OpenBindings.Flink;

      while (CurrentLink != &Adapter->OpenBindings) {

          Open = CONTAINING_RECORD(
              CurrentLink,
              LANCE_OPEN,
              OpenList
              );

          Open->References++;
          NdisReleaseSpinLock(&Adapter->Lock);

          NdisIndicateStatus(
             Open->NdisBindingContext,
             NDIS_STATUS_RESET_END,
             NULL,
             0
             );

          NdisIndicateStatusComplete(Open->NdisBindingContext);

          NdisAcquireSpinLock(&Adapter->Lock);
          Open->References--;

          CurrentLink = CurrentLink->Flink;

            }

            Adapter->IndicatingResetEnd = FALSE;

            ResettingOpen->References--;

         } else {
            // Here we should pend this request and service
            // when the current request indication end is
            // completed.  I stll have to find out the way to
            // do this. God bless Devang.

         }

   } else {

         StatusToReturn = NDIS_STATUS_CLOSING;

   }

    } else {

   StatusToReturn = NDIS_STATUS_RESET_IN_PROGRESS;

    }

   Adapter->References--;
   NdisReleaseSpinLock(&Adapter->Lock);

   return StatusToReturn;

}

VOID
StartAdapterReset(
   IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    This is the first phase of resetting the adapter hardware.

    It makes the following assumptions:

    1) That the hardware has been stopped.

    2) That it can not be preempted.

    3) That no other adapter activity can occur.

    When this routine is finished all of the adapter information
    will be as if the driver was just initialized.

Arguments:

    Adapter - The adapter whose hardware is to be reset.

Return Value:

    None.

--*/
{

   PNDIS_PACKET Packet;
   PLANCE_PACKET_RESERVED Reserved;
   PLANCE_OPEN Open;

    //
    // Shut down the chip.  We won't be doing any more work until
    // the reset is complete.
    //

   LanceStopChip(Adapter);

    //
    // Once the chip is stopped we can't get any more interrupts.
    // Any interrupts that are "queued" for processing could
    // only possibly service this reset.  It is therefore safe for
    // us to clear the adapter global csr value.
    //
   Adapter->Csr0Value = 0;


   //
   // Go through the various loopback list and abort every packet.
   //

  while (Adapter->FirstLoopBack) {

         Packet = Adapter->FirstLoopBack;
         Reserved = PLANCE_RESERVED_FROM_PACKET(Packet);
         Adapter->FirstLoopBack = Reserved->Next;
         Open = PLANCE_OPEN_FROM_BINDING_HANDLE(Reserved->MacBindingHandle);

         //
         // The completion of the packet is one less reason
         // to keep the open around.
         //

         ASSERT(Open->References);

         NdisCompleteSend(
            Open->NdisBindingContext,
            Packet,
            NDIS_STATUS_REQUEST_ABORTED
            );

         Open->References--;

      }

   Adapter->FirstLoopBack = NULL;
   Adapter->LastLoopBack = NULL;

   while (Adapter->FirstSendPacket) {

         Packet = Adapter->FirstSendPacket;
         Reserved = PLANCE_RESERVED_FROM_PACKET(Packet);
         Adapter->FirstSendPacket = Reserved->Next;
         Open = PLANCE_OPEN_FROM_BINDING_HANDLE(Reserved->MacBindingHandle);

         //
         // The completion of the packet is one less reason
         // to keep the open around.
         //

         ASSERT(Open->References);

         NdisCompleteSend(
            Open->NdisBindingContext,
            Packet,
            NDIS_STATUS_REQUEST_ABORTED
            );

         Open->References--;

      }

    Adapter->FirstSendPacket = NULL;
    Adapter->LastSendPacket = NULL;
}

BOOLEAN
SetupRegistersAndInit(
    IN PLANCE_ADAPTER Adapter
    )

/*++

Routine Description:

    It is this routines responsibility to make sure that the
    initialization block is filled and the chip is initialized
    *but not* started.

    NOTE: ZZZ This routine is NT specific.

    NOTE: This routine assumes that it is called with the lock
    acquired OR that only a single thread of execution is working
    with this particular adapter.

Arguments:

    Adapter - The adapter whose hardware is to be initialized.

Return Value:

    None.

--*/
{

    //
    // Iteration index
    //
    UCHAR i;

    //
    // Variable to store the value read from a port
    //
    ULONG Data;

    USHORT Time = 0;

    //
    // Initialize the Rx/Tx descriptor ring structures
    //

    PLANCE_TRANSMIT_DESCRIPTOR TransmitDescriptorRing;
    PLANCE_TRANSMIT_DESCRIPTOR_HI TransmitDescriptorRingHi;


    NDIS_PHYSICAL_ADDRESS TransmitBufferPointerPhysical;

    PLANCE_RECEIVE_DESCRIPTOR ReceiveDescriptorRing;
    PLANCE_RECEIVE_DESCRIPTOR_HI ReceiveDescriptorRingHi;


    NDIS_PHYSICAL_ADDRESS ReceiveBufferPointerPhysical;

    //
    // Local Pointer to the Initialization Block.
    //
    PLANCE_INIT_BLOCK InitializationBlock;
    PLANCE_INIT_BLOCK_HI InitializationBlockHi;


    TransmitDescriptorRing = Adapter->TransmitDescriptorRing;
    TransmitBufferPointerPhysical = Adapter->TransmitBufferPointerPhysical;

    InitializationBlockHi = (PLANCE_INIT_BLOCK_HI)Adapter->InitializationBlock;

    //
    // Set the Software Style to 32 Bits (PCNET-PCI).
    //

    TransmitDescriptorRingHi = (PLANCE_TRANSMIT_DESCRIPTOR_HI)Adapter->
                                                           TransmitDescriptorRing;

    //
    // Read the Orignal Settings in Csr58.
    //
    LANCE_READ_PORT(Adapter, LANCE_CSR58, &Data);

    //
    // Mask IOStyle bits.
    //
    Data &= 0xff00 ;

    //
    // Enable SSIZE32 
    // CSRPCNET + PCNET PCI II Sw style, will be set by the controller.
    //

    Data |= 0x02 ;
    LANCE_WRITE_PORT(Adapter, LANCE_CSR58, Data);

    for (i = 0; i < Adapter->NumberOfTransmitDescriptors;
      i++,TransmitDescriptorRingHi++) {

      TransmitDescriptorRingHi->LanceBufferPhysicalLow =
         LANCE_GET_LOW_PART_ADDRESS(
            NdisGetPhysicalAddressLow(TransmitBufferPointerPhysical) +
               (i * TRANSMIT_BUFFER_SIZE));

      TransmitDescriptorRingHi->LanceBufferPhysicalHighL =
         LANCE_GET_HIGH_PART_ADDRESS(
            NdisGetPhysicalAddressLow(TransmitBufferPointerPhysical) +
               (i * TRANSMIT_BUFFER_SIZE));

      TransmitDescriptorRingHi->LanceBufferPhysicalHighH =
         LANCE_GET_HIGH_PART_ADDRESS_H(
            NdisGetPhysicalAddressLow(TransmitBufferPointerPhysical) +
               (i * TRANSMIT_BUFFER_SIZE));

      TransmitDescriptorRingHi->ByteCount = (SHORT)0xF000;
      TransmitDescriptorRingHi->TransmitError = 0;

      // Set STP and ENP bits.
      TransmitDescriptorRingHi->LanceTMDFlags =
                      (STP | ENP);

    }
	 
    //
    // Reset Power Management STOP flag
    //

    Adapter->stop_set = 0;

    //
    // Initialize the DescriptorAvailable flags.
    //
    for (i=0; i< Adapter->NumberOfTransmitDescriptors; i++)
       Adapter->TransmitDescriptorAvailable[i] = TRUE;

    //
    // Initialize NextTransmitDescriptorIndex.
    //
    Adapter->NextTransmitDescriptorIndex = 0;

    ReceiveBufferPointerPhysical = Adapter->ReceiveBufferPointerPhysical;

    ReceiveDescriptorRingHi = (PLANCE_RECEIVE_DESCRIPTOR_HI)Adapter->
                                                           ReceiveDescriptorRing;
    for (i = 0; i < Adapter->NumberOfReceiveDescriptors;
       i++, ReceiveDescriptorRingHi++) {

       ReceiveDescriptorRingHi->LanceBufferPhysicalLow =
          LANCE_GET_LOW_PART_ADDRESS(
            NdisGetPhysicalAddressLow(ReceiveBufferPointerPhysical) +
             (i * RECEIVE_BUFFER_SIZE));

       ReceiveDescriptorRingHi->LanceBufferPhysicalHighL =
          LANCE_GET_HIGH_PART_ADDRESS(
            NdisGetPhysicalAddressLow(ReceiveBufferPointerPhysical) +
             (i * RECEIVE_BUFFER_SIZE));

       ReceiveDescriptorRingHi->LanceBufferPhysicalHighH =
          LANCE_GET_HIGH_PART_ADDRESS_H(
            NdisGetPhysicalAddressLow(ReceiveBufferPointerPhysical) +
             (i * RECEIVE_BUFFER_SIZE));

      //
      // Make Lance the owner of the descriptor
      //
      ReceiveDescriptorRingHi->LanceRMDFlags = OWN;
      ReceiveDescriptorRingHi->BufferSize = -RECEIVE_BUFFER_SIZE;
      ReceiveDescriptorRingHi->ByteCount = 0;
      ReceiveDescriptorRingHi->LanceRMDReserved1 = 0;

    }

    //
    // Initialize NextReceiveDescriptorIndex.
    //
    Adapter->NextReceiveDescriptorIndex = 0;

    //
    // Initialize CSR registers
    //

    // Enable Automatic Padding for Transmit Packets.
    // Disable Transmit Polling. Not needed for a TDMD.
	 
    LANCE_READ_PORT(Adapter, LANCE_CSR4, &Data);
    Data |= 0x0800 ;

    LANCE_WRITE_PORT(Adapter, LANCE_CSR4, Data|LANCE_CSR4_DPOLL);

    // Program LEDs
    //
    if(Adapter->led0 == LED_DEFAULT)
      Adapter->led0 = 0x00c0;

    if(Adapter->led0 != LED_DEFAULT)
      LANCE_WRITE_BCR(Adapter->LanceBaseAddress, 4, Adapter->led0, Adapter);
	 
    if(Adapter->led1 == LED_DEFAULT)
      Adapter->led1 = 0x00b0;

    if(Adapter->led1 != LED_DEFAULT)
      LANCE_WRITE_BCR(Adapter->LanceBaseAddress, 5, Adapter->led1, Adapter);

    if(Adapter->led2 == LED_DEFAULT)
      Adapter->led2 = 0x4088;

    if(Adapter->led2 != LED_DEFAULT)
      LANCE_WRITE_BCR(Adapter->LanceBaseAddress, 6, Adapter->led2, Adapter);

    if(Adapter->led3 == LED_DEFAULT)
      Adapter->led3 = 0x0081;

    if(Adapter->led3 != LED_DEFAULT)
      LANCE_WRITE_BCR(Adapter->LanceBaseAddress, 7, Adapter->led3, Adapter);

    // Program RX and TX busmastering mode
    //
    //

    LANCE_READ_BCR (Adapter->LanceBaseAddress, 18, &Data, Adapter);
    #if DBG
    _Debug_Printf_Service("BCR18: %x\n", Data);
    #endif

    if(Adapter->burst == BURST_RXTX)
    {
        // BWRITE and BREADE bits should be set
        // (= RX/TX bursting enabled) for optimal
        // DMA performance
        Data |= (1 << 5) | (1 << 6);
    }
    else if (Adapter->burst == BURST_TX)
    {
        Data &= ~(1 << 5);
        Data |=  (1 << 6);
    }
    else
    {
        Data |= (1 << 5) | (1 << 6);
    }      

    #if DBG
    _Debug_Printf_Service("BCR18: %x\n", Data);
    #endif    
    LANCE_WRITE_BCR(Adapter->LanceBaseAddress, 18, Data, Adapter);
  
    LANCE_WRITE_PORT(Adapter, LANCE_CSR3, LANCE_CSR3_IDONM|
                                          LANCE_CSR3_DXSUFLO|
                                          LANCE_CSR3_MERRM|
                                          LANCE_CSR3_BABLM);

   LANCE_WRITE_PORT(Adapter, LANCE_CSR2, LANCE_GET_HIGH_PART_PCI_ADDRESS(
                    NdisGetPhysicalAddressLow(					
                    Adapter->InitializationBlockPhysical))
        );

    LANCE_WRITE_PORT(Adapter, LANCE_CSR1, LANCE_GET_LOW_PART_ADDRESS(
                     NdisGetPhysicalAddressLow(
                        Adapter->InitializationBlockPhysical))
          );

    // Clear the IDON bit in CSR0 with interrupts disabled.
    //

    LANCE_WRITE_PORT(Adapter,LANCE_CSR0,0xFF00) ;

    return(TRUE);

}

STATIC
VOID
SetupForReset(
   IN PLANCE_ADAPTER Adapter,
   IN PLANCE_OPEN Open
    )

/*++

Routine Description:

    This routine is used to fill in the who and why a reset is
    being set up as well as setting the appropriate fields in the
    adapter.

    NOTE: This routine must be called with the lock acquired.

Arguments:

    Adapter - The adapter whose hardware is to be initialized.

   Open - A pointer to a Lance     open structure.

Return Value:

    None.

--*/
{

    PNDIS_REQUEST CurrentRequest;
    PNDIS_REQUEST * CurrentNextLocation;
    PLANCE_OPEN TmpOpen;

    PLANCE_REQUEST_RESERVED Reserved;

    //
    // Shut down the chip.  We won't be doing any more work until
    // the reset is complete. We take it out of reset mode, however.
    //

    LanceStopChip(Adapter);

    Adapter->ResetInProgress = TRUE;

    //
    // If there is a close at the top of the queue, then
    // it may be in two states:
    //
    // 1- Has interrupted, and the InterruptDpc got the
    // interrupt out of Adapter->Csr0Value before we zeroed it.
    //
    // 2- Has interrupted, but we zeroed Adapter->Csr0Value
    // before it read it, OR has not yet interrupted.
    //
    // In case 1, the interrupt will be processed and the
    // close will complete without our intervention. In
    // case 2, the open will not complete. In that case
    // the CAM will have been updated for that open, so
    // all that remains is for us to dereference the open
    // as would have been done in the interrupt handler.
    //
    // Closes that are not at the top of the queue we
    // leave in place; when we restart the queue after
    // the reset, they will get processed.
    //

    CurrentRequest = Adapter->FirstRequest;

    if (CurrentRequest) {

      Reserved = PLANCE_RESERVED_FROM_REQUEST(CurrentRequest);

   //
   // If the first request is a close, take it off the
   // queue, and "complete" it.
   //

   if (CurrentRequest->RequestType == NdisRequestClose) {
       Adapter->FirstRequest = Reserved->Next;
       --(Reserved->OpenBlock)->References;
       CurrentRequest = Adapter->FirstRequest;
   }

   CurrentNextLocation = &(Adapter->FirstRequest);

   while (CurrentRequest) {

         Reserved = PLANCE_RESERVED_FROM_REQUEST(CurrentRequest);

       if ((CurrentRequest->RequestType == NdisRequestClose) ||
      (CurrentRequest->RequestType == NdisRequestOpen)) {

      //
      // Opens are inoffensive, we just leave them
      // on the list. Closes that were not at the
      // head of the list were not processing and
      // can be left on also.
            //

      CurrentNextLocation = &(Reserved->Next);

       } else {

      //
      // Not a close, remove it from the list and
      // fail it.
      //

      *CurrentNextLocation = Reserved->Next;
      TmpOpen = Reserved->OpenBlock;

      NdisReleaseSpinLock(&Adapter->Lock);

      NdisCompleteRequest(
          TmpOpen->NdisBindingContext,
          CurrentRequest,
               NDIS_STATUS_REQUEST_ABORTED
          );

      NdisAcquireSpinLock(&Adapter->Lock);

      --TmpOpen->References;

       }

       CurrentRequest = *CurrentNextLocation;

   }

   Adapter->RequestInProgress = FALSE;

    }

}

#ifdef NDIS_WIN
    #ifndef DEBUG
      #ifndef CHICAGO // SNOWBALL
        #pragma code_seg ("_ITEXT","ICODE")
      #endif // SNOWBALL
    #endif //DEBUG
#endif //NDIS_WIN

STATIC
ULONG
LanceHardwareDetails(
    IN PLANCE_ADAPTER Adapter,
    IN NDIS_HANDLE ConfigurationHandle
    )

/*++

Routine Description:

    This routine scans for PCNET family devices and gets the resources
    for the AMD hardware.

    ZZZ This routine is *not* portable.  It is specific to NT and
    to the Lance implementation.

Arguments:

    Adapter - Where to store the network address, base address, irq and dma.

Return Value:

    LANCE_INIT_OK if successful, LANCE_INIT_ERROR_<NUMBER> if failed,
    LANCE_INIT_WARNING_<NUMBER> if we continue but need to warn user.


--*/

{
#define PROM_DATA_LENGTH 0x10

    //
    // Iteration variable.
    //
    UINT i;

    //
    // Holds the data from read from PROM ID.
    //
    ULONG val;

    board_found = 0;    //reset the flag 

    #if DBG
    if(LanceDbg)
        _Debug_Printf_Service("LanceHardwareDetails\n");
    #endif

    scan_mca(Adapter, ConfigurationHandle);  
    if (board_found != MCA)
    {
        #if DBG
        if(LanceDbg)
          _Debug_Printf_Service("%s \n",msg21);
        #endif

        return(LANCE_INIT_ERROR_21);
    }

    if(check_conflict(LanceBaseAddress, LanceInterruptVector, LanceDmaChannel) == 0)
    {          
      #if DBG
      if(LanceDbg)
        _Debug_Printf_Service("%s \n",msg13);
      #endif
      return(LANCE_INIT_ERROR_13);    
    }

    #if DBG
    if (LanceDbg)
    {
       _Debug_Printf_Service("%s %x\n",msg14, LanceBaseAddress);
       _Debug_Printf_Service("%s %d\n",msg15, LanceInterruptVector);
       _Debug_Printf_Service("%s %d\n",msg16, LanceDmaChannel);
     }
    #endif
    //
    // Everything is fine, initialize keywords and parameters
    //

    Adapter->LanceBaseAddress = LanceBaseAddress;
    Adapter->LanceInterruptVector = LanceInterruptVector;
    Adapter->LanceDmaChannel = LanceDmaChannel;
    Adapter->led0 = led0;
    Adapter->led1 = led1;
    Adapter->led2 = led2;
    Adapter->led3 = led3;
    Adapter->burst = burst;
    Adapter->board_found = board_found;
    Adapter->BusTimer = BusTimer;

    //
    // Do all San Remo specific ASIC and PCnet initializations
    //

    sanremo_init(ConfigurationHandle, LanceBaseAddress);

    // Through the ASIC we're in word-only access mode, so we read 4 bytes at once
    NdisImmediateWritePortUlong(ConfigurationHandle,
        LanceBaseAddress + ASIC_IO_ADDRESS_REGISTER,
        LanceBaseAddress + ASIC_IO_OFFSET + 0x00);
    NdisImmediateReadPortUlong(ConfigurationHandle,
        LanceBaseAddress + ASIC_IO_DATA_REGISTER,
        &val);
    Adapter->PermanentNetworkAddress[0] = (val & 0xFF);
    Adapter->PermanentNetworkAddress[1] = ((val >> 8)  & 0xFF);
    Adapter->PermanentNetworkAddress[2] = ((val >> 16) & 0xFF);
    Adapter->PermanentNetworkAddress[3] = ((val >> 24) & 0xFF);

    NdisImmediateWritePortUlong(ConfigurationHandle,
        LanceBaseAddress + ASIC_IO_ADDRESS_REGISTER,
        LanceBaseAddress + ASIC_IO_OFFSET + 0x04);
    NdisImmediateReadPortUlong(ConfigurationHandle,
        LanceBaseAddress + ASIC_IO_DATA_REGISTER,
        &val);
    Adapter->PermanentNetworkAddress[4] = (val & 0xFF);
    Adapter->PermanentNetworkAddress[5] = ((val >> 8)  & 0xFF);

    #if DBG
        if (LanceDbg) {
          _Debug_Printf_Service("[ %.x-%.x-%.x-%.x-%.x-%.x ] ",
                  (UCHAR)Adapter->PermanentNetworkAddress[0],
                  (UCHAR)Adapter->PermanentNetworkAddress[1],
                  (UCHAR)Adapter->PermanentNetworkAddress[2],
                  (UCHAR)Adapter->PermanentNetworkAddress[3],
                  (UCHAR)Adapter->PermanentNetworkAddress[4],
                  (UCHAR)Adapter->PermanentNetworkAddress[5]);
          _Debug_Printf_Service("\n");
        }
    #endif

    #if DBG
    if(LanceDbg)
      _Debug_Printf_Service("LanceHW Config value: %x \n",LanceHardwareConfig);
    #endif

    return (LanceHardwareConfig);

}

BOOLEAN
check_conflict(
    IN ULONG io,
    IN CHAR irq,
    IN CHAR dma
    )
{
    UINT i;
    for (i = 0; i < MAXIMUM_NUMBER_OF_ADAPTERS; i++)
    {
      if((dma != NO_DMA)&&(adapter_dma[i] == dma))
        return FALSE;

      if(adapter_io[i] == 0)
      {
         adapter_io[i] = io;
         adapter_irq[i] = irq;
         adapter_dma[i] = dma;
         break ;
      }
    }

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
LanceShutdown(
    IN PVOID ShutdownContext
    )

/*++

Routine Description:

    Turns off the card during a powerdown of the system.

Arguments:

    ShutdownContext - Really a pointer to the adapter structure.

Return Value:

    None.

--*/

{
    //
    // Holds dummy data read from the RESET port.
    //
    ULONG Data;
    PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)(ShutdownContext);

    //
    // Set the flag
    //
    Adapter->Removed = TRUE;

    //
    // Shut down the chip.  We won't be doing any more work until
    // the reset is complete.
    //
    LanceStopChip(Adapter);

    //
    // Reset the controller to avoid DMA during Removal.
    //
    NdisWritePortUlong(Adapter->NdisAdapterHandle,
        Adapter->LanceBaseAddress + ASIC_IO_ADDRESS_REGISTER,
        Adapter->LanceBaseAddress + ASIC_IO_OFFSET + PCNET32_DWIO_RESET); 
    NdisReadPortUlong(Adapter->NdisAdapterHandle,
        Adapter->LanceBaseAddress + ASIC_IO_DATA_REGISTER,
        &Data);

    NdisStallExecution(160000);
}
