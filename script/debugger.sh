#!/bin/bash

# GDB flash/debug script
#
# The TI GDB bridge is first started, then the generic ARM GDB is invoked as a
# remote client.
#
# Usage: $0 <flash|debug> <binary.elf>
#
# Author: Jeff Schornick

TI_GDB_DIR=${HOME}/ti/msp432_gcc/emulation/common/uscif
TI_GDB=${TI_GDB_DIR}/gdb_agent_console
 
GDB=/usr/bin/arm-none-eabi-gdb

echo -n Starting GDB agent in background...
${TI_GDB} -f MSP432 ${TI_GDB_DIR}/xds110_msp432_swd.dat >gdb_agent.log &
AGENT_PID=$!
echo " PID: ${AGENT_PID}"

trap cleanup EXIT

case $1 in
    debug)
        cp $2 debug.elf
        echo Starting GDB debug session...
        ${GDB} -q -x script/gdb_debug_msp432 2>/dev/null
        rm debug.elf
        ;;
    flash)
        cp $2 flash.elf
	echo Flashing via GDB...
	${GDB} -batch -nh -x script/gdb_flash_msp432
	rm flash.elf
        ;;
esac

function cleanup {
    echo "Killing GDB agent (PID: ${AGENT_PID})"
    kill ${AGENT_PID} 2>/dev/null
}

