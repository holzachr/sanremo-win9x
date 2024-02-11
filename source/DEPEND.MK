$(BIN)/ndisdev.obj $(BIN)/ndisdev.lst: ndisdev.asm c:/ddk/inc32/NETVXD.INC \
	c:/ddk/inc32/VMM.INC c:/ddk/net/inc/NDIS.INC

$(BIN)/loopback.obj $(BIN)/loopback.lst: loopback.c ./lancehrd.h \
	./lancesft.h c:/ddk/inc32/basedef.h c:/ddk/inc32/configmg.h \
	c:/ddk/inc32/dbt.h c:/ddk/inc32/ndis.h c:/ddk/inc32/vmm.h \
	c:/ddk/inc32/vmmreg.h c:/ddk/inc32/vxdwraps.h c:/ddk/net/inc/efilter.h

$(BIN)/lance.obj $(BIN)/lance.lst: lance.c ./lancehrd.h ./lancesft.h \
	c:/ddk/inc32/basedef.h c:/ddk/inc32/configmg.h c:/ddk/inc32/dbt.h \
	c:/ddk/inc32/ndis.h c:/ddk/inc32/vmm.h c:/ddk/inc32/vmmreg.h \
	c:/ddk/inc32/vxdwraps.h c:/ddk/net/inc/efilter.h lance.inc

$(BIN)/interrup.obj $(BIN)/interrup.lst: interrup.c ./lancehrd.h \
	./lancesft.h c:/ddk/inc32/basedef.h c:/ddk/inc32/configmg.h \
	c:/ddk/inc32/dbt.h c:/ddk/inc32/ndis.h c:/ddk/inc32/vmm.h \
	c:/ddk/inc32/vmmreg.h c:/ddk/inc32/vxdwraps.h c:/ddk/net/inc/efilter.h

$(BIN)/alloc.obj $(BIN)/alloc.lst: alloc.c ./lancehrd.h ./lancesft.h \
	c:/ddk/inc32/basedef.h c:/ddk/inc32/configmg.h c:/ddk/inc32/dbt.h \
	c:/ddk/inc32/ndis.h c:/ddk/inc32/vmm.h c:/ddk/inc32/vmmreg.h \
	c:/ddk/inc32/vxdwraps.h c:/ddk/net/inc/efilter.h

$(BIN)/packet.obj $(BIN)/packet.lst: packet.c ./lancehrd.h ./lancesft.h \
	c:/ddk/inc32/basedef.h c:/ddk/inc32/ndis.h c:/ddk/net/inc/efilter.h

$(BIN)/request.obj $(BIN)/request.lst: request.c ./lancehrd.h ./lancesft.h \
	c:/ddk/inc32/basedef.h c:/ddk/inc32/configmg.h c:/ddk/inc32/dbt.h \
	c:/ddk/inc32/ndis.h c:/ddk/inc32/vmm.h c:/ddk/inc32/vmmreg.h \
	c:/ddk/inc32/vxdwraps.h c:/ddk/net/inc/efilter.h

$(BIN)/send.obj $(BIN)/send.lst: send.c ./lancehrd.h ./lancesft.h \
	c:/ddk/inc32/basedef.h c:/ddk/inc32/configmg.h c:/ddk/inc32/dbt.h \
	c:/ddk/inc32/ndis.h c:/ddk/inc32/vmm.h c:/ddk/inc32/vmmreg.h \
	c:/ddk/inc32/vxdwraps.h c:/ddk/net/inc/efilter.h

$(BIN)/transfer.obj $(BIN)/transfer.lst: transfer.c ./lancehrd.h \
	./lancesft.h c:/ddk/inc32/basedef.h c:/ddk/inc32/ndis.h \
	c:/ddk/net/inc/efilter.h

