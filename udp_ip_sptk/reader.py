#coding:utf-8
import struct
import sys
from pylab import *

if len(sys.argv) != 2:
    print "usage: python draw_pitch.py [pitch file]"
    sys.exit()
pitch_file = sys.argv[1]

# ピッチファイルをロード
pitch = []
f = open(pitch_file, "rb")
while True:
    # 4バイト（FLOAT）ずつ読み込む
    b = f.read(4)
    if b == "": break;
    # 読み込んだデータをFLOAT型（f）でアンパック
    val = struct.unpack("f", b)[0]
    pitch.append(val)
f.close()

# プロット
plot(range(len(pitch)), pitch)
xlabel("time (frame)")
ylabel("F0 (Hz)")
show()