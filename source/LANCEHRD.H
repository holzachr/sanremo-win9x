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

    lancehrd.h

Abstract:

    This file contains the hardware-related definitions for
    the LANCE driver.

Environment:

    This driver is expected to work in DOS, OS2 and NT at the equivalent
    of kernel mode.

    Architecturally, there is an assumption in this driver that we are
    on a little endian machine.

Revision History:


--*/

/* mode register */
#define LANCE_PROMISCIOUS_MODE    0x8000        /* promiscuous receiver mode */
#define LANCE_NORMAL_MODE    0x0000        /* Normal operation mode */

/* Common xxRing->hadr flags */

#define OWN     0x80
#define DERR    0x40  /* descriptor ERR, not CSR0 ERR */
#define STP     0x02
#define ENP     0x01

/* Unique TxRing->hadr flags */

#define LANCE_TRANSMIT_MORE_COLLISION    0x10  /* frame sent after 2 or more collisions */
#define LANCE_TRANSMIT_ONE_COLLISION     0x08  /* frame sent after 1 collision */
#define LANCE_TRANSMIT_DEF_ERROR         0x04  /* frame sent after channel busy dropped */

/* Unique RxRing->hadr flags */

#define LANCE_RECEIVE_FRAME_ERROR    0x20 /* framing error */
#define LANCE_RECEIVE_OVERFLOW_ERROR 0x10 /* SILO overflow error */
#define LANCE_RECEIVE_CRC_ERROR      0x08 /* Frame checksum error */
#define LANCE_RECEIVE_BUFFER_ERROR   0x04 /* buffer overrun error */

/* TxRing->status flags */

#define LANCE_TRANSMIT_BUF_ERROR      0x8000
#define LANCE_TRANSMIT_UFLO_ERROR     0x4000
#define LANCE_TRANSMIT_LCOL_ERROR     0x1000
#define LANCE_TRANSMIT_LCAR_ERROR     0x0800
#define LANCE_TRANSMIT_RTRY_ERROR     0x0400

/* CSR Registers */

#define LANCE_CSR0    0    /* CSR0 Register */
#define LANCE_CSR1    1    /* CSR1 Register */
#define LANCE_CSR2    2    /* CSR2 Register */
#define LANCE_CSR3    3    /* CSR3 Register */
#define LANCE_CSR4    4    /* CSR4 Register */
#define LANCE_CSR88   88   /* CSR88 Register */
#define LANCE_CSR89   89   /* CSR89 Register */
#define LANCE_CSR15   15   /* CSR15 Register */
#define LANCE_CSR58   58   /* CSR58 Register */
#define LANCE_CSR82   82   /* CSR82 Register */

/* csr0 bits: */
#define LANCE_CSR0_INIT    0x0001
#define LANCE_CSR0_START   0x0002
#define LANCE_CSR0_STOP    0x0004
#define LANCE_CSR0_TDMD    0x0008
#define LANCE_CSR0_TXON    0x0010
#define LANCE_CSR0_RXON    0x0020
#define LANCE_CSR0_INEA    0x0040
#define LANCE_CSR0_INTR    0x0080
#define LANCE_CSR0_IDON    0x0100
#define LANCE_CSR0_TINT    0x0200  /* got a tx int */
#define LANCE_CSR0_RINT    0x0400  /* got a recieve int */
#define LANCE_CSR0_MERR    0x0800
#define LANCE_CSR0_MISS    0x1000
#define LANCE_CSR0_CERR    0x2000
#define LANCE_CSR0_BABL    0x4000
#define LANCE_CSR0_ERR     0x8000  /* MERR | MISS | CERR | BABL */
#define LANCE_CSR0_RUNNING 0x0033  /* Value to check that the chip is running  */
#define    LANCE_CSR0_CLEAR   0xFF00
/* define csr3 bits: */
#define LANCE_CSR3_BCON    0x0001
#define LANCE_CSR3_ACON    0x0002
#define LANCE_CSR3_BSWP    0x0004
#define LANCE_CSR3_TINTM   0x0200
#define LANCE_CSR3_TINTM_IDONM 0x0300
#define LANCE_CSR3_IDONM   0x0100
#define LANCE_CSR3_DXSUFLO 0x0040
#define LANCE_CSR3_MERRM   0x0800
#define LANCE_CSR3_MISSM   0x1000
#define LANCE_CSR3_BABLM   0x4000

