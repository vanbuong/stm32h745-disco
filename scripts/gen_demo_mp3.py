#!/usr/bin/env python3
"""Synthesize the public-domain Ode to Joy demo MP3.

The M7 1 MiB flash budget only has ~530 KiB left for the clip after the
rest of the image, so this renders about 80 s at 48 kb/s stereo.
"""

from __future__ import annotations

import argparse
import math
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

SR = 44100
BPM = 108.0
BEAT = 60.0 / BPM

NOTE_INDEX = {
    "C": 0,
    "C#": 1,
    "Db": 1,
    "D": 2,
    "D#": 3,
    "Eb": 3,
    "E": 4,
    "F": 5,
    "F#": 6,
    "Gb": 6,
    "G": 7,
    "G#": 8,
    "Ab": 8,
    "A": 9,
    "A#": 10,
    "Bb": 10,
    "B": 11,
}


def hz(name: str) -> float:
    if name in ("R", "REST", ""):
        return 0.0
    octave = int(name[-1])
    pitch = NOTE_INDEX[name[:-1]]
    midi = (octave + 1) * 12 + pitch
    return 440.0 * (2.0 ** ((midi - 69) / 12.0))


def synth_tone(freq: float, beats: float, vol: float) -> list[float]:
    n = int(SR * beats * BEAT)
    if n <= 0:
        return []
    if freq <= 0.0:
        return [0.0] * n
    attack = max(1, int(0.012 * SR))
    decay = max(1, int(0.045 * SR))
    release = max(1, int(min(0.09 * SR, n * 0.28)))
    sustain = 0.70
    out = [0.0] * n
    two_pi = 2.0 * math.pi
    for i in range(n):
        t = i / SR
        wave = math.sin(two_pi * freq * t) * 0.88 + math.sin(two_pi * freq * 2.0 * t) * 0.08
        if i < attack:
            env = i / attack
        elif i < attack + decay:
            env = 1.0 - (1.0 - sustain) * ((i - attack) / decay)
        else:
            env = sustain
        if i >= n - release:
            env *= (n - 1 - i) / release
        out[i] = wave * env * vol
    return out


def mix_add(dst: list[float], src: list[float], offset: int) -> None:
    end = offset + len(src)
    if end > len(dst):
        dst.extend([0.0] * (end - len(dst)))
    for i, s in enumerate(src):
        dst[offset + i] += s


def render_voice(score: list[tuple[str, float]], vol: float, transpose: int = 0) -> list[float]:
    buf: list[float] = []
    off = 0
    for name, beats in score:
        freq = hz(name)
        if freq > 0.0 and transpose != 0:
            freq *= 2.0 ** (transpose / 12.0)
        tone = synth_tone(freq, beats, vol)
        mix_add(buf, tone, off)
        off += int(SR * beats * BEAT)
    return buf


THEME: list[tuple[str, float]] = [
    ("E4", 1),
    ("E4", 1),
    ("F4", 1),
    ("G4", 1),
    ("G4", 1),
    ("F4", 1),
    ("E4", 1),
    ("D4", 1),
    ("C4", 1),
    ("C4", 1),
    ("D4", 1),
    ("E4", 1),
    ("E4", 1.5),
    ("D4", 0.5),
    ("D4", 2),
    ("E4", 1),
    ("E4", 1),
    ("F4", 1),
    ("G4", 1),
    ("G4", 1),
    ("F4", 1),
    ("E4", 1),
    ("D4", 1),
    ("C4", 1),
    ("C4", 1),
    ("D4", 1),
    ("E4", 1),
    ("D4", 1.5),
    ("C4", 0.5),
    ("C4", 2),
    ("D4", 1),
    ("D4", 1),
    ("E4", 1),
    ("C4", 1),
    ("D4", 1),
    ("E4", 0.5),
    ("F4", 0.5),
    ("E4", 1),
    ("C4", 1),
    ("D4", 1),
    ("E4", 0.5),
    ("F4", 0.5),
    ("E4", 1),
    ("D4", 1),
    ("C4", 1),
    ("D4", 1),
    ("G3", 2),
    ("E4", 1),
    ("E4", 1),
    ("F4", 1),
    ("G4", 1),
    ("G4", 1),
    ("F4", 1),
    ("E4", 1),
    ("D4", 1),
    ("C4", 1),
    ("C4", 1),
    ("D4", 1),
    ("E4", 1),
    ("D4", 1.5),
    ("C4", 0.5),
    ("C4", 2),
]

