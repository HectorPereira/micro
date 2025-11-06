import serial
import threading
import time
from datetime import datetime
from datetime import datetime, timedelta
import matplotlib.dates as mdates
from collections import deque
import matplotlib.pyplot as plt
import matplotlib.animation as animation

PORT = "COM7"
BAUD = 9600
EOL_ENVIO = "\r\n"  # o "\n"

# ===== Estado gráfico =====
plt.style.use('ggplot')
x_data = []
y_data = []
y2_data = []    
y3_data = []                        

VENTANA_S = 20  # segundos de ventana visible


figure, ax = plt.subplots()
line1, = ax.plot(x_data, y_data, '-', label='Temperatura Actual')     
line2, = ax.plot([], [], '-', label='Temperatura minima') 
line3, = ax.plot([], [], '-', label='Temperatura Maxima')         

ax.grid(True)
ax.legend()   
ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
figure.autofmt_xdate()




# ===== Buffer numérico compartido =====
buf  = deque(maxlen=5000)    # Canal 1
buf2 = deque(maxlen=5000)    # Canal 2 (promedio tras 'temp')
buf3 = deque(maxlen=5000)    # Canal 2 (promedio tras 'temp')

grabando = threading.Event()

# Estado para capturar los dos números después de 'temp'
temp_mode = False
temp_vals = []


# ----------------- Hilos -----------------
def lector(ser):
    global temp_mode, temp_vals
    while True:
        try:
            raw = ser.readline()
            if not raw:
                continue

            s = raw.decode("utf-8", errors="replace").strip()
            print(s)  # eco en consola

            grabando.set()

            # --- Protocolo 'temp': tomar 2 números y promediar ---
            if s.lower() == "temp":
                temp_mode = True
                temp_vals = []
                continue

            if temp_mode:
                # intenta parsear número para los dos siguientes renglones
                try:
                    v = float(s.replace(",", "."))
                    temp_vals.append(v)
                    if len(temp_vals) >= 2:
                        avg = temp_vals[0] 
                        avg2 = temp_vals[1]

                        buf2.append(avg)  
                        buf3.append(avg2)            
                                  
                        temp_mode = False
                        temp_vals = []
                except ValueError:
                    # si no es número, simplemente lo ignoramos y seguimos esperando
                    pass
                continue  # no permitir que estos números caigan al canal normal

            # --- Caso normal: un número por línea -> Canal 1 ---
            if grabando.is_set():
                try:
                    v = float(s.replace(",", "."))
                    buf.append(v)
                except ValueError:
                    pass

        except Exception as e:
            print(f"\n[Lectura] {e}")
            break


def escritor(ser):
    while True:
        try:
            linea = input()
            if not (linea.endswith("\n") or linea.endswith("\r")):
                linea += EOL_ENVIO
            ser.write(linea.encode("ascii", errors="ignore"))
            ser.flush()
        except Exception as e:
            print(f"[Escritura] {e}")
            break

# ----------------- Gráfica (consume el buffer) -----------------
def grafica(_frame=None):
    # vacía buffers sincronizando timestamps
    while buf or buf2:
        t = datetime.now()
        y1 = buf.popleft()  if buf  else (y_data[-1]  if y_data  else float('nan'))
        y2 = buf2.popleft() if buf2 else (y2_data[-1] if y2_data else float('nan'))
        y3 = buf3.popleft() if buf3 else (y3_data[-1] if y3_data else float('nan'))

        x_data.append(t)
        y_data.append(y1)
        y2_data.append(y2)
        y3_data.append(y3)


    if not x_data:
        return line1, line2, line3

    # --- Ventana deslizante de los últimos VENTANA_S segundos ---
    now = x_data[-1]
    t0  = now - timedelta(seconds=VENTANA_S)

    # busca el primer índice >= t0 (scan hacia atrás, O(n) pero simple)
    idx = len(x_data) - 1
    while idx >= 0 and x_data[idx] >= t0:
        idx -= 1
    start = max(0, idx + 1)

    xv  = x_data[start:]
    yv  = y_data[start:]
    y2v = y2_data[start:]
    y3v = y3_data[start:]


    # actualiza curvas
    line1.set_data(xv,  yv)
    line2.set_data(xv,  y2v)
    line3.set_data(xv,  y3v)


    # fija X a [t0, now] y autoscale solo en Y
    ax.set_xlim(t0, now)
    ax.relim()
    ax.autoscale_view(scalex=False, scaley=True)
    figure.canvas.draw_idle()
    return line1, line2, line3


serialPort = serial.Serial(
    port=PORT, baudrate=BAUD, bytesize=8, timeout=0.1, stopbits=serial.STOPBITS_ONE
)

def main():
    time.sleep(2)  # dar tiempo si el Arduino se resetea

    t1 = threading.Thread(target=lector,   args=(serialPort,), daemon=True)
    t2 = threading.Thread(target=escritor, args=(serialPort,), daemon=True)
    t1.start()
    t2.start()

    # Actualiza la gráfica en “tiempo real”
    ani = animation.FuncAnimation(figure, grafica, interval=100, blit=False)

    try:
        plt.show()
    except KeyboardInterrupt:
        pass
    finally:
        serialPort.close()

if __name__ == '__main__':
    main()
