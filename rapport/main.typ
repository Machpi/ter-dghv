#import "@preview/ilm:1.4.1" : *
#import "@preview/lovelace:0.3.0" : *
#import "@preview/tablex:0.0.9" : *

#set text(lang: "fr")

#show: ilm.with(
  title: [Rapport de TER :\ Chiffrement Homomorphe],
  author: "Mathias Loiseau, Jonathan Long,\n Tom Noel, Ryan Germain, Aly Elhussieny",
  date: none,
  abstract: [
    Année 2024—2025
  ],
  figure-index: (enabled: true, title : "Figures"),
  bibliography: bibliography("refs.bib"),
  table-index: (enabled: false),
  listing-index: (enabled: false),
  chapter-pagebreak: true
)

= Sujet

Un schéma de chiffrement complètement homomorphe garantit que pour $n$ clairs $m_1, ..., m_n$, leurs chiffrés respectifs $c_1, ..., c_n$ et une fonction $f$ :\ $"Decrypt"_("sk")(f(c_1, ..., c_n)) = f(m_1, ..., m_n)$.\

Notre projet consiste en l'étude et en l'implémentation du schéma de chiffrement complètement homomorphe de van Dijk, Gentry, Halevi et Vaikuntanathan (DGHV) @dghv.\
Le schéma DGHV est l'extension d'un schéma dit _somewhat homomorphic_.\
À ne pas confondre avec un schéma partiellement homomorphe comme le RSA (pour la multiplication) ou le cryptosystème de Paillier (pour l'addition), un schéma _somewhat homomorphic_ est un schéma qui permet d'effectuer n'importe quelle opération sur les données chiffrées, mais pas de manière illimitée.\

Un tel schéma peut servir à construire des protocoles de calcul distribué,
où les données sont chiffrées et envoyées à un serveur qui exécute la fonction $f$
sur les données chiffrées, sans jamais connaître les données elles-mêmes.\

#show link: underline
Le code est disponible sur #link("https://github.com/Machpi/ter-dghv/tree/main")[GitHub]


= BigInt

Nous avons dans un premier temps essayé d'implémenter en C une bibliothèque de calcul sur des entiers de taille arbitraire. Nous avons plus tard décidé de nous concentrer sur l'implémentation du schéma DGHV, mais nous avons tout de même gardé cette bibliothèque car la théorie qu'elle met en œuvre est intéressante.

Le type BigInt est un tableau d'entiers de 32 bits, qui représente un entier de taille arbitraire. 

== Théorie

=== Multiplication de Karatsuba

Si l'on veut multiplier deux entiers binaires de $n$ chiffres, la méthode naïve consiste à faire $O(n^2)$ opérations (méthode scolaire).\
La méthode de Karatsuba consiste à diviser les deux entiers en deux moitiés :\
$A = a_n a_(n-1)... a_1 = underbrace(a_n...a_(n/2) times 2^(n/2),A_1) + underbrace(a_(n/2-1) ... a_1, A_0)$\
Ainsi,\

$A times B &= (A_1 times 2^(n/2) + A_0) + (B_1 times 2^(n/2) + B_0)\
&= A_1 B_1 times 2^n + (A_1 B_0 + A_0 B_1) times 2^(n/2) + A_0 B_0\
&= A_1 B_1 times 2^n + (A_1 B_1 + A_0 B_0 - (A_1 - A_0)(B_1 - B_0)) times 2^(n/2) + A_0 B_0$\
Ce calcul peut sembler plus complexe, mais en réalité il ne consiste qu'en 3 multiplications de taille $n/2$ et des bitshifts (détaillés plus tard). On applique alors la technique diviser pour régner, jusqu'à obtenir des multiplications de taille 32-bits, qui sont gérées très efficacement par le matériel et dont le résultat est sur 64 bits.\

=== Algorithme de Miller-Rabin

Nous avons utilisé l'algorithme de Miller-Rabin pour générer des nombres premiers dans notre biliothèque BigInt :\
- On génère un nombre aléatoire $n$ d'un nombre arbitraire de mots (32 bits)
- On met son bit de poids faible à 1 (pour qu'il soit impair dans tous les cas)
- On teste la division par des premiers triviaux
- S'il passe les tests, on le teste avec l'algorithme de Miller-Rabin :\
  - On écrit $n-1$ sous la forme $2^s times d$ avec $d$ impair
  - On choisit un entier aléatoire $a in [|2,n-2|]$
  - On calcule $x = a^d mod n$
  - Si $x = 1$ ou $x = n-1$, on continue
  - Sinon, on fait $s-1$ itérations de la boucle suivante :\
    - On calcule $x = x^2 mod n$
    - Si $x = n-1$, on continue
    - Sinon, on sort de la boucle
  - Si on est sorti de la boucle, c'est que le nombre n'est pas premier
On répète ce test un certain nombre de fois pour se convaincre que le nombre est bien premier.\
C'est un algorithme probabiliste fondé sur le petit théorème de Fermat : si $p$ est premier et $p divides.not a$, alors $a^(p-1) = (a^d)^2^s = 1 mod p$.\
Donc par contraposée, si $a^d != 1 mod p$, et $exists r in [|0,s-1|], a^(2^r d) != 1 mod p$,\ c'est que $p$ n'est pas premier.\

La réciproque du théorème de Fermat est fausse, mais l'algorithme, avec un nombre suffisant d'itérations, est très efficace pour tester la primalité d'un nombre.\

== Implémentation

#let shift = $italic("shift")$
#let bsl = $italic("bitshift_left")$
#let mots = $italic("mots")$
#let chiffres = $italic("chiffres")$

Le seul détail d'implémentation qui mérite d'être mentionné est la multiplication par une puissance de 2 (bitshift à gauche) des BigInt : 

#pseudocode-list(title:[$bsl(T[n], shift)$],booktabs:true)[
  + $mots <- floor(shift\/ 32)$
  + $chiffres <- shift mod 32$
  + $n' <- n + mots + (1 "ssi" chiffres != 0)$
  + Allouer un nouveau tableau $T'[n']$
  + *Pour* $i = n' - 1$ à $i = n - 1$
    + *Si* $chiffres = 0$
      + $T'[i] <- T'[i-mots]$
    + *Sinon*
      + $D <- T[i-mots-1] >> (32 - chiffres)$
      - \% ie on ne garde que les $32 - chiffres$
      - \% premiers bits du mot prédécent 
      + *Si* $i != n'$
        + $G <- T[i-mots] << chiffres$
      + *Sinon* $G <- 0$
      + $T'[i] <- G + D$
  + Supprimer $T'[n]$
]

