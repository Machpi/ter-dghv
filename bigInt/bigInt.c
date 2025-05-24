#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <gmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef enum { false, true } bool;

typedef struct {
  uint32_t *digits;
  int size;
  int capacity;
} BigInt;

BigInt *bigint_init(uint32_t value) {
  BigInt *num = malloc(sizeof(BigInt));
  num->capacity = 1;
  num->size = 1;
  num->digits = malloc(sizeof(uint32_t) * num->capacity);
  num->digits[0] = value;
  return num;
}

BigInt *bigint_init_size(size_t size) {
  BigInt *bi = malloc(sizeof(BigInt));
  bi->capacity = size;
  bi->size = 1;
  bi->digits = calloc(size, sizeof(uint32_t));
  return bi;
}

BigInt *bigint_copy(const BigInt *num) {
  BigInt *copy = malloc(sizeof(BigInt));
  copy->size = num->size;
  copy->capacity = num->capacity;
  copy->digits = malloc(sizeof(uint32_t) * copy->capacity);
  memcpy(copy->digits, num->digits, sizeof(uint32_t) * copy->size);
  return copy;
}

void bigint_free(BigInt *num) {
  free(num->digits);
  free(num);
}

void bigint_free_debug(BigInt *num, const char *nom) {
  printf("Libération de %s\n", nom);
  free(num->digits);
  free(num);
}

void bigint_resize(BigInt *num, int new_capacity) {
  num->digits = realloc(num->digits, sizeof(uint32_t) * new_capacity);
  if (!num->digits) {
    perror("Échec de l'allocation mémoire");
    exit(EXIT_FAILURE);
  }
  num->capacity = new_capacity;
}

void bigint_print(const BigInt *num) {
  printf("0x");
  for (int i = num->size - 1; i >= 0; i--) {
    printf("%08X", num->digits[i]);
  }
  printf("\n");
}

BigInt *bigint_from_hex(const char *hex_str) {
  int len = strlen(hex_str);
  BigInt *num = malloc(sizeof(BigInt));
  num->size = 0;
  num->capacity = (len + 7) / 8;
  num->digits = calloc(num->capacity, sizeof(uint32_t));

  int char_index = len - 1;
  uint32_t current = 0;
  int shift = 0;

  while (char_index >= 0) {
    char hex_digit = hex_str[char_index--];
    if (!isxdigit(hex_digit)) continue;

    uint8_t value =
        (hex_digit >= '0' && hex_digit <= '9')   ? (hex_digit - '0')
        : (hex_digit >= 'A' && hex_digit <= 'F') ? (hex_digit - 'A' + 10)
        : (hex_digit >= 'a' && hex_digit <= 'f') ? (hex_digit - 'a' + 10)
                                                 : 0;

    current |= (value << shift);
    shift += 4;

    if (shift == 32) {
      num->digits[num->size++] = current;
      current = 0;
      shift = 0;
    }
  }

  if (shift > 0) num->digits[num->size++] = current;

  return num;
}

BigInt *bigint_read_from_file(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("Erreur d'ouverture du fichier");
    return NULL;
  }

  int len;
  if (fscanf(file, "%d\n", &len) != 1 || len <= 0) {
    perror("Erreur de lecture de la taille");
    fclose(file);
    return NULL;
  }

  char *hex_str = malloc(len + 1);
  if (!fgets(hex_str, len + 1, file)) {
    perror("Erreur de lecture du contenu hexadécimal");
    free(hex_str);
    fclose(file);
    return NULL;
  }

  return bigint_from_hex(hex_str);
  free(hex_str);
  fclose(file);
}

// Remove leading zeros
void bigint_trim(BigInt *a) {
  if (!a || a->size == 0) return;
  while (a->size > 1 && a->digits[a->size - 1] == 0) {
    a->size--;
  }
}

BigInt *bigint_add(const BigInt *a, const BigInt *b) {
  int maxSize = (a->size > b->size) ? a->size : b->size;
  BigInt *result = malloc(sizeof(BigInt));
  result->capacity = maxSize + 1;
  result->digits = calloc(result->capacity, sizeof(uint32_t));
  result->size = 0;
  uint64_t carry = 0;
  for (int i = 0; i < maxSize; i++) {
    uint64_t tmp = carry;
    if (i < a->size) {
      tmp += a->digits[i];
    }
    if (i < b->size) {
      tmp += b->digits[i];
    }
    result->digits[result->size++] = (uint32_t)(tmp & 0xFFFFFFFF);
    carry = tmp >> 32;
  }
  if (carry) {
    result->digits[result->size++] = (uint32_t)carry;
  }
  if (result->size > result->capacity) {
    result->capacity = result->size;
    result->digits =
        realloc(result->digits, result->capacity * sizeof(uint32_t));
  }
  while (result->size > 1 && result->digits[result->size - 1] == 0) {
    result->size--;
  }
  return result;
}

// Returns a subarray of BigInt starting at index s with length l
BigInt *bigint_subarray(const BigInt *a, int s, int l) {
  if (l == 0) {
    return bigint_init(0);
  }

  int start = s >= 0 ? s : 0;
  int length = (start + l > a->size) ? a->size - start : l;

  BigInt *sub = malloc(sizeof(BigInt));
  sub->size = length;
  sub->capacity = length;
  sub->digits = calloc(sub->capacity, sizeof(uint32_t));

  for (int i = 0; i < length; i++) {
    sub->digits[i] = a->digits[start + i];
  }

  return sub;
}

