#import "@preview/typslides:1.2.6": *

// Project configuration
#show: typslides.with(
  ratio: "16-9",
  theme: "purply",
  font: "Fira Sans",
  link-style: "color",
)

// The front slide is the first slide of your presentation
#front-slide(
  title: "This is a sample presentation",
  subtitle: [Année 2024-2025],
  authors: "Mathias Loiseau, Jonathan Long,\n Tom Noel, Ryan Germain, Aly Elhussieny",
  info: [#link("https://github.com/manjavacas/typslides")],
)

// Title slides create new sections
#title-slide[
  Sujet
]

#title-slide[
  BigInt
]

#slide(title: "Algorithme de Karatsuba", outlined: true)[
  - Multiplication $A times B$ en $cal(O)(n^1.54)$ (au lieu de $cal(O)(n^2)$)
  - Principe diviser pour régner :
    - On divise $A = A_1 dot 2^(n/2) + A_0$, idem pour $B$
    - $A times B &= A_1 B_1 times 2^n + (A_1 B_0 + A_0 B_1) times 2^(n/2) + A_0 B_0\
    &= A_1 B_1 times 2^n + (A_1 B_1 + A_0 B_0 - (A_1 - A_0)(B_1 - B_0)) times 2^(n/2) + A_0 B_0$
  - Trois produits à calculer, et des bitshifts
  - On peut diviser jusqu'à multiplier sur des entiers de 32-bits
]

#slide(title: "Miller-Rabin", outlined: true)[
  - Test de primalité probabiliste
  - Théorème de Fermat : $p in PP, a in [2, p-1] => a^(p-1) mod p = 1$
  - Ainsi, si $a^(p-1) mod p != 1$, alors $p$ n'est pas premier
  - La réciproque du théorème de Fermat n'est pas vraie, mais on peut l'utiliser pour tester la primalité en effectuant plusieurs tests
]

#title-slide[
  Schéma _somewhat homomorphic_ : client
]

#slide(title: "Un premier schéma basique", outlined: true)[
  - On choisit un premier $p$ comme clé privée
  - Pour chiffrer un bit $b$, on choisit $q$ et $r$ aléatoires, et $E(b) = b + p q + 2r$
  - $D(c) = c mod 2 mod p$
]

#slide(title: "Premier Schéma DGHV", outlined: true)[
  Introduction d'une clé publique :
  - Prendre $tau+1$ échantillons $x_i = p q_i + r_i$
  - Garder $x_0 >= x_i forall x_i, x_0 "pair", x_0 mod p "impair"$
  - Pour chiffrer un bit $b$, on choisit des $x_i in [|x_1, ..., x_Theta|]$ et $E(b) = b + 2r + 2 sum x_i mod x_0$
  - Le déchiffrement reste le même
  Première étape vers un schéma complètement homomorphe : chiffrement sans clé privée.
]

#title-slide[
  Schéma _somewhat homomorphic_ : serveur
]

#title-slide[
  Schéma _fully homomorphic_
]