L'implémentation est évidemment moins efficace que celle de bibliothèques spécialisées, mais elle peut tout de même trouver un nombre premier de 512 bits en moins de dix secondes, et un premier de 1024 bits en une à quelques minutes sans paralélisme.\

Nous utiliserons dans la suite la bibliothèque GNU Multiple Precision Arithmetic Library (GMP) pour les calculs sur des entiers de taille arbitraire, car elle est bien plus efficace que notre implémentation.

= Schéma _somewhat homomorphic_ : opérations côté client

== Théorie

Contrairement à d'autres schémas qui reposent sur des problèmes de factorisation, le schéma DGHV repose sur le problème du PGCD Approché (AGCD).\
Le problème du PGCD approché, étant donné des $x_i = q_i p + r_i$ (avec $p$, les $q_i$, $r_i$ secrets), de distinguer les $x_i$ d'une distribution aléatoire uniforme.\
Plus précisément, on se permet un nombre polynomial en $lambda$ (paramètre de sécurité) de $x_i$ où $p$ est de taille $eta = tilde(cal(O))(lambda^2)$, les $q_i$ sont de taille $gamma = tilde(cal(O))(lambda^3)$ et les $r_i$ sont de taille $rho = cal(O)(lambda)$.\
(Où $tilde(cal(O))(lambda^2) = cal(O)(lambda^2log^k lambda)$ pour un certain $k$)\

