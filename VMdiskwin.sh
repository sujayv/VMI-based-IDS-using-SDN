#!/bin/bash
sudo modprobe kvm_intel
qemu-img create -f qcow2 Windows.qcow2 30G
./VMcreatewindows.sh

