import re
import sys
import time
import datetime

import matplotlib.pyplot as plt
import serial
import threading
import queue
import subprocess

event_queue = queue.Queue() 

## DUT configs
SERIAL_PORT = "/dev/ttyUSB1"
BAUDRATE = 921600

## LASER configs
LASER_ADDRESS = "127.0.0.1"
LASER_PORT = 54321

TIMEOUT = 1.0

LINE_RE = re.compile(r"curr:\s*([0-9]+(?:\.[0-9]+)?)")

CMD_INIT  =	"INIT"
ACK_INIT  =	"INITACK"
CMD_START =	"STARTTEST"
ACK_START = "STARTACK"
CMD_EVENT =	"GETEVENTS"
CMD_STOP  =	"STOPTEST"
ACK_STOP  = "STOPACK"
CMD_END = "\n"

def init_socket():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    #print("Binding IP:" + TCP_HOST + ":" + str(TCP_PORT))
    server_socket.bind((TCP_HOST, TCP_PORT))
    server_socket.listen(1)
    return server_socket

def main(port=SERIAL_PORT, baud=BAUDRATE):
    dut_thread = threading.Thread(target=DUT_Communication, args=(port, baud))
    # laser_thread = threading.Thread(target=LASER_Communication, args=(LASER_ADDRESS, LASER_PORT))
    dut_thread.start()
    # laser_thread.start()
    dut_thread.join()
    # laser_thread.join()
    return 0


def DUT_Communication(port=SERIAL_PORT, baud=BAUDRATE):
    try:
        ser = serial.Serial(port, baud, timeout=TIMEOUT)
    except serial.SerialException as e:
        print(f"Cannot open serial port {port}: {e}", file=sys.stderr)
        return 1

    MA_WINDOW = 5
    times = []
    temps = []
    smooth_temps = []
    event_times = []
    start = time.time()

    plt.ion()
    fig, ax = plt.subplots()
    ax.set_title("Live RPU Temperature")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Temperature (°C)")
    ax.grid(True)

    f = open(f"laser_src/Experiments/experiment_{datetime.datetime.now().strftime('%Y-%m-%d_%H-%M-%S')}.txt", "x", buffering=1)

    line_count = 0
    while True:
        try:
            raw = ser.readline()
            if not raw:
                continue
            line = raw.decode(errors="replace").strip()
            if "Hello from Freertos example main" in line:
                event_times.append(time.time() - start)
            line_count += 1

            m = LINE_RE.search(line)
            if not m:
                event_queue.put(line)
                print(f"[DUT] {datetime.datetime.now()}: {line}")
                if "Flip at" in line:
                    subprocess.Popen(["paplay", "/usr/share/sounds/freedesktop/stereo/complete.oga"], shell=False)
                f.write(line + "\n")
                continue
            else:
                if(line_count % 3 == 0):
                    print(f"[DUT] {datetime.datetime.now()}: Alive", end="\r")
                    f.write(line + "\n")
                pass


            t = time.time() - start
            temp = float(m.group(1))

            times.append(t)
            temps.append(temp)

            window = temps[-MA_WINDOW:]
            avg_temp = sum(window) / len(window)
            smooth_temps.append(avg_temp)

            if len(times) > 1:
                ax.clear()
                ax.set_title("Live RPU Temperature")
                ax.set_xlabel("Time (s)")
                ax.set_ylabel("Temperature (°C)")
                ax.grid(True)
                ax.plot(times, smooth_temps, "-", markersize=4)

                for et in event_times:
                    ax.axvline(et, color="red", linestyle="--", linewidth=1, alpha=0.5)

                ax.set_xlim(0, t + 1)
                ax.set_ylim(min(smooth_temps) - 1, max(smooth_temps) + 1)
                fig.canvas.draw()
                fig.canvas.flush_events()

        except KeyboardInterrupt:
            print("Stopping.")
            break
        except Exception as e:
            print(f"Read error: {e}", file=sys.stderr)
            time.sleep(0.1)

    ser.close()
    f.close()

def LASER_Communication(address=LASER_ADDRESS, port=LASER_PORT):
    laser_socket = init_socket()
    
    # Send INIT command to the laser and wait for ACK
    laser_socket.send((CMD_INIT + CMD_END).encode())
    msg = laser_socket.recv(1024).decode()
    if msg == (ACK_INIT + CMD_END):
        print("[LASER] connected.")

    # Start a loop to orchestrate experiment
    try:
        while True:
            # Send start experiment command to the laser
            laser_socket.send((CMD_START + CMD_END).encode())
            msg = laser_socket.recv(1024).decode()
            if msg == (ACK_START + CMD_END):
                print("[LASER] experiment started.")

            while True:
                line = event_queue.get()  # Get the next line from the DUT communication
                if line == "STOPTEST":
                    laser_socket.send((CMD_STOP + CMD_END).encode())
                    msg = laser_socket.recv(1024).decode()
                    if msg == (ACK_STOP + CMD_END):
                        print("[LASER] experiment stopped.")
                        break
                elif line == "GETEVENTS":
                    laser_socket.send((CMD_EVENT + CMD_END).encode())
                    data = laser_socket.recv(1024).decode()
                    print(f"[LASER] Event count: {data.strip()}")

            print(data)
    except KeyboardInterrupt:
        print("Stopping.")
    finally:
        laser_socket.close()

if __name__ == "__main__":
    sys.exit(main())