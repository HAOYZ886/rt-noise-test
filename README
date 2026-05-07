# rt-noise-test

一组 Linux 内核模块，用来往指定 CPU 上"丢噪声"——模拟不同类型的时间延迟，方便你观察实时任务的反应。

搞 RT 系统的人应该都用得上：想看看中断处理太重会怎样，或者临界区关抢占太久会怎样，直接装模块跑一下就行。

## 项目结构

```
rt-noise-test/
├── irq-noise/          # 硬中断延迟模块
│   ├── irq-noise.c
│   └── Makefile
├── preempt-noise/      # 关抢占延迟模块
│   ├── preempt-noise.c
│   └── Makefile
└── README
```

---

## 编译

两个模块都是 ARM64 交叉编译，Makefile 里的路径是占位符，先改成你自己的：

```bash
# 可以直接传参覆盖
make -C irq-noise KERNELDIR=/path/to/your/kernel \
    ARCH=arm64 \
    CROSS_COMPILE=/path/to/toolchain/bin/aarch64-linux-gnu-
```

或者改 Makefile 里的 `SDKDIR`：

```makefile
SDKDIR ?= /path/to/your/sdk/
```

然后分别编译：

```bash
cd irq-noise && make
cd ../preempt-noise && make
```

生成的 `.ko` 文件 `insmod` 到目标板用。

---

## irq-noise

### 这是干啥的

用 hrtimer 在**硬中断上下文**里跑 `udelay()`，模拟一个又重又慢的中断处理程序。

关键点：
- 定时器模式 `HRTIMER_MODE_REL_HARD`，保证回调走硬中断路径
- 用 `smp_call_function_single` 把定时器钉死在指定 CPU 上
- 往 sysfs 写值就触发，写完立等可取（同步执行）

### 参数

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `cpu` | 模块参数 | 0 | 定时器跑在哪个 CPU 上 |
| `latency_us` | sysfs 可写 | 0 | 延迟微秒数，写下去立刻触发，上限 1 秒 |

### 用法

```bash
# 加载到 CPU 1
insmod irq-noise.ko cpu=1

# 扔 500 us 的硬中断延迟
echo 500 > /sys/module/irq_noise/parameters/latency_us

# 看日志确认
dmesg | tail
# irq_noise: Burned 500 us in Hard IRQ context on CPU1
```

---

## preempt-noise

### 这是干啥的

创建一个内核线程，在**关抢占**的区间里跑 `udelay()`，模拟长时间占着锁不放的临界区。

关键点：
- 线程默认设 `SCHED_FIFO` 实时优先级，抢占不开门
- 线程固定到指定 CPU，不会跑到别的核上去
- `latency_us` 写 0 的话，改成"抢占开启"模式跑 500 us，方便对照组实验
- 每次写入都会新起一个线程，跑完就退，不残留

### 参数

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `cpu` | 模块参数 | 0 | 线程绑到哪个 CPU |
| `rt_priority` | 模块参数 / sysfs | 50 | SCHED_FIFO 优先级（-1 ~ 99，-1 表示非实时） |
| `latency_us` | sysfs 可写 | 0 | 延迟微秒数，写下去就起线程，上限 1 秒 |

### 用法

```bash
# 加载到 CPU 2，实时优先级设 80
insmod preempt-noise.ko cpu=2 rt_priority=80

# 注入 1 ms 关抢占延迟
echo 1000 > /sys/module/preempt_noise/parameters/latency_us

dmesg | tail
# preempt_noise: Burned 1000 us in Preempt-Off context on CPU2

# 写 0 切到"抢占开启"模式
echo 0 > /sys/module/preempt_noise/parameters/latency_us
# preempt_noise: Burned 500 us in Preempt-On context on CPU2
```

### 额外注意
需要检查此模块用到的 sched_setscheduler_nocheck() 是否被内核设定为 GPL 外部引出，否则会编译失败。

---

## 两个模块对比

| 维度 | irq-noise | preempt-noise |
|------|-----------|---------------|
| 执行上下文 | 硬中断（hard IRQ） | 进程上下文 |
| 抢占状态 | 不适用（中断不可抢占） | 关抢占（preempt_disable） |
| 触发方式 | sysfs 写入，同步阻塞 | sysfs 写入，异步起线程 |
| 每次触发 | 回调一次性执行 | 线程跑完就退 |
| 适合模拟 | 中断处理太重、中断风暴 | 临界区太长、spinlock 持有过久 |

---

## 提醒

- **别在生产机器上乱跑。** 这玩意就是来制造延迟的，跑上去系统响应肯定受影响。
- `latency_us` 写下去立刻就生效，如果搞个接近 1 秒的值，小心系统卡住。
- 目前一个模块实例只能往一个 CPU 上丢噪声。要测多核场景，可以加载多个实例。
- Makefile 里的 SDK、内核、工具链路径都要改成你自己的实际路径。

