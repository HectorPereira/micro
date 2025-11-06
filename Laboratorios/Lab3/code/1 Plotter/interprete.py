
# Configuracion
step_mm = 1.0          # tamaño de paso 
scale = 1.0            # factor de escala general
radius = 20.0          # radio aproximado

# Direcciones y desplazamientos relativos
DIRS = {
    "UP": (0, 1),
    "DOWN": (0, -1),
    "LEFT": (-1, 0),
    "RIGHT": (1, 0),
    "UPLEFT": (-1, 1),
    "UPRIGHT": (1, 1),
    "DOWNLEFT": (-1, -1),
    "DOWNRIGHT": (1, -1),
}

# Datos de entrada
input_data = [
    "SOLENOID_DOWN","DOWNLEFT","LEFT","DOWNLEFT","LEFT","DOWNLEFT","LEFT","DOWNLEFT",
    "LEFT","DOWNLEFT","LEFT","DOWNLEFT","LEFT","DOWNLEFT","LEFT","DOWNLEFT",
    "LEFT","DOWNLEFT","LEFT","DOWNLEFT","LEFT","DOWNLEFT","LEFT","DOWNLEFT",
    "LEFT","DOWNLEFT","LEFT","DOWNLEFT","LEFT","DOWNLEFT","LEFT","DOWNLEFT",
    "LEFT","DOWNLEFT","LEFT","DOWNLEFT","LEFT","DOWNLEFT","DOWN","DOWNLEFT",
    "DOWN","DOWNLEFT","DOWN","DOWNLEFT","DOWN","DOWNLEFT","DOWN","DOWNLEFT",
    "DOWN","DOWNLEFT","DOWN","DOWNLEFT","DOWN","DOWNLEFT","DOWN","DOWNLEFT",
    "SOLENOID_UP"
]

# Conversión a coordenadas 
x, y = 0.0, 0.0
moves = []
pen = 'U'

for token in input_data:
    if token == "SOLENOID_DOWN":
        pen = 'D'
        continue
    elif token == "SOLENOID_UP":
        pen = 'U'
        moves.append((pen, x * scale, y * scale))
        continue

    if token in DIRS:
        dx, dy = DIRS[token]
        x += dx * step_mm
        y += dy * step_mm
        moves.append((pen, x * scale, y * scale))
    else:
        print(f"Advertencia: dirección desconocida '{token}'")

#  Exportar como formato C 
print("const Move generated_path[] = {")
print("    {'U', 0.000f, 0.000f},")
for m in moves:
    print(f"    {{'{m[0]}', {m[1]:.3f}f, {m[2]:.3f}f}},")
print("};")
