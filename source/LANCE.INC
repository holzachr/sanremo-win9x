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

++*/

#define ENGLISH 1

NDIS_STRING DefaultAdapterName=NDIS_STRING_CONST("\\Device\\SANREMO01");
static NDIS_STRING MacName = NDIS_STRING_CONST("SANREMO") ;
static const UCHAR VendorDescription[] = "AMD PCNET Family Ethernet Adapter";

#ifdef ENGLISH
CHAR * msg1 = "NdisRegisterMac successful";
CHAR * msg2 = "BusScan = ";
CHAR * msg3 = "led0 = ";
CHAR * msg4 = "led1 = ";
CHAR * msg5 = "led2 = ";
CHAR * msg6 = "led3 = ";
CHAR * msg7 = "tp = ";
CHAR * msg8 = "New Address = ";
CHAR * msg9 = "IO base address is already in use by another device";
CHAR * msg10 = "You can't specify IOBase address for PCI device";
CHAR * msg11 = "IRQ number found doesn't match PROTOCOL.INI";
CHAR * msg12 = "DMA number found doesn't match protocol.ini ";
CHAR * msg13 = "IRQ and/or DMA number is already in use by another device";
CHAR * msg14 = "IO base address = ";
CHAR * msg15 = "Interrupt number =";
CHAR * msg16 = "DMA channel =";
CHAR * msg17 = "PCI scan specified, PCI bus not found";
CHAR * msg18 = "PCI scan specified, device not found";
CHAR * msg19 = "LanceOpenAdapter failed";
CHAR * msg20 = " Device at specified IO base address not found";
CHAR * msg21 = " Device not found";
CHAR * msg22 = "22: PnP scan specified, device not found";
CHAR * msg23 = "23: VESA scan specified, device not found";
CHAR * msg24 = "24: ISA scan specified, device not found";
#endif

#ifdef GERMAN
CHAR * msg1 = "NdisRegisterMac erfolgreich";
CHAR * msg2 = "BusScan = ";
CHAR * msg3 = "led0 = ";
CHAR * msg4 = "led1 = ";
CHAR * msg5 = "led2 = ";
CHAR * msg6 = "led3 = ";
CHAR * msg7 = "tp = ";
CHAR * msg8 = "Neue Adresse = ";
CHAR * msg9 = "I/O-Basisadresse wird bereits von anderem GerÑt verwendet";
CHAR * msg10 = "I/O-Basisadresse kann nicht fÅr PCI-GerÑt angegeben werden";
CHAR * msg11 = "Festgestellte IRQ-Nummer stimmt nicht mit PROTOCOL.INI Åberein";
CHAR * msg12 = "Festgestellte DMA-Nummer stimmt nicht mit PROTOCOL.INI Åberein";
CHAR * msg13 = "IRQ- und/oder DMA-Nummer wird/werden bereits von anderem GerÑt verwendet";
CHAR * msg14 = "I/O-Basisadresse = ";
CHAR * msg15 = "Interrupt-Nummer =";
CHAR * msg16 = "DMA-Kanal =";
CHAR * msg17 = "PCI-Abtastung angegeben, PCI-Bus nicht gefunden";
CHAR * msg18 = "PCI-Abtastung angegeben, GerÑt nicht gefunden";
CHAR * msg19 = "Fehler LanceOpenAdapter";
CHAR * msg20 = " GerÑt mit der angegeben I/O-Basisadresse nicht gefunden";
CHAR * msg21 = " GerÑt nicht gefunden";
CHAR * msg22 = "22: PnP-Abtastung angegeben, GerÑt nicht gefunden";
CHAR * msg23 = "23: VESA-Abtastung angegeben, GerÑt nicht gefunden";
CHAR * msg24 = "24: ISA-Abtastung angegeben, GerÑt nicht gefunden";
#endif

