#import "@preview/ilm:1.4.1" : *
#import "@preview/lovelace:0.3.0" : *

#set text(lang: "fr")

#show: ilm.with(
  title: [Rapport de TER :\ Chiffrement Homomorphe],
  author: "Mathias Loiseau, Jonathan Long, ",
  date: none,
  abstract: [
    Année 2024—2025
  ],
  bibliography: none,
  figure-index: (enabled: true),
  table-index: (enabled: true),
  listing-index: (enabled: true),
  chapter-pagebreak: true
)

= Sujet

Un schéma de chiffrement complètement homomorphe garantit que pour $n$ clairs $m_1, ..., m_n$, leurs chiffrés respectifs $c_1, ..., c_n$ et une fonction f, $"Decrypt"_("SK")(f(c_1, ..., c_n)) = f(m_1, ..., m_n)$.\

Notre projet consiste en l'étude et en l'implémentation du schéma de chiffrement complètement homomorphe de van Dijk, Gentry, Halevi et Vaikuntanathan (DGHV).\
Le schéma DGHV est l'extension d'un schéma dit _somewhat homomorphic_ :\
À ne pas confondre avec un schéma partiellement homomorphe comme le RSA (pour la multiplication) ou le cryptosystème de Paillier (pour l'addition), un schéma _somewhat homomorphic_ est un schéma qui permet d'effectuer un nombre limité d'opérations sur les données chiffrées, mais pas de manière illimitée.\

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
Plus précisément, on se permet un nombre polynomial en $lambda$ (paramètre de sécurité) de $x_i$ où $p$ est de taille $tilde(cal(O))(lambda^2)$, les $q_i$ sont de taille $tilde(cal(O))(lambda^3)$ et les $r_i$ sont de taille $cal(O)(lambda)$.\
(Où $tilde(cal(O))(lambda^2) = cal(O)(lambda^2log^k lambda)$ pour un certain $k$)\

== Implémentation

On utilise comme expliqué précédemment la bibliothèque GMP pour effectuer les calculs sur des entiers de taille arbitraire.\


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

Cependant, on peut construire l'opération $or$ à partir de $and$ et $xor$ :\
$a or b = a xor b xor (a and b)$\
On remarque déjà que l'opération $or$ sera plus aussi coûteuse que le $and$, et que l'on devra favoriser l'utilisation du $xor$.

=== Circuits

Un circuit peut être vu comme un DAG où les nœuds sont des opérations de base,
et les arêtes sont des entrées et sorties de ces opérations.\

== Implémentation : exemple des images

Nous avons implémenté dans le fichier _server.py_ un exemple de serveur
qui reçoit des images chiffrées et effectue des opérations de base sur ces images.



= Chiffrement complètement homomorphe

Le schéma DGHV construit un schéma de chiffrement complètement homomorphe à partir du schéma initial _somewhat homomorphic_ en ajoutant une étape de bootstrapping

== Déchiffrement par approximation

== Bootstrapping

