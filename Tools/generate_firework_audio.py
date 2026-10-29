"""Synthesize the original firework sounds. No external audio or dependencies required."""
import array
import math
from pathlib import Path
import random
import wave

RATE = 48000
OUT = Path(__file__).resolve().parents[1] / 'output/audio/Fireworks'


def write(name, samples):
    peak = max(abs(x) for x in samples)
    assert peak <= 1, (name, peak)
    with wave.open(str(OUT / (name + '.wav')), 'wb') as wav:
        wav.setparams((1, 2, RATE, 0, 'NONE', 'not compressed'))
        wav.writeframes(array.array('h', (round(x * 32767) for x in samples)).tobytes())
    print(f'{name}: {len(samples)/RATE:.2f}s, peak {peak:.3f}')


def launch():
    rng = random.Random(42)
    samples = []
    low = phase = 0.
    for i in range(round(.45 * RATE)):
        t = i / RATE
        u = t / .45
        noise = rng.uniform(-1, 1)
        low += .2 * (noise - low)
        phase += math.tau * (650 + 1700 * u * u) / RATE
        envelope = min(t / .018, 1) * min((.45 - t) / .022, 1)
        samples.append(envelope * (.18 * low + .025 * math.sin(phase)))
    return samples


def burst(seed):
    rng = random.Random(seed)
    samples = [0.] * round(1.5 * RATE)
    low = mid = 0.
    for i in range(len(samples)):
        t = i / RATE
        n = rng.uniform(-1, 1)
        low += .025 * (n - low)
        mid += .3 * (n - mid)
        attack = min(t / .002, 1)
        body = .95 * low * math.exp(-t / .18)
        snap = .31 * mid * math.exp(-t / .035)
        rumble = .15 * math.sin(math.tau * (76*t - 17*t*t)) * math.exp(-t/.12)
        samples[i] = attack * (body + snap + rumble)
    # Individual sizzling embers: progressively fewer and softer as the bloom dies.
    for _ in range(75):
        start = rng.uniform(.025, 1.15) ** 1.45
        duration = rng.uniform(.008, .035)
        gain = rng.uniform(.035, .12) * math.exp(-start/.5)
        filtered = 0.
        for j in range(round(duration * RATE)):
            index = round(start * RATE) + j
            if index >= len(samples):
                break
            n = rng.uniform(-1, 1)
            filtered += .15 * (n - filtered)
            samples[index] += gain * (n-filtered) * math.exp(-j/(RATE*.004)) * min(j/12, 1)
    # A short, quiet diffuse tail gives the burst space without a long noisy wash.
    dry = samples[:]
    for delay, gain in ((.047, .16), (.083, .10), (.137, .06)):
        offset = round(delay * RATE)
        for i in range(offset, len(samples)):
            samples[i] += gain * dry[i-offset]
    for i in range(len(samples)-round(.08*RATE), len(samples)):
        samples[i] *= (len(samples)-1-i)/(.08*RATE)
    return samples


if __name__ == '__main__':
    OUT.mkdir(parents=True, exist_ok=True)
    rise = launch()
    write('FireworkLaunch', rise)
    variants = [burst(seed) for seed in (11, 29, 47)]
    for i, sound in enumerate(variants, 1):
        write(f'FireworkBurst{i:02}', sound)
    preview = []
    for sound in variants:
        preview.extend(rise + sound + [0.] * round(.35 * RATE))
    write('FireworkPreview', preview)
