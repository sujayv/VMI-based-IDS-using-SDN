#!/bin/bash/
virt-install \
             --connect qemu:///system \
             --virt-type kvm \
             --name Windows \
             --ram 2048 \
             --disk path=/home/sujay/Downloads/windows8.qcow2,bus=virtio,device=disk,format=qcow2 \
             --graphics type=vnc,listen=0.0.0.0 \
             --vcpus=2 --cpu host \
             --network bridge=virbr0,mac=CA:FE:BA:BE:42:44,model=virtio \
             --os-variant win7 \
             --accelerate \
             --boot hd
