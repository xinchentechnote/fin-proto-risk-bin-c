# fin-proto-risk-bin-c

RBP(Risk Binary Protocol)风控二进制协议 v0.2.0 的 C11 消息库,用于
RGW 网关 ↔ 风控引擎的点对点内部通信,基于
[fin-proto-runtime-bin-c](https://github.com/xinchentechnote/fin-proto-runtime-bin-c) 运行时。
对应 Rust 版本:[fin-proto-risk-bin-rs](https://github.com/xinchentechnote/fin-proto-risk-bin-rs)。

`src/` 下的全部 `.h/.c` 文件由 [fin-protoc](https://github.com/xinchentechnote/fin-protoc)
从 `proto/risk_v0.2.0.pdsl` 生成(`-C` 指定输出目录),请勿手改:

```bash
make compile    # fin-protoc -f proto/risk_v0.2.0.pdsl -C src/
```

## 线上格式(帧头定长 32B,全部大端)

```
RbpBinary:
┌──────────────┬────┬────────────────────────────────────────────────┐
│ MsgType u32  │ 4B │ 消息类型,决定 Body 的具体消息                      │
│ Version u16  │ 2B │ 协议版本,首版 = 1                                │
│ Flags u16    │ 2B │ bit0=REPLY_EXPECTED bit2=DROPPABLE               │
│              │    │ bit3=NO_TEXT bit4=NO_CHECKSUM                    │
│ MsgSeq u64   │ 8B │ 发送方会话内单调递增(从 1 开始)                     │
│ TsNs u64     │ 8B │ 发送时刻(纳秒,同源时钟 NTP/PTP)                    │
│ BodyLen u32  │ 4B │ Body 字节数                                       │
│ Checksum u32 │ 4B │ Body 字节累加和 & 0xFF,由应用层计算/校验            │
│              │    │ (flags.NO_CHECKSUM 时置 0 跳过)                    │
│ Body         │变长│ 9 种消息之一,字段全部定长                          │
└──────────────┴────┴────────────────────────────────────────────────┘
```

内部协议不含登录/心跳/会话管理消息,会话由传输层(TCP/SHM)承担。9 种消息:
NewOrderReq、CancelReq、OrderConfirm、TradeReport、CancelReject、OrderReject、
RiskResult、Instruction、InstrAck。

C 版以 `kind` 枚举 + 匿名 union 表达 match 多态:

```c
rbp_binary_rbp_binary msg;
memset(&msg, 0, sizeof(msg));
msg.body_tag = RBP_BINARY_BODY_NEW_ORDER_REQ;
msg.body.new_order_req.cl_ord_id = FC_STR("C0001");
```

## 使用

```bash
make test      # 生成代码自带的 round-trip 测试(每个消息一个)
make bench     # 与 Rust/Zig 同口径的编解码基准
```

与 fin-proto-risk-bin-rs 逐字节兼容(相同输入编码出相同报文)。解码零拷贝:
字符串借用输入缓冲,仅 `repeat` 字段分配外层数组,`<msg>_free` 负责释放。

## 依赖

编译时引用运行时头文件与源文件(可用 `RUNTIME=` 覆盖路径):

```make
RUNTIME ?= ../fin-proto-runtime-bin-c
$(CC) $(CFLAGS) -I$(RUNTIME)/include -Isrc src/*.c $(RUNTIME)/src/*.c ...
```
