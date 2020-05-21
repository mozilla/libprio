#include <stdint.h>
#include <stdlib.h>

#include "prio/client.h"
#include "prio/encrypt.h"
#include "prio/server.h"

// Add some FATAL macro checks to crash the program immediately if some
// operation unexpectedly fails. This ensures that we catch programming
// errors and misconfiguration as early as possible in the fuzzing target.
#define PT_CHECKB_FATAL(s)                                                     \
  do {                                                                         \
    bool v = (s);                                                              \
    if (!v) {                                                                  \
      abort();                                                                 \
    }                                                                          \
  } while (0);

#define PT_CHECKC_FATAL(s)                                                     \
  do {                                                                         \
    rv = (s);                                                                  \
    if (rv != SECSuccess) {                                                    \
      abort();                                                                 \
    }                                                                          \
  } while (0);

// We will initialize these once and continue using them each iteration
// to increase the overall performance.
static PublicKey pkA = NULL;
static PublicKey pkB = NULL;
static PrivateKey skA = NULL;
static PrivateKey skB = NULL;
static PrioPRGSeed seed;

// Used to free all memory at the end to help track down memory leaks.
void
LLVMFuzzerShutdown()
{
  if (pkA)
    PublicKey_clear(pkA);

  if (pkB)
    PublicKey_clear(pkB);

  if (skA)
    PrivateKey_clear(skA);

  if (skB)
    PrivateKey_clear(skB);

  Prio_clear();
}

// This is called once at startup to initialize everything.
int
LLVMFuzzerInitialize()
{
  SECStatus rv = SECSuccess;

  Prio_init();
  memset(seed, 0, PRG_SEED_LENGTH);

  PT_CHECKC_FATAL(Keypair_new(&skA, &pkA));
  PT_CHECKC_FATAL(Keypair_new(&skB, &pkB));

  // Install our atexit handler to free memory
  if (atexit(LLVMFuzzerShutdown)) {
    fprintf(stderr, "Failed to register shutdown hook!\n");
    exit(1);
  }

  return 0;
}

// Forward declaration for libFuzzer's internal mutation routine.
size_t LLVMFuzzerMutate(uint8_t*, size_t, size_t);

// The server verify target requires two data blobs, once for server A and one
// for server B.
// There are many ways to achieve this, the simplest here seems to use a custom
// mutator that
// splits the data according to the indicated sizes. The layout of each file in
// the sample
// corpus is as follows:
//
//       | size of A |    data for A   | size of B |    data for B   |
//       |  1 byte   | size of A bytes |  1 byte   | size of B bytes |
//           ^               ^             ^               ^
// Index --- 0 ------------- 1 ----------- size A + 1 ---- size A + 2
//
// The custom mutator ensures that the data structures remain intact. Based on
// the given
// seed, we decide whether we should mutate the A or B part. The actual mutation
// is then
// performed by calling `LLVMFuzzerMutate` which is the internal libFuzzer
// mutation routine.
size_t
LLVMFuzzerCustomMutator(uint8_t* data, size_t size, size_t max,
                        unsigned int seed)
{
  // In our structure, we can only encode a limited amount of data.
  // Enforce this here to make the mutator work regardless of size settings.
  size_t max_allowed = UINT8_MAX * 2 + 2;
  if (max > max_allowed) {
    max = max_allowed;
  }

  // Check if this is a valid sample. If not, we will attempt to fix it.
  // The second case means that B part (size field or data) is guaranteed
  // out of bounds. The third case means that the size indicated in B
  // will be out of bounds.
  if (!size || data[0] + 2 >= size || data[0] + data[data[0] + 1] + 2 > size) {
    size_t hsize = (max - 2) / 2;
    hsize = hsize > 255 ? 255 : hsize;

    data[0] = (uint8_t)hsize;
    data[hsize + 1] = (uint8_t)hsize;
  }

  uint8_t sizeA = data[0];
  uint8_t sizeB = data[sizeA + 1];

  if (seed % 2) {
    /* Mutate A */
    uint8_t backupB[255];
    memcpy(backupB, &data[sizeA + 2], sizeB);
    size_t maxSizeA = max - sizeB - 2;
    maxSizeA = maxSizeA > 255 ? 255 : maxSizeA;
    size_t newSizeA = LLVMFuzzerMutate(&data[1], sizeA, maxSizeA);
    data[0] = (uint8_t)newSizeA;
    data[newSizeA + 1] = sizeB;
    memcpy(&data[newSizeA + 2], backupB, sizeB);
    return newSizeA + sizeB + 2;
  } else {
    /* Mutate B */
    size_t maxSizeB = max - sizeA - 2;
    maxSizeB = maxSizeB > 255 ? 255 : maxSizeB;
    size_t newSizeB = LLVMFuzzerMutate(&data[sizeA + 2], sizeB, maxSizeB);
    data[sizeA + 1] = (uint8_t)newSizeB;
    return sizeA + newSizeB + 2;
  }
}

