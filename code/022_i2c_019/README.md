[toc]
### 001I2C协议与EEPROM
#### 1.1相关介绍
- I2C Bus——Inter-Integrated Circuit 即**集成电路总线**
- I2C总线端口为**漏开结构**，因此一般需要**加上拉电阻**Rp，通常选用5K~10KΩ
- I2C总线**以字节为单位**收发数据，一个字节为一帧，**数据传输的次序**为从**最高位到最低位**
- [x] I2C由**数据线（SDA）** 与 **时钟线（SCL）** 组成
 - **SDA**：Synchronous Data Adapter （同步数据衔接器）
 - **SCL**：Serial Communication Loop（串行通讯环路
#### 1.2硬件原理SCH
【主从结构：**一主[Master]多从[Slave]**】
![4bd4d9d902ef86bf7dc2cab7bd6ab397.png](en-resource://database/2560:1)

#### 1.3数据传输格式

![a18572c7da5f3e16f57e61883c6f2c51.png](en-resource://database/2604:1)

* [x] 主机写数据：
![60ff75edea88652bb52de34bbf5fcf18.png](en-resource://database/2584:1)

* [x] 主机读数据：
![38960b1d1d5291708c5f65b6cb768d5c.png](en-resource://database/2582:1)

#### 1.4信号传输时序图
![f06332419f3dbd2073708b674cacdbc8.png](en-resource://database/2608:1)

>- **SCL上拉电阻的作用**：在第9个时钟之后，双方中若**有一方处理还未完成**，它可以**一直把SCL拉低**，当**SCL一直为低电平**时，双方都**不应该操作SDA总线**。

##### 读写流程
- 1.前8个clk（7bit地址 + 1bit读/写）
   - 从设备不影响总线，从设备不去动三极管
   - 主设备决定数据
- 2.第9个CLK，由从设备决定数据
   - 主设备不驱动三极管
   - 从设备决定数据
       - SDA高：无ACK
       - SDA低：有ACK
#### 1.5信号解析
* [x] **起始**信号[Start]：在**SCL保持高电平**时，**SDA下降沿**。
* [x] 读写位：【0写1读】
* [x] **数据**位：**SCL低电平**时，**SDA可变化**；**SCL高电平**，**SDA不变**
* [x] **终止**信号：在**SCL高电平**时，**SDA上升沿**。
* [x] **回应**信号[ACK]：SDA低电平，低电平
![eb4677018e9f013e2bb919abba173269.png](en-resource://database/2586:1)




#### 1.6如何在SDA上实现双线传输

* [x] 1.主设备发送时，从设备不发送：可以通过SCL区分控制
* [x] 2.主设备发送时，从设备的”发送“引脚不影响数据：**使用开极电路**。
##### 开极电路
![22ad383b4820547e4c7c0044ff15bdac.png](en-resource://database/2578:1)

| A | B | SDA |
| --- | --- | :-: |
| 0 | 0 | 1（由上拉电阻决定） |
| 0 | 1 | 0 |
| 1 | 0 | 0 |
| 1 | 1 | 1 |
>- **SDA上拉电阻的作用**：当**A、B都低电平**时，三极管都不导通时，**SDA取决于外界电路**，当什么都不接时，**SDA悬空**，**无法获取电平**，故接上拉电阻。
***
##### 开极电路驱动思想
- 当某个设备**不想影响SDA总线**时，就**不驱动三极管**。
- 想**输出高电平**，都**不驱动三极管**。
- 想**输出低电平**，就**驱动三极管**。
***

### 002 S3C2440的IIC控制器
>一般主控芯片都有IIC控制器，没有IIC控制器的可以用**管脚模拟**
- IIC控制器简化了IIC操作

#### 2.1主从关系
|  | Master | Slave |
| --- | --- | --- |
| 写 | Transmitter | Receiver |
| 读 | Receiver | Transmitter |

#### 2.2IIC控制器流程框图
![493263bc8837641ad55701644a07fffb.png](en-resource://database/2602:1)

#### 2.3读写操作步骤
![50de97a22d85981eb839ed39a03e88f3.png](en-resource://database/2610:1)

在发送模式下，当数据已经发送出去后，IIC会等待IICDS寄存器接收新的数据
 
 - 1.IICDS = val；
 - 2.发完数据，产生中断，拉低SCL
 - 3.中断程序里，判断状态，IICDS = val2，IIC继续工作

在接收模式下，IIC接口会等待直到你把IICDS中的数据读走，在此期间，SCL拉低

- 1.发起传输，接收DATA
- 2.接收到数据之后，产生中断，SCL拉低
- 3.中断程序中，判断，设置中断，val = IICDS，IIC继续工作，继续接收收据

### 003 程序框架
![8e3877d603bf3d1c331b813aa5fe2ffe.png](en-resource://database/2616:1)

* [x] 框图解析

![568288374fd72328ca9e0e3314972266.png](en-resource://database/2618:1)

* [x] 各个文件的功能

![df32e10f59b74b5a5958a0279159e714.png](en-resource://database/2632:1)

![268a9a8e62569edde67cd30f4fbf84b2.png](en-resource://database/2614:1)

### 004 编程
![d901d4796d155c435ff1826e26c6c864.png](en-resource://database/2638:1)


![4f3c8120b2232ded3bc687e39f44dc9e.png](en-resource://database/2642:1)


