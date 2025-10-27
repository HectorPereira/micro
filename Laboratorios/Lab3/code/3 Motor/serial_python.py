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

plt.style.use("dark_background")
plt.ion()

fig, ax = plt.subplots(facecolor="#2e2e2e")
line1, = ax.plot(t_data, ref_data, color="#FF00CC", label="Motor")
line2, = ax.plot(t_data, mot_data, color="#0091FF", label="Reference")
line3, = ax.plot(t_data, pwm_data, color="#FFFA66", label="PWM")

ax.set_ylim(0, 1023)
ax.set_xlabel("Samples", color="white")
ax.set_ylabel("ADC / PWM value", color="white")
ax.set_title("Real-time Arduino Serial Plot", color="white")
ax.legend(facecolor="#3e3e3e", edgecolor="white")

ax.set_facecolor("#2e2e2e")
for spine in ax.spines.values():
    spine.set_color("white")
ax.tick_params(colors="white")

plt.show()

# ==============================
# MAIN LOOP
# ==============================
while True:
    line = ser.readline().decode("ascii", errors="ignore").strip()
    if not line:
        continue

    parts = line.split()
    if len(parts) != 4:
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

    plt.pause(0.001)