#define LANCE_CSR4_DPOLL   0x1000

// Maximum Number of Adapters Supported by driver.
//
#define MAXIMUM_NUMBER_OF_ADAPTERS 16

//
// The MAC header size for ethernet
//
#define MAC_HEADER_SIZE (UINT)14

//
// The maximum number of bytes that we will pass to an NDIS
// indication (since we receive packets contiguously, there is
// no reason to limit this).
//
#define LANCE_INDICATE_MAXIMUM     1514

//
// The maximum number of bytes we will pass to a loopback
// indication (unless it all is in one buffer). This number
// includes data only, not the header.
//

#define LANCE_LOOPBACK_MAXIMUM 208

//
// Maximum number of multicast entries in the table
//
#define LANCE_MAX_MULTICAST      16

//
// Default transmit settings
//

//
// Default # of buffers
//
#define TRANSMIT_BUFFERS         32

//
// Default size of transmit buffer
//

#define TRANSMIT_BUFFER_SIZE    (LANCE_INDICATE_MAXIMUM + 6)

//
// Default receive settings
//

//
// Default # of buffers
//
#define RECEIVE_BUFFERS          32

//
// Default size of receive buffer
//
#define RECEIVE_BUFFER_SIZE     (LANCE_INDICATE_MAXIMUM + 6)

//
// Minimum packet size for Ethernet.
//
#define LANCE_MIN_PACKET_SIZE    60

/* Offsets from base I/O address. */
#define PCNET32_DWIO_RDP  	0x10
#define PCNET32_DWIO_RAP	  0x14
#define PCNET32_DWIO_RESET	0x18
#define PCNET32_DWIO_BDP	  0x1C

#define PCNET32_TOTAL_SIZE  0x30

/* IBM MCA 10/100 Mbps Ethernet (9-K) "San Remo" specific */

/* The Micro Channel Adapter ID */
#define SANREMO_ADAPTER_ID              0x8F62          
#define SANREMO_ADAPTER_NAME            "IBM 10/100 Mbps Ethernet (9-K)"

/* The ASIC puts the PCnet at an offset of 
   0x1000 above the ASICs slave IO space */
#define ASIC_IO_OFFSET				    0x00001000	    

/* The ASIC's register to write the PCI_CONFIG_CMD 
   to to access the daughter card's PCI config space */
#define ASIC_PCI_CONFIG_CMD_REGISTER    0x04           

/* Magic bit to access the daughter card's PCI 
   configuration space */
#define ASIC_PCI_CONFIG_CMD             0x01            

/* The ASIC's daughter card IO tunnel address 
   register to write the PCnet's register address to */
#define ASIC_IO_ADDRESS_REGISTER	    0x08			

/* The ASIC's daughter card IO tunnel data 
   register to read/write the PCnet's selected register */
#define ASIC_IO_DATA_REGISTER		    0x0C			

/* The ASIC's interrupt enable/disable register */
#define ASIC_IRQ_ENABLE_REGISTER        0x1C            

/* The ASIC's index register for accessing
    the VPD data, byte by byte */
#define ASIC_VPD_INDEX_REGISTER		    0x24            

/* The ASIC's data register for reading(/writing?) 
   the VPD data, byte by byte */
#define ASIC_VPD_DATA_REGISTER		    0x25  
       
/* The ASIC's <unknown> register, related to 
   the VPD data. "Checksum ok" indicator? */   
#define ASIC_VPD_UNKNOWN_REGISTER	    0x26        

//
// Lance Initialization Block
//

