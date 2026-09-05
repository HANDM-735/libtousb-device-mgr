# libtousb-device-mgr 架构图

## 一句话理解
这是一个 **C++ 客户端库**：负责通过 **TCP Socket** 连接后台的“设备管理器服务端”(Device Manager Server)，由该服务端再通过 **USB** 去驱动各种硬件板卡（PGB/PPS/SYNC/ASIC/DIG 等）。本库本身**不直接管 USB**，它只把用户的请求封装成二进制报文发出去并等待应答。

---

## 一、分层架构总览

```
┌───────────────────────────────────────────────────────────────────────┐
│                    ① 测试/调用层   main.cpp                          │
│        交互式命令行测试程序（做一些功能演示、打印结果用）               │
└───────────────────────────────┬───────────────────────────────────────┘
                                │ 调用 C 语言接口 (extern "C")
┌───────────────────────────────▼───────────────────────────────────────┐
│                    ② 对外 API 层   lib_interface.{h,cpp}             │
│   usb_init / usb_ota_* / usb_fetch_real_data / DIG_Get* ...          │
│   （一堆薄封装函数，全部委托给下方 mgr_session 单例）                  │
└───────────────────────────────┬───────────────────────────────────────┘
┌───────────────────────────────▼───────────────────────────────────────┐
│                    ③ 会话管理层                                       │
│   mgr_session (单例管理器) ───► xsession                              │
│   • 给每个请求分配 session id，等待并匹配应答                          │
│   • 按应答类型分发：OTA / 实时数据 / 板信息 / 寄存器 / 电源等           │
└───────────────────────────────┬───────────────────────────────────────┘
┌───────────────────────────────▼───────────────────────────────────────┐
│                    ④ 网络层                                          │
│   mgr_network (单例，boost::asio xtcp_client)                        │
│   • 建连设备管理器服务端、心跳(heartbeat)、发送/回调                 │
└───────────────────────────────┬───────────────────────────────────────┘
┌───────────────────────────────▼───────────────────────────────────────┐
│                    ⑤ 协议/报文层                                      │
│   usbadapter_package.{h,cpp}  usbadapter_bin_packer/unpacker         │
│   • 二进制报文封装/解析（包头 0xFFA5，14 字节头）                       │
│   • 报文类型：REQUEST / NOTIFY / RESPOND；多种命令字                   │
└───────────────────────────────┬───────────────────────────────────────┘
┌───────────────────────────────▼───────────────────────────────────────┐
│                    ⑥ 通用框架/基础库层                                │
│   xbasicmgr.hpp  定时工作线程+单例管理(xmgr_basic)                     │
│   xpackage.hpp   报文基类(xpacket)                                     │
│   xsession.h     请求-应答同步机制(xrequest)                           │
│   xbasicasio.hpp TCP/UDP socket 封装                                   │
│   xconfig.hpp    配置加载                                             │
│   mgr_log        日志管理                                             │
└───────────────────────────────┬───────────────────────────────────────┘
                                │ TCP Socket
┌───────────────────────────────▼───────────────────────────────────────┐
│              ⑦ 设备管理器服务端 (adapter_server)                      │
│            config 中 adapter_server = ip:port (如172.16.4.33:9000)    │
└───────────────────────────────┬───────────────────────────────────────┘
                                │ USB 协议
┌───────────────────────────────▼───────────────────────────────────────┐
│              ⑧ 硬件板卡                                               │
│      PGB板 / PPS板 / SYNC板 / ASIC板 / DIG板 / 监控板 ...              │
└───────────────────────────────────────────────────────────────────────┘
```

---

## 二、代码目录与职责映射

