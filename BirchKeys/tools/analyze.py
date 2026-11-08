"""Checks rendered test WAVs: level, pitch accuracy of the last note, brightness and decay.

Usage: python3 -I tools/analyze.py renders
"""
import sys
import wave
from pathlib import Path

import numpy as np

SR = 48000


def load(path):
    with wave.open(str(path)) as w:
        data = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.float32) / 32768
    return data.reshape(-1, 2).mean(axis=1)


def pitch(x, expected):
    # strongest spectral peak within a semitone of the expected fundamental
    # (autocorrelation gets fooled by 16' drawbars, sub oscillators and inharmonic bars)
    n = 1 << 18
    spec = np.abs(np.fft.rfft((x - x.mean()) * np.hanning(len(x)), n))
    freqs = np.fft.rfftfreq(n, 1 / SR)
    band = (freqs > expected * 2 ** (-1 / 12)) & (freqs < expected * 2 ** (1 / 12))
    idx = np.flatnonzero(band)
    k = idx[np.argmax(spec[idx])]
    a, b, c = np.log(spec[k - 1:k + 2] + 1e-12)
    return freqs[k] + 0.5 * (a - c) / (a - 2 * b + c) * (freqs[1] - freqs[0])


def centroid(x):
    spec = np.abs(np.fft.rfft(x * np.hanning(len(x))))
    freqs = np.fft.rfftfreq(len(x), 1 / SR)
    return float((spec * freqs).sum() / (spec.sum() + 1e-12))


def main():
    folder = Path(sys.argv[1])
    print(f"{'instrument':22} {'rms dB':>7} {'peak':>6} {'f0 exp':>7} {'f0 got':>7} {'cents':>6} {'centroid':>9} {'ring dB':>8}")
    for path in sorted(folder.glob("*.wav")):
        x = load(path)
        bass = any(word in path.stem for word in ("Bass",))
        # last note: keys C5 (523.25 Hz) from 2.66 s, bass Eb2 (77.78 Hz) from 2.667 s
        start = int(2.85 * SR)
        expected = 77.78 if bass else 523.25
        seg = x[start:start + int(0.4 * SR)]
        got = pitch(seg, expected)
        cents = 1200 * np.log2(got / expected)
        rms = 20 * np.log10(np.sqrt(np.mean(x[: int(3.6 * SR)] ** 2)) + 1e-9)
        peak = np.abs(x).max()
        # how much the chord / first note dropped over its first second
        a = np.sqrt(np.mean(x[int(0.05 * SR):int(0.15 * SR)] ** 2)) + 1e-9
        b = np.sqrt(np.mean(x[int(1.0 * SR):int(1.1 * SR)] ** 2)) + 1e-9
        ring = 20 * np.log10(b / a)
        print(f"{path.stem:22} {rms:7.1f} {peak:6.2f} {expected:7.1f} {got:7.1f} {cents:6.1f} {centroid(seg):9.0f} {ring:8.1f}")


main()
