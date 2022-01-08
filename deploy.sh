#!/bin/bash

/usr/bin/ssh pi@$1 "sudo systemctl stop ledctrl"
/usr/bin/scp $2 pi@$1:/home/pi
/usr/bin/ssh pi@$1 "sudo systemctl start ledctrl"
