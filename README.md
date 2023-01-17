# HB_roaster_TC42websocket

HB M2S/M6S roaster TC4 data to websocket with WIFI

## 前言
本人是HB（爱趣焙）M2S-e的用户，之所以有这个项目，是因为厂家配的无线连接软件功能比不上Artisan来的灵活、强大和跟更新迭代快。再加上之前所有的烘焙记录（狙击手M1）都是在Artisan的，所以个人还是偏向于使用Artisan 。HB虽然提供了Artisan方案，不过是有线的，还要搞个电脑再搞个数据线，太累赘了。本着平头哥的性格，不服就干！研究了一下HB的设计，Artisan采用了TC4 的协议，所以用linux+python+wifi方式来实现转码和无线传输并不是一个太难和太耗资源的事情。

#### 基本功能

1. 将HB 系列的 USB输出的TC4协议的数据转化为websocket格式，再传输给artisan。
2. 硬件部分都是采用无损式改造，也就是随时可以恢复成出厂的样子，（不影响卖价）最多就是需要换一些扎带来理线。
3. 硬件零件都可以在X宝上买到。
4. 由于使用的是linux+python+wifi的基本架构，所以硬件部分的是可以按照自己手上硬件来配置。
   
 ## 硬件部分
 #### 买材料部分
1. linux小主机，需要有USB接口。比较好的选择如下：
- **Orangepi zero** （推荐，有USB有wifi 插卡就可以了，**记得要把散热器一并买了。官方是有一个套装的！**）[连接](https://item.taobao.com/item.htm?spm=a1z09.2.0.0.706f2e8dOUH9aR&id=670536815398&_u=g1ap2648ab1)
- 友善 nano pi   （有USB，没有Wi-Fi，需要自己接线
- 其他。有USB 、有wifi、支持 Armbian的就可以了。
2. TF 卡 ， 只要4GB以上就可以了。不过为了稳定，推荐大厂行车记录仪的那种卡。
   
3. USB数据线。 type A  方口 右弯。记住这个几个关键词就不会迷路。烘焙机主板的接口是用打印机那种 Type A 口，普通的直的会跟冷却风机怼不到一块去。必须用右弯头。 
   
4. wifi天线。Orangepi zero 用的 IPEX-K 这个型号的天线，可以买 25cm长的 IPEX-K型号的延长线或者15cm长的粘贴式软天线都可以。延长线就接烘焙机主板上天线转接头。不过对于手残党来说，粘贴式天线会更好操作。
     [IPEX连接线连接](https://detail.tmall.com/item.htm?_u=g1ap2646809&id=620154736020&spm=a1z09.2.0.0.706f2e8dOUH9aR)
     [粘贴式天线连接](https://item.taobao.com/item.htm?spm=a1z09.2.0.0.706f2e8dOUH9aR&id=615205217261&_u=g1ap264bc02)


5. usb供电线。micro USB,双头 50CM长的。
        [micarUSB供电线连接](https://detail.tmall.com/item.htm?_u=g1ap2644820&id=591913993020&spm=a1z09.2.0.0.706f2e8dOUH9aR)
6. 海绵双面胶、扎带、热熔胶
#### 安装   
   
## 软件部分
#### 操作系统   

1. Armbian [详情](https://www.armbian.com/orange-pi-zero/) bullseye or jammy 都可以
2. python3  系统自带的是 3.9.2 
3. pip3     可以用以下命令安装
   >`sudo apt install python3-pip`
4. 需要安装的python库 ：pyserial(默认自带的)，asyncio，websockets，json，可以用以下命令安装
   >`sudo pip3 install pyserial asyncio websockets json`

   安装完成后检查是否安装成功   
   > pip3 list 
5. 电脑上还需要一个Thonny 通过python（SSH）方式来上传代码、测试 还是有配置服务等等
   
#### Thonny的操作

[请看B站视频-录制中](https://www.armbian.com/orange-pi-zero/)

#### 自作成开机自动启动的服务
1. 在　`/lib/systemd/system/`　新建　`HB_connect.service` **所有路径都用绝对路径**
    >`sudo nano /lib/systemd/system/HB_connect.service`

   输入以下内容
    >[Unit]
    >Description=HB_connect_service
    >
    >[Service]
    >User=root
    >Group=root
    >
    >#这段非常重要，start.sh 是你开机要执行的shell 脚本,全部用绝对路径
    >ExecStart=/bin/bash /root/start.sh
    >ExecStop=/bin/kill $MAINPID
    >PrivateTmp=true
    >
    >[Install]
    >WantedBy=multi-user.target

    保存关闭


2. 写个 /root/start.sh 脚本，用于启动HB_connect.py 的python3 程序
    >`sudo nano  /root/start.sh`

    复制黏贴以下内容

    >#!/bin/bash
    >path=\$(cd \$(dirname \$0);pwd)
    >cd "$path"
    >
    >#which python3 :/usr/bin/
    >/usr/bin/python3 HB_connect.py >> output_\$(date +"%Y%m%d_%H%M%S").log

    保存关闭

3. 赋予start.sh 文件夹读写权限，这一步非常重要
    >`sudo chmod 777 -R *`  
    >`sudo systemctl enable HB_connect.service`

4. 查看有无报错
    >`sudo systemctl daemon-reload`

    >`sudo systemctl status HB_connect.service`

    如果显示不成功再执行

    >`sudo systemctl start HB_connect.service`
    
    >`sudo systemctl status HB_connect.service`



#### Artisan端的配置

1. [连接Artisan](https://www.bilibili.com/video/BV1et4y1w7i5/)
2. [Artisna简单操作](https://www.bilibili.com/video/BV1AV4y1L7yL/)
3. 配置文件见`/refer参考资料`里面的aset 文件。这导入artisan就OK了。