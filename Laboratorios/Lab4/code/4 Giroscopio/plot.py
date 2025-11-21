import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque

# =====================
# USER SETTINGS
# =====================
PORT = "COM9"       # <-- change this to your USB serial port
BAUD = 9600
MAX_POINTS = 200     # number of points to keep visible

# =====================
# BUFFERS
# =====================
ax_buf = deque(maxlen=MAX_POINTS)
ay_buf = deque(maxlen=MAX_POINTS)
az_buf = deque(maxlen=MAX_POINTS)

gx_buf = deque(maxlen=MAX_POINTS)
gy_buf = deque(maxlen=MAX_POINTS)
gz_buf = deque(maxlen=MAX_POINTS)

# =====================
# SERIAL INIT
# =====================
ser = serial.Serial(PORT, BAUD, timeout=1)
print("Connected to", PORT)

# =====================
# PLOT SETUP
# =====================
plt.style.use("ggplot")
fig, (ax_plot, gyro_plot) = plt.subplots(2, 1, figsize=(10, 7))

ax_plot.set_title("Accelerometer (ax, ay, az)")
gyro_plot.set_title("Gyroscope (gx, gy, gz)")

line_ax, = ax_plot.plot([], [], label="ax")
line_ay, = ax_plot.plot([], [], label="ay")
line_az, = ax_plot.plot([], [], label="az")

line_gx, = gyro_plot.plot([], [], label="gx")
line_gy, = gyro_plot.plot([], [], label="gy")
line_gz, = gyro_plot.plot([], [], label="gz")

ax_plot.legend()
gyro_plot.legend()

# =====================
# UPDATE FUNCTION
# =====================
def update(frame):
    try:
        raw = ser.readline().decode().strip()

        if not raw:
            return

        # Parse comma-separated values
        parts = raw.split(",")
        if len(parts) != 6:
            return

        ax, ay, az, gx, gy, gz = map(int, parts)

        # Store values
        ax_buf.append(ax)
        ay_buf.append(ay)
        az_buf.append(az)

        gx_buf.append(gx)
        gy_buf.append(gy)
        gz_buf.append(gz)

        # Update accel plot
        line_ax.set_data(range(len(ax_buf)), ax_buf)
        line_ay.set_data(range(len(ay_buf)), ay_buf)
        line_az.set_data(range(len(az_buf)), az_buf)

        ax_plot.relim()
        ax_plot.autoscale_view()

        # Update gyro plot
        line_gx.set_data(range(len(gx_buf)), gx_buf)
        line_gy.set_data(range(len(gy_buf)), gy_buf)
        line_gz.set_data(range(len(gz_buf)), gz_buf)

        gyro_plot.relim()
        gyro_plot.autoscale_view()

    except Exception as e:
        print("Error:", e)


# =====================
# RUN ANIMATION
# =====================
ani = FuncAnimation(fig, update, interval=10)
plt.tight_layout()
plt.show()
