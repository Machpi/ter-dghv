import json
import random
import sys
import subprocess
import os

sys.set_int_max_str_digits(10**6)
CLIENT_PATH = "./homomorphic_encryption/client"

# Secret key
with open("sk.json") as f:
    sk_data = json.load(f)
    p = sk_data["p"]
    s = sk_data["s"]

# Encrypts a single bit with the C file
def encrypt_with_client(b, p):
  b_str = str(b)
  p_str = str(p)
  try:
      result = subprocess.run(
          [CLIENT_PATH, "encrypt", b_str, p_str],
          check=True,
          stdout=subprocess.PIPE,
          stderr=subprocess.PIPE,
          text=True
      )
      output = result.stdout.strip()
      return int(output)
  except subprocess.CalledProcessError as e:
      print("Erreur d'exécution du programme C:")
      print(e.stderr)
      raise
  except ValueError:
      print(f"Sortie inattendue : {output!r}")
      raise

# Decrypts a single message with the C file
def decrypt_with_client(ciphertext, secret_key):
    c_str = str(ciphertext)
    p_str = str(secret_key)
    try:
        result = subprocess.run(
            [CLIENT_PATH, "decrypt", c_str, p_str],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        output = result.stdout.strip()
        return int(output)
    except subprocess.CalledProcessError as e:
        print("Erreur d'exécution du programme C:")
        print(e.stderr)
        raise
    except ValueError:
        print(f"Sortie inattendue : {output!r}")
        raise

# Encrypts a 16x16 image to a file with 256 lines
def encrypt_image(input_filename, p):
    encrypted_values = []
    with open(input_filename, 'r') as f:
        for line in f:
            line = line.strip()
            if len(line) != 16:
                raise ValueError("Chaque ligne doit contenir 16 caractères 0/1")
            for char in line:
                if char not in ('0', '1'):
                    raise ValueError("Seuls '0' et '1' sont autorisés")
                bit = int(char)
                encrypted_bit = encrypt_with_client(bit, p)
                encrypted_values.append(str(encrypted_bit))
    output_filename = os.path.join(os.path.basename(input_filename) + ".enc")
    with open(output_filename, 'w') as f:
        f.write('\n'.join(encrypted_values))
    print(f"Chiffrement réussi. {len(encrypted_values)} valeurs écrites dans {output_filename}")
    return output_filename

# Decrypts a file with 256 lines to 16x16 image
def decrypt_image(encrypted_filename, p):
    try:
        with open(encrypted_filename, 'r') as f:
            encrypted_values = [line.strip() for line in f if line.strip()]
        decrypted_bits = []
        for i, enc_val in enumerate(encrypted_values):
            try:
                decrypted_bit = decrypt_with_client(enc_val, p)
                decrypted_bits.append(str(decrypted_bit))
            except Exception as e:
                print(f"Erreur ligne {i+1}: {str(e)}")
                decrypted_bits.append("0")  # Valeur par défaut
        decrypted_lines = []
        for i in range(0, len(decrypted_bits), 16):
            line = ''.join(decrypted_bits[i:i+16])
            decrypted_lines.append(line)
        output_filename = os.path.join(os.path.basename(encrypted_filename).replace('.enc', '.dec'))
        with open(output_filename, 'w') as f:
            f.write('\n'.join(decrypted_lines))
        print(f"Résultat déchiffré sauvegardé dans {output_filename}")
        return output_filename 
    except Exception as e:
        print(str(e))
        return None
      
if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 read_image.py <encrypt> | <decrypt> <image>")
        sys.exit(1)

    action = sys.argv[1]
    if action not in ["encrypt", "decrypt"]:
        print("Usage: python3 read_image.py <encrypt> | <decrypt> <image>")
        sys.exit(1)
    image_filename = sys.argv[2]
    if action == "encrypt":
        encrypt_image(image_filename, p)
    elif action == "decrypt":
        decrypt_image(image_filename, p)
