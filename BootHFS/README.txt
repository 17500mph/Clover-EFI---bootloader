
                        Installation
                        ============
  
  Normal Install :
  --------------------
  
  Suppose that your installation is on /dev/disk0s2
  
   - Install boot0 to the MBR:
   		sudo fdisk -f boot0 -u -y /dev/rdisk0
  
   - Install boot1h to the partition's bootsector:
  		sudo dd if=boot1h of=/dev/rdisk0s2
  
   - Install clover to the partition's root directory:
  		sudo cp clover /
  
  
  EFI Partition Install (HFS) :
  ---------------------------------
  
  Suppose that your installation is on /dev/disk0s1
   
   - Prepare EFI partition:
		sudo mkdir /Volumes/EFI
		sudo newfs_hfs -v EFI /dev/disk0s1
		mount_hfs /dev/disk0s1 /Volumes/EFI
  
   - Install boot0 to the MBR:
   		sudo fdisk -f boot0 -u -y /dev/rdisk0
  
   - Install boot1h to the partition's bootsector:
  		sudo dd if=boot1h of=/dev/rdisk0s1
  
   - Install clover to the partition's root directory:
  		sudo cp clover /Volumes/EFI

