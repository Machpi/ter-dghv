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

#slide(title:"Que fait le serveur ?", outlined: true)[
  - Manipule uniquement des chiffrés $E(b)$
  - Additionne des chiffrés : $D(E(a)+E(b)) = a xor b$
  - Multiplie des chiffrés : $D(E(a) times E(b)) = a and b$
]

#slide(title: "Que peut-on en faire ?", outlined: true)[
  $->$ Tout.\
  Plus précisément, tout circuit booléen peut être évalué.

  - Déjà, $a or b = a xor b xor (a and b)$
  - Puis $not a = a xor 1$
]

#slide(title : "Exemple sur des tableaux de 256 chiffrés")[
  Le chiffrement se fait sur des bits seuls, mais on peut en chiffrer plusieurs et les faire interagir.
  #cols(columns: (2fr, 2fr), gutter: 2em)[
    #align(center)[
      #image("img/squaremix.png", width:70%)
      Tableau de 256 bits affiché en 16×16
    ]
  ][
    #align(center)[
      #image("img/squaremixcompress.png", width:70%)
      Même tableau, après la fonction _compress_
    ]
  ]
]

#title-slide[
  Schéma _fully homomorphic_
]

#let sk = $"sk"$
#let pk = $"pk"$

#slide(title: "Principe du bootstrapping", outlined: true)[
  - Soit $f : sk -> D_sk (c)$
  - Alors on peut construire un circuit $cal(C)$ tel que $D_sk (c) = cal(C)(c)$
  - Les bits de la clé sont les entrées du circuit
  - On chiffre ces entrées et on évalue $cal(C)$ homomorphiquement
  - Et on obtient $E(D_sk (c))$ = c'
  - $D_sk (c') = D_sk (c)$
]

#slide(title: "Que changer ?")[
  - Idée : simplifier le déchiffrement pour que le circuit correspondant
  - Solutions : 
    - Diminuer la taille de la clé privée
    - Simplifier la fonction de déchiffrement
]

#slide(title: "Schéma _fully homomorphic_ DGHV", outlined: true)[
  - On génère une clé publique $pk^*$ et une clé privée $sk^*$ comme précédemment
]
