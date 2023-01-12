#库引用
import time,serial,json,socket
# import websockets #pip3 install websockets
import  websockets
import asyncio 

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
IP_ADD = ''

command = ''
#对象声明
#声明串口
serial_port = serial.Serial(serial_port,buadrate,8,'N',1,timeout=0.5)
#声明WS client

class WSClient:
    def __init__(self,address) :
        self.ws=create_connection(address)

    def quit(self):
        self.ws.close()        




#函数定义
def str2cmd(s):
    return bytes(s,'ascii')


def get_host_ip():
    """
    查询本机ip地址
    :return: ip
    """
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    finally:
        s.close()
        return ip



#主程序

port_ok_flag = serial_port.isOpen()

print(get_host_ip()) #显示本机IP地址



while (port_ok_flag) :      
            #print("port_ok_flag:",port_ok_flag)

    command = 'CHAN;1300\n'
    serial_port.write(str2cmd(command))
    serial_port.flush()
    time.sleep(0.3)
    rl = serial_port.readline().decode('utf-8', 'ignore')[:-2]
    # if (rl.startswith('#')):
    #     print("CHAN 1300:",rl)

    command = 'READ\n'
    serial_port.write(str2cmd(command))
    time.sleep(0.2)
    serial_port.flush()
    rl = serial_port.readline().decode('utf-8', 'ignore')[:-2]
    if (not len(rl) == 0 and not rl.startswith('#')):
        print("READ 1300:",rl)
        res1300 = rl.rsplit(',')
        bt = float(res1300[0])
        Air =float(res1300[1])

    command = 'CHAN;2400\n'
    serial_port.write(str2cmd(command))
    serial_port.flush()
    time.sleep(0.3)
    rl = serial_port.readline().decode('utf-8', 'ignore')[:-2]
    # if (rl.startswith('#')):
    #     print("CHAN 2400:",rl)

    command = 'READ\n'
    serial_port.write(str2cmd(command))
    time.sleep(0.2)
    serial_port.flush()
    rl = serial_port.readline().decode('utf-8', 'ignore')[:-2]
    if (not len(rl) == 0 and not rl.startswith('#')):
        print("READ 2400: ",rl)
        res2400 = rl.rsplit(',')
        et = float(res2400[0])
        drum =float(res2400[1])
    
    # print("BT:",bt,"ET:",bt,"AIR:",Air,"DURM:",drum)






# import asyncio
# from websockets import connect

# async def hello(uri):
#     async with connect(uri) as websocket:
#         await websocket.send("Hello world!")
#         await websocket.recv()

# asyncio.run(hello("ws://localhost:8765"))



# import asyncio
# from websockets import serve

# async def echo(websocket):
#     async for message in websocket:
#         await websocket.send(message)

# async def main():
#     async with serve(echo, "localhost", 8765):
#         await asyncio.Future()  # run forever

# asyncio.run(main())