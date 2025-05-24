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
    output_filename = image_filename.replace(".enc", "_inverted.enc")
    with open(output_filename, "w") as f:
        f.write("\n".join(inverted_image))
    print(f"Image inversée sauvegardée dans {output_filename}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 server.py <invert | compress | add | xor | multiply> <img1> [img2]")
        sys.exit(1)

    action = sys.argv[1]
    if action not in ["invert", "add", "xor", "multiply"]:
        print("Usage: python3 server.py <invert | compress | add | xor | multiply> <img1> [img2]")
        sys.exit(1)
    img1 = sys.argv[2]
    if action == "invert":
        invert_image(img1)
    # if action == "compress":
    #     compress_image(img1)
    # if action == "add":
    #     add_image(img1, img2)
    # elif action == "xor":
    #     xor_image(img1, img2)
    # elif action == "multiply":
    #     multiply_image(img1, img2)
