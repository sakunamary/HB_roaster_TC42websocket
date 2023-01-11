#库引用
import serial
import serial.tools.list_ports
import time
#import json
#import websocket
#from websocket import WebSocketApp


#全局变量定义
#port ='/dev/ttyAMA0'
serial_port = '/dev/cu.usbmodem00001'
buadrate = 115200
rl = ''
res = ''

command = '' 
#对象声明

serial_port = serial.Serial(serial_port,buadrate,8,'N',1)

#函数定义
def str2cmd(s):
    return bytes(s,'ascii')

#主程序

port_ok_flag = serial_port.isOpen()

while (True) :
    
    if (port_ok_flag):
        print("port_ok_flag:",port_ok_flag)
        #time.sleep(1)
        serial_port.reset_input_buffer()
        serial_port.reset_output_buffer()
               
        command = 'READ\n'
        serial_port.write(str2cmd(command))
        serial_port.flush()
        time.sleep(0.1)
        rl = serial_port.readline().decode('utf-8', 'ignore')[:-2]
        #res = rl.rsplit(',')
        time.sleep(1)