#ifdef FRENCH 
CHAR * msg1 = "NdisRegisterMac rÇussi";
CHAR * msg2 = "BusScan = ";
CHAR * msg3 = "led0 = ";
CHAR * msg4 = "led1 = ";
CHAR * msg5 = "led2 = ";
CHAR * msg6 = "led3 = ";
CHAR * msg7 = "tp = ";
CHAR * msg8 = "Nouvelle adresse = ";
CHAR * msg9 = "Adresse de base d'E/S dÇjÖ utilisÇ par un autre pÇriphÇrique";
CHAR * msg10 = "Vous ne pouvez pas spÇcifier une adresse de base d'E/S pour un pÇriphÇrique PCI";
CHAR * msg11 = "Le numÇro d'IRQ trouvÇ ne correspond pas Ö celui de PROTOCOL.INI";
CHAR * msg12 = "Le numÇro de DMA trouvÇ ne correspond pas Ö celui de PROTOCOL.INI ";
CHAR * msg13 = "NumÇro(s) de IRQ et/ou de DMA dÇjÖ utilisÇ(s) par un autre pÇriphÇrique";
CHAR * msg14 = "Adresse de base d'E/S = ";
CHAR * msg15 = "NumÇro d'interruption =";
CHAR * msg16 = "Canal de DMA  =";
CHAR * msg17 = "Balayage PCI spÇcifiÇ, bus PCI non trouvÇ";
CHAR * msg18 = "Balayage PCI spÇcifiÇ, pÇriphÇrique non trouvÇ";
CHAR * msg19 = "Echec LanceOpenAdapter";
CHAR * msg20 = " PÇriphÇrique Ö l'addresse de base d'E/S spÇcifiÇe non trouvÇ";
CHAR * msg21 = " PÇriphÇrique non trouvÇ";
CHAR * msg22 = "22: Balayage PnP spÇcifiÇ, pÇriphÇrique non trouvÇ";
CHAR * msg23 = "23: Balayage VESA spÇcifiÇ, pÇriphÇrique non trouvÇ";
CHAR * msg24 = "24: Balayage ISA spÇcifiÇ, pÇriphÇrique non trouvÇ";
#endif

#ifdef ITALIAN
CHAR * msg1 = "NdisRegisterMac riuscito";
CHAR * msg2 = "BusScan = ";
CHAR * msg3 = "led0 = ";
CHAR * msg4 = "led1 = ";
CHAR * msg5 = "led2 = ";
CHAR * msg6 = "led3 = ";
CHAR * msg7 = "tp = ";
CHAR * msg8 = "Nuovo indirizzo  = ";
CHAR * msg9 = "Indirizzo base IO giÖ utilizzato da un'altra periferica";
CHAR * msg10 = "Non si puï speficare un indirizzo base IO per periferica PCI";
CHAR * msg11 = "Il numero IRQ trovato non corrisponde a PROTOCOL.INI";
CHAR * msg12 = "Il numero DMA trovato non corrisponde a PROTOCOL.INI ";
CHAR * msg13 = "Numeri IRQ e/o DMA giÖ utilizzati da un'altra periferica";
CHAR * msg14 = "Indirizzo base IO = ";
CHAR * msg15 = "Numero di interrupt =";
CHAR * msg16 = "Canale DMA  =";
CHAR * msg17 = "Analisi PCI specificata, bus PCI non trovato";
CHAR * msg18 = "Analisi PCI specificata, periferica non trovata";
CHAR * msg19 = "LanceOpenAdapter ha fallito";
CHAR * msg20 = " Periferica all'indirizzo base IO non trovata";
CHAR * msg21 = " Periferica non trovata";
CHAR * msg22 = "22: Analisi PnP specificata, periferica non trovata";
CHAR * msg23 = "23: Analisi VESA specificata, periferica non trovata";
CHAR * msg24 = "24: Analisi ISA specificata, periferica non trovata";
#endif

#ifdef SPANISH
CHAR * msg1 = "NdisRegisterMac satisfactorio";
CHAR * msg2 = "BusScan = ";
CHAR * msg3 = "led0 = ";
CHAR * msg4 = "led1 = ";
CHAR * msg5 = "led2 = ";
CHAR * msg6 = "led3 = ";
CHAR * msg7 = "tp = ";
CHAR * msg8 = "Nueva direcci¢n = ";
CHAR * msg9 = "Otro dispositivo ya usa la direcci¢n de base de E/S";
CHAR * msg10 = "No se puede especificar una direcci¢n de base de E/S para dispositivo PCI.";
CHAR * msg11 = "N£mero de IRQ no coincide con PROTOCOL.INI";
CHAR * msg12 = "N£mero de DMA no coincide con PROTOCOL.INI";
CHAR * msg13 = "Otro dispositivo ya usa el n£mero de IRQ y/o de DMA";
CHAR * msg14 = "Direcci¢n de base de E/S = ";
CHAR * msg15 = "N£mero de interrupci¢n =";
CHAR * msg16 = "Canal de DMA =";
CHAR * msg17 = "Especificada b£squeda PCI, bus PCI no hallado";
CHAR * msg18 = "Especificada b£squeda PCI, dispositivo no hallado";
CHAR * msg19 = "Fall¢ LanceOpenAdapter";
CHAR * msg20 = " Dispositivo en la direcci¢n de base de E/S especificada no hallado";
CHAR * msg21 = " Dispositivo no hallado";
CHAR * msg22 = "22: Especificada b£squeda PnP, dispositivo no hallado";
CHAR * msg23 = "23: Especificada b£squeda VESA, dispositivo no hallado";
CHAR * msg24 = "24: Especificada b£squeda ISA, dispositivo no hallado";
#endif