BASS: list[tuple[str, float]] = [
    ("C3", 2),
    ("G2", 2),
    ("C3", 2),
    ("G2", 2),
    ("A2", 2),
    ("E2", 2),
    ("F2", 2),
    ("G2", 2),
    ("C3", 2),
    ("G2", 2),
    ("C3", 2),
    ("G2", 2),
    ("A2", 2),
    ("E2", 2),
    ("F2", 2),
    ("C3", 2),
    ("G2", 2),
    ("C3", 2),
    ("G2", 2),
    ("C3", 2),
    ("G2", 2),
    ("C3", 2),
    ("G2", 2),
    ("C3", 2),
    ("C3", 2),
    ("G2", 2),
    ("C3", 2),
    ("G2", 2),
    ("A2", 2),
    ("F2", 2),
    ("G2", 2),
    ("C3", 2),
]

INTRO: list[tuple[str, float]] = [
    ("G3", 1),
    ("C4", 1),
    ("E4", 1),
    ("G4", 2),
    ("R", 1),
]

INTRO_BASS: list[tuple[str, float]] = [
    ("G2", 2),
    ("C3", 2),
    ("G2", 2),
]

CODA: list[tuple[str, float]] = [
    ("C4", 1),
    ("E4", 1),
    ("G4", 1),
    ("C5", 4),
    ("R", 1),
]

CODA_BASS: list[tuple[str, float]] = [
    ("C3", 2),
    ("G2", 2),
    ("C2", 4),
]


def concat(parts: list[list[float]]) -> list[float]:
    out: list[float] = []
    for p in parts:
        out.extend(p)
    return out


def stereo_pair(mono_l: list[float], mono_r: list[float]) -> bytes:
    n = max(len(mono_l), len(mono_r))
    if len(mono_l) < n:
        mono_l = mono_l + [0.0] * (n - len(mono_l))
    if len(mono_r) < n:
        mono_r = mono_r + [0.0] * (n - len(mono_r))
    peak = 0.0
    for a, b in zip(mono_l, mono_r):
        peak = max(peak, abs(a), abs(b))
    scale = 0.92 / peak if peak > 0.0 else 1.0
    raw = bytearray()
    for a, b in zip(mono_l, mono_r):
        raw += struct.pack("<ff", a * scale, b * scale)
    return bytes(raw)


def encode_mp3(pcm: bytes, dest: Path, bitrate: str) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(suffix=".f32", delete=False) as tmp:
        tmp.write(pcm)
        tmp_path = tmp.name
    try:
        cmd = [
            "ffmpeg",
            "-y",
            "-f",
            "f32le",
            "-ar",
            str(SR),
            "-ac",
            "2",
            "-i",
            tmp_path,
            "-c:a",
            "libmp3lame",
            "-b:a",
            bitrate,
            "-ar",
            str(SR),
            "-ac",
            "2",
            "-write_xing",
            "0",
            "-joint_stereo",
            "0",
            str(dest),
        ]
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    finally:
        Path(tmp_path).unlink(missing_ok=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=Path("firmware/src/bsp/stm32h745i_disco/demo.mp3"),
    )
    parser.add_argument("--bitrate", default="48k")
    args = parser.parse_args()

    lead = concat(
        [
            render_voice(INTRO, 0.26),
            render_voice(THEME, 0.28),
            render_voice(THEME, 0.30, transpose=0),
            render_voice(CODA, 0.32),
        ]
    )
    bass = concat(
        [
            render_voice(INTRO_BASS, 0.16),
            render_voice(BASS, 0.18),
            render_voice(BASS, 0.17),
            render_voice(CODA_BASS, 0.20),
        ]
    )
    harmony = concat(
        [
            render_voice(INTRO, 0.10, transpose=-12),
            render_voice(THEME, 0.12, transpose=-12),
            render_voice(THEME, 0.11, transpose=-7),
            render_voice(CODA, 0.12, transpose=-12),
        ]
    )

    n = max(len(lead), len(bass), len(harmony))
    left = [0.0] * n
    right = [0.0] * n
    for i in range(n):
        l = lead[i] if i < len(lead) else 0.0
        h = harmony[i] if i < len(harmony) else 0.0
        b = bass[i] if i < len(bass) else 0.0
        left[i] = l + h * 0.55 + b
        right[i] = l * 0.92 + h + b * 0.85

    pcm = stereo_pair(left, right)
    seconds = len(left) / SR
    encode_mp3(pcm, args.output, args.bitrate)
    size = args.output.stat().st_size
    print(f"wrote {args.output}  {seconds:.1f}s  {size} bytes  {args.bitrate}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
