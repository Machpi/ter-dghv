import numpy as np
import sys
import matplotlib.pyplot as plt

sys.set_int_max_str_digits(10**6)

# Reads a 16x16 file and converts it to a binary matrix
def lire_fichier_binaire(chemin_fichier):
    matrice = []
    with open(chemin_fichier, 'r') as fichier:
        for ligne in fichier:
            ligne = ligne.strip()
            if len(ligne) != 16 or not set(ligne).issubset({'0', '1'}):
                raise ValueError("Chaque ligne doit contenir exactement 16 caractères 0 ou 1.")
            matrice.append([int(c) for c in ligne])
    if len(matrice) != 16:
        raise ValueError("Le fichier doit contenir exactement 16 lignes.")
    return np.array(matrice, dtype=np.uint8)

def afficher_image(img_binaire, titre):
    plt.gcf().set_facecolor('0.8')
    plt.imshow(img_binaire, cmap='gray', vmin=0, vmax=1)
    plt.title(titre)
    plt.axis('off')
    plt.show()

# Read images
if len(sys.argv) < 2:
  raise ValueError("Veuillez fournir le nom du fichier image en argument.")

image_filename = sys.argv[1]
img = lire_fichier_binaire(image_filename)

# Affichage
afficher_image(img, "")