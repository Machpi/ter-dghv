#include <gmp.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <time.h>

#define ETA 512           // η : taille en bits de la clé secrète p
#define RHO 16             // ρ : taille en bits du bruit r
#define GAMMA 8192         // γ : taille en bits du bruit q
#define RHOP (RHO + 16)    // ρ' : bruit étendu pour chiffrement
#define TAU (GAMMA + 128)  // τ : nombre d'éléments xi dans la clé publique

#define KAPPA 262144  // κ : précision binaire des y_i
#define THETA 256     // Θ : nombre total de hints y_i
#define WEIGHT 32     // θ : poids de Hamming de s[]
#define PREC_BITS 11  // Précision en bits des y_i

gmp_randstate_t state;

void init_rand() {
  gmp_randinit_default(state);
  unsigned long seed;
  getrandom(&seed, sizeof(seed), 0);
  gmp_randseed_ui(state, seed);
}

// q in [0, 2^γ / p[
void generate_q(mpz_t q, const mpz_t p, unsigned int gamma) {
  mpz_t max_q;
  mpz_init(max_q);
  mpz_ui_pow_ui(max_q, 2, gamma);
  mpz_fdiv_q(max_q, max_q, p);
  mpz_urandomm(q, state, max_q);
  mpz_clear(max_q);
}

// r in ]−p/4, p/4[
void generate_r(mpz_t r, unsigned int rho) {
  mpz_t bound;
  mpz_init(bound);
  mpz_ui_pow_ui(bound, 2, rho);
  mpz_urandomm(r, state, bound);
  if (rand() % 2 == 1) mpz_neg(r, r);  // randomly negate
  mpz_clear(bound);
}

void generate_prime(mpz_t prime) {
  mpz_urandomb(prime, state, ETA);
  mpz_nextprime(prime, prime);
}

void generate_public_key(mpz_t *pk, const mpz_t p) {
  for (int i = 0; i <= TAU; i++) {
    mpz_init(pk[i]);
    mpz_t q, r, x;
    mpz_inits(q, r, x, NULL);
    generate_q(q, p, GAMMA);
    generate_r(r, RHO);
    mpz_mul(x, p, q);
    mpz_mul_ui(r, r, 2);
    mpz_add(pk[i], x, r);
    mpz_clears(q, r, x, NULL);
  }

  // x0 must be the largest
  for (int i = 1; i <= TAU; i++)
    if (mpz_cmp(pk[i], pk[0]) > 0) mpz_swap(pk[i], pk[0]);

  // x0 must be odd
  if (mpz_even_p(pk[0])) return generate_public_key(pk, p);

  mpz_t rem;
  mpz_init(rem);
  mpz_mod(rem, pk[0], p);
  // x0 mod p must be odd
  if (mpz_odd_p(rem)) return generate_public_key(pk, p);
  mpz_clear(rem);
}

void generate_bootstrap_secret_vector(int *s) {
  for (int i = 0; i < THETA; i++) s[i] = 0;
  int count = 0;
  while (count < WEIGHT) {
    int pos = rand() % THETA;
    if (s[pos] == 0) {
      s[pos] = 1;
      count++;
    }
  }
}

void generate_u_hints(mpz_t *u, const mpz_t p, const int *s) {
  mpz_t xp, modulus, sum, diff;
  mpz_inits(xp, modulus, sum, diff, NULL);

  // modulus = 2^{κ+1}
  mpz_ui_pow_ui(modulus, 2, KAPPA + 1);

  // xp = floor(2^κ / p)
  mpz_ui_pow_ui(xp, 2, KAPPA);
  mpz_fdiv_q(xp, xp, p);

  // generate u[i] in [0, 2^{κ+1}[
  for (int i = 0; i < THETA; i++) {
    mpz_init(u[i]);
    mpz_urandomm(u[i], state, modulus);
  }

  mpz_set_ui(sum, 0);
  for (int i = 0; i < THETA; i++)
    if (s[i]) mpz_add(sum, sum, u[i]);

  // Enforce: sum = xp mod 2^{κ+1}
  // => sum - u[k] + u'[k] = xp  => u'[k] = (xp - sum + u[k]) mod 2^{κ+1}
  for (int i = 0; i < THETA; i++) {
    if (s[i]) {
      // correction = (xp - sum + u[i]) % modulus
      mpz_sub(diff, xp, sum);
      mpz_add(diff, diff, u[i]);
      mpz_mod(diff, diff, modulus);
      mpz_set(u[i], diff);
      break;  // correct only one u[i]
    }
  }
  mpz_clears(xp, modulus, sum, diff, NULL);
}

