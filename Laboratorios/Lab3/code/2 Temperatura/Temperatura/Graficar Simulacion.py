import matplotlib.pyplot as plt
from pathlib import Path


ruta = Path(r"C:\Users\isacm\OneDrive\Documents\Escritorio\Micro\Github - UTEC\micro\Laboratorios\Lab3\code\2 Temperatura\Temperatura\Temperatura\Datos.txt")

valores = []
bandas = []   # aquí guardaremos pares (tmin, tmax) cuando aparezca 'temp'

lista_promedios = []

with ruta.open("r", encoding="utf-8") as f:
    it = iter(f)  # iterador de líneas
    for raw in it:
        s = raw.strip()
        if not s:
            continue  # salta líneas vacías
        
        buff = []

        # ¿es la palabra clave?
        if s.lower() == "temp":
            try:
                buff.clear
                tmin = float(next(it).strip())
                tmax = float(next(it).strip())
                tprom = (tmin + tmax)/2
                buff.append(tmin)
                buff.append(tmax)
              
                lista_promedios.append(buff)

                print(f"[TEMP] tmin={tmin}, tmax={tmax}, tprom ={tprom}")
            except StopIteration:
                print("[TEMP] ¡faltan líneas para tmin/tmax!")
            except ValueError:
                print("[TEMP] líneas siguientes no son números válidos")
            continue

        
        # si no es 'temp', debe ser un número de la serie
        try:
            valores.append(float(s))
        except ValueError:
            print(f"[IGNORADO] línea no numérica: {s!r}")

print(lista_promedios)

# === Construir índice 0..N-1 ===
x = list(range(len(valores)))
lista_promedios2 = []
for i in lista_promedios:
    h = (i[1] + i[0])/2
    lista_promedios2.append(h)

list2 = []
largo = len(lista_promedios2)
print(largo)
Unidades = len(valores)/largo
for i in lista_promedios2:
    for i2 in range(int(Unidades)):
        list2.append(i)
 

print(list2)


# === Graficar ===
plt.figure()
plt.plot(x, valores, marker='o')   # serie simple
plt.plot(x,list2, marker='o')
plt.title("Serie de datos")
plt.xlabel("Índice (muestra)")
plt.ylabel("Valor")
plt.grid(True)

# === Guardar y mostrar ===
plt.tight_layout()
plt.savefig("serie.png", dpi=150)
plt.show()

print(f"N = {len(valores)}. Gráfico guardado como 'serie.png'.")
