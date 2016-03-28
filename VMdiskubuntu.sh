#!/bin/bash
sudo modprobe kvm_intel
qemu-img create -f qcow2 Ubuntu.qcow2 20G
./VMcreateubuntu.sh

