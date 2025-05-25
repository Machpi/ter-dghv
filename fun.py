import os
import subprocess
import sys

# Paths
IMAGE_DIR = "./image_examples/"
CLIENT_PATH = "./homomorphic_encryption/client"
SERVER_PATH = "./homomorphic_encryption/server.py"
IMAGE_PATH = "./homomorphic_encryption/read_image.py"
DISPLAY_PATH = "./image_display/display.py"
SK_PATH = "sk.json"
PK_PATH = "pk.json"

IMAGE1 = "squares.txt"
IMAGE2 = "cross.txt"

def run_command(command):
  try:
    result = subprocess.run(command, check=True, text=True, capture_output=True)
    return result.stdout.strip()
  except subprocess.CalledProcessError as e:
    print(f"Échec lors de l'exécution de : {e}")
    print(f"Erreur : {e.output}")
    sys.exit(1)
    
def generate_keys():
  print("Génération des clés...")
  command = [CLIENT_PATH, "key"]
  run_command(command)
    
def handle_keys():
  if os.path.exists(SK_PATH) and os.path.exists(PK_PATH):
    print("Clés existantes trouvées")
  else :
    generate_keys()
  print(f"Clés : {SK_PATH}, {PK_PATH}\n")
  
def get_result_name(action):
  name1 = IMAGE1
  name2 = IMAGE2
  if action in ["invert", "compress", "compress_black"]:
    return f"{name1}_{action}.enc"
  else :
    return f"{name1}+{name2}_{action}.enc"
  
if __name__ == "__main__":
  # Check for images
  if not all(os.path.exists(os.path.join(IMAGE_DIR, img)) for img in [IMAGE1, IMAGE2]):
    print(f"Images manquantes dans {IMAGE_DIR} : {IMAGE1}, {IMAGE2}.")
    sys.exit(1)
        
  # Find or generate keys
  handle_keys()
  
  # Encrypt images
  command1 = ["python3", IMAGE_PATH, "encrypt", os.path.join(IMAGE_DIR, IMAGE1)]
  command2 = ["python3", IMAGE_PATH, "encrypt", os.path.join(IMAGE_DIR, IMAGE2)]
  run_command(command1)
  run_command(command2)
  encrypted_img1 = os.path.join(os.path.basename(IMAGE1) + ".enc")
  encrypted_img2 = os.path.join(os.path.basename(IMAGE2) + ".enc")
  print(f"Images chiffrées : {encrypted_img1}, {encrypted_img2}\n")
  
  
  # Transform
  transformations = [
    ["invert", encrypted_img1],
    ["add", encrypted_img1, encrypted_img2],
    ["xor", encrypted_img1, encrypted_img2],
    ["multiply", encrypted_img1, encrypted_img2],
    ["compress", encrypted_img1],
    ["compress_black", encrypted_img1]
  ]
  
  for transform in transformations:
    action = transform[0]
    args = transform[1:]
    print(f"Transformation : {action} sur {args}")
    run_command(["python3", SERVER_PATH, action] + args)
    result_name = get_result_name(action)
    print(f"Déchiffrement de {result_name}...")
    run_command(["python3", IMAGE_PATH, "decrypt", result_name])
    decrypted_image = result_name.replace(".enc", ".dec")
    run_command(["python3", DISPLAY_PATH, decrypted_image])
    
  # Clean up
  print("Nettoyage des fichiers")
  extensions = (".enc", ".dec", ".json")
  for filename in os.listdir('.'):
      if filename.endswith(extensions):
          try:
              os.remove(filename)
              print(f"Supprimé : {filename}")
          except Exception as e:
              print(f"Erreur lors de la suppression de {filename} : {e}")
