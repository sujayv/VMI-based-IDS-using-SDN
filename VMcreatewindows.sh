#!/bin/bash
export QEMU_AUDIO_DRV=alsa 
DISKIMG=~/Downloads/Windows.qcow2
WINIMG=~/Downloads/windows.iso
VIRTIMG=~/Downloads/virtio-win-0.1.102.iso
qemu-system-x86_64 --enable-kvm -drive file=${DISKIMG},if=virtio -m 1024 \
-net nic,model=virtio -net user -cdrom ${WINIMG} \
-drive file=${VIRTIMG},index=3,media=cdrom \
-rtc base=localtime,clock=host -smp cores=2,threads=4 \
-usbdevice tablet -soundhw ac97 -cpu host -vga vmware