typedef struct _LANCE_INIT_BLOCK {

      //
      // Mode register
      //
      USHORT Mode;

      //
      // Physical Network Address
      //
      UCHAR PhysicalNetworkAddress[ETH_LENGTH_OF_ADDRESS];

      //
      // Logical Address Fileter
      //
      UCHAR LogicalAddressFilter[8];

      //
      // Receive descriptor ring pointer - low 16 bits
      //
      USHORT ReceiveDescriptorRingPhysicalLow;

      //
      // Receive descriptor ring pointer - high 8 bits (23:16)
      //
      UCHAR ReceiveDescriptorRingPhysicalHighL;

      //
      // Number of Receive Descriptor Ring Entries.
      //
      UCHAR RLen;
	   
      //
      // Transmit descriptor ring pointer - low 16 bits
      //
      USHORT TransmitDescriptorRingPhysicalLow;

      //
      // Transmit descriptor ring pointer - high 8 bits (23:16)
      //
      UCHAR TransmitDescriptorRingPhysicalHighL;

      //
      // Number of Receive Descriptor Ring Entries.
      //
      UCHAR TLen;


      } LANCE_INIT_BLOCK, *PLANCE_INIT_BLOCK;

typedef struct _LANCE_INIT_BLOCK_HI {

      //
      // Mode register
      //
      USHORT Mode;

      //
      // Number of Receive Descriptor Ring Entries.
      //
      UCHAR RLen;

      //
      // Number of Receive Descriptor Ring Entries.
      //
      UCHAR TLen;

      //
      // Physical Network Address
      //
      UCHAR PhysicalNetworkAddress[ETH_LENGTH_OF_ADDRESS];

      //
      // Reserved word.
      //
      USHORT Reserved;

      //
      // Logical Address Fileter
      //
      UCHAR LogicalAddressFilter[8];

      //
      // Receive descriptor ring pointer - low 16 bits
      //
      USHORT ReceiveDescriptorRingPhysicalLow;

      //
      // Receive descriptor ring pointer - high 8 bits 
      //
      UCHAR ReceiveDescriptorRingPhysicalHighL;

      //
      // Receive descriptor ring pointer - high 8 bits 
      //
      UCHAR ReceiveDescriptorRingPhysicalHighH;
	   
      //
      // Transmit descriptor ring pointer - low 16 bits
      //
      USHORT TransmitDescriptorRingPhysicalLow;

      //
      // Transmit descriptor ring pointer - high 8 bits 
      //
      UCHAR TransmitDescriptorRingPhysicalHighL;

      //
      // Transmit descriptor ring pointer - high 8 bits 
      //
      UCHAR TransmitDescriptorRingPhysicalHighH;


      } LANCE_INIT_BLOCK_HI, *PLANCE_INIT_BLOCK_HI;

//
// Exponent of buffer length.
//
#define BUFFER_LENGTH_EXPONENT    0x20
#define BUFFER_LENGTH_EXPONENT_H  0x10

typedef struct _LANCE_TRANSMIT_DESCRIPTOR_HI {

     //
     // A Physical pointer to a buffer - low 16 bits.
     //
     USHORT LanceBufferPhysicalLow;

     //
     // A Physical pointer to a buffer - low 8 bits
     //
     UCHAR LanceBufferPhysicalHighL;

     //
     // A Physical pointer to a buffer - high 8 bits
     //
     UCHAR LanceBufferPhysicalHighH;

     //
     // Holds the length of data placed into the buffer.  This
     // should be less than the actual buffer length.
     // Bits 15:12 should always be ones.
     //
     SHORT ByteCount;

     //
     // Reserved Byte.
     //
     UCHAR LanceTMDReserved1;

     //
     // Transmit Message Flags.
     //
     UCHAR LanceTMDFlags;

     //
     // Transmit Retry Count. (15:4) Reserved.
     //
     USHORT LanceTMDTrc;

     //
     // Time Domain Reflectrometry (TDR)(25:16) + Error bits (26:31)
     //
     USHORT TransmitError;

     //
     // Reserved word 2
     //
     USHORT Reserved2;

     //
     // Reserved word 3
     //
     USHORT Reserved3;

   } LANCE_TRANSMIT_DESCRIPTOR_HI,*PLANCE_TRANSMIT_DESCRIPTOR_HI;

