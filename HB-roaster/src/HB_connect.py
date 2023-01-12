#库引用
import time,serial

#import re

#import json
#import websocket
#from websocket import WebSocketApp


#全局变量定义
#port ='/dev/ttyAMA0'
serial_port = '/dev/cu.usbmodem00001'
buadrate = 57600
rl = ''
res1300 = ''
res2400 = ''
bt = 0.0
et = 0.0 
drum = 0.0
Air  = 0.0

command = ''
#对象声明

serial_port = serial.Serial(serial_port,buadrate,8,'N',1,timeout=1)

#函数定义
def str2cmd(s):
    return bytes(s,'ascii')

#主程序

port_ok_flag = serial_port.isOpen()

while (port_ok_flag) :      
            #print("port_ok_flag:",port_ok_flag)

    command = 'CHAN;1300\n'
    serial_port.write(str2cmd(command))
    serial_port.flush()
    time.sleep(0.3)
    rl = serial_port.readline().decode('utf-8', 'ignore')[:-2]
    if (rl.startswith('#')):
        print("CHAN 1300:",rl)

    command = 'READ\n'
    serial_port.write(str2cmd(command))
    time.sleep(0.2)
    serial_port.flush()
    rl = serial_port.readline().decode('utf-8', 'ignore')[:-2]
    if (not len(rl) == 0 and not rl.startswith('#')):
        print("READ 1300:",rl)
        res1300 = rl.rsplit(',')
        bt = float(res1300[1])
        air =float(res1300[3])
        print("bt:",bt)
        print("air",air)


    command = 'CHAN;2400\n'
    serial_port.write(str2cmd(command))
    serial_port.flush()
    time.sleep(0.3)
    rl = serial_port.readline().decode('utf-8', 'ignore')[:-2]
    if (rl.startswith('#')):
        print("CHAN 2400:",rl)


    command = 'READ\n'
    serial_port.write(str2cmd(command))
    time.sleep(0.2)
    serial_port.flush()
    rl = serial_port.readline().decode('utf-8', 'ignore')[:-2]
    if (not len(rl) == 0 and not rl.startswith('#')):
        print("READ 2400: ",rl)
        res2400 = rl.rsplit(',')

        bt = float(res2400[2])
        air =float(res2400[4])