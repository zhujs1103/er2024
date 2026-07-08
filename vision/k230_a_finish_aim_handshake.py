import time
import os
import gc
from machine import PWM, FPIOA, Pin
from media.sensor import *
from media.display import *
from media.media import *

def main():
    W = 1280
    H = 720

    BLACK_THRESHOLD = [(0, 35)]

    # Laser spot aim point in the camera image.
    AIM_X = 640
    AIM_Y = 360

    CONTROL_X = True
    CONTROL_Y = True

    # K230 handshake pins for final A-point aiming.
    # START is driven by MSPM0 PB11. DONE drives MSPM0 PB14.
    START_GPIO = 3
    DONE_GPIO = 4

    FIRE_MS = 3000

    WAIT = 0
    AIMING = 1
    FIRING = 2
    DONE = 3

    fpioa = FPIOA()

    # Y servo: PWM4 / IO52
    fpioa.set_function(52, fpioa.PWM4)
    servo_y = PWM(4, 50, 7.5, enable=True)

    # X servo: PWM3 / IO47
    fpioa.set_function(47, fpioa.PWM3)
    servo_x = PWM(3, 50, 7.5, enable=True)

    # Laser: GPIO06
    fpioa.set_function(6, fpioa.GPIO6)
    laser = Pin(6, Pin.OUT, pull=Pin.PULL_NONE, drive=7)

    # START/DONE handshake.
    fpioa.set_function(START_GPIO, fpioa.GPIO3)
    start_pin = Pin(START_GPIO, Pin.IN, pull=Pin.PULL_DOWN)
    fpioa.set_function(DONE_GPIO, fpioa.GPIO4)
    done_pin = Pin(DONE_GPIO, Pin.OUT, pull=Pin.PULL_NONE, drive=7)

    LASER_ON = 1
    LASER_OFF = 0
    laser.value(LASER_OFF)
    done_pin.value(0)

    x_duty = 10.3
    y_duty = 7.5
    servo_x.duty(x_duty)
    servo_y.duty(y_duty)

    X_DIR = -1
    Y_DIR = -1

    STEP_X = 0.03
    STEP_Y = 0.03

    DEAD_X = 30
    DEAD_Y = 30

    X_MIN = 5.0
    X_MAX = 15.0
    Y_MIN = 5.0
    Y_MAX = 10.0

    AIM_HOLD_FRAMES = 5
    aim_count = 0

    # Fallback when the target is not found.
    LEFT_TURN_ANGLE = 120
    LEFT_TURN_DUTY = 15.0
    LEFT_TURN_STEP = 0.04


    def clamp(v, lo, hi):
        if v < lo:
            return lo
        if v > hi:
            return hi
        return v


    def ticks_elapsed(start_ms):
        return time.ticks_diff(time.ticks_ms(), start_ms)


    def start_active():
        return start_pin.value() != 0


    def set_safe_idle():
        nonlocal aim_count
        aim_count = 0
        laser.value(LASER_OFF)
        done_pin.value(0)


    def select_target(blobs):
        target = None
        best_score = 0

        for b in blobs:
            x, y, w, h = b.rect()

            if w < 80 or h < 50:
                continue

            ratio = w / h
            if ratio < 1.0 or ratio > 3.5:
                continue

            density = b.pixels() / (w * h)
            if density > 0.75:
                continue

            score = w * h
            if score > best_score:
                best_score = score
                target = b

        return target


    def draw_aim_mark(img):
        img.draw_cross(AIM_X, AIM_Y, color=255)
        img.draw_rectangle(AIM_X - 25, AIM_Y - 25, 50, 50, color=255)
        img.draw_string(AIM_X + 8, AIM_Y + 8, "AIM", color=255, scale=1)


    sensor = Sensor(width=W, height=H)
    sensor.reset()
    sensor.set_framesize(width=W, height=H)
    sensor.set_pixformat(Sensor.GRAYSCALE)

    Display.init(Display.VIRT, width=W, height=H, fps=30, to_ide=True)
    MediaManager.init()
    sensor.run()

    print("A finish aim handshake ready: START=GPIO03 input, DONE=GPIO04 output, fire=3000ms")

    mode = WAIT
    fire_start_ms = 0
    frame_count = 0

    try:
        while True:
            os.exitpoint()
            img = sensor.snapshot()

            if mode == WAIT:
                set_safe_idle()
                draw_aim_mark(img)
                img.draw_string(10, 10, "WAIT START", color=255, scale=2)
                img.draw_string(10, 40, "DONE LOW LASER OFF", color=255, scale=2)

                if start_active():
                    aim_count = 0
                    mode = AIMING
                    print("START high, aiming")

            elif mode == AIMING:
                if not start_active():
                    set_safe_idle()
                    mode = WAIT
                    print("START low, abort aiming")
                else:
                    blobs = img.find_blobs(
                        BLACK_THRESHOLD,
                        pixels_threshold=1500,
                        area_threshold=2500,
                        merge=True,
                        margin=20,
                    )
                    target = select_target(blobs)
                    draw_aim_mark(img)

                    if target:
                        x, y, w, h = target.rect()
                        tx = x + w // 2
                        ty = y + h // 2
                        err_x = tx - AIM_X
                        err_y = ty - AIM_Y
                        aligned = abs(err_x) <= DEAD_X and abs(err_y) <= DEAD_Y

                        img.draw_rectangle(target.rect(), color=255)
                        img.draw_cross(tx, ty, color=255)
                        img.draw_line(AIM_X, AIM_Y, tx, ty, color=255)

                        x_msg = "X OK"
                        y_msg = "Y OK"

                        if abs(err_x) > DEAD_X:
                            if err_x > 0:
                                x_duty += X_DIR * STEP_X
                                x_msg = "target RIGHT"
                            else:
                                x_duty -= X_DIR * STEP_X
                                x_msg = "target LEFT"

                            x_duty = clamp(x_duty, X_MIN, X_MAX)
                            if CONTROL_X:
                                servo_x.duty(x_duty)

                        if abs(err_y) > DEAD_Y:
                            if err_y > 0:
                                y_duty += Y_DIR * STEP_Y
                                y_msg = "target DOWN"
                            else:
                                y_duty -= Y_DIR * STEP_Y
                                y_msg = "target UP"

                            y_duty = clamp(y_duty, Y_MIN, Y_MAX)
                            if CONTROL_Y:
                                servo_y.duty(y_duty)

                        if aligned:
                            aim_count += 1
                            img.draw_string(10, 10, "AIM OK", color=255, scale=2)
                            img.draw_rectangle(AIM_X - 40, AIM_Y - 40, 80, 80, color=255)
                        else:
                            aim_count = 0
                            img.draw_string(10, 10, "AIMING", color=255, scale=2)

                        img.draw_string(10, 40, "target:%d,%d" % (tx, ty), color=255, scale=2)
                        img.draw_string(10, 70, "err:%d,%d" % (err_x, err_y), color=255, scale=2)
                        img.draw_string(10, 100, "x:%.2f y:%.2f" % (x_duty, y_duty), color=255, scale=2)
                        img.draw_string(10, 130, x_msg, color=255, scale=2)
                        img.draw_string(10, 160, y_msg, color=255, scale=2)
                        img.draw_string(10, 190, "aim_count:%d" % aim_count, color=255, scale=2)

                        if frame_count % 3 == 0:
                            print(
                                "aim:",
                                AIM_X,
                                AIM_Y,
                                "target:",
                                tx,
                                ty,
                                "err:",
                                err_x,
                                err_y,
                                "x:",
                                x_duty,
                                "y:",
                                y_duty,
                                "aim_count:",
                                aim_count,
                            )

                        if aim_count >= AIM_HOLD_FRAMES:
                            laser.value(LASER_ON)
                            fire_start_ms = time.ticks_ms()
                            mode = FIRING
                            print("aim stable, laser on")

                    else:
                        aim_count = 0
                        laser.value(LASER_OFF)

                        if x_duty < LEFT_TURN_DUTY:
                            x_duty = clamp(x_duty + LEFT_TURN_STEP, X_MIN, LEFT_TURN_DUTY)
                        elif x_duty > LEFT_TURN_DUTY:
                            x_duty = clamp(x_duty - LEFT_TURN_STEP, LEFT_TURN_DUTY, X_MAX)

                        if CONTROL_X:
                            servo_x.duty(x_duty)

                        img.draw_string(10, 10, "NO TARGET", color=255, scale=2)
                        img.draw_string(10, 40, "LEFT %d x:%.2f" % (LEFT_TURN_ANGLE, x_duty), color=255, scale=2)
                        img.draw_string(10, 70, "LASER OFF", color=255, scale=2)

                        if frame_count % 10 == 0:
                            print("no target, left turn:", LEFT_TURN_ANGLE, "x:", x_duty)

            elif mode == FIRING:
                if not start_active():
                    set_safe_idle()
                    mode = WAIT
                    print("START low, abort firing")
                else:
                    laser.value(LASER_ON)
                    draw_aim_mark(img)
                    remain = FIRE_MS - ticks_elapsed(fire_start_ms)
                    if remain < 0:
                        remain = 0
                    img.draw_string(10, 10, "FIRING", color=255, scale=2)
                    img.draw_string(10, 40, "remain:%dms" % remain, color=255, scale=2)

                    if ticks_elapsed(fire_start_ms) >= FIRE_MS:
                        laser.value(LASER_OFF)
                        done_pin.value(1)
                        mode = DONE
                        print("fire complete, DONE high")

            elif mode == DONE:
                laser.value(LASER_OFF)
                done_pin.value(1)
                draw_aim_mark(img)
                img.draw_string(10, 10, "DONE HIGH", color=255, scale=2)
                img.draw_string(10, 40, "WAIT START LOW", color=255, scale=2)

                if not start_active():
                    set_safe_idle()
                    mode = WAIT
                    print("START low, DONE low, ready")

            Display.show_image(img)
            frame_count += 1
            gc.collect()
            time.sleep_ms(30)

    finally:
        laser.value(LASER_OFF)
        done_pin.value(0)
main()
