#!/usr/bin/env python3
"""ER2024 串口监视器 — 连开发板 CH340 (COM9, 115200-8N1)"""
import serial
import sys
import time

PORT = "COM9"
BAUD = 115200

def main():
    print(f"\n{'='*50}")
    print(f"  ER2024 Serial Monitor")
    print(f"  Port: {PORT}  Baud: {BAUD}  8N1")
    print(f"  Press Ctrl+C to quit")
    print(f"{'='*50}\n")

    try:
        ser = serial.Serial(PORT, BAUD, timeout=1)
        # 给板子一点启动时间
        time.sleep(0.5)

        import select
        while True:
            # 从串口读
            if ser.in_waiting:
                data = ser.read(ser.in_waiting)
                sys.stdout.write(data.decode("utf-8", errors="replace"))
                sys.stdout.flush()

            # 键盘输入 → 发给板子（测试 echo）
            if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
                ch = sys.stdin.read(1)
                if ch:
                    ser.write(ch.encode())

    except KeyboardInterrupt:
        print(f"\n\nBye.")
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        print("提示：确保板子已通过 Type-C USB 连接，且没被其他程序占用 COM9")
    finally:
        if "ser" in locals():
            ser.close()

if __name__ == "__main__":
    main()
