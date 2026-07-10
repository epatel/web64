#!/usr/bin/env python3
"""Generate the 16x16 blue-noise dither table in main.asm (bnoise).

Void-and-cluster algorithm (Ulichney), toroidal wrap, gaussian
energy kernel. Deterministic: same seed -> same table. Output is
16 levels (0-15), each appearing exactly 16 times, emitted as
assembler .byte rows.

Usage: python3 bluenoise.py [seed]      (default seed 42)
"""
import math, random, sys

random.seed(int(sys.argv[1]) if len(sys.argv) > 1 else 42)
N = 16
SIGMA = 1.5

kern = [[math.exp(-(min(dx, N - dx) ** 2 + min(dy, N - dy) ** 2) / (2 * SIGMA * SIGMA))
         for dx in range(N)] for dy in range(N)]

def energy(pat):
    e = [[0.0] * N for _ in range(N)]
    for y in range(N):
        for x in range(N):
            if pat[y][x]:
                for j in range(N):
                    for i in range(N):
                        e[j][i] += kern[(j - y) % N][(i - x) % N]
    return e

def tightest(pat):
    e = energy(pat); best = None; bv = -1
    for y in range(N):
        for x in range(N):
            if pat[y][x] and e[y][x] > bv:
                bv = e[y][x]; best = (y, x)
    return best

def largest_void(pat):
    e = energy(pat); best = None; bv = 1e18
    for y in range(N):
        for x in range(N):
            if not pat[y][x] and e[y][x] < bv:
                bv = e[y][x]; best = (y, x)
    return best

ones = N * N * 10 // 100
pat = [[0] * N for _ in range(N)]
for y, x in random.sample([(y, x) for y in range(N) for x in range(N)], ones):
    pat[y][x] = 1
while True:
    ty, tx = tightest(pat); pat[ty][tx] = 0
    vy, vx = largest_void(pat); pat[vy][vx] = 1
    if (vy, vx) == (ty, tx):
        break

rank = [[0] * N for _ in range(N)]
p1 = [r[:] for r in pat]
for r in range(ones - 1, -1, -1):
    y, x = tightest(p1); p1[y][x] = 0; rank[y][x] = r
p2 = [r[:] for r in pat]
for r in range(ones, N * N):
    y, x = largest_void(p2); p2[y][x] = 1; rank[y][x] = r

thr = [[rank[y][x] // 16 for x in range(N)] for y in range(N)]
for row in thr:
    print('        .byte ' + ', '.join('%2d' % v for v in row))

from collections import Counter
c = Counter(v for row in thr for v in row)
assert all(c[i] == 16 for i in range(16)), c
