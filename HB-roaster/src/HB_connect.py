#!/usr/bin/python


#库引用
import asyncio
import time,serial,json,socket
import websockets


#全局变量定义
#定义USB 接口，
# port ='/dev/ttyACM0'  # ls /dev/tty* 查询
port = '/dev/tty.usbserial-14310'
# port = '/dev/ttyUSB0'
#定义buadrate
buadrate = 57600

#对象声明
#声明串口
serial_port = serial.Serial(port,buadrate,8,'N',1,timeout=0.5)

#定义Websocket 数据反写的dict 类 
class Data_JSON(object):
    def __init__(self,BT,ET,AT,INLET) : 
        self.BT = BT
        self.ET = ET 
        self.AT = AT
        self.INLET = INLET
        #字段顺序可以自定义，如修改顺序artisan对应的设置顺序也要修改


# 函数定义
# 字符串转换
def str2cmd(s):
    return bytes(s,'ascii')

# 获取本机IP地址
def get_host_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    finally:
        s.close()
        return ip


# 获取温度并用JSON序列化
def get_tempeture():
    RESULT_LINE = ''
    res1300 = ''
    res2400 = ''
    AT = 0.0
    BT = 0.0
    ET = 0.0
    Inlet = 0.0
    command = ''
    # port_ok_flag = serial_port.isOpen()
    # print("port is open:",port_ok_flag)

    if (serial_port.isOpen()) :
        #print("port_ok_flag:",port_ok_flag)

        command = 'CHAN;1300\n'
        serial_port.write(str2cmd(command))
        serial_port.flush()
        RESULT_LINE = serial_port.readline().decode('utf-8', 'ignore')[:-2] # for debug using
        # if (rl.startswith('#')):
        #     print("CHAN 1300:",rl)

        command = 'READ\n'
        serial_port.write(str2cmd(command))
        time.sleep(0.1)
        serial_port.flush()
        RESULT_LINE = serial_port.readline().decode('utf-8', 'ignore')[:-2]
        if (not len(RESULT_LINE) == 0 and not RESULT_LINE.startswith('#')):
            # print("READ 1300:",RESULT_LINE)
            res1300 = RESULT_LINE.rsplit(',')
            AT = float(res1300[0])
            ET = float(res1300[1])
            BT = float(res1300[2])

        command = 'CHAN;2400\n'
        serial_port.write(str2cmd(command))
        serial_port.flush()
       # time.sleep(0.1)
        RESULT_LINE = serial_port.readline().decode('utf-8', 'ignore')[:-2] # for debug using
        # if (rl.startswith('#')):
        #     print("CHAN 2400:",rl)

        command = 'READ\n'
        serial_port.write(str2cmd(command))
        time.sleep(0.1)
        serial_port.flush()
        RESULT_LINE = serial_port.readline().decode('utf-8', 'ignore')[:-2]
        if (not len(RESULT_LINE) == 0 and not RESULT_LINE.startswith('#')):
            # print("READ 2400: ",rl)
            res2400 = RESULT_LINE.rsplit(',')
            Inlet =float(res2400[1])


        # 打包数据成json格式
        p = Data_JSON(BT,ET,AT,Inlet)
        send_back = json.dumps(p.__dict__)
        return send_back


async def handler(websocket, path):
    while True:
        try:
            message = await websocket.recv()
            data = json.loads(message)
        # {"command": "getData", "id": 35632, "roasterID": 0}
        # {"data":{"BT":24.875,"ET":23.25},"id":35632}
        
            if data['command'] == 'getData':
           # print(data['command'])
           # get_tempeture(data['id']) # 获取一次温度数据

                send_back_json = '{"data":'+ get_tempeture() +',"id":'+ str(data['id']) + '}'
                # print(message)
                # print(send_back_json)
                await websocket.send(send_back_json)
            #elif data['command'] == 'getData':

                            
        except websockets.exceptions.ConnectionClosedError:
            print("Websockets is closed")
            break

        except websockets.exceptions.ConnectionClosedOK:
            print("Websockets is closed")
            break
    # else:
    #     await websocket.close()
    #     print("connection is closed")            
        # else:
        #     send_back_json = 'websokcet is closed'
        #     await websocket.send(send_back_json)
        #     # await websocket.close()
        
 
async def main():
    async with websockets.serve(handler, "", 8080):
        print("Websockets started")
        await asyncio.Future()  # run forever






#主程序
print("WebSockets server :",get_host_ip()) #显示本机IP地址
print("Serial Port:",serial_port.name)
print("Baudrate:",serial_port.baudrate)

asyncio.run(main())