// Returns a - b assuming a >= b
BigInt *bigint_sub(const BigInt *a, const BigInt *b) {
  BigInt *result = malloc(sizeof(BigInt));
  result->capacity = a->capacity;
  result->digits = calloc(result->capacity, sizeof(uint32_t));

  uint64_t borrow = 0;
  int i = 0;
  while (i < a->size) {
    uint64_t ai = a->digits[i];
    uint64_t bi = (i < b->size) ? b->digits[i] : 0;
    uint64_t temp = ai - bi - borrow;

    if (ai < bi + borrow) {
      borrow = 1;
      temp += ((uint64_t)1 << 32);  // add 2^32 if underflow
    } else {
      borrow = 0;
    }

    result->digits[i] = (uint32_t)(temp & 0xFFFFFFFF);
    i++;
  }

  result->size = i;
  bigint_trim(result);

  if (result->size < result->capacity) {
    bigint_resize(result, result->size);
  }

  return result;
}

BigInt *bigint_shift_left(const BigInt *a, int shift) {
  if (shift <= 0) return bigint_subarray(a, 0, a->size);

  int shift_words = shift / 32;
  int shift_bits = shift % 32;

  int new_size = a->size + shift_words + (shift_bits ? 1 : 0);
  BigInt *result = malloc(sizeof(BigInt));
  result->size = new_size;
  result->capacity = new_size;
  result->digits = calloc(result->capacity, sizeof(uint32_t));

  for (int i = 0; i < a->size; i++) {
    int new_pos = i + shift_words;
    result->digits[new_pos] = a->digits[i] << shift_bits;

    if (shift_bits && new_pos + 1 < new_size) {
      result->digits[new_pos + 1] |= a->digits[i] >> (32 - shift_bits);
    }
  }

  bigint_trim(result);
  return result;
}

BigInt *bigint_small_mul(uint32_t a, uint32_t b) {
  uint64_t result = (uint64_t)a * (uint64_t)b;
  uint32_t r0 = (uint32_t)(result & 0xFFFFFFFF);
  uint32_t r1 = (uint32_t)((result >> 32) & 0xFFFFFFFF);
  BigInt *res = bigint_init(r0);
  if (r1 != 0) {
    bigint_resize(res, 2);
    res->size = 2;
    res->digits[1] = r1;
  }
  return res;
}

int bigint_cmp(const BigInt *a, const BigInt *b) {
  if (a->size > b->size) return 1;
  if (a->size < b->size) return -1;

  for (int i = a->size - 1; i >= 0; i--) {
    if (a->digits[i] > b->digits[i]) return 1;
    if (a->digits[i] < b->digits[i]) return -1;
  }
  return 0;
}

int bigint_cmp_small(const BigInt *a, uint32_t b) {
  if (a->size > 1) return 1;
  if (a->size < 1) return -1;

  if (a->digits[0] > b) return 1;
  if (a->digits[0] < b) return -1;

  return 0;
}

// Karatsuba multiplication
BigInt *bigint_mul(const BigInt *a, const BigInt *b) {
  // Zero if either is null
  if ((a->size == 1 && a->digits[0] == 0) ||
      (b->size == 1 && b->digits[0] == 0)) {
    return bigint_init(0);
  }

  // Base case
  if (a->size == 1 && b->size == 1) {
    return bigint_small_mul(a->digits[0], b->digits[0]);
  }

  // Divide both numbers into two halves
  int m = (a->size > b->size ? a->size : b->size) / 2;  // Half size

  // High and low parts
  int a0_len = (m < a->size) ? m : a->size;
  int a1_len = a->size - a0_len;
  BigInt *a0 = bigint_subarray(a, 0, a0_len);
  BigInt *a1 = bigint_subarray(a, a0_len, a1_len);
  int b0_len = (m < b->size) ? m : b->size;
  int b1_len = b->size - b0_len;
  BigInt *b0 = bigint_subarray(b, 0, b0_len);
  BigInt *b1 = bigint_subarray(b, b0_len, b1_len);

  // Three products
  BigInt *z0 = bigint_mul(a0, b0);
  BigInt *aa = bigint_add(a1, a0);
  bigint_free(a0);
  BigInt *bb = bigint_add(b1, b0);
  bigint_free(b0);
  BigInt *z1 = bigint_mul(aa, bb);
  bigint_free(aa);
  bigint_free(bb);
  BigInt *z2 = bigint_mul(a1, b1);
  bigint_free(a1);
  bigint_free(b1);
  BigInt *temp1 = bigint_add(z2, z0);
  BigInt *temp2 = bigint_sub(z1, temp1);
  bigint_free(temp1);
  bigint_free(z1);
  BigInt *temp3 = bigint_shift_left(temp2, m * 32);
  bigint_free(temp2);
  BigInt *temp4 = bigint_shift_left(z2, 2 * m * 32);
  bigint_free(z2);
  BigInt *temp5 = bigint_add(temp3, temp4);
  bigint_free(temp3);
  bigint_free(temp4);
  BigInt *result = bigint_add(temp5, z0);
  bigint_free(z0);
  bigint_free(temp5);

  return result;
}

// Subtracts b from a in place (assumes a >= b)
void bigint_sub_inplace(BigInt *a, const BigInt *b) {
  uint64_t borrow = 0;
  for (int i = 0; i < a->size; i++) {
    uint64_t sub =
        (uint64_t)a->digits[i] - (i < b->size ? b->digits[i] : 0) - borrow;
    if ((int64_t)sub < 0) {
      borrow = 1;
      sub += ((uint64_t)1 << 32);
    } else
      borrow = 0;
    a->digits[i] = (uint32_t)(sub & 0xFFFFFFFF);
  }
  while (a->size > 1 && a->digits[a->size - 1] == 0) a->size--;
}

