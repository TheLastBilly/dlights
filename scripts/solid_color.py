import socket, pickle, time, sys

MAX_LEDS = 100
LEDS = [0] * (MAX_LEDS * 3)

class RGB:
    r = 0
    g = 0
    b = 0

    def __init__(self, r, g, b):
        self.r = r
        self.g = g
        self.b = b

def setLED(index : int, color : RGB):
    if(index >= MAX_LEDS or index < 0):
        return
    LEDS[index * 3] = color.r
    LEDS[index * 3 + 1] = color.g
    LEDS[index * 3 + 2] = color.b

def sendLEDS():
    p = ""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("10.0.0.210", 1000))
    try:
        s.send(bytes(LEDS))
    except:
        pass
    s.close()


c = RGB(int(sys.argv[1]),int(sys.argv[2]), int(sys.argv[3]))

for i in range(MAX_LEDS):
    setLED(i, c)

sendLEDS()