void convert_u_to_y(mpf_t *y, const mpz_t *u) {
  mpz_t denom_z;
  mpf_t denom;
  mpz_init(denom_z);
  mpf_init2(denom, KAPPA + 64);

  mpz_ui_pow_ui(denom_z, 2, KAPPA);
  mpf_set_z(denom, denom_z);  // denom = 2^κ en float

  for (int i = 0; i < THETA; i++) {
    mpf_init2(y[i], KAPPA + 64);
    mpf_set_z(y[i], u[i]);
    mpf_div(y[i], y[i], denom);  // y[i] = u[i] / 2^κ
  }

  mpz_clear(denom_z);
  mpf_clear(denom);
}

void encrypt(mpz_t c, const mpz_t p, int m) {
  mpz_t q, r, tmp;
  mpz_inits(q, r, tmp, NULL);
  generate_q(q, p, GAMMA);
  generate_r(r, RHOP);
  mpz_mul(c, p, q);
  mpz_mul_ui(tmp, r, 2);
  mpz_add(c, c, tmp);
  if (m != 0) mpz_add_ui(c, c, 1);
  mpz_clears(q, r, tmp, NULL);
}

void encrypt_public(mpz_t c, const mpz_t *pk, const int m) {
  mpz_t r, sum, tmp;
  mpz_inits(r, sum, tmp, NULL);
  mpz_set_ui(sum, 0);

  // Random subset S incl {1, ..., TAU}
  for (int i = 1; i <= TAU; i++) {
    if (rand() % 2) {
      mpz_add(sum, sum, pk[i]);
    }
  }

  generate_r(r, RHOP);
  mpz_mul_ui(r, r, 2);
  mpz_mul_ui(sum, sum, 2);
  mpz_add(c, sum, r);
  if (m != 0) mpz_add_ui(c, c, 1);
  mpz_mod(c, c, pk[0]);

  mpz_clears(r, sum, tmp, NULL);
}

void decrypt(mpz_t result, const mpz_t c, const mpz_t p) {
  mpz_t mod, half_p;
  mpz_inits(mod, half_p, NULL);
  mpz_mod(mod, c, p);
  mpz_fdiv_q_ui(half_p, p, 2);
  if (mpz_cmp(mod, half_p) >= 0) mpz_sub(mod, mod, p);
  mpz_mod_ui(result, mod, 2);
  mpz_clears(mod, half_p, NULL);
}