// Adds a small value to a BigInt in place
void bigint_add_small(BigInt *a, uint32_t value) {
  uint64_t carry = value;
  int i = 0;
  while (carry && i < a->size) {
    uint64_t sum = (uint64_t)a->digits[i] + carry;
    a->digits[i] = (uint32_t)(sum & 0xFFFFFFFF);
    carry = sum >> 32;
    i++;
  }
  if (carry) {
    if (a->size == a->capacity) bigint_resize(a, a->capacity + 1);
    a->digits[a->size++] = (uint32_t)carry;
  }
}

// Returns the number of bits needed to represent the BigInt
int bigint_bit_length(const BigInt *a) {
  if (a->size == 0) return 0;
  uint32_t highest = a->digits[a->size - 1];
  int bits = (a->size - 1) * 32;
  while (highest) {
    bits++;
    highest >>= 1;
  }
  return bits;
}

// Sub function used in mod
void bigint_shift_left_one_bit_and_add(BigInt *r, int bit) {
  uint32_t carry = bit;
  for (size_t i = 0; i < r->size; ++i) {
    uint64_t val = ((uint64_t)r->digits[i] << 1) | carry;
    carry = (val >> 32) & 1;
    r->digits[i] = (uint32_t)val;
  }
  if (carry) {
    r->digits = realloc(r->digits, sizeof(uint32_t) * (r->size + 1));
    r->digits[r->size] = 1;
    r->size += 1;
  }
}

// Calculates a mod m using binary long division algorithm
BigInt *bigint_mod(const BigInt *a, const BigInt *m) {
  BigInt *r = bigint_init_size(m->size + 1);
  int n = bigint_bit_length(a);
  for (int i = n - 1; i >= 0; i--) {
    int bit = (a->digits[i / 32] >> (i % 32)) & 1;
    bigint_shift_left_one_bit_and_add(r, bit);

    if (r->size >= m->size) {
      if (bigint_cmp(r, m) >= 0) bigint_sub_inplace(r, m);
    }
  }

  bigint_trim(r);
  return r;
}

BigInt *bigint_random(int num_blocks) {
  BigInt *num = malloc(sizeof(BigInt));
  num->capacity = num_blocks;
  num->size = num_blocks;
  num->digits = malloc(sizeof(uint32_t) * num_blocks);

  int fd = open("/dev/urandom", O_RDONLY);
  if (fd < 0) {
    perror("open /dev/urandom");
    exit(EXIT_FAILURE);
  }

  ssize_t expected_bytes = sizeof(uint32_t) * num_blocks;
  ssize_t read_bytes = read(fd, num->digits, expected_bytes);
  if (read_bytes != expected_bytes) {
    perror("read /dev/urandom");
    close(fd);
    bigint_free(num);
    exit(EXIT_FAILURE);
  }

  close(fd);
  while (num->size > 1 && num->digits[num->size - 1] == 0) num->size--;
  return num;
}

// Generates a random BigInt between min and max (exclusive)
BigInt *bigint_random_range(const BigInt *min, const BigInt *max) {
  assert(bigint_cmp(min, max) < 0);
  int size = max->size;
  int bits = bigint_bit_length(max);
  BigInt *num = bigint_random(size);
  num->digits[size - 1] &=
      (1u << (bits % 32)) - 1;  // Set the last bits to zero

  while (bigint_cmp(num, min) <= 0 || bigint_cmp(num, max) >= 0) {
    bigint_free(num);
    num = bigint_random(size);
    num->digits[size - 1] &=
        (1u << (bits % 32)) - 1;  // Set the last bits to zero
  }
  return num;
}

// Multiplies a * b and keeps only the k_bits least significant bits
BigInt *bigint_mul_low(const BigInt *a, const BigInt *b, int k_bits) {
  int k_words = (k_bits + 31) / 32;
  BigInt *result = bigint_init_size(k_words);
  uint64_t carry = 0;

  for (int i = 0; i < a->size; i++) {
    carry = 0;
    for (int j = 0; j < b->size && (i + j) < k_words; j++) {
      uint64_t mul = (uint64_t)a->digits[i] * b->digits[j];
      uint64_t sum = (uint64_t)result->digits[i + j] + mul + carry;

      // Low part to result
      result->digits[i + j] = (uint32_t)sum;
      // High part to carry
      carry = sum >> 32;
    }

    // Add carry to next word
    if ((i + b->size) < k_words) result->digits[i + b->size] += (uint32_t)carry;
  }

  bigint_resize(result, k_words);
  result->size = k_words;
  bigint_trim(result);
  return result;
}

// Calculates the modular inverse of m modulo 2^k
BigInt *bigint_modinv_pow2(const BigInt *a, int k) {
  assert(a->digits[0] & 1);
  int words = (k + 31) / 32;

  BigInt *x = bigint_init_size(words);
  x->digits[0] = 1;
  x->size = 1;

  for (int i = 1; i < k; i *= 2) {
    BigInt *t = bigint_mul_low(a, x, 2 * i);

    BigInt *two = bigint_init_size(words);
    two->digits[0] = 2;
    two->size = 1;

    uint64_t borrow = 0;
    for (int j = 0; j < words; j++) {
      uint64_t two_val = (j < two->size) ? two->digits[j] : 0;
      uint64_t t_val = (j < t->size) ? t->digits[j] : 0;
      uint64_t diff = two_val - t_val - borrow;

      if (j < two->size) {
        two->digits[j] = (uint32_t)diff;
      } else {
        if (diff != 0) {
          bigint_resize(two, j + 1);
          two->digits[j] = (uint32_t)diff;
          two->size = j + 1;
        }
      }
      borrow = (diff >> 32) ? 1 : 0;
    }
    bigint_free(t);

    BigInt *new_x = bigint_mul_low(x, two, 2 * i);
    bigint_free(two);
    bigint_free(x);
    x = new_x;
  }

  BigInt *one = bigint_init(1);
  BigInt *result = bigint_mul_low(x, one, k);
  bigint_free(one);
  bigint_free(x);
  return result;
}

