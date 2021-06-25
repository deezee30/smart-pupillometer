def colour_rgb565to16bit(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def colour_16bittorgb565(colour):
    r = (colour >> 8) & 0xF8; r |= (r >> 5)
    g = (colour >> 3) & 0xFC; g |= (g >> 6)
    b = (colour << 3) & 0xF8; b |= (b >> 5)

    return r, g, b
    #return (r << 16) | ((g << 8) | (b << 0))

print(colour_rgb565to16bit(255, 97, 97))
print(colour_16bittorgb565(25567))

# Grayscale

def gray_rgb565to16bit(shade):
    return ((shade & 0xF8) << 8) | ((shade & 0xFC) << 3) | (shade >> 3)

def gray_16bittorgb565(shade):
    r = (shade >> 8) & 0xF8; r |= (r >> 5)
    g = (shade >> 3) & 0xFC; g |= (g >> 6)
    b = (shade << 3) & 0xF8; b |= (b >> 5)

    return round((r+g+b)/3)

print(gray_rgb565to16bit(131))
print(gray_16bittorgb565(33808))