void encrypt_fhe(mpz_t c_star, mpf_t *z, const mpz_t *pk, const mpf_t *y, int m,
                 int theta) {
  // Set sufficient precision for all operations
  const int total_precision = KAPPA + PREC_BITS + 64;
  mpf_set_default_prec(total_precision);

  // Initialize variables with proper precision
  mpf_t tmp, c_star_f, mod_f, fractional_part;
  mpf_inits(tmp, c_star_f, mod_f, fractional_part, NULL);

  // Encrypt the message m to get c_star
  encrypt_public(c_star, pk, m);

  for (int i = 0; i < theta; i++) {
    // Convert c_star to mpf_t
    mpf_set_z(c_star_f, c_star);

    // Multiply c_star with y[i]
    mpf_mul(tmp, c_star_f, y[i]);

    // Extract integer part
    mpz_t integer_part;
    mpz_init(integer_part);
    mpz_set_f(integer_part, tmp);  // Truncates toward zero

    // Get exact fractional part
    mpf_set_z(mod_f, integer_part);
    mpf_sub(fractional_part, tmp, mod_f);

    // Compute mod 2 of integer part
    int mod_result = mpz_mod_ui(integer_part, integer_part, 2);

    // Scale fractional part to PREC_BITS precision
    mpf_mul_2exp(fractional_part, fractional_part, PREC_BITS);
    mpz_t scaled_frac;
    mpz_init(scaled_frac);
    mpz_set_f(scaled_frac, fractional_part);

    // Combine results
    mpf_set_ui(z[i], mod_result);
    mpf_set_z(tmp, scaled_frac);
    mpf_div_2exp(tmp, tmp, PREC_BITS);
    mpf_add(z[i], z[i], tmp);

    mpz_clears(integer_part, scaled_frac, NULL);
  }
  mpf_clears(tmp, c_star_f, mod_f, fractional_part, NULL);
}

void decrypt_fhe(mpz_t result, const mpz_t c, const mpf_t *z, const int *sk) {
  mpf_t sum, round;
  mpf_init(sum);
  mpf_init_set_d(round, 0.5);
  mpf_t tmp;
  mpf_t s;
  for (int i = 0; i < THETA; i++) {
    mpf_init(tmp);
    mpf_init_set_ui(s, sk[i]);
    mpf_mul(tmp, z[i], s);
    mpf_add(sum, sum, tmp);
    mpf_clear(tmp);
  }
  mpz_t diff;
  mpz_init(diff);
  mpf_add(sum, sum, round);  // Arrondi : + 0.5 |> floor
  mpz_set_f(diff, sum);
  mpz_sub(diff, c, diff);       // diff = c - sum
  mpz_mod_ui(result, diff, 2);  // result = diff mod 2

  mpf_clear(sum);
  mpf_clear(round);
  mpz_clear(diff);
}

