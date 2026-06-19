import os
Import("env")

# Locates and runs esptool.py to trigger a hard reset
env.Replace(
    RESETTOOL=os.path.join(env.PioPlatform().get_package_dir("tool-esptoolpy") or "", "esptool.py"),
    RESETFLAGS=["--chip", env.BoardConfig().get("build.mcu", "esp32"), "--port", "$UPLOAD_PORT", "hard_reset"],
    RESETCMD='"$PYTHONEXE" "$RESETTOOL" $RESETFLAGS',
)

env.AddCustomTarget(
    name="reset_target",
    actions=["$RESETCMD"],
    title="Reset ESP32",
    description="Resets the target via esptool"
)