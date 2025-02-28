import struct
import serial
import sys

def checksum(data):
    ret = 0

    for byte in data:
        ret += byte

    return ret & 0xffffffff

with open('kernel8.img', 'rb') as file:
    kernel_data = file.read()

header = struct.pack('<III',
    0x544f4f42,
    len(kernel_data),
    checksum(kernel_data)
)

if (len(sys.argv) < 2):
    tty_path = '/dev/tty.usbserial-0001'
else:
    tty_path = sys.argv[1]

tty = serial.Serial(tty_path, baudrate=115200, timeout=1)
tty.write(header)  
tty.flush()
tty.write(kernel_data)
tty.flush()
print("File sent")
while True:
    if tty.read(200).decode() == "":
        print("File received")
        break