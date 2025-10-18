import matplotlib.pyplot as plt
import numpy as np

# Mediciones de corriente (mA)
corrientes = [58.7, 10.39, 58.7, 6.06, 58.7, 4.77, 58.7, 4.77, 10.39]

# Cada estado dura 5 segundos
duracion_por_etapa = 5
tiempo = np.arange(0, len(corrientes) * duracion_por_etapa, duracion_por_etapa)

# Crear datos para una gráfica escalonada
tiempo_graf = []
corriente_graf = []
for i, c in enumerate(corrientes):
    tiempo_graf.extend([i * duracion_por_etapa, (i + 1) * duracion_por_etapa])
    corriente_graf.extend([c, c])

# Graficar
plt.figure(figsize=(10, 4))
plt.step(tiempo_graf, corriente_graf, where='post', linewidth=2, color='blue')
plt.scatter(np.arange(duracion_por_etapa/2, len(corrientes)*duracion_por_etapa, duracion_por_etapa),
            corrientes, color='red', label='Mediciones')

plt.title("Prueba de consumo en modos de suspensión - ATmega328P (Arduino)")
plt.xlabel("Tiempo (s)")
plt.ylabel("Corriente (mA)")
plt.grid(True, linestyle='--', alpha=0.6)
plt.legend()
plt.tight_layout()
plt.show()