// Right shift
BigInt *bigint_div_pow2(const BigInt *n, int shift) {
  if (shift <= 0) return bigint_copy(n);

  int shift_words = shift / 32;
  int shift_bits = shift % 32;
  int new_size = n->size - shift_words;

  BigInt *result = malloc(sizeof(BigInt));
  result->size = new_size;
  result->capacity = new_size;
  result->digits = calloc(result->capacity, sizeof(uint32_t));

  for (int i = 0; i < new_size; i++) {
    if (shift_bits == 0)
      result->digits[i] = n->digits[i + shift_words] >> shift_bits;
    else {
      uint32_t low = n->digits[i + shift_words] >> shift_bits;
      uint32_t high = (i + shift_words + 1 < n->size)
                          ? n->digits[i + shift_words + 1] << (32 - shift_bits)
                          : 0;
      result->digits[i] = low | high;
    }
  }

  bigint_trim(result);
  return result;
}

void bigint_shift_left_inplace(BigInt *a, int shift) {
  if (shift <= 0 || a->size == 0) return;

  int shift_words = shift / 32;
  int shift_bits = shift % 32;
  int extra_word = (shift_bits != 0) ? 1 : 0;
  int new_size = a->size + shift_words + extra_word;

  // Realloc if necessary
  if (a->capacity < new_size) {
    a->digits = realloc(a->digits, new_size * sizeof(uint32_t));
    memset(a->digits + a->capacity, 0,
           (new_size - a->capacity) * sizeof(uint32_t));
    a->capacity = new_size;
  }

  for (int i = a->size - 1; i >= 0; i--) a->digits[i + shift_words] = a->digits[i];
  for (int i = 0; i < shift_words; i++) a->digits[i] = 0;
  a->size += shift_words;
  if (shift_bits == 0) return;

  // Bit by bit
  uint32_t carry = 0;
  for (int i = shift_words; i < a->size; i++) {
    uint32_t new_carry = a->digits[i] >> (32 - shift_bits);
    a->digits[i] = (a->digits[i] << shift_bits) | carry;
    carry = new_carry;
  }

  if (carry != 0) {
    if (a->size == a->capacity) {
      a->digits = realloc(a->digits, (a->capacity + 1) * sizeof(uint32_t));
      a->capacity += 1;
    }
    a->digits[a->size++] = carry;
  }
}

BigInt *montgomery_reduce(const BigInt *T, const BigInt *m, const BigInt *m_inv, int k_bits) {
  BigInt *m_low = bigint_mul_low(T, m_inv, k_bits);
  BigInt *mn = bigint_mul(m_low, m);
  BigInt *sum = bigint_add(T, mn);
  BigInt *result = bigint_div_pow2(sum, k_bits);

  if (bigint_cmp(result, m) >= 0) {
    BigInt *tmp = bigint_sub(result, m);
    bigint_free(result);
    result = tmp;
  }

  bigint_free(m_low);
  bigint_free(mn);
  bigint_free(sum);
  return result;
}

// Montgomery multiplication
BigInt *montgomery_mul(const BigInt *a, const BigInt *b, const BigInt *m, const BigInt *m_inv, int k_bits) {
  BigInt *T = bigint_mul(a, b);
  BigInt *res = montgomery_reduce(T, m, m_inv, k_bits);
  bigint_free(T);
  return res;
}

int bigint_test_bit(const BigInt *a, int bit_index) {
  assert(bit_index >= 0);
  int word = bit_index / 32;
  int pos = bit_index % 32;
  if (word >= a->size) return 0;
  return ((a->digits[word] >> pos) & 1u) ? 1 : 0;
}

void bigint_negate(BigInt *a) {
  for (int i = 0; i < a->size; i++) a->digits[i] = ~a->digits[i];

  uint64_t carry = 1;
  for (int i = 0; i < a->size && carry; i++) {
    uint64_t sum = (uint64_t)a->digits[i] + carry;
    a->digits[i] = (uint32_t)sum;
    carry = sum >> 32;
  }

  if (carry) {
    if (a->size < a->capacity) {
      a->digits[a->size++] = 1;
    } else {
      bigint_resize(a, a->capacity + 1);
      a->digits[a->size++] = 1;
    }
  }

  bigint_trim(a);
}

BigInt *create_R(int k_bits) {
  int words_needed = (k_bits + 31) / 32 + 1;
  BigInt *R = bigint_init_size(words_needed);
  R->digits[words_needed - 1] = 1;
  R->size = words_needed;
  return R;
}

BigInt *montgomery_powm(const BigInt *base, const BigInt *exp, const BigInt *modulus) {
  int k_bits = 32 * (modulus->size);
  assert(modulus->digits[0] & 1);

  BigInt *R = create_R(k_bits);
  BigInt *R2 = bigint_mul(R, R);
  BigInt *R2_mod_n = bigint_mod(R2, modulus);

  BigInt *n_prime = bigint_modinv_pow2(modulus, k_bits);
  BigInt *tmp = bigint_sub(R, n_prime);
  bigint_free(n_prime);
  n_prime = tmp;

  BigInt *baseM = montgomery_mul(base, R2_mod_n, modulus, n_prime, k_bits);

  BigInt *one = bigint_init(1);
  BigInt *x = montgomery_mul(one, R2_mod_n, modulus, n_prime, k_bits);

  int nbits = bigint_bit_length(exp);
  for (int i = nbits - 1; i >= 0; i--) {
    BigInt *tmp = montgomery_mul(x, x, modulus, n_prime, k_bits);
    bigint_free(x);
    x = tmp;

    if (bigint_test_bit(exp, i)) {
      tmp = montgomery_mul(x, baseM, modulus, n_prime, k_bits);
      bigint_free(x);
      x = tmp;
    }
  }

  BigInt *result = montgomery_mul(x, one, modulus, n_prime, k_bits);

  bigint_free(R);
  bigint_free(R2);
  bigint_free(R2_mod_n);
  bigint_free(n_prime);
  bigint_free(baseM);
  bigint_free(x);
  bigint_free(one);
  bigint_trim(result);
  return result;
}