Le problème que l'on va rencontrer est la taille du bruit des chiffrés, qui augmente rapidement avec le nombre d'opérations effectuées :\
- L'addition de deux chiffrés de taille $n$ donne un chiffré de taille $n + 1$
- La multiplication de deux chiffrés de taille $n$ donne un chiffré de taille $2n + 1$\
On va donc vouloir faire une opération dite _bootstrapping_, qui consiste à "ré-évaluer" un chiffré pour réinitialiser son bruit, afin de pouvoir répéter des opérations.

== Premier schéma basique

Le schéma DGHV basique dont nous avons vu le principe en cours est le suivant :\
- Choisir $p$ comme clé privée
- Chiffrement : choisir $q$ et $r$ aléatoires, et $E(m) = m + q p + 2r$
- Déchiffrement : $D(c) = c mod 2 mod p$
Ce schéma permet les additions et les multiplications, mais en quantité limitée, car la taille des chiffrés augmente rapidement.\

== Schéma DGHV

Le schéma DGHV initial permet de créer un chiffré sans avoir à connaître la clé privée. Cela peut sembler inutile, mais on verra une application dans la suite.\
- La clé privée est toujours la même, $p$
- Pour générer la clé publique, on va prendre $tau + 1$ échantillons de la forme $x_i = p q_i + r_i$ avec $forall i in [|0, tau|], x_0 >= x_i$. Il faut également que $x_0$ soit pair, et $x_0 mod p$ soit impair.
- #[Chiffrement : on choisit un $r$ aléatoire, et un sous-ensemble $S$ de ces $x_i$ ($i in [|1, tau|]$), et\
$E(m) = m + 2r + 2 limits(sum)_(i in S)x_i mod x_0$]
- Déchiffrement : $D(c) = c mod 2 mod p$

On peut voir les $2x_i$ comme des chiffrés de 0. En quelque sorte, on a un ensemble de chiffrés de 0 (pas exactement vu qu'on a $r$ et non $2r$ dans la formule).

Remarque : le déchiffrement est le même que pour le schéma basique. Nous l'avons également constaté lors de nos tests, mais ces deux schémas sont "compatibles".

À ce stade, on peut déjà effectuer un bootstrapping en ayant un circuit binaire comme abordé en cours, mais l'article conclut qu'un tel circuit est trop profond et génère des chiffrés avec des bruits trop importants.

== Implémentation

On utilise comme expliqué précédemment la bibliothèque GMP pour effectuer les calculs sur des entiers de taille arbitraire.\
Cela nécessite d'utiliser le flag _-lgmp_ lors de la compilation pour lier la bibliothèque GMP.\

Il est important de noter que nous utilisons des valeurs très petites comparées à celles recommandées @paramVals pour le schéma DGHV : 

#figure(
  image("img/paramVals.png"),
  caption: "Valeurs de paramètres recommandées pour le schéma DGHV",
)

Pour nos valeurs (proches du niveau _toy_), nous avons constaté qu'au bout de 52 opérations $and$ ou $or$, le bruit était trop important pour pouvoir déchiffrer.

= Schéma _somewhat homomorphic_ : opérations côté serveur

Le serveur reçoit des données chiffrées et effectue des opérations sur ces données.\

== Théorie

=== Opérations de base

Étant en arithmétique modulo 2, les opérations de base sont l'addition et la multiplication dans
$ZZ\/2ZZ$. On peut donc établir la correspondance suivante :

Supposons $a,b in {0,1}$,\
$D(E(a) * E(b)) = a and b$\
$D(E(a) + E(b)) = a xor b$\
En effet, il faut faire attention : dans $ZZ\/2ZZ$, $overline(1)+overline(1) = overline(0)$

