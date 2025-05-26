import json
import random
import sys
from decimal import Decimal

# Parameters
sys.set_int_max_str_digits(10**6)
N_BITS_PREC = 11
WEIGHT = 32

# Public key
with open("pk.json") as f:
    pk_data = json.load(f)

pk_star = list(map(int, pk_data["pk_star"]))
y = list(map(Decimal, pk_data["y"]))

TAU = len(pk_star) - 1
THETA = len(y)
x0 = pk_star[0]

# Public encryption
def encrypt_public(m, pk, TAU=8320, RHOP=32):
    if TAU is None:
        TAU = len(pk) - 1
    x0 = pk[0]

    # Random subset S incl {1, ..., TAU}
    sum_pk = 0
    for i in range(1, TAU + 1):
        if random.randint(0, 1):
            sum_pk += pk[i]
    r = random.randint(-RHOP, RHOP)
    r2 = 2 * r
    c = (2 * sum_pk) + r2
    if m != 0:
        c += 1
    c = c % x0
    return c

def and_h(a, b):
    return (a*b)

def xor_h(a, b):
    return (a+b)
    
def or_h(a,b):
    return (a+b) + (a*b)

def not_h(a):
    x = encrypt_public(1, pk_star)
    return (a+x)

def add_noise(a):
    x = encrypt_public(0, pk_star)
    return (a+x)

def compress(a,b,c,d):
    cond1 = (a*b)
    cond2 = (c*d)
    cond3 = (a+b)*(c+d)
    or1 = or_h(cond1, cond2)
    return or_h(or1, cond3)

def compress_black(a,b,c,d):
    pre1 = (a*b)
    pre2 = (c*d)
    or1 = or_h(pre1*c, pre1*d)
    or2 = or_h(pre2*a, pre2*b)
    return or_h(or1, or2) 

# In theory, should keep the same bit, but we have no bootstrap
def destroy_bit(a):
    x = a
    for _ in range(53):
        x = x * a
    return x

def compress_function(img):
    new_img = [[str(0) for _ in range(16)] for _ in range(16)]
    for i in range(8):
        for j in range(8):
            compressed = compress(int(img[2*i][2*j]), int(img[2*i+1][2*j]), int(img[2*i][2*j+1]), int(img[2*i+1][2*j+1]))
            new_img[2*i][2*j] = str(compressed)
            new_img[2*i+1][2*j] = str(compressed)
            new_img[2*i][2*j+1] = str(compressed)
            new_img[2*i+1][2*j+1] = str(compressed)
    return new_img

def compress_black_function(img):
    new_img = [[str(0) for _ in range(16)] for _ in range(16)]
    for i in range(8):
        for j in range(8):
            compressed = compress_black(int(img[2*i][2*j]), int(img[2*i+1][2*j]), int(img[2*i][2*j+1]), int(img[2*i+1][2*j+1]))
            new_img[2*i][2*j] = str(compressed)
            new_img[2*i+1][2*j] = str(compressed)
            new_img[2*i][2*j+1] = str(compressed)
            new_img[2*i+1][2*j+1] = str(compressed)
    return new_img

def load_encrypted_image(encrypted_filename):
    encrypted_image = [[None for _ in range(16)] for _ in range(16)]
    with open(encrypted_filename, 'r') as f:
        for i in range(16):
            for j in range(16):
                line = f.readline()
                if not line:
                    raise ValueError("Le fichier est trop court (moins de 256 lignes)")
                encrypted_image[i][j] = line.strip()
    
    return encrypted_image

def invert_image(image_filename):
    image = load_encrypted_image(image_filename)
    inverted_image = [str(not_h(int(image[i][j]))) for i in range(16) for j in range(16)]
    output_filename = image_filename.replace(".enc", "_invert.enc")
    with open(output_filename, "w") as f:
        f.write("\n".join(inverted_image))
    
def add_images(image1_filename, image2_filename):
    image1 = load_encrypted_image(image1_filename)
    image2 = load_encrypted_image(image2_filename)
    
    added_image = [[str(or_h(int(image1[i][j]), int(image2[i][j]))) for j in range(16)] for i in range(16)]
    base1 = image1_filename[:-4]
    base2 = image2_filename[:-4]
    output_filename = f"{base1}+{base2}_add.enc"
    with open(output_filename, "w") as f:
        for row in added_image:
            f.write("\n".join(row) + "\n")
    
def xor_images(image1_filename, image2_filename):
    image1 = load_encrypted_image(image1_filename)
    image2 = load_encrypted_image(image2_filename)
    
    xor_image = [[str(xor_h(int(image1[i][j]), int(image2[i][j]))) for j in range(16)] for i in range(16)]
    base1 = image1_filename[:-4]
    base2 = image2_filename[:-4]
    output_filename = f"{base1}+{base2}_xor.enc"
    with open(output_filename, "w") as f:
        for row in xor_image:
            f.write("\n".join(row) + "\n")
            
def multiply_images(image1_filename, image2_filename):
    image1 = load_encrypted_image(image1_filename)
    image2 = load_encrypted_image(image2_filename)
    
    multiplied_image = [[str(and_h(int(image1[i][j]), int(image2[i][j]))) for j in range(16)] for i in range(16)]
    base1 = image1_filename[:-4]
    base2 = image2_filename[:-4]
    output_filename = f"{base1}+{base2}_multiply.enc"
    with open(output_filename, "w") as f:
        for row in multiplied_image:
            f.write("\n".join(row) + "\n")
            
def compress_image(image_filename):
    image = load_encrypted_image(image_filename)
    compressed_image = compress_function(image)
    output_filename = image_filename.replace(".enc", "_compress.enc")
    with open(output_filename, "w") as f:
        for row in compressed_image:
            f.write("\n".join(row) + "\n")
            
def compress_black_image(image_filename):
    image = load_encrypted_image(image_filename)
    compressed_image = compress_black_function(image)
    output_filename = image_filename.replace(".enc", "_compress_black.enc")
    with open(output_filename, "w") as f:
        for row in compressed_image:
            f.write("\n".join(row) + "\n")
            
def destroy_image(image_filename):
    image = load_encrypted_image(image_filename)
    inverted_image = [str(destroy_bit(int(image[i][j]))) for i in range(16) for j in range(16)]
    output_filename = image_filename.replace(".enc", "_destroy.enc")
    with open(output_filename, "w") as f:
        f.write("\n".join(inverted_image))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 server.py <invert | compress | compress_black | destroy | add | xor | multiply> <img1> [img2]")
        sys.exit(1)

    action = sys.argv[1]
    if action not in ["invert", "compress", "compress_black", "destroy", "add", "xor", "multiply"]:
        print("Usage: python3 server.py <invert | compress | compress_black | destroy | add | xor | multiply> <img1> [img2]")
        sys.exit(1)
    img1 = sys.argv[2]
    img2 = None
    if len(sys.argv) > 3:
        img2 = sys.argv[3]
    if action == "invert":
        invert_image(img1)
    if action == "compress":
        compress_image(img1)
    if action == "compress_black":
        compress_black_image(img1)
    if action == "destroy":
        destroy_image(img1)
    if action == "add":
        add_images(img1, img2)
    elif action == "xor":
        xor_images(img1, img2)
    elif action == "multiply":
        multiply_images(img1, img2)