void export_secret_key_json(const mpz_t p, const int *s, const char *filename) {
  struct json_object *root = json_object_new_object();
  struct json_object *arr_s = json_object_new_array();

  for (int i = 0; i < THETA; i++) {
    json_object_array_add(arr_s, json_object_new_int(s[i]));
  }

  json_object_object_add(root, "p",
                         json_object_new_string(mpz_get_str(NULL, 10, p)));
  json_object_object_add(root, "s", arr_s);

  FILE *f = fopen(filename, "w");
  if (f) {
    fprintf(f, "%s\n",
            json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
    fclose(f);
    printf("Clé privée exportée dans %s\n", filename);
  } else {
    perror("fopen");
  }

  json_object_put(root);
}

void export_public_key_json(mpz_t *pk, int pk_len, mpf_t *y,
                            const char *filename) {
  struct json_object *root = json_object_new_object();
  struct json_object *arr_pk = json_object_new_array();
  struct json_object *arr_y = json_object_new_array();
  char buffer[4096];

  for (int i = 0; i < pk_len; i++) {
    gmp_sprintf(buffer, "%Zd", pk[i]);
    json_object_array_add(arr_pk, json_object_new_string(buffer));
  }

  for (int i = 0; i < THETA; i++) {
    gmp_sprintf(buffer, "%.128Ff", y[i]);
    json_object_array_add(arr_y, json_object_new_string(buffer));
  }

  json_object_object_add(root, "pk_star", arr_pk);
  json_object_object_add(root, "y", arr_y);

  FILE *f = fopen(filename, "w");
  if (f) {
    fprintf(f, "%s\n",
            json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
    fclose(f);
    printf("Clé publique exportée dans %s\n", filename);
  } else {
    perror("fopen");
  }

  json_object_put(root);
}

// Exports 20 ciphertexts for testing
void export_ciphertexts_json(mpz_t *pk, const char *filename) {
  struct json_object *root = json_object_new_object();
  struct json_object *arr_c = json_object_new_array();
  struct json_object *arr_m = json_object_new_array();

  mpz_t c;
  mpz_init(c);

  srand(time(NULL));

  for (int i = 0; i < 20; i++) {
    int m = rand() % 2;
    encrypt_public(c, pk, m);
    json_object_array_add(arr_c,
                          json_object_new_string(mpz_get_str(NULL, 10, c)));

    json_object_array_add(arr_m, json_object_new_int(m));
  }
  mpz_clear(c);

  json_object_object_add(root, "ciphertexts", arr_c);
  json_object_object_add(root, "messages", arr_m);

  FILE *f = fopen(filename, "w");
  if (f) {
    fprintf(f, "%s\n",
            json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
    fclose(f);
    printf("Chiffrés exportés dans %s\n", filename);
  } else {
    perror("fopen");
  }

  json_object_put(root);
}

void print_sep() {
  printf("--------------------------------------------------\n");
}

void print_title(const char *title) {
  int len = strlen(title);
  int sep_len = 50;
  int s = (sep_len - len) / 2;
  int padding = s > 0 ? s : 0;
  char space = ' ';
  print_sep();
  for (int i = 0; i < padding; i++) printf("%c", space);
  printf("%s", title);
  for (int i = 0; i < sep_len - len - padding; i++) printf("%c", space);
  printf("\n");
  print_sep();
}

int main(int argc, char *argv[]) {
  init_rand();

  // Tests
  if (argc == 1) {
    printf("Syntaxe : %s tests | key | export_test | encrypt | decrypt \n",
           argv[0]);
    return 1;
  }

  if (strncmp(argv[1], "test", 4) == 0) {
    print_title("Tests");
    mpz_t prime, encr, decr;
    mpz_t *pk = malloc((TAU + 1) * sizeof(mpz_t));
    mpz_inits(prime, pk, encr, decr, NULL);
    generate_prime(prime);
    generate_public_key(pk, prime);
    int s[THETA];
    generate_bootstrap_secret_vector(s);
    mpz_t u[THETA];
    for (int i = 0; i < THETA; i++) {
      mpz_init(u[i]);
    }
    generate_u_hints(u, prime, s);
    mpf_t y[THETA];
    convert_u_to_y(y, u);

    for (int i = 0; i < 30; i++) {
      int m = rand() % 2;
      mpz_set_ui(encr, 0);
      mpz_set_ui(decr, 0);
      encrypt(encr, prime, m);
      decrypt(decr, encr, prime);
      gmp_printf("S%i -> %Zd ", m, decr);

      mpz_set_ui(encr, 0);
      mpz_set_ui(decr, 0);
      encrypt_public(encr, pk, m);
      decrypt(decr, encr, prime);
      gmp_printf(" %Zd <- %iP \n", decr, m);
    }

    // Test addition
    mpz_t c1, c2, res;
    mpz_inits(c1, c2, res, NULL);
    mpz_set_ui(c1, 0);
    mpz_set_ui(c2, 0);
    mpz_set_ui(res, 0);
    mpz_set_ui(decr, 0);
    encrypt(c1, prime, 1);
    encrypt(c2, prime, 1);
    mpz_mul(res, c1, c2);
    decrypt(decr, res, prime);
    gmp_printf("Déchiffrement du produit : %Zd\n", decr);

    mpz_t c3, c4, d3, d4, res2;
    mpz_inits(c3, c4, d3, d4, res2, NULL);
    mpz_set_ui(c3, 0);
    mpz_set_ui(c4, 0);
    mpz_set_ui(d3, 0);
    mpz_set_ui(d4, 0);
    mpz_set_ui(res2, 0);
    encrypt_public(c3, pk, 0);
    encrypt_public(c4, pk, 1);

    // Test FHE
    mpf_t z[THETA];
    for (int i = 0; i < THETA; i++) {
      mpf_init2(z[i], KAPPA + 64);
    }
    encrypt_fhe(c3, z, pk, y, 0, THETA);
    decrypt_fhe(res, c3, z, s);
    gmp_printf("Déchiffrement FHE : %Zd\n", res);
    encrypt_fhe(c4, z, pk, y, 1, THETA);
    decrypt_fhe(res2, c4, z, s);
    gmp_printf("Déchiffrement FHE : %Zd\n", res2);

    mpz_clears(prime, encr, decr, NULL);
  }

  else if (strncmp(argv[1], "key", 3) == 0) {
    print_title("Génération de la clé");
    // Génération de la clé
    mpz_t clef;
    mpz_init(clef);
    generate_prime(clef);
    gmp_printf("Clé générée : %Zd\n", clef);
    mpz_t *pk = malloc((TAU + 1) * sizeof(mpz_t));
    for (int i = 0; i <= TAU; i++) mpz_init(pk[i]);
    generate_public_key(pk, clef);
    int s[THETA];
    generate_bootstrap_secret_vector(s);
    mpz_t u[THETA];
    for (int i = 0; i < THETA; i++) mpz_init(u[i]);
    generate_u_hints(u, clef, s);
    mpf_t y[THETA];
    convert_u_to_y(y, u);
    export_secret_key_json(clef, s, "sk.json");
    export_public_key_json(pk, TAU + 1, y, "pk.json");
  }

  else if (strcmp(argv[1], "export_test") == 0) {
    print_title("Exportation de la clé");
    // Exporter la clé
    mpz_t clef;
    mpz_init(clef);
    generate_prime(clef);
    gmp_printf("Clé générée : %Zd\n", clef);
    mpz_t *pk = malloc((TAU + 1) * sizeof(mpz_t));
    for (int i = 0; i <= TAU; i++) mpz_init(pk[i]);
    generate_public_key(pk, clef);
    int s[THETA];
    generate_bootstrap_secret_vector(s);
    mpz_t u[THETA];
    for (int i = 0; i < THETA; i++) mpz_init(u[i]);
    generate_u_hints(u, clef, s);
    mpf_t y[THETA];
    convert_u_to_y(y, u);
    export_secret_key_json(clef, s, "sk.json");
    export_public_key_json(pk, TAU + 1, y, "pk.json");
    export_ciphertexts_json(pk, "ciphertexts.json");
  }

  else if (strncmp(argv[1], "encrypt", 8) == 0) {
    if (argc != 4) {
      printf("Syntaxe : %s -e <bit> <clé>\n", argv[0]);
      return 1;
    }
    if ((strncmp(argv[2], "0", 1) == 1) && (strncmp(argv[2], "1", 1) == 1)) {
      printf("Syntaxe : %s -e <bit> <clé>\n", argv[0]);
      return 1;
    }
    int m = (strncmp(argv[2], "0", 1) == 0) ? 0 : 1;
    char *clef_str = argv[3];
    mpz_t clef, res;
    mpz_inits(clef, res, NULL);
    mpz_set_str(clef, clef_str, 10);
    encrypt(res, clef, m);
    gmp_printf("%Zd\n", res);
    mpz_clears(clef, res, NULL);
  }

  else if (strncmp(argv[1], "decrypt", 8) == 0) {
    if (argc != 4) {
      printf("Syntaxe : %s -d <chiffré> <clé>\n", argv[0]);
      return 1;
    }
    char *ch_str = argv[2];
    char *clef_str = argv[3];
    mpz_t c, clef, res;
    mpz_inits(c, clef, res, NULL);
    mpz_set_str(c, ch_str, 10);
    mpz_set_str(clef, clef_str, 10);
    decrypt(res, c, clef);
    gmp_printf("%Zd\n", res);
    mpz_clears(clef, res, NULL);
  }

  else {
    printf("Syntaxe : %s test | key | encrypt | decrypt \n", argv[0]);
    return 1;
  }

  gmp_randclear(state);
  return 0;
}