typedef struct _LANCE_TRANSMIT_DESCRIPTOR {

     //
     // A Physical pointer to a buffer - low 16 bits.
     //
     USHORT LanceBufferPhysicalLow;

     //
     // A Physical pointer to a buffer - high 8 bits + flags
     //
     UCHAR LanceBufferPhysicalHighL;

     //
     // Transmit Message Flags.
     //
     UCHAR LanceTMDFlags;

     //
     // Holds the length of data placed into the buffer.  This
     // should be less than the actual buffer length.
     // Bits 15:12 should always be ones.
     //
     SHORT ByteCount;

     //
     // Time Domain Reflectrometry (TDR)(9:0) + Error bits (15:10)
     //
     USHORT TransmitError;

   } LANCE_TRANSMIT_DESCRIPTOR,*PLANCE_TRANSMIT_DESCRIPTOR;

//
// Lance receive desriptor structure
//

typedef struct _LANCE_RECEIVE_DESCRIPTOR_HI {

     //
     // A Physical pointer to a buffer - low 16 bits.
     //
     USHORT LanceBufferPhysicalLow;
	 
     //
     // A Physical pointer to a buffer - high 8 bits + flags
     //
     UCHAR LanceBufferPhysicalHighL;

     //
     // A Physical pointer to a buffer - high 8 bits + flags
     //
     UCHAR LanceBufferPhysicalHighH;

     //
     // Size of the buffer pointed by this discriptor
     // Bits 15:12 should always be ones.
     //
     SHORT BufferSize;

     //
     // Reserved Byte.
     //
     UCHAR LanceRMDReserved1;

     //
     // Received Message Flags.
     //
     UCHAR LanceRMDFlags;

     //
     // Holds the length of data placed into the buffer.  This
     // should be less that the actual buffers lengths
     //
     USHORT ByteCount;

     //
     // Runt Packet Count.
     //
     UCHAR LanceRMDRpCnt;

     //
     // Receive Collision Count.
     //
     UCHAR LanceRMDRcCnt;

     //
     // Reserved word.
     //
     USHORT LanceRMDReserved2;

     //
     // Reserved word.
     //
     USHORT LanceRMDReserved3;

   } LANCE_RECEIVE_DESCRIPTOR_HI,*PLANCE_RECEIVE_DESCRIPTOR_HI;

typedef struct _LANCE_RECEIVE_DESCRIPTOR {

     //
     // A Physical pointer to a buffer - low 16 bits.
     //
     USHORT LanceBufferPhysicalLow;

     //
     // A Physical pointer to a buffer - high 8 bits + flags
     //
     UCHAR LanceBufferPhysicalHighL;

     //
     // Received Message Flags.
     //
     UCHAR LanceRMDFlags;

     //
     // Size of the buffer pointed by this discriptor
     // Bits 15:12 should always be ones.
     //
     SHORT BufferSize;

     //
     // Holds the length of data placed into the buffer.  This
     // should be less that the actual buffers lengths
     //
     USHORT ByteCount;

   } LANCE_RECEIVE_DESCRIPTOR,*PLANCE_RECEIVE_DESCRIPTOR;

//
// Handy macros to read out of Lance ports.
//

#define LANCE_ADDRESS_PORT(_Adapter)    (_Adapter->LanceBaseAddress + PCNET32_DWIO_RAP)

#define LANCE_DATA_PORT(_Adapter)    (_Adapter->LanceBaseAddress + PCNET32_DWIO_RDP)

#define LANCE_WRITE_PORT(_Adapter, _Port, _Value) \
{\
    NdisWritePortUlong(_Adapter->NdisAdapterHandle, _Adapter->LanceBaseAddress + ASIC_IO_ADDRESS_REGISTER, \
                        (USHORT)(_Adapter->LanceBaseAddress + ASIC_IO_OFFSET + PCNET32_DWIO_RAP)); \
    NdisWritePortUlong(_Adapter->NdisAdapterHandle, _Adapter->LanceBaseAddress + ASIC_IO_DATA_REGISTER, \
                        (USHORT)(_Port)); \
    NdisWritePortUlong(_Adapter->NdisAdapterHandle, _Adapter->LanceBaseAddress + ASIC_IO_ADDRESS_REGISTER, \
                        (USHORT)(_Adapter->LanceBaseAddress + ASIC_IO_OFFSET + PCNET32_DWIO_RDP)); \
    NdisWritePortUlong(_Adapter->NdisAdapterHandle, _Adapter->LanceBaseAddress + ASIC_IO_DATA_REGISTER, \
                        (USHORT)(_Value)); \
}