int
LLVMFuzzerTestOneInput(uint8_t* data, size_t size)
{
  SECStatus rv = SECSuccess;
  PrioConfig cfg = NULL;
  PrioServer sA = NULL;
  PrioServer sB = NULL;
  PrioVerifier vA = NULL;
  PrioVerifier vB = NULL;
  PrioPacketVerify1 p1A = NULL;
  PrioPacketVerify1 p1B = NULL;
  PrioPacketVerify2 p2A = NULL;
  PrioPacketVerify2 p2B = NULL;

  unsigned int aLen = 0;
  unsigned int bLen = 0;

  unsigned int writtenA = 0;
  unsigned int writtenB = 0;

  unsigned char* dataA = NULL;
  unsigned char* dataB = NULL;

  // Same checks as in the custom mutator.
  // The smallest possible sample is 4 bytes
  if (size < 4 || data[0] + 2 >= size ||
      data[0] + data[data[0] + 1] + 2 > size) {
    return 0;
  }

  size_t sizeA = data[0];
  size_t sizeB = data[sizeA + 1];

  // We now create the config, server objects, verifiers, etc.
  // Each of these operations should succeed. If it doesn't, there
  // might be something wrong with the setup, so we use FATAL macros here.

  // TODO: Number of fields (3) and name chosen by fair dice roll.

  PT_CHECKB_FATAL(cfg =
                    PrioConfig_new(3, pkA, pkB, (unsigned char*)"test4", 5));

  PT_CHECKB_FATAL(sA = PrioServer_new(cfg, 0, skA, seed));
  PT_CHECKB_FATAL(sB = PrioServer_new(cfg, 1, skB, seed));

  PT_CHECKC_FATAL(PublicKey_encryptSize(sizeA, &aLen));
  PT_CHECKC_FATAL(PublicKey_encryptSize(sizeB, &bLen));

  PT_CHECKB_FATAL(dataA = malloc(aLen));
  PT_CHECKB_FATAL(dataB = malloc(bLen));

  // Encrypt the data for A and B so it can be successfully decrypted later
  PT_CHECKC_FATAL(PublicKey_encrypt(pkA, dataA, &writtenA, aLen,
                                    (unsigned char*)&data[1], sizeA));
  PT_CHECKC_FATAL(PublicKey_encrypt(pkB, dataB, &writtenB, bLen,
                                    (unsigned char*)&data[sizeA + 2], sizeB));

  PT_CHECKB_FATAL(writtenA == aLen);
  PT_CHECKB_FATAL(writtenB == bLen);

  PT_CHECKB_FATAL(vA = PrioVerifier_new(sA));
  PT_CHECKB_FATAL(vB = PrioVerifier_new(sB));

  PT_CHECKB_FATAL(p1A = PrioPacketVerify1_new());
  PT_CHECKB_FATAL(p1B = PrioPacketVerify1_new());

  PT_CHECKB_FATAL(p2A = PrioPacketVerify2_new());
  PT_CHECKB_FATAL(p2B = PrioPacketVerify2_new());

  // At this point, calls to the APIs are allowed to fail. However, we arrange
  // these calls in such a way that both the calls for A and B are performed,
  // regardless of the outcome of the first call. This ensures that the fuzzer
  // can progress evenly for both paths.

  if (PrioVerifier_set_data(vA, dataA, aLen) == SECSuccess) {
    rv = PrioPacketVerify1_set_data(p1A, vA);
  }

  if (PrioVerifier_set_data(vB, dataB, bLen) == SECSuccess) {
    if (PrioPacketVerify1_set_data(p1B, vB) != SECSuccess || rv != SECSuccess) {
      goto cleanup;
    }
  } else {
    goto cleanup;
  }

  rv = PrioPacketVerify2_set_data(p2A, vA, p1A, p1B);
  if (PrioPacketVerify2_set_data(p2B, vB, p1A, p1B) == SECSuccess) {
    if (rv != SECSuccess)
      goto cleanup;
  } else {
    goto cleanup;
  }

  // Hitting the valid case in the fuzzer is probably unlikely, but the fprintf
  // calls ensure that the calls to `PrioVerifier_isValid` aren't optimized out.

  if (PrioVerifier_isValid(vA, p2A, p2B) == SECSuccess) {
    fprintf(stderr, "Incredible, it is valid?!\n");
  }

  if (PrioVerifier_isValid(vB, p2A, p2B) == SECSuccess) {
    fprintf(stderr, "Incredible, it is valid?!\n");
  }

cleanup:
  PrioPacketVerify2_clear(p2A);
  PrioPacketVerify2_clear(p2B);

  PrioPacketVerify1_clear(p1A);
  PrioPacketVerify1_clear(p1B);

  PrioVerifier_clear(vA);
  PrioVerifier_clear(vB);

  PrioServer_clear(sA);
  PrioServer_clear(sB);
  PrioConfig_clear(cfg);

  if (dataA)
    free(dataA);
  if (dataB)
    free(dataB);

  return 0;
}
