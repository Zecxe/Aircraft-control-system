:: Select external signal for input of channel 1
modpoll.exe -b 115200 -p none -m rtu -a 1 -t 0 -r 3000 COM7 1

modpoll.exe -b 115200 -p none -m rtu -a 1 -t 0 -r 3003 COM7 1

modpoll.exe -b 115200 -p none -m rtu -a 1 -t 0 -r 3001 COM7 0 

modpoll.exe -b 115200 -p none -m rtu -a 1 -t 0 -r 3002 COM7 0

modpoll.exe -b 115200 -p none -m rtu -a 1 -t 4:int -r 1 COM7 1024

modpoll.exe -b 115200 -p none -m rtu -a 1 -t 4:int -r 2 COM7 2048

modpoll.exe -b 115200 -p none -m rtu -a 1 -t 4:int -r 257 COM7 1024

modpoll.exe -b 115200 -p none -m rtu -a 1 -t 4:int -r 258 COM7 4096

modpoll.exe -b 115200 -p none -m rtu -a 1 -t 4:int -r 513 COM7 2

modpoll.exe -b 115200 -p none -m rtu -a 1 -t 4:int -r 514 COM7 1024

modpoll.exe -b 115200 -p none -m rtu -a 1 -t 0 -r 3030 COM7 1