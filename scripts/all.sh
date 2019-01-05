#!/bin/bash
#
# Parameter is optional, provide target if wanted. If left out, default target is crownstone

target=${1:-crownstone}
targetoption="-t $target"

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

###################
### Softdevice
###################

cs_info "Build and upload softdevice ..."
$path/softdevice.sh all $target
checkError
cs_succ "Softdevice DONE"

###################
### Firmware
###################

cs_info "Build firmware ..."
$path/firmware.sh -c build $targetoption
checkError
cs_succ "Build DONE"

cs_info "Write hardware version ..."
$path/firmware.sh -c writeHardwareVersion $targetoption
checkError
cs_succ "Write DONE"

cs_info "Upload firmware ..."
$path/firmware.sh -c upload $targetoption
checkError
cs_succ "Upload DONE"

###################
### Bootloader
###################

cs_info "Compile and upload bootloader"
$path/firmware.sh -c bootloader $targetoption
checkError

cs_succ "Success!!"
