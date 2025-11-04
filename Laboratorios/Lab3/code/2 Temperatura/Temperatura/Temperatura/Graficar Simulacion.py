with open('Datos.txt', 'r', encoding='utf-8') as f:
    valores = [float(line.strip()) for line in f if line.strip() != ""]

# === Construir índice 0..N-1 ===
x = list(range(len(valores)))

# === Graficar ===
plt.figure()
plt.plot(x, valores, marker='o')  
plt.title("Serie de datos")
plt.xlabel("Índice (muestra)")
plt.ylabel("Valor")
plt.grid(True)

# === Guardar y mostrar ===
plt.tight_layout()
plt.savefig("serie.png", dpi=150)
plt.show()

print(f"N = {len(valores)}. Gráfico guardado como 'serie.png'.")