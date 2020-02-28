[toc]
### 001Nand flash操作原理
Nand Flash是一个**存储芯片**
那么：这样的操作很合理“读地址A的地址，把数据B写到地址A”
![0209c2de45b9d7071aee1407c3e449c9.png](en-resource://database/1772:1)

- 问：原理图上nand flash和S3C2440之间只有数据线，怎么传输地址
答：在DATA0~DATA7上既传输数据，又传输地址
     在**ALE为高电平时，传输的是地址**
     
- 问：从nand flash芯片手册可知，要操作nand flash需要先发出命令。怎么传入命令？
答：在**DATA0~DATA7上既传输数据，又传输地址，也传输命令**
     在**ALE为高电平时，传输的是地址**
     在**CLE为高电平时，传输的是命令**
     在**ALE和CLE都为低电平时，传输的是数据**

- 问：数据线既接到nand flash,也接到norflash，还接sdram，DM9000等等，怎么**避免干扰**？
答：这些设备，要访问就必须“**选中**”，没有选中的芯片不会工作，相当于没接

- 问：假设少些nand flash，把命令、地址、数据发给它之后，nand flash肯定不可能瞬间完成烧写，**怎么判断烧写完成**
答：通过**状态引脚RnB**来判断：它为**高电平表示就绪，它为低电平表示正忙。**

- 问：怎么操作NAND FLASH
答：根据nand flash芯片手册，一般的过程是：
发出命令
发出地址
发出数据/读数据


|  | nand flash | s3c2440 |
| --- | --- | --- |
| 发命令 | 选中芯片；CLE设为高电平；在DATA0~DATA7上输出命令值；发出一个写脉冲 | NFCMMD = 命令值 |
| 发地址 | 选中芯片；ALE设为高电平；在DATA0~DATA7上输出地址值；发出一个写脉冲 | NFADDR = 地址值 |
| 发数据 | 选中芯片；ALE、CLE设为低电平；在DATA0~DATA7上输出数据值；发出一个写脉冲 | NFDATA = 数据值  |
| 读数据 | 选中芯片；发出读麦种；读DATA0~DATA7的数据 | val = NFDATA |



#### 1.读ID
![31b93bb886a31ed6cbc237088a1ed138.png](en-resource://database/1778:1)

![b370068669e0c84fbcafe5af4424f9a0.png](en-resource://database/1776:1)
|  | S3C2440 | u-boot |
| --- | --- | --- |
| 选中 | NFCONT的bit1设为0 | md.l 0x4e000004 1;mw.l 0x4e00 0004 1 |
| 发出命令0x90 | MFCMMD = 0x90 | mw.b 0x4e000004 0x90 |
| 发出地址0x00 | NFADDR = 0x00 | mw.b 0x4e000004 0x00 |
| 读数据得到0xec | val = NFDATA | md.b 0x4e000004 1 |
| 读数据得到device code | val = NFDATA | md.b 0x4e000004 1 |
| 退出读ID的状态 | NFCMMD = 0xff | mw.b 0x4e00 0004 0xff |

#### 2.读内存：读0地址的数据
![6beaa32d601c4afa6d5bbfd5a8e84ce8.png](en-resource://database/1780:1)

使用UBOOT命令：
>nand dump 0
>Page 00000000 dump：
            >17 00 00 ea 14 f0 9f e5  14 f0 9f e5 14 f0 9f e5

|  | S3C2440 | u-boot |
| --- | --- | --- |
| 选中 | NFCONT的bit1设为0 | md.l 0x4e000004 1;mw.l 0x4e00 0004 1 |
| 发出命令0x00 | MFCMMD = 0x00 | mw.b 0x4e000004 0x00 |
| 发出命令0x00 | MFCMMD = 0x00 | mw.b 0x4e000008 0x00 |
| 发出命令0x00 | MFCMMD = 0x00 | mw.b 0x4e00000c 0x00 |
| 发出命令0x00 | MFCMMD = 0x00 | mw.b 0x4e00000c 0x00 |
| 发出地址0x00 | NFADDR = 0x00 | mw.b 0x4e00000c 0x00 |
| 读数据得到0x17 | val = NFDATA | md.b 0x4e000010 1 |
| 读数据得到0x00 | val = NFDATA | md.b 0x4e000010 1 |
| 读数据得到0x00 | val = NFDATA | md.b 0x4e000010 1 |
| 读数据得到0xea | val = NFDATA | md.b 0x4e000010 1 |
| ... | ... | ... |
| 退出读ID的状态 | NFCMMD = 0xff | mw.b 0x4e00 0008 0xff |

### 002nand flash时序及初始化
#### 

| 存储芯片的编程 | nand flash存储芯片 |
| --- | --- |
| 初始化 | 主控芯片的nand flash控制器 |
| 识别 | 读取ID |
| 读 | 一次读一个页（page） |
| 写 | 一次写一个页（page） |
| 擦除 | 一次擦除一个块（block） |
#### 1.设置nand flash 时序
![5ac18959fb6f5fa2c68a26258738e4e6.png](en-resource://database/1782:1)
#### 2.使能nand flash控制器
![d2c19b772daaba3baaf4ed337108560e.png](en-resource://database/1784:1)

### 003nandflash的芯片id读取
![5338552021307b8b4e902a31fc7097ad.png](en-resource://database/1788:1)

- [ ] bin文件9K，前4k被复制到？？？，后面没办法使用，代码烧写到nor，从nor启动

- 4th ID = 0x95 = 0b 1001 0101
![7e0bd239705b045f5b712fc93e538333.png](en-resource://database/1792:1)

### 004 nand flash数据读取
- 实现数据读取并从nand启动


|  |  |  |  |  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  |  |  |  |  |  |  |  |  |
| page 3 | 0 | 1 | ...... | 2046 | 2047 | 2048 | 2049 | ...... | 2111 |
| page 2 | 0 | 1 | ...... | 2046 | 2047 | 2048 | 2049 | ...... | 2111 |
| page 1 | 0 | 1 | ...... | 2046 | 2047 | 2048 | 2049 | ...... | 2111 |
| page 0 | 0 | 1 | ...... | 2046 | 2047 | 2048 | 2049 | ...... | 2111 |

>0 - 2047：2k的page data；
2048-2111：64k oob
![23b5e9be5a5f85bdbeac42403a9c2e40.png](en-resource://database/1794:1)

- 问：CPU读取nand flash上第2048个数据，它是哪一个
答：是page1的第0个byte



#### OOB（out of bank）
- nand flash缺点：它在读写数据的时候可能会发生错误（**位反转**），读数据的时候，可能有某一位是错误的，
- 写数据时，把**数据写进去**，同时会**生成校验码**，校验码写在**oob区** 中，当他读出一页数据的时候，里面可能有错误，**使用oob区中的校验码修正数据**。
- 是为了解决nand flash的缺点存在的
- cpu：只关心数据，不需要看到OOB
![d0ed03cf1e3573ba23b488298b3e6219.png](en-resource://database/1796:1)


#### 读flash
![770beab8f9e7af041e68ae73264c1ceb.png](en-resource://database/1798:1)

- 等待就绪寄存器
![d4e453c591b532441538f92b77e45dff.png](en-resource://database/1800:1)


* [X] 同一工程不同源文件应用**同一函数名**的解决方法
- c的所有全局变量都在同一个命名空间中，所以全局变量名字不能重复；可以试试把函数定义为内部的**static**



### 005 nandflash的擦除与烧写
- 实现nand_erase
![3d98fb9b0f4621d45142bd27b24a9a42.png](en-resource://database/1906:1)

- 实现nand_write
![9b79145fc650ff750ad06831f1f66bf1.png](en-resource://database/1904:1)

- 实现测试菜单