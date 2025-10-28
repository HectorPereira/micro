from svgpathtools import svg2paths
import numpy as np

# Load SVG and extract paths
paths, attributes = svg2paths("drawing.svg")

# Resolution (mm per step)
STEP_RES = 0.2    # sample points every 0.2 mm
FEED = 1000       # mm/min

with open("output.gcode", "w") as f:
    f.write("G21\nG90\n")  # mm units, absolute

    for path in paths:
        length = path.length()
        samples = int(length / STEP_RES)
        for i in range(samples + 1):
            point = path.point(i / samples)
            x, y = point.real, point.imag
            f.write(f"G1 X{x:.2f} Y{y:.2f} F{FEED}\n")

    f.write("M2\n")

print("✅ G-code written to output.gcode")
