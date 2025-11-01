import serial
import threading
import time
from datetime import datetime
from matplotlib import pyplot as plt
import matplotlib.animation as animation
from random import randrange


PORT = "COM7"
BAUD = 9600
EOL_ENVIO = "\r\n"  # o "\n"


# Dos hilos, uno de lectura y el otro de escritura

def lector(ser):
    while True:
        try:
            data = ser.readline()
            if data:
                print(data.decode("utf-8", errors="replace"), end="")
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




serialPort = serial.Serial(
    port=PORT, baudrate=BAUD, bytesize=8, timeout=0.1, stopbits=serial.STOPBITS_ONE
)

def graficar():
    plt.style.use('ggplot')
    x_data = []
    y_data = []

    figure = pylt.figure()
    line = pyplot.plot(x_data, y_data, '-')


def grafica():
    x_data.append(datetime.now())
    y_data.append(serialPort.readline(0,100))
    line.set_data(x_data, y_data)
    fig.gca().relim()
    fig.gca().autoscale_view()
    return line



def main():
    time.sleep(2)  # dar tiempo si el Arduino se resetea

    t1 = threading.Thread(target=lector,   args=(serialPort,), daemon=True)
  #  t2 = threading.Thread(target=escritor, args=(serialPort,), daemon=True)
    t1.start()


    try:
        escritor(serialPort)
    except KeyboardInterrupt:
        pass
    finally:
        serialPort.close()


if __name__ == '__main__':
    main()