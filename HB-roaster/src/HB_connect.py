#库引用
import asyncio

import time,serial,json,socket
# import websockets #pip3 install websockets
# import  websockets
import websockets

#全局变量定义
# port ='/dev/ttyAMA0'
# serial_port = '/dev/cu.usbmodem00001'
serial_port = '/dev/cu.usbserial-14310'
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


# 函数定义
# 字符串转换
def str2cmd(s):
    return bytes(s,'ascii')

# 获取本机IP地址
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

def get_tempeture():
    if (port_ok_flag) :
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
            # print("READ 1300:",rl)
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
            # print("READ 2400: ",rl)
            res2400 = rl.rsplit(',')
            et = float(res2400[0])
            drum =float(res2400[1])
        
        print("BT:",bt,"ET:",bt,"AIR:",Air,"DURM:",drum)
    #asyncio.run(main())


async def echo(websocket):
    async for message in websocket:
        await websocket.send(message)


async def handler(websocket, path):
    while True:
        message = await websocket.recv()
        get_tempeture() # 获取一次温度数据
        print(message)
 
async def main():
    async with websockets.serve(handler, "", 8080):
        await asyncio.Future()  # run forever






#主程序

port_ok_flag = serial_port.isOpen()

print(get_host_ip()) #显示本机IP地址

asyncio.run(main())

# async def counter(websocket, path):
#     # register(websocket) sends user_event() to websocket
#     await register(websocket)
#     try:
#         await websocket.send(state_event())
#         async for message in websocket:
#             data = json.loads(message)
#             if data['action'] == 'minus':
#                 STATE['value'] -= 1
#                 await notify_state()
#             elif data['action'] == 'plus':
#                 STATE['value'] += 1
#                 await notify_state()
#             else:
#                 logging.error(
#                     "unsupported event: {}", data)
#     finally:
#         await unregister(websocket)
# //更多请阅读：https://www.yiibai.com/websocket/python-websockets-library.html