// Montgomery powm with precomputed R, R2_mod_n, and n_prime
BigInt *montgomery_powm_precalc(const BigInt *base, const BigInt *exp,
                                const BigInt *modulus, BigInt *R,
                                BigInt *R2_mod_n, BigInt *n_prime) {
  int k_bits = 32 * (modulus->size);
  assert(modulus->digits[0] & 1);

  BigInt *R_mod_n = bigint_mod(R, modulus);
  BigInt *baseM = montgomery_mul(base, R2_mod_n, modulus, n_prime, k_bits);

  BigInt *one = bigint_init(1);
  BigInt *x = montgomery_mul(one, R2_mod_n, modulus, n_prime, k_bits);

  int nbits = bigint_bit_length(exp);
  for (int i = nbits - 1; i >= 0; i--) {
    BigInt *tmp = montgomery_mul(x, x, modulus, n_prime, k_bits);
    bigint_free(x);
    x = tmp;

    if (bigint_test_bit(exp, i)) {
      tmp = montgomery_mul(x, baseM, modulus, n_prime, k_bits);
      bigint_free(x);
      x = tmp;
    }
  }

  BigInt *result = montgomery_mul(x, one, modulus, n_prime, k_bits);

  bigint_free(R_mod_n);
  bigint_free(baseM);
  bigint_free(x);
  bigint_free(one);
  bigint_trim(result);
  return result;
}

// Right shift (division par 2)
void bigint_shift_right_inplace(BigInt *n) {
  uint32_t carry = 0;
  for (int i = n->size - 1; i >= 0; i--) {
    uint32_t new_carry = n->digits[i] & 1;
    n->digits[i] = (n->digits[i] >> 1) | (carry << 31);
    carry = new_carry;
  }
  bigint_trim(n);
}

// Miller-Rabin primality test
bool is_probable_prime(BigInt *n, int iterations) {
  int k_bits = 32 * (n->size);
  assert(n->digits[0] & 1);

  BigInt *R = create_R(k_bits);
  BigInt *R2 = bigint_mul(R, R);
  BigInt *R2_mod_n = bigint_mod(R2, n);

  BigInt *n_prime = bigint_modinv_pow2(n, k_bits);
  BigInt *tmp = bigint_sub(R, n_prime);
  bigint_free(n_prime);
  n_prime = tmp;

  BigInt *one = bigint_init(1);

  BigInt *n_minus_1 = bigint_sub(n, one);
  BigInt *d = bigint_copy(n_minus_1);
  int s = 0;
  while ((d->digits[0] & 1) == 0) {
    bigint_shift_right_inplace(d);
    s++;
  }

  for (int i = 0; i < iterations; i++) {
    BigInt *a = bigint_random_range(one, n_minus_1);

    BigInt *x = montgomery_powm_precalc(a, d, n, R, R2_mod_n, n_prime);

    if (bigint_cmp(x, one) == 0 || bigint_cmp(x, n_minus_1) == 0) {
      bigint_free(x);
      continue;
    }

    bool witness = true;
    for (int r = 1; r < s; r++) {
      BigInt *x_squared = bigint_mul(x, x);
      bigint_free(x);
      x = bigint_mod(x_squared, n);
      bigint_free(x_squared);

      if (bigint_cmp(x, n_minus_1) == 0) {
        witness = false;
        break;
      }
    }

    if (witness) {
      // Not prime
      bigint_free(n_minus_1);
      bigint_free(x);
      bigint_free(R);
      bigint_free(R2_mod_n);
      bigint_free(n_prime);
      bigint_free(one);
      bigint_free(d);
      return false;
    }
  }

  // Probably prime
  bigint_free(n_minus_1);
  bigint_free(R);
  bigint_free(R2_mod_n);
  bigint_free(n_prime);
  bigint_free(one);
  bigint_free(d);
  return true;
}

bool is_trivial_composite(BigInt *n) {
  int size = 5;
  char **hexes = malloc(size * sizeof(char *));
  hexes[0] = "0x3";  // 3
  hexes[1] = "0x5";  // 5
  hexes[2] = "0x7";  // 7
  hexes[3] = "0xB";  // 11
  hexes[4] = "0xD";  // 13

  for (int i = 0; i < size; i++) {
    BigInt *prime = bigint_from_hex(hexes[i]);
    if (bigint_mod(n, prime) == 0) {
      bigint_free(prime);
      free(hexes);
      return true;
    }
    bigint_free(prime);
  }
  free(hexes);
  return false;
}

// Generate a prime
BigInt *bigint_generate_prime(int bits, int iterations) {
  while (true) {
    BigInt *num = bigint_random(bits / 32 + 1);
    // Ensure it's odd
    num->digits[0] |= 1;
    // Ensure it's bits bits long
    num->digits[bits / 32] |= 1 << (bits % 32);

    // printf("Testing primality of: ");
    // bigint_print(num);
    if (!is_trivial_composite(num)) {
      if (is_probable_prime(num, iterations)) return num;
    }
    bigint_free(num);
  }
}