- Cependant, on peut construire l'opération $or$ à partir de $and$ et $xor$ :\
$a or b = a xor b xor (a and b)$\
On remarque déjà que l'opération $or$ sera plus aussi coûteuse que le $and$, et que l'on devra favoriser l'utilisation du $xor$.

- On peut également construire l'opération $not$ à partir de $xor$ :
$not a = a xor 1$\
Cependant, cela implique de pouvoir chiffrer un 1 "à la volée", c'est-à-dire sans disposer de la clé privée. C'est ce que nous avons vu dans la partie précédente.

=== Circuits et calculs

Un circuit est une représentation d'une fonction booleénne sous forme de portes logiques connectées. Nous avons vu en TD que n'importe quelle fonction boolénne\ $f : (ZZ\/2ZZ)^n arrow.long ZZ\/2ZZ$ pouvait s'écrire comme un polynôme en $n$ variables et de degré au plus $n$. Or, il existe une équivalence formelle entre les circuits et les polynômes que nous ne détaillerons pas ici.\
Ce qu'il faut retenir, c'est que dans le schéma actuel, les circuits sont en clair, c'est-à-dire que le serveur sait quelles opérations il effectue sur les données chiffrées. Il existe cependant des schémas de chiffrement homomorphe où le serveur n'a pas conscience des opérations qu'il effectue.

== Implémentation : exemple des images

Nous avons implémenté dans le fichier _server.py_ un exemple de serveur
qui reçoit des images chiffrées et effectue des opérations de base sur ces images.

Nous avons vu précédemment comment nous avons encodé les opérations binaires de base en ne partant que de l'addition et de la multiplication dans $ZZ\/2ZZ$.\
Pour notre exemple, nous avons également implémenté l'opération de compression.

#grid(
  columns:3,
  align:center,
  figure(
    image("img/squaremix.png", width: 80%),
    caption: "Image squaremix.png",
  ),
  
  figure(
    image("img/squaremixcompress.png", width: 80%),
    caption: "Image compressée à égalité blanche",
  ),

  figure(
    image("img/squaremixcompressblack.png", width: 80%),
    caption: "Image compressée à égalité noire",
  ),
)

Pour ce faire, nous avons eu besoin d'implémenter les opérations de compression, qui prennent en entrée 4 bits et en sortie 1 bit :

#figure(
  grid(
    columns:2,
    gutter:.5cm,
    tablex(
    columns: 5,
    [], [00], [01], [10], [11],
    [00], [0], [0], [0], [1],
    [01], [0], [0], [1], [1],
    [10], [0], [1], [1], [1],
    [11], [1], [1], [1], [1],
    ),
    tablex(
    columns: 5,
    [], [00], [01], [10], [11],
    [00], [0], [0], [0], [0],
    [01], [0], [0], [0], [1],
    [10], [0], [0], [1], [1],
    [11], [0], [1], [1], [1],
    )
  ),
  caption : "Tables de vérité de l'opération de compression à égalité blanche et noire",
)

Il faut rappeler que l'opération $and$ est plus coûteuse que l'opération $xor$, en plus de significativement augmenter la taille du chiffré. On souhaite donc, dans nos circuits, minimiser l'utilisation de $and$ et $or$ (que l'on a construit à partir d'un $and$), mais surtout minimiser leur imbrication.\

Nous avons donc proposé ce circuit pour l'opération de compression à égalité blanche :

#grid(
  columns:2,
  gutter:.5cm,
  figure(
    image("img/circuit1.png", width: 120%),
    caption: "Circuit pour l'opération de compression à égalité blanche",
  ),
  grid(
    columns:1,
    gutter:.5cm,
    figure(
    image("img/circuitcode.png", width: 58%),
    caption: "Code Python correspondant aux deux circuits",
    ),
    [On doit faire attention à ne pas répéter les calculs, c'est pour cela qu'on doit utiliser des variables intermédiaires pour simuler les fils du circuit.]
  )
  
)
  
= Chiffrement complètement homomorphe

L'article construit un schéma de chiffrement complètement homomorphe à partir du schéma initial _somewhat homomorphic_ en ajoutant une étape de bootstrapping

