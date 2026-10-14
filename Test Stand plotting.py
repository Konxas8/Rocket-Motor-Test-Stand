from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt

# -------------------------------------------------------------------------
# 1. File Paths & Loading
# -------------------------------------------------------------------------
BASE_DIR = Path(__file__).resolve().parent

#make sure the name is right: on file manager go to view > show > File name extensions
log_path = BASE_DIR / "LOG.TXT"
fire_path = BASE_DIR / "FIRE_LOG.TXT"

# LOG.TXT: skip 2 lines (banner + original header), provide clean column names
df_log = pd.read_csv(
    log_path, 
    skiprows=2, 
    names=["Time_ms", "Force_kg"]
)

# FIRE_LOG.TXT: skip 1 line (banner only), assign column names
df_fire = pd.read_csv(
    fire_path, 
    skiprows=1, 
    names=["Time_ms", "Force_kg"]
)

# -------------------------------------------------------------------------
# 2. Conversions
# -------------------------------------------------------------------------
# Convert Time from ms to seconds
df_log["Time_s"] = df_log["Time_ms"] / 1000.0
df_fire["Time_s"] = df_fire["Time_ms"] / 1000.0

# Convert Force from kg to Newtons (1 kgf ≈ 9.80665 N)
g = 9.80665
df_fire["Thrust_N"] = df_fire["Force_kg"] * g

# -------------------------------------------------------------------------
# 3. Plotting
# -------------------------------------------------------------------------
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))

# Subplot 1: Baseline Standby Data (LOG.TXT)
ax1.plot(df_log["Time_s"], df_log["Force_kg"], color="#3498db", marker="o", markersize=4, linestyle="-", label="Tare / Idle Force")
ax1.axhline(0, color="gray", linestyle="--", linewidth=0.8)
ax1.set_title("Static Standby / Baseline Telemetry (LOG.TXT)", fontsize=12, fontweight="bold")
ax1.set_xlabel("System Time (s)")
ax1.set_ylabel("Force (kg)")
ax1.grid(True, linestyle=":", alpha=0.6)
ax1.legend(loc="upper right")

# Subplot 2: Firing Sequence Thrust Curve (FIRE_LOG.TXT)
ax2.plot(df_fire["Time_s"], df_fire["Force_kg"], color="#e74c3c", marker="o", markersize=4, linewidth=2, label="Measured Force (kg)")
ax2.set_title("Ignition Test Thrust Profile (FIRE_LOG.TXT)", fontsize=12, fontweight="bold")
ax2.set_xlabel("Burn Time from Ignition (s)")
ax2.set_ylabel("Force (kg)", color="#e74c3c")
ax2.tick_params(axis="y", labelcolor="#e74c3c")
ax2.grid(True, linestyle=":", alpha=0.6)

# Secondary Y-axis for Thrust in Newtons (N)
ax2_twin = ax2.twinx()
ax2_twin.plot(df_fire["Time_s"], df_fire["Thrust_N"], alpha=0)  # Invisible trace to sync limits
ax2_twin.set_ylabel("Thrust (N)", color="#b03a2e")
ax2_twin.set_ylim([df_fire["Force_kg"].min() * g, df_fire["Force_kg"].max() * g * 1.1])
ax2.set_ylim([df_fire["Force_kg"].min(), df_fire["Force_kg"].max() * 1.1])

# Annotate Peak Force
peak_idx = df_fire["Force_kg"].idxmax()
peak_time = df_fire.loc[peak_idx, "Time_s"]
peak_val = df_fire.loc[peak_idx, "Force_kg"]

ax2.annotate(
    f"Peak: {peak_val:.2f} kg ({peak_val*g:.1f} N)",
    xy=(peak_time, peak_val),
    xytext=(peak_time + 0.3, peak_val - 0.5),
    arrowprops=dict(facecolor="black", shrink=0.05, width=1, headwidth=6),
    fontweight="bold"
)

plt.tight_layout()
plt.show()