void test_modinv_with_gmp(const char *m_hex, int k) {
  mpz_t gmp_m, gmp_result;
  mpz_init(gmp_m);
  mpz_init(gmp_result);

  BigInt *m = bigint_from_hex(m_hex);
  mpz_set_str(gmp_m, m_hex, 16);

  mpz_t gmp_mod;
  mpz_init(gmp_mod);
  mpz_ui_pow_ui(gmp_mod, 2, k);  // 2^k

  if (mpz_invert(gmp_result, gmp_m, gmp_mod) == 0) {
    printf("Aucun inverse modulaire n'existe.\n");
    return;
  }

  BigInt *result = bigint_modinv_pow2(m, k);

  printf("Résultat GMP     : 0x");
  gmp_printf("%Zx\n", gmp_result);

  printf("Résultat BigInt  : 0x");
  for (int i = result->size - 1; i >= 0; i--) {
    printf("%08X", result->digits[i]);
  }
  printf("\n");

  bigint_free(m);
  bigint_free(result);
  mpz_clear(gmp_mod);
  mpz_clear(gmp_m);
  mpz_clear(gmp_result);
}

void test_montgomery_reduce_once(const char *hex_T, const char *hex_m,
                                 const char *hex_m_inv, int k_bits) {
  BigInt *T = bigint_from_hex(hex_T);
  BigInt *m = bigint_from_hex(hex_m);
  BigInt *m_inv = bigint_from_hex(hex_m_inv);

  BigInt *res_my = montgomery_reduce(T, m, m_inv, k_bits);

  mpz_t G_T, G_m, G_m_inv, G_R, G_u, G_sum, G_t;
  mpz_inits(G_T, G_m, G_m_inv, G_R, G_u, G_sum, G_t, NULL);

  mpz_set_str(G_T, hex_T, 16);
  mpz_set_str(G_m, hex_m, 16);
  mpz_set_str(G_m_inv, hex_m_inv, 16);

  mpz_ui_pow_ui(G_R, 2, k_bits);

  mpz_mul(G_u, G_T, G_m_inv);
  mpz_mod(G_u, G_u, G_R);

  mpz_mul(G_sum, G_u, G_m);
  mpz_add(G_sum, G_sum, G_T);

  mpz_tdiv_q_2exp(G_t, G_sum, k_bits);

  mpz_mod(G_t, G_t, G_m);
  char *str_gmp = mpz_get_str(NULL, 16, G_t);

  printf("T       = 0x%s\n", hex_T);
  printf("m       = 0x%s\n", hex_m);
  printf("m_inv   = 0x%s\n", hex_m_inv);
  printf("k_bits  = %d\n", k_bits);
  printf("résultat attendu GMP = 0x%s\n", str_gmp);
  printf("résultat BigInt      = ");
  bigint_print(res_my);

  free(str_gmp);
  bigint_free(T);
  bigint_free(m);
  bigint_free(m_inv);
  bigint_free(res_my);
  mpz_clears(G_T, G_m, G_m_inv, G_R, G_u, G_sum, G_t, NULL);
}

void test_montgomery_mul_once(const char *hex_a, const char *hex_b,
                              const char *hex_m, int k_bits) {
  BigInt *a = bigint_from_hex(hex_a);
  BigInt *b = bigint_from_hex(hex_b);
  BigInt *m = bigint_from_hex(hex_m);

  BigInt *minv = bigint_modinv_pow2(m, k_bits);
  bigint_negate(minv);

  BigInt *res_my = montgomery_mul(a, b, m, minv, k_bits);

  mpz_t G_a, G_b, G_m, G_minv, G_R, G_T, G_u, G_sum, G_t, G_res;
  mpz_inits(G_a, G_b, G_m, G_minv, G_R, G_T, G_u, G_sum, G_t, G_res, NULL);

  mpz_set_str(G_a, hex_a, 16);
  mpz_set_str(G_b, hex_b, 16);
  mpz_set_str(G_m, hex_m, 16);

  mpz_ui_pow_ui(G_R, 2, k_bits);

  if (mpz_invert(G_minv, G_m, G_R) == 0) {
    fprintf(stderr, "mpz_invert failed\n");
    exit(1);
  }
  mpz_neg(G_minv, G_minv);
  mpz_mod(G_minv, G_minv, G_R);

  mpz_mul(G_T, G_a, G_b);

  mpz_mul(G_u, G_T, G_minv);
  mpz_mod(G_u, G_u, G_R);

  mpz_mul(G_sum, G_u, G_m);
  mpz_add(G_sum, G_sum, G_T);

  mpz_tdiv_q_2exp(G_t, G_sum, k_bits);
  mpz_mod(G_res, G_t, G_m);

  char *str_gmp = mpz_get_str(NULL, 16, G_res);

  printf("a       = 0x%s\n", hex_a);
  printf("b       = 0x%s\n", hex_b);
  printf("m       = 0x%s\n", hex_m);
  printf("k_bits  = %d\n", k_bits);
  printf("Résultat GMP    = 0x%s\n", str_gmp);
  printf("Résultat BigInt = ");
  bigint_print(res_my);

  free(str_gmp);
  bigint_free(a);
  bigint_free(b);
  bigint_free(m);
  bigint_free(minv);
  bigint_free(res_my);
  mpz_clears(G_a, G_b, G_m, G_minv, G_R, G_T, G_u, G_sum, G_t, G_res, NULL);
}

void test_montgomery_powm(const char *hex_base, const char *hex_exp,
                          const char *hex_mod, int k_bits) {
  BigInt *B = bigint_from_hex(hex_base);
  BigInt *E = bigint_from_hex(hex_exp);
  BigInt *M = bigint_from_hex(hex_mod);

  BigInt *R_my = montgomery_powm(B, E, M);

  mpz_t gB, gE, gM, gR;
  mpz_inits(gB, gE, gM, gR, NULL);
  mpz_set_str(gB, hex_base, 16);
  mpz_set_str(gE, hex_exp, 16);
  mpz_set_str(gM, hex_mod, 16);
  mpz_powm(gR, gB, gE, gM);

  char *s_gmp = mpz_get_str(NULL, 16, gR);

  printf("base       = 0x%s\n", hex_base);
  printf("exp        = 0x%s\n", hex_exp);
  printf("mod        = 0x%s\n", hex_mod);
  printf("k_bits     = %d\n", k_bits);
  printf("résultat GMP    = 0x%s\n", s_gmp);
  printf("résultat BigInt = ");
  bigint_print(R_my);

  free(s_gmp);
  bigint_free(B);
  bigint_free(E);
  bigint_free(M);
  bigint_free(R_my);
  mpz_clears(gB, gE, gM, gR, NULL);
}

