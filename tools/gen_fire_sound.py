"""Procedural bonfire crackle — a seamless ~4s loop for the Blood-Moon intro.

Pure stdlib (wave/struct/math/random), no deps. Run from anywhere:
    python tools/gen_fire_sound.py
Writes a 16-bit mono WAV to assets/audio/sfx/fire.wav (relative to this file's
parent project). The bed is filtered brown noise (low roar); on top sit randomly
scattered short "crackle" pops. The last 0.25s equal-power cross-fades into the
head so the loop seam is inaudible.
"""
from pathlib import Path
import math
import random
import struct
import wave

SR = 22050
DUR = 4.0
OUT = Path(__file__).resolve().parent.parent / "assets" / "audio" / "sfx" / "fire.wav"


def main():
    random.seed(1234)
    n = int(SR * DUR)
    out = [0.0] * n

    # --- low roar: brown noise through a one-pole low-pass ---
    lp = 0.0
    for i in range(n):
        white = random.uniform(-1.0, 1.0)
        lp += (white - lp) * 0.04          # ~heavy low-pass -> rumble
        out[i] += lp * 1.6

    # --- crackles: short filtered-noise pops scattered through the loop ---
    pops = int(DUR * 14)                    # ~14 crackles per second
    for _ in range(pops):
        start = random.randint(0, n - 1)
        length = random.randint(int(SR * 0.004), int(SR * 0.03))
        amp = random.uniform(0.08, 0.34)
        cut = random.uniform(0.3, 0.8)      # brighter = sharper snap
        clp = 0.0
        for k in range(length):
            idx = start + k
            if idx >= n:
                break
            clp += (random.uniform(-1.0, 1.0) - clp) * cut
            env = (1.0 - k / length) ** 2   # fast decay
            out[idx] += clp * amp * env

    # --- seamless loop: equal-power cross-fade the tail into the head ---
    xf = int(SR * 0.25)
    for k in range(xf):
        a = math.cos(0.5 * math.pi * k / xf)        # tail weight 1 -> 0
        b = math.sin(0.5 * math.pi * k / xf)        # head weight 0 -> 1
        i_tail = n - xf + k
        out[i_tail] = out[i_tail] * a + out[k] * b

    # --- normalize to a comfortable peak ---
    peak = max(1e-6, max(abs(s) for s in out))
    g = 0.7 / peak
    out = [s * g for s in out]

    OUT.parent.mkdir(parents=True, exist_ok=True)
    frames = bytearray()
    for s in out:
        v = int(max(-1.0, min(1.0, s)) * 32767)
        frames += struct.pack("<h", v)
    with wave.open(str(OUT), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(bytes(frames))
    print("wrote", OUT.resolve(), "samples:", n)


if __name__ == "__main__":
    main()