#let sk = $"sk"$
#let pk = $"pk"$

On souhaite toujours obtenir un bootstrapping en chiffrant un circuit, mais ce circuit doit être plus simple que celui que l'on a vu précédemment.\

On ajoute 3 paramètres :\
$kappa = (gamma*eta)/(rho')$, $theta = lambda$, et $Theta = omega(kappa*log lambda)$.

== Génération de clé
On génère dans un premier temps une clé privée $sk^*$ et une clé publique $pk^*$ comme vu précédemment.\

Pour générer notre nouvelle clé privée, on va générer aléatoirement un vecteur de taille $Theta$ de bits, dont seulement $theta$ bits sont à 1. On appelle ce vecteur $arrow(s)$.\

On va ensuite calculer un vecteur $arrow(y)$ de $Theta$ valeurs à la clé publique où $forall i in [|1, Theta|], y_i in [0,2[$ avec une précision de $kappa$ bits après la virgule tel qu'il existe un sous-ensemble $S subset {1,...,Θ}$ de taille $theta$ tel que $limits(sum)_(i in S) y_i = limits(sum)_(i=1)^Theta s_i y_i approx 1/p (mod 2)$.\
(Plus précisément, on est à $2^(-kappa)$ près de $1/p$)

Nos nouvelles clés sont :
- Clé privée : $arrow(s)$
- Clé publique : $(pk^*, arrow(y))$

On remarque déjà que le nombre premier $p$ n'apparait plus explicitement dans les clés. Il n'est présent qu'implicitement dans le vecteur $arrow(y)$. Intuitivement, on a réduit la taille de la clé pour simplifier le circuit de déchiffrement que l'on utilise dans le bootstrapping.

== Chiffrement

Maintenant, quand on veut chiffrer un message $m$, en plus de calculer son chiffré $c^*$ avec la clé publique comme vu précédemment, on va calculer un vecteur $arrow(z)$ de taille $Theta$ tel que :\
$forall i in [|1, Theta|], z_i = c^* dot y_i mod 2$, en conservant uniquement $ceil(log theta) + 3$ bits de précision après la virgule.
Plus précisément, l'opération mod 2 ci-dessus conserve la partie décimale, mais applique le modulo 2 à la partie entière.\
On a donc $forall i in [|1, Theta|], z_i in [0,2[$

== Déchiffrement 
On simplifie la division $c^* /p$ par une somme pondérée des $z_i$, on utilise la clé secrète $arrow(s)$ pour ne combiner que les bons $z_i$ et on retrouve le message $m = c^* - floor.l sum_i s_i * z_i ceil.r mod 2$.\
L'article démontre que pour des bonnes valeurs de $kappa, theta, "et" Theta$, la différence entre la valeur réelle et la valeur déchiffrée avant l'arrondi est inférieure à $1/4$, ce qui assure la validité du message déchiffré.\

== Bootstrapping
Nous n'avons pas pu implémenter le bootstrapping pour le schéma DHGV.\
Dans les grandes lignes, l'article démontre qu'il est possible de décomposer l'opération de déchiffrement $m = c^* - floor.l sum s_i z_i ceil.r mod 2$ en trois sous-circuits qui peuvent être exprimés comme des polynômes pouvant être évalués homomorphiquement par le schéma, à condition que les paramètres soient choisis de façon à ce que leur degré et leur taille restent dans les bornes supportées par le bruit maximal autorisé.

= Conclusion

Nous avons partiellement implémenté le schéma DGHV, assez pour en comprendre les principe et en dériver une application simple. 
Cela nous a permis de nous familiariser avec les schémas de chiffrement homomorphe et d'en comprendre l'utilité et les limitations.\
Nous avons au passage implémenté une petite biliothèque de calcul sur des entiers de taille arbitraire, ce qui nous a permis de mieux comprendre les algorithmes mis en œuvre, même si la bibliothèque GMP est immensément plus efficace.