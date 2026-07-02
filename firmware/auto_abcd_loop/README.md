# ER2024 F/G Stable Baseline

这个目录当前已经回退到 `firmware/bringup/main.c` 的稳定基线。

目的：

- 先恢复已经验证过的 `F` 白区直行到黑线。
- 先恢复已经验证过的 `G` 黑线循迹。
- 暂停新的 `A` 衔接策略，等讨论确认后再改。

当前 `main.c` 与稳定基线一致：

```text
firmware/bringup/main.c
firmware/auto_abcd_loop/main.c
```

当前 build tag：

```text
2026-07-02-auto-direct-pid
```

Keil 工程：

```text
firmware/auto_abcd_loop/keil/auto_abcd_loop_mspm0g3507_keil.uvprojx
```

命令行重建：

```bat
D:\Keil_v5\UV4\UV4.exe -r D:\steven\work\er2024\firmware\auto_abcd_loop\keil\auto_abcd_loop_mspm0g3507_keil.uvprojx -o D:\steven\work\er2024\firmware\auto_abcd_loop\keil\rebuild.log
```

当前重建结果：

```text
".\Objects\auto_abcd_loop_mspm0g3507_keil.axf" - 0 Error(s), 0 Warning(s).
```

烧录 hex：

```text
firmware/auto_abcd_loop/keil/Objects/auto_abcd_loop_mspm0g3507_keil.hex
```

下一步讨论 `A` 时，只在这份稳定基线上做最小改动。
