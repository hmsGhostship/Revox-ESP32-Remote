import os
Import("env")

# 1. Ermittle den Pfad zur esptool.py, die PlatformIO bereits installiert hat
platform = env.PioPlatform()
esptool_path = os.path.join(
    platform.get_package_dir("tool-esptoolpy") or "", 
    "esptool.py"
)

# 2. Definiere den Befehl für den Hard-Reset via esptool
# Nutzt die automatische Port-Erkennung ($UPLOAD_PORT) aus deiner platformio.ini
reset_command = [
    '"$PYTHONEXE"', 
    f'"{esptool_path}"',
    "--chip", "esp32",
    "--port", '"$UPLOAD_PORT"',
    "run"  # Der Befehl 'run' startet das Board nach dem Verbinden einfach neu
]

# 3. Registriere das eigene Target in PlatformIO
env.AddCustomTarget(
    name="reset_board",
    dependencies=None,
    actions=[
        " ".join(reset_command)
    ],
    title="Reset ESP32",
    description="Startet den ESP32 neu, ohne Code zu kompilieren"
)