# HB_M2s/M6s 无线化改造项目 （完全无损化改造版本）


HB M2S/M6S roaster TC4 data to websocket with WIFI

## 前言
#### HB M2S/M6S系列的吐槽点
本人是HB（爱趣焙）M2S-e的用户，厂家对于M2S/M6S系列的机器已经貌似已经不更新，让其自生自灭的意思了，再加上app的功能是相当的智能，以至于我智商跟不上（没错，我是在吐槽！）。所以有这个项目HB虽然提供了Artisan方案，不过是有线的，还要搞个电脑再搞个数据线，太累赘了。
#### 基础知识点
我学习了一下HB M2S/M6S的设计，设计相当的有特点，我始终搞不明白为什这么设计的：
1. 数据传输的特点是：
   1. 主板实际上采集了4路温度数据，M2S只有三路（这个OK，三路是够用的啦）
   2. Artisan采用了TC4协议的数据格式。如：“27，0.0，0.0，C” 这样的。
   3. 通过CHAN；1300 和CHAN；2400 这个命令来切换获取数据格式。温度抽取就是不停的轮询数据。通讯方式如下：
>PC：CHAN；1300
>HB：OK
>PC：READ
>HB：27，0.0(ET)，0.0(BT),C
>PC:CHAN;2400
>HB:OK
>PC:READ
>HB:27,0.0(INLET),0.0(EXhust?),C //我手上没有M6 ，不确定是不是exhust


#### 基本功能

1. 将HB 系列的 USB输出的TC4协议的数据转化为websocket格式，再传输给artisan。
2. 硬件部分都是采用无损式改造，也就是随时可以恢复成出厂的样子，（不影响卖价）最多就是需要换一些扎带来理线。
3. 硬件零件都可以在X宝上买到。
4. 由于使用的是linux+python+wifi的基本架构，所以硬件部分的是可以按照自己手上硬件来配置。其实理想的方案是直接连接USB接口的，只是我才疏学浅。。。。对于 USB HOST 这个协议了解不深，所以才凑合使用。后期还会在linux服务器上搞些其他功能，毕竟全志H3+512M的性能整点转码发送的采集数据什么还是绰绰有余得很。
5. 基本上图方便用的都是root账号，要是有安全考的。对应的/root就改为自己的用户和策略走就好了。
   
 ## 硬件部分
 #### 买材料部分
1. linux小主机，需要有USB接口。比较好的选择如下：
- **Orangepi zero** （推荐，有USB有wifi 插卡就可以了，**记得要把散热器一并买了。官方是有一个套装的！**）[连接](https://item.taobao.com/item.htm?spm=a1z09.2.0.0.706f2e8dOUH9aR&id=670536815398&_u=g1ap2648ab1)
- 友善 nano pi   （有USB，没有Wi-Fi，需要自己接线,)
- 其他有USB 、有wifi、支持 Armbian的就可以了。
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

1. Armbian kernel 改kernel为5.10.60 以下 [固件下载连接]](https://www.armbian.com/orange-pi-zero/)
   具体方法如下：
   1. `sudo armbian-config`
   2. 菜单选择 system -> Other > 确认继续->kernel 选 5.10.43.然后让系统自己安装，完成后会自动重启。
   3. 再次进入`armbian-config`
   4. 菜单选择 system -> Freeze 锁定内核，这样在apt upgrade 的时候不会误伤kernel
2. 其他开发板的操作也是一样的流程。
   
3. python3  系统自带的是 3.9.2 
4. pip3     可以用以下命令安装
   >`sudo apt install python3-pip`
5. 需要安装的python库 ：pyserial(默认自带的)，asyncio，websockets，json，可以用以下命令安装
   >`sudo pip3 install setuptools pyserial asyncio websockets`

   安装完成后检查是否安装成功   
   > pip3 list 
6. 电脑上还需要一个Thonny 通过python（SSH）方式来上传代码、测试 还是有配置服务等等
   
#### Thonny的操作

[Thonny python IDE ssh登陆远程python演示](https://www.bilibili.com/video/BV1PA411d74k/?share_source=copy_web&vd_source=9c7b2e32c205ad765a4540d8b93a9eeb)

#### 自作成开机自动启动的服务

1. 设置固定的MAC地址：
    查询wifi对应的MAC 地址
    >nmcli  
    输出结果
    >wlan0: connected to rainly
        "ST-Ericsson Wi-Fi"
        wifi (xradio_wlan), 12:81:9E:A2:A1:DE, hw, mtu 1500 //记录MAC 地址，下一个命令会有用
    >

    执行这个命令来固定wifi网卡的MAC 地址 
    > nmcli con modify "your SSID" wifi.cloned-mac-address 12:81:9:EA2:A1:DE

2. 在　`/lib/systemd/system/`　新建　`HB_connect.service` **所有路径都用绝对路径**
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

    保存 ctrl+s 关闭 ctrl+X


3. 写个 /root/start.sh 脚本，用于启动HB_connect.py 的python3 程序
    >`sudo nano  /root/start.sh`

    复制黏贴以下内容

    >#!/bin/bash
    >cd "/root"
    >
    >#which python3 :/usr/bin/
    >/usr/bin/python3 HB_connect.py >> output_\$(date +"%Y%m%d_%H%M%S").log

    保存 ctrl+s 关闭 ctrl+X

4. 赋予start.sh 文件夹读写权限，这一步非常重要
    >`sudo chmod 777 -R *`  
    >`sudo systemctl enable HB_connect.service`

5. 查看有无报错
    >`sudo systemctl daemon-reload`

    >`sudo systemctl start HB_connect.service`

    >`sudo systemctl status HB_connect.service`

    如果显示不成功再执行

    >`sudo systemctl start HB_connect.service`

    >`sudo systemctl status HB_connect.service`



#### Artisan端的配置

1. [连接Artisan](https://www.bilibili.com/video/BV1et4y1w7i5/)
2. [Artisna简单操作](https://www.bilibili.com/video/BV1AV4y1L7yL/)
3. 配置文件见`/refer参考资料`里面的aset 文件。这导入artisan就OK了。
