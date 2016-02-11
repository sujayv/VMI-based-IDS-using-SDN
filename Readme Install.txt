##################################################################################
README to install libVMI on Ubuntu successfully [Tested on 15.10 (Wily Werewolf)]
##################################################################################



Install Virtual Machine:-

1. Download the virtio disk for drivers from fedora (https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/stable-virtio/virtio-win.iso)
2. Download a Windows 7 or above installation .iso file from somewhere. (Google it)
3. Run sudo apt-get install synaptic
4. Now open Synaptic Package Manager by typing sudo synaptic
5. Go to File>Read Markings and select the file packagelist.txt. Click on apply and install all the packages and reboot.
6. Now in Terminal type chmod +x VMinstall.sh
			chmod +x VMdiskwin.sh
			chmod +x VMcreate.sh
			chmod +x RekallInstall.sh
			chmod +x Installnew.sh
7. Run disk creator by typing ./VMdiskwin.sh
8. Open VMcreate.sh and change the name of the Windows Installation .iso file you downloaded in the field WINIMG.(It assumes that all your files are present in the Downloads folder of the current user)
9. Install Windows and then shut down.
10. Open VMinstall.sh and change the --disk path=/home/USERNAME/..../NAME_OF_DISK_CREATED(Windows in this case).qcow2 
11. Connect the disk to qemu host system by running ./VMinstall.sh
NOTE: If the VMinstall script gives an error of network bridge doesn't exist, then run ifconfig and check the name of the Virtual Ethernet Adaptor running in your system and make changes accordingly.
12. The VM should boot up and open up in a new window or it should echo that the machine is successfully installed.
13. Check that it is present in the qemu:///system domain by typing virsh -c qemu:///system list --all


NOTE1: If you want to install Ubuntu then instead at step 11 run the script ./Ubuntuinstall.sh and make changes accordingly in the path.
NOTE2: While installing Windows, go to custom install and select browse and navigate to the virt-io CD and install the drivers from 'virtiosto' and 'virtionet' in order to install drivers for hard disk as well as internet.


Install libVMI:-

1. Run the script ./Installnew.sh


Run examples of libVMI:-

1. Start your guest machine and download on the guest.
2. Open Terminal and run sudo apt-get update build-essential make check
3. Now cd to libvmi/tools/linux-offset-finder/
4. Now type sudo make.
5. Run sudo insmod findoffsets.ko
6. cd to /var/log/syslog for the offset values. (Scroll to the end of the log)
7. Copy these values and store them.
8. On the HOST machine, create a file in /etc/libvmi.conf if not present otherwise concatenate the following at the end of the file:

Ubuntu{
     ostype = "Linux";
     sysmap = "/boot/System.map-3.19.0-25-generic";
     linux_name = 0x460;
     linux_tasks = 0x240;
     linux_mm = 0x278;
     linux_pid = 0x2b4;
     linux_pgd = 0x48;
}

REPLACE WITH VALUES FROM LOG FILE.
NOTE: Replace the value in system map with the value obtained on the guest machine by running 'sudo uname -r'.

9. Now on the guest machine, open Terminal and type sudo uname -r.
10. cd to /boot/ and copy the appropriate System.map file to the HOST machine in the /boot/ directory.

NOTE: You can copy small files from the GUest to Host easily by using Dropbox. If you have trouble uploading irregular filenames, then just change the extension to .txt while uploading and then delete the extension before using the file on the Host.


Install Rekall:-

1. First install all the pre-requisites for Rekall by running ./RekallInstall.sh
2. 