#define LANCE_READ_PORT(_Adapter, _Port, _Value) \
{\
    NdisWritePortUlong(_Adapter->NdisAdapterHandle, _Adapter->LanceBaseAddress + ASIC_IO_ADDRESS_REGISTER, \
                        (USHORT)(_Adapter->LanceBaseAddress + ASIC_IO_OFFSET + PCNET32_DWIO_RAP)); \
    NdisWritePortUlong(_Adapter->NdisAdapterHandle, _Adapter->LanceBaseAddress + ASIC_IO_DATA_REGISTER, \
                        (USHORT)(_Port)); \
    NdisWritePortUlong(_Adapter->NdisAdapterHandle, _Adapter->LanceBaseAddress + ASIC_IO_ADDRESS_REGISTER, \
                        (USHORT)(_Adapter->LanceBaseAddress + ASIC_IO_OFFSET + PCNET32_DWIO_RDP)); \
    NdisReadPortUlong(_Adapter->NdisAdapterHandle, _Adapter->LanceBaseAddress + ASIC_IO_DATA_REGISTER, \
                        (PULONG)(_Value)); \
	*_Value &= 0xFFFF; \
}

#define LANCE_READ_BCR_BEFORE_REGISTRATION(_IOBASE, _Port, _Value, _Handle) \
{\
    NdisImmediateWritePortUlong(_Handle, _IOBASE + ASIC_IO_ADDRESS_REGISTER, (USHORT)(_IOBASE + ASIC_IO_OFFSET + PCNET32_DWIO_RAP)); \
    NdisImmediateWritePortUlong(_Handle, _IOBASE + ASIC_IO_DATA_REGISTER, (USHORT)(_Port)); \
    NdisImmediateWritePortUlong(_Handle, _IOBASE + ASIC_IO_ADDRESS_REGISTER, (USHORT)(_IOBASE + ASIC_IO_OFFSET + PCNET32_DWIO_BDP)); \
    NdisImmediateReadPortUlong(_Handle, _IOBASE + ASIC_IO_DATA_REGISTER, (PULONG)(_Value)); \
}

#define LANCE_WRITE_BCR_BEFORE_REGISTRATION(_IOBASE, _Port, _Value, _Handle) \
{\
    NdisImmediateWritePortUlong(_Handle, _IOBASE + ASIC_IO_ADDRESS_REGISTER, (USHORT)(_IOBASE + ASIC_IO_OFFSET + PCNET32_DWIO_RAP)); \
    NdisImmediateWritePortUlong(_Handle, _IOBASE + ASIC_IO_DATA_REGISTER, (USHORT)(_Port)); \
    NdisImmediateWritePortUlong(_Handle, _IOBASE + ASIC_IO_ADDRESS_REGISTER, (USHORT)(_IOBASE + ASIC_IO_OFFSET + PCNET32_DWIO_BDP)); \
    NdisImmediateWritePortUlong(_Handle, _IOBASE + ASIC_IO_DATA_REGISTER, (USHORT)(_Value)); \
}

#define LANCE_READ_BCR(_IOBASE, _Port, _Value, _Adapter) \
{\
    NdisWritePortUlong(_Adapter->NdisAdapterHandle, _IOBASE + ASIC_IO_ADDRESS_REGISTER, (USHORT)(_IOBASE + ASIC_IO_OFFSET + PCNET32_DWIO_RAP)); \
    NdisWritePortUlong(_Adapter->NdisAdapterHandle, _IOBASE + ASIC_IO_DATA_REGISTER, (USHORT)(_Port)); \
    NdisWritePortUlong(_Adapter->NdisAdapterHandle, _IOBASE + ASIC_IO_ADDRESS_REGISTER, (USHORT)(_IOBASE + ASIC_IO_OFFSET + PCNET32_DWIO_BDP)); \
    NdisReadPortUlong(_Adapter->NdisAdapterHandle, _IOBASE + ASIC_IO_DATA_REGISTER, (PULONG)(_Value)); \
	*_Value &= 0xFFFF; \
}

