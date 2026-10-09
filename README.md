# fin-proto-sse-bin-c

上交所(step)二进制行情协议 v0.2.0 的 C11 消息库,基于
[fin-proto-runtime-bin-c](../fin-proto-runtime-bin-c) 运行时。
对应 Rust 版本:[fin-proto-risk-bin-rs](https://github.com/xinchentechnote/fin-proto-sse-bin-rs)。

`src/` 下的全部 `.h/.c` 文件由 [fin-protoc](https://github.com/xinchentechnote/fin-protoc)
从 `proto/sse_bin_v0.2.0.pdsl` 生成(`-C` 指定输出目录),请勿手改:

```bash
make compile    # fin-protoc -f proto/sse_bin_v0.2.0.pdsl -C src/
```

## 线上格式(全部大端)

```
SseBinary:
┌──────────────┬────┬────────────────────────────────────────────────┐
│ MsgType u32  │ 4B │ 消息类型,决定 Body 的具体消息                      │
│ MsgSeqNum u64│ 8B │ 序列号                                          │
│ MsgBodyLen u32│ 4B│ Body 字节数(不含头部与校验和)                     │
│ Body         │变长│ 14 种消息之一,字段全部定长                         │
│ Checksum u32 │ 4B │ SSE_BIN 算法(除校验和外全部字节累加和 & 0xFF)      │
└──────────────┴────┴────────────────────────────────────────────────┘
```

C 版以 `kind` 枚举 + 匿名 union 表达 match 多态:

```c
sse_binary_sse_binary msg;
memset(&msg, 0, sizeof(msg));
msg.body_tag = SSE_BINARY_BODY_LOGON;
msg.body.logon.sender_comp_id = FC_STR("SENDER");
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
