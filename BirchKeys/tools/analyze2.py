"""Per-instrument sound fingerprint from the test renders: level, pitch error, harmonic
balance (H2/H1, H3/H1 in dB), spectral centroid of the attack vs the body, and how much the
note has fallen after 0.5 s. Used to check that instruments are distinct and in tune.

Usage: python3 tools/analyze2.py renders
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


def spectrum(x):
    n = 1 << 19
    spec = np.abs(np.fft.rfft((x - x.mean()) * np.hanning(len(x)), n))
    return spec, np.fft.rfftfreq(n, 1 / SR)


def peak_near(spec, freqs, f, tol=0.06):
    band = (freqs > f * (1 - tol)) & (freqs < f * (1 + tol))
    idx = np.flatnonzero(band)
    if len(idx) == 0:
        return 1e-9, f
    k = idx[np.argmax(spec[idx])]
    a, b, c = np.log(spec[k - 1:k + 2] + 1e-12)
    off = 0.5 * (a - c) / (a - 2 * b + c) if (a - 2 * b + c) != 0 else 0.0
    return spec[k], freqs[k] + off * (freqs[1] - freqs[0])


def centroid(x):
    spec, freqs = spectrum(x)
    return float((spec * freqs).sum() / (spec.sum() + 1e-12))


def main():
    folder = Path(sys.argv[1])
    print(f"{'instrument':20} {'rms':>6} {'peak':>5} {'cents':>6} {'H2':>6} {'H3':>6} {'cent@10ms':>9} {'cent@300ms':>10} {'drop0.5s':>8}")
    for path in sorted(folder.glob("*.wav")):
        x = load(path)
        bass = "Bass" in path.stem or "808" in path.stem
        # last note: keys C5 (523.25 Hz) from 2.66 s, bass Eb2 (77.78 Hz) from 2.667 s
        t0 = 2.667 if bass else 2.66
        expected = 77.78 if bass else 523.25
        start = int(t0 * SR)
        body = x[start + int(0.1 * SR):start + int(0.5 * SR)]
        spec, freqs = spectrum(body)
        h1, f1 = peak_near(spec, freqs, expected)
        h2, _ = peak_near(spec, freqs, expected * 2)
        h3, _ = peak_near(spec, freqs, expected * 3)
        cents = 1200 * np.log2(f1 / expected)
        rms = 20 * np.log10(np.sqrt(np.mean(x[: int(3.6 * SR)] ** 2)) + 1e-9)
        peak = np.abs(x).max()
        attack = x[start + int(0.003 * SR):start + int(0.018 * SR)]
        later = x[start + int(0.3 * SR):start + int(0.4 * SR)]
        a = np.sqrt(np.mean(x[start + int(0.02 * SR):start + int(0.06 * SR)] ** 2)) + 1e-9
        b = np.sqrt(np.mean(x[start + int(0.5 * SR):start + int(0.55 * SR)] ** 2)) + 1e-9
        print(f"{path.stem[3:]:20} {rms:6.1f} {peak:5.2f} {cents:6.1f} {20*np.log10(h2/h1):6.1f} {20*np.log10(h3/h1):6.1f}"
              f" {centroid(attack):9.0f} {centroid(later):10.0f} {20*np.log10(b/a):8.1f}")


main()