#define LANCE_WRITE_BCR(_IOBASE, _Port, _Value, _Adapter) \
{\
    NdisWritePortUlong(_Adapter->NdisAdapterHandle, _IOBASE + ASIC_IO_ADDRESS_REGISTER, (USHORT)(_IOBASE + ASIC_IO_OFFSET + PCNET32_DWIO_RAP)); \
    NdisWritePortUlong(_Adapter->NdisAdapterHandle, _IOBASE + ASIC_IO_DATA_REGISTER, (USHORT)(_Port)); \
    NdisWritePortUlong(_Adapter->NdisAdapterHandle, _IOBASE + ASIC_IO_ADDRESS_REGISTER, (USHORT)(_IOBASE + ASIC_IO_OFFSET + PCNET32_DWIO_BDP)); \
    NdisWritePortUlong(_Adapter->NdisAdapterHandle, _IOBASE + ASIC_IO_DATA_REGISTER, (USHORT)(_Value)); \
}

//
// Macros to enable/diable interrupt forwarding in the San Remo ASIC

#define SANREMO_ENABLE_INTERRUPTS(_Adapter) \
{\
    NdisWritePortUchar(_Adapter->NdisAdapterHandle, _Adapter->LanceBaseAddress + ASIC_IRQ_ENABLE_REGISTER, \
                        (UCHAR)(1)); \
}

#define SANREMO_DISABLE_INTERRUPTS(_Adapter) \
{\
    NdisWritePortUchar(_Adapter->NdisAdapterHandle, _Adapter->LanceBaseAddress + ASIC_IRQ_ENABLE_REGISTER, \
                        (UCHAR)(0)); \
}

//
// Macros to get MSB and LSB of an address.
//

#define LANCE_GET_LOW_PART_ADDRESS(Adr) ((USHORT)((Adr) & 0xffff))
#define LANCE_GET_HIGH_PART_ADDRESS(Adr) ((UCHAR)(((Adr) & 0xff0000) >> 16))
#define LANCE_GET_HIGH_PART_ADDRESS_H(Adr) ((UCHAR)(((Adr) & 0xff000000) >> 24))
#define LANCE_GET_HIGH_PART_PCI_ADDRESS(Adr) ((USHORT)(((Adr) & 0xffff0000) >> 16))

//
// Macros to include the NdisImmediateReadPort calls in Win 95.
// These are in the Ndis3.1 spec but have not been included in Win 95. They are
// present in NT 3.51.

#ifdef NDIS_WIN

  #ifndef NdisImmediateReadPortUchar
    #define NdisImmediateReadPortUchar(ConfigurationHandle,Port,Data)  *(Data)=_inp(Port)
  #endif

  #ifndef NdisImmediateReadPortUshort
    #define NdisImmediateReadPortUshort(ConfigurationHandle,Port,Data) *(Data)=_inpw(Port)
  #endif

  #ifndef NdisImmediateReadPortUlong
    #define NdisImmediateReadPortUlong(ConfigurationHandle,Port,Data)  *(Data)=_inpd(Port)
  #endif

  #ifndef NdisImmediateWritePortUchar
    #define NdisImmediateWritePortUchar(ConfigurationHandle,Port,Data)  _outp(Port,Data)
  #endif

  #ifndef NdisImmediateWritePortUshort
    #define NdisImmediateWritePortUshort(ConfigurationHandle,Port,Data) _outpw(Port,Data)
  #endif

  #ifndef NdisImmediateWritePortUlong
    #define NdisImmediateWritePortUlong(ConfigurationHandle,Port,Data)  _outpd(Port,Data)
  #endif

#endif