| 目录/文件 | 对应层 | 作用 |
|-----------|--------|------|
| `main.cpp` | ① | 测试/命令行交互程序，演示各 API |
| `libusb/lib_interface.{h,cpp}` | ② | **对外提供的 C API**（最关键，用户只需看这个头文件） |
| `libusb/mgr_session.{h,cpp}` | ③ | 会话管理器单例，接收 API 调用并发报文 |
| `libusb/xsession.{h,cpp}` | ③ | 具体请求-应答同步逻辑，匹配 session id |
| `libusb/mgr_network.{h,cpp}` | ④ | 网络管理器单例，TCP 收发、心跳 |
| `libusb/xbasicasio.hpp` | ④⑥ | boost::asio 的 TCP/UDP 客户端/服务端封装 |
| `libusb/usbadapter_package.{h,cpp}` | ⑤ | USB 适配器二进制报文打包/解包 |
| `libusb/xpackage.hpp` | ⑥ | 报文基类 `xpacket` |
| `libusb/xbasicmgr.hpp` | ⑥ | 单例 + 后台循环工作线程框架 |
| `libusb/xbasic.hpp` | ⑥ | 基础工具（调试输出、字符串处理等） |
| `libusb/xconfig.hpp` | ⑥ | 读取 `config.ini` 等配置 |
| `libusb/mgr_log.{h,cpp}` | ⑥ | 日志 |
| `libusb/xconvert.hpp` | ⑥ | 报文字节串 ↔ 结构体 转换（温度/电压/状态等） |
| `libjson/` | 依赖 | cJSON 库（用于解析 JSON 报文体） |
| `mgr_shm.{h,cpp}` / `linux_shm.{h,cpp}` | 辅助 | **共享内存通道**：读写 ASIC 板 AD9528 寄存器（不经过 TCP，用 SysV 共享内存） |
| `config/*.ini` | 配置 | `config.ini`(服务器地址等)、`g_libtousb.conf`、`libusb_conf.ini` |
| `script/upgrade_ip.sh` | 脚本 | 升级 IP 的部署脚本 |
| `CMakeLists.txt` / `build.sh` | 构建 | 编译、打包、生成 release |

---

## 三、一次请求的调用链（例：OTA 升级）

```
main.cpp  ─►  usb_ota_start_upgrade()
                │
                ▼                    （② lib_interface.cpp，薄封装）
        mgr_session::get_instance()->call_ota_start_upgrade()
                │
                ▼                    （③ mgr_session → xsession）
        xsession 分配 session id，构造 xusbadapter_package 请求包
                │
                ▼                    （④ mgr_network）
        mgr_network->send_to_adapter()  →  xtcp_client 发送
                │         │
                │         │                 TCP
                ▼         ▼              Socket ───►  设备管理器服务端
        线程内 work() 等待应答、心跳          （服务端负责真正操作 USB 板卡）
                │
                ▼                    （⑤ 收到 RESPOND 报文）
        xsession::handle_*_response() 匹配 session，Semaphore 唤醒调用者
                ▼
        API 返回结果给 main.cpp
```

> 整体是典型的 **同步阻塞请求-应答**：调用线程发请求后阻塞，后台线程收到对应应答（按 session id 匹配）后用信号量唤醒调用线程。

---

## 四、两条数据通路

1. **TCP 主通路**（绝大多数操作）
   板卡信息、OTA 升级、实时数据、电源控制、寄存器读写、频率设置……全部走 Socket → 服务端 → USB。

2. **共享内存通路**（仅 ASIC 板 AD9528 寄存器）
   `mgr_shm/linux_shm` 用 SysV 共享内存 (`ftok/shmat/shmdt`) 直接读写，构造一个 `asic_msg` 结构供外部进程交换 ASIC 寄存器读写请求/应答。

---

## 五、重点：你只需要看这个头文件

**`libusb/lib_interface.h`** 是这个库的“说明书”，包含：

```cpp
int usb_init(const char* adapter_srv);         // 初始化，指定服务端 ip:port
int usb_ota_start_upgrade(...);                // OTA 升级
int usb_ota_query_progress(...);               // 查询升级进度
struct real_data* usb_fetch_real_data(...);    // 获取实时数据
int DIG_GetBoardExist(...);                    // 板卡是否存在
int DIG_GetBoardID/SN/Temp/Vol(...);           // 板卡信息查询
int DIG_GetClockChipState(...);                // 时钟锁定状态
int DIG_GetPwmFrequency(...);                  // PWM 频率
int DIG_GetSerdesState(...);                   // serdes 状态
int DIG_GetUtpRegister(...);                   // UTP 芯片寄存器
...
```

里面的 `board_type`（板卡类型）和 `ota_type`（升级类型）两个大的枚举开头，把支持的板子和升级类型列得非常清楚。

---

## 六、板卡类型速查

| 类型码 | 含义 | 类型码 | 含义 |
|--------|------|--------|------|
| 0x11 | CP_SYNC | 0x21 | FT_SYNC |
| 0x12 | CP_PGB | 0x22 | FT_PGB |
| 0x13 | CP_DPS | 0x23 | FT_PPS |
| 0x14 | CP_PEM | 0x2B | FT_DIG |
| 0x15 | CP_RCA | 0x20 | ASIC |
| 0x1B | CP_DIG | 0x40/0x4F | TH/MF_MONITOR 监控板 |