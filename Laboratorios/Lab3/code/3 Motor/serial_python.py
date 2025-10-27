import serial
import time
import matplotlib.pyplot as plt
from collections import deque

# ==============================
# CONFIGURATION
# ==============================
PORT = "COM12"          
BAUD = 9600
WINDOW = 100            

# ==============================
# SETUP
# ==============================
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)  

ref_data = deque([0]*WINDOW, maxlen=WINDOW)
mot_data = deque([0]*WINDOW, maxlen=WINDOW)
pwm_data = deque([0]*WINDOW, maxlen=WINDOW)
t_data = deque(range(WINDOW), maxlen=WINDOW)

plt.ion()
fig, ax = plt.subplots()
line1, = ax.plot(t_data, ref_data, label="Reference")
line2, = ax.plot(t_data, mot_data, label="Motor")
line3, = ax.plot(t_data, pwm_data, label="PWM Output")

ax.set_ylim(0, 1023)
ax.set_xlabel("Samples")
ax.set_ylabel("ADC / PWM value")
ax.set_title("Real-time Arduino Serial Plot")
ax.legend()
plt.show()

# ==============================
# MAIN LOOP
# ==============================
while True:
    try:
        line = ser.readline().decode("ascii", errors="ignore").strip()
        if not line:
            continue

        # Expect: "ref motor pwm"
        parts = line.split()
        if len(parts) != 3:
            continue

        ref_val = int(parts[0])
        mot_val = int(parts[1])
        pwm_val = int(parts[2])

        ref_data.append(ref_val)
        mot_data.append(mot_val)
        pwm_data.append(pwm_val)

        line1.set_ydata(ref_data)
        line2.set_ydata(mot_data)
        line3.set_ydata(pwm_data)

        ax.relim()
        ax.autoscale_view(True, True, True)
        plt.pause(0.001)

    except KeyboardInterrupt:
        print("\nExiting...")
        break
    except Exception as e:
        print("Error:", e)
        continue

ser.close()
