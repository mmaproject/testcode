#!/bin/bash

for file in /home/xvk/test/FACEBOOK_PRETRIALS_22MAR*.cap; do 
./bitrate $file -m 1000 --iface d01 --ip.dst 192.168.186.103 >vkk1.txt
./bitrate $file -m 1000 --iface d00 --ip.src 192.168.186.103 >vkk2.txt
 matlab -nodesktop -nosplash -r "drawUs('vkk1.txt','vkk2.txt','$file.png')"
done