void test_primality_with_gmp(const char *hex_n, int iterations) {
  mpz_t gmp_n;
  mpz_init(gmp_n);
  mpz_set_str(gmp_n, hex_n, 16);
  BigInt *n = bigint_from_hex(hex_n);

  int is_prime = mpz_probab_prime_p(gmp_n, iterations);
  int is_prime_bigint = is_probable_prime(n, iterations);

  printf("Résultat GMP : %s\n", is_prime ? "Premier" : "Non premier");
  printf("Résultat BigInt : %s\n", is_prime_bigint ? "Premier" : "Non premier");

  mpz_clear(gmp_n);
}

int main() {
  char *sep = "----------------------------------------\n";
  if (true) {
    printf("%s", sep);
    printf("Addition\n");

    mpz_t gmp_a, gmp_b, gmp_result;
    mpz_init_set_str(gmp_a, "123456789012345678901234567890", 16);
    mpz_init_set_str(gmp_b, "987654321098765432109876543210", 16);
    mpz_init(gmp_result);
    mpz_add(gmp_result, gmp_a, gmp_b);
    BigInt *a = bigint_from_hex("123456789012345678901234567890");
    BigInt *b = bigint_from_hex("987654321098765432109876543210");
    BigInt *result = bigint_add(a, b);

    gmp_printf("Résultat GMP     : 0x%Zx\n", gmp_result);
    printf("Résultat BigInt  : ");
    bigint_print(result);
    bigint_free(a);
    bigint_free(b);
    bigint_free(result);
    mpz_clears(gmp_a, gmp_b, gmp_result, NULL);
  }

  if (true) {
    printf("%s", sep);
    printf("Soustraction\n");

    const char *hex_a = "0000000100000000000000000000000000000000";
    const char *hex_b = "0004FFFFFFFFFFFFFFFFFFFFFFFFFFFF";

    mpz_t gmp_a, gmp_b, gmp_result;
    mpz_init_set_str(gmp_a, hex_a, 16);
    mpz_init_set_str(gmp_b, hex_b, 16);
    mpz_init(gmp_result);
    mpz_sub(gmp_result, gmp_a, gmp_b);
    BigInt *a = bigint_from_hex(hex_a);
    BigInt *b = bigint_from_hex(hex_b);
    BigInt *result = bigint_sub(a, b);

    gmp_printf("Résultat GMP     : 0x%Zx\n", gmp_result);
    printf("Résultat BigInt  : ");
    bigint_print(result);
    bigint_free(a);
    bigint_free(b);
    bigint_free(result);
    mpz_clears(gmp_a, gmp_b, gmp_result, NULL);
  }

  if (true) {
    printf("%s", sep);
    printf("Multiplication\n");

    mpz_t gmp_a, gmp_b, gmp_result;
    mpz_init_set_str(gmp_a, "123456789012345678901234567890", 16);
    mpz_init_set_str(gmp_b, "987654321098765432109876543210", 16);
    mpz_init(gmp_result);
    mpz_mul(gmp_result, gmp_a, gmp_b);
    BigInt *a = bigint_from_hex("123456789012345678901234567890");
    BigInt *b = bigint_from_hex("987654321098765432109876543210");
    BigInt *result = bigint_mul(a, b);

    gmp_printf("Résultat GMP     : 0x%Zx\n", gmp_result);
    printf("Résultat BigInt  : ");
    bigint_print(result);
    bigint_free(a);
    bigint_free(b);
    bigint_free(result);
    mpz_clears(gmp_a, gmp_b, gmp_result, NULL);
  }

  if (true) {
    printf("%s", sep);
    printf("Modulo\n");

    mpz_t gmp_a, gmp_b, gmp_result;
    mpz_init_set_str(gmp_a, "123456789012345678901234567890", 16);
    mpz_init_set_str(gmp_b, "987654321098765432109876543210", 16);
    mpz_init(gmp_result);
    mpz_mod(gmp_result, gmp_b, gmp_a);
    BigInt *a = bigint_from_hex("123456789012345678901234567890");
    BigInt *b = bigint_from_hex("987654321098765432109876543210");
    BigInt *result = bigint_mod(b, a);

    gmp_printf("Résultat GMP     : 0x%Zx\n", gmp_result);
    printf("Résultat BigInt  : ");
    bigint_print(result);
    bigint_free(a);
    bigint_free(b);
    bigint_free(result);
    mpz_clears(gmp_a, gmp_b, gmp_result, NULL);
  }

  if (true) {
    printf("%s", sep);
    printf("Multiplication basse\n");

    mpz_t gmp_a, gmp_b, gmp_res, gmp_mask;
    mpz_inits(gmp_a, gmp_b, gmp_res, gmp_mask, NULL);

    mpz_set_str(gmp_a, "FFFFFFFFFFFFFFFFFFFFFFFF", 16);
    mpz_set_str(gmp_b, "12345678", 16);

    mpz_mul(gmp_res, gmp_a, gmp_b);

    // Mask on k_bits (simuling bigint_mul_low)
    mpz_setbit(gmp_mask, 96);
    mpz_sub_ui(gmp_mask, gmp_mask, 1);
    mpz_and(gmp_res, gmp_res, gmp_mask);

    char *gmp_str = mpz_get_str(NULL, 16, gmp_res);

    BigInt *a = bigint_from_hex("FFFFFFFFFFFFFFFFFFFFFFFF");
    BigInt *b = bigint_from_hex("12345678");
    BigInt *my_res = bigint_mul_low(a, b, 96);

    bigint_print(a);
    bigint_print(b);
    printf("Résultat GMP      : 0x%s\n", gmp_str);
    printf("Résultat BigInt   : ");
    bigint_print(my_res);

    free(gmp_str);
    bigint_free(a);
    bigint_free(b);
    bigint_free(my_res);
    mpz_clears(gmp_a, gmp_b, gmp_res, gmp_mask, NULL);
  }

  if (true) {
    printf("%s", sep);
    printf("Inverse modulaire\n");

    const char *m = "FFFAFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
    const char *m_inv = "4ffffffffffffffffffffffffffff";
    int k = 128;

    test_modinv_with_gmp(m, k);
    test_modinv_with_gmp(m_inv, k);
  }

  if (true) {
    printf("%s", sep);
    printf("Shift à droite (division par 2)\n");

    int s = 24;
    BigInt *a = bigint_from_hex("123456789012345678901234567890");
    BigInt *result = bigint_div_pow2(a, s);
    bigint_print(a);
    printf(">> %i  : \n", s);
    bigint_print(result);
    bigint_free(a);
    bigint_free(result);
  }

  if (true) {
    printf("%s", sep);
    printf("Réduction de Montgomery\n");
    test_montgomery_reduce_once(
        "123456789ABCDEF0123456789ABCDEF0",  // T
        "FFFAFFFFFFFFFFFFFFFFFFFFFFFFFFFF",  // m
        "FFFB0000000000000000000000000001",  // m_inv = –m⁻¹ mod 2^128
        128);
  }

  if (true) {
    printf("%s", sep);
    printf("Négation\n");
    BigInt *a = bigint_from_hex("123456789ABCDEF0");
    printf("Avant négation : ");
    bigint_print(a);
    bigint_negate(a);
    printf("Après négation : ");
    bigint_print(a);
    bigint_free(a);
  }

  if (true) {
    printf("%s", sep);
    printf("Longueur en bits\n");
    BigInt *a = bigint_from_hex("523456789ABCDEF0");
    bigint_print(a);
    printf("Nombre de bits : %d\n", bigint_bit_length(a));
    bigint_free(a);
  }

  if (true) {
    printf("%s", sep);
    printf("Multiplication de Montgomery\n");
    test_montgomery_mul_once("0000004A3414C4C0", "000000010000000000000000",
                             "00000080C970E2C9", 64);
  }

  if (true) {
    printf("%s", sep);
    printf("Génération de nombres aléatoires\n");
    int num_blocks = 4;
    BigInt *num = bigint_random(num_blocks);
    printf("Nombre aléatoire : ");
    bigint_print(num);
    bigint_free(num);
  }

  if (true) {
    printf("%s", sep);
    printf("Exponentiation modulaire\n");
    test_montgomery_powm("0000005B3164DB0C", "3", "00000080C970E2C9", 128);
    printf("\n");
  }

  if (true) {
    printf("%s", sep);
    printf("Test de primalité\n");

    char *n_hex =
        "90CBE39B245DB9B5C4B637BC43576D9B01131DE08CDBE598D4EA8EA6328E28D2B3"
        "7BAE0D51C9B003D4F1F47D2F8650B991CA0B4D71E93B3280EFA4DDC0C4020593E5"
        "EC8F3491BD0D5322A5CC7030CA2A41899BA1D002396DF09D23D650A6951D3AC763"
        "270DC9EE539493A9793370DADA6409A68ED77E0E5930477C66DE26FCA9";
    test_primality_with_gmp(n_hex, 10);
  }

  if (true) {
    printf("%s", sep);
    printf("Vitesse du modulo avec petits modulos\n");

    char *n_hex =
        "90CBE39B245DB9B5C4B637BC43576D9B01131DE08CDBE598D4EA8EA6328E28D2B3"
        "7BAE0D51C9B003D4F1F47D2F8650B991CA0B4D71E93B3280EFA4DDC0C4020593E5"
        "EC8F3491BD0D5322A5CC7030CA2A41899BA1D002396DF09D23D650A6951D3AC763"
        "270DC9EE539493A9793370DADA6409A68ED77E0E5930477C66DE26FCA9";
    BigInt *three = bigint_from_hex("3");
    BigInt *five = bigint_from_hex("5");
    BigInt *seven = bigint_from_hex("7");
    BigInt *eleven = bigint_from_hex("11");
    BigInt *thirteen = bigint_from_hex("13");
    BigInt *a = bigint_from_hex(n_hex);

    BigInt *result1 = bigint_mod(a, three);
    printf("Résultat 1 : ");
    bigint_print(result1);
    BigInt *result2 = bigint_mod(a, five);
    printf("Résultat 2 : ");
    bigint_print(result2);
    BigInt *result3 = bigint_mod(a, seven);
    printf("Résultat 3 : ");
    bigint_print(result3);
    BigInt *result4 = bigint_mod(a, eleven);
    printf("Résultat 4 : ");
    bigint_print(result4);
    BigInt *result5 = bigint_mod(a, thirteen);
    printf("Résultat 5 : ");
    bigint_print(result5);
  }

  if (true) {
    printf("%s", sep);
    printf("Génération de nombres premiers\n");
    int bits = 1024;
    int iterations = 10;
    BigInt *prime = bigint_generate_prime(bits, iterations);
    printf("Nombre premier généré : ");
    bigint_print(prime);
    bigint_free(prime);
  }

  return 0;
}