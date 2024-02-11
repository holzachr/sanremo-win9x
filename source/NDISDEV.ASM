.LALL
;*****************************************************************************
;
;       (C) Copyright MICROSOFT Corp, 1991
;
;       Title:      LANCELNK.ASM sourced from:
;       Title:      NDISLNK.ASM - Assembly linkage to NDIS Wrapper for MACs
;                                  and Protocols
;
;       Version:    3.00
;
;       Date:       05-Nov-1991
;
;       Author:     AWO
;
;-----------------------------------------------------------------------------
;
;       Change log:
;
;          DATE     REV                    DESCRIPTION
;       ----------- --- ------------------------------------------------------
;       05-Nov-1991 AWO Original
;       12-May-1992 GJ  Modified for LANCE
;
;=============================================================================
    TITLE $LANCELNK
    .386P

INCLUDE VMM.INC
INCLUDE NDIS.INC
INCLUDE NETVXD.INC          ; Net VxD initialization oredr


;Declare_Virtual_Device MACRO Name, Major_Ver, Minor_Ver, Ctrl_Proc, Device_Num, Init_Order, V86_Proc, PM_Proc, Reference_Data
;	LOCAL	V86_API_Offset, PM_API_Offset, Serv_Tab_Offset, Serv_Tab_Len, Ref_Data_Offset

DECLARE_VIRTUAL_DEVICE %MAC, 3, 10, <%MAC>_Control, 3560h, \
        MAC_Init_Order,Dmi_Interface,Dmi_Interface

VxD_LOCKED_CODE_SEG

BeginProc C_Device_Init
extern _DriverEntry:NEAR
; Make sure the wrapper (Ndis.386) is loaded
    VxDcall NdisGetVersion
    jc   Fail_Init_Phase
    push 0
    push 0
    call _DriverEntry
    add  esp,8
    cmp  eax,NDIS_STATUS_SUCCESS
    jne  Fail_Init_Phase
    clc
    ret
Fail_Init_Phase:
    stc
    ret
EndProc C_Device_Init

;***********************************************************
;       Function Name: Dmi_Interface
;       Function     : Entry point for the PM or V86 APIs.
;       Input        : Seg:ofs address (PM or V86) in ES:DI of 
;                    : DMI request block.
;       Output       : None
;***********************************************************

BeginProc Dmi_Interface
extern _Dmi_Request:NEAR
;   call MapFlatRoutine
    mov  ax, (Client_ES shl 8) + Client_DI ;Uses structure offsets
    VMMcall Map_Flat
    push eax                          ;Equivalent FLAT32 address in EAX
    call _Dmi_Request
    add  esp,4
    clc
    ret
Fail_Dmi:
    stc
    ret
EndProc Dmi_Interface

;***********************************************************
;       Function Name: MapFlatRoutine
;       Function     : Turn sef:ofs address into 32-bit FLAT address
;       Input        : Seg:ofs address (PM of V86) in ES:DI
;       Output       : Equivalent FLAT32 address in EAX
;***********************************************************
BeginProc MapFlatRoutine
    mov  ax, (Client_ES shl 8) + Client_DI ;Uses structure offsets
    VMMcall Map_Flat
    ret
EndProc MapFlatRoutine
;***********************************************************
;***********************************************************

Begin_Control_Dispatch %MAC
    Control_Dispatch Device_Init,       C_Device_Init
End_Control_Dispatch %MAC



VxD_LOCKED_CODE_ENDS

;******************************************************************************
;                 R E A L   M O D E   I N I T   C O D E
;******************************************************************************

;******************************************************************************
;
;   MAC_Real_Init
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

VxD_REAL_INIT_SEG

BeginProc MAC_Real_Init

;
;   If another us is loaded then don't load -- Just abort our load
;
     test    bx, Duplicate_From_INT2F OR Duplicate_Device_ID
     jnz     SHORT Ndis_RI_Abort_Load



;   No other  is loaded.

     xor     bx, bx
     xor     si, si
     mov     ax, Device_Load_Ok
     ret

Ndis_RI_Abort_Load:
     xor     bx, bx
     xor     si, si
     mov     ax, Abort_Device_Load
     ret

EndProc MAC_Real_Init

VxD_REAL_INIT_ENDS

END MAC_Real_Init
