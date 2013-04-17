// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#if defined(WIN32)
#include <Windows.h>
#include <wmmintrin.h>
#endif

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <getopt.h>
#include <boost/crc.hpp>
#include <limits.h>
#include <malloc.h>
#include "mersenne_twister.h"
#include "stopwatch.h"
#include "cpufeatures.h"
#include "aesni.h"

#if defined(__GNUC__)
#include <string.h>
#include <strings.h>
#include <sched.h>
#include <pthread.h>
typedef pthread_t HANDLE;
typedef uint32_t DWORD;
typedef uint64_t DWORD64;
typedef void* LPVOID;
#endif

#if defined(__GNUC__)
#include <smmintrin.h>
#endif
static const int DEFAULT_ITERATIONS = 16;
static const int DEFAULT_BUF_SIZE = 128;
static const int DEFAULT_NUM_THREADS = 1;
static const int MAX_NUM_THREADS = 256;

enum CoreBinding {
  NoCoreBinding,
  LinearCoreBinding,
  EvenFirstCoreBinding,
  OddFirstCoreBinding,
  AutomaticCoreBinding,
  _LastCoreBinding
};

std::string CoreBindingString[_LastCoreBinding] =
{
  "keine (none)",
  "linear (linear)",
  "Kerne mit gerader Nummer zuerst (evenfirst)",
  "Kerne mit ungerader Nummer zuerst (oddfirst)",
  "automatisch (auto)"
};

int gIterations = DEFAULT_ITERATIONS;
uint8_t* gInBuf = NULL;
uint8_t* gEncBuf = NULL;
uint8_t* gDecBuf = NULL;
int gBufSize = DEFAULT_BUF_SIZE;
int gNumThreads[MAX_NUM_THREADS] = { DEFAULT_NUM_THREADS };
int gMaxNumThreads = 1;
int gThreadIterations = 0;
int gThreadPriority;
int gNumSockets = 1;
int gVerbose = 0;
CoreBinding gCoreBinding = AutomaticCoreBinding;

ALIGN16 uint8_t AES_CBC_IV[] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};

ALIGN16 uint8_t AES128_TEST_KEY[] = {0x7E,0x24,0x06,0x78,0x17,0xFA,0xE0,0xD7,0x43,0xD6,0xCE,0x1F,0x32,0x53,0x91,0x63};
ALIGN16 uint8_t AES192_TEST_KEY[] = {0x7C,0x5C,0xB2,0x40,0x1B,0x3D,0xC3,0x3C,0x19,0xE7,0x34,0x08,0x19,0xE0,0xF6,0x9C,0x67,0x8C,0x3D,0xB8,0xE6,0xF6,0xA9,0x1A};
ALIGN16 uint8_t AES256_TEST_KEY[] = {0xF6,0xD6,0x6D,0x6B,0xD5,0x2D,0x59,0xBB,0x07,0x96,0x36,0x58,0x79,0xEF,0xF8,0x86,0xC6,0x6D,0xD5,0x1A,0x5B,0x6A,0x99,0x74,0x4B,0x50,0x59,0x0C,0x87,0xA2,0x38,0x84};

enum _long_options {
  SELECT_HELP,
  SELECT_CORE_BINDING,
  SELECT_NUM_SOCKETS,
  SELECT_ITERATIONS,
  SELECT_THREADS
};
static struct option long_options[] = {
  { "core-binding",  required_argument, 0, SELECT_CORE_BINDING },
  { "sockets",       required_argument, 0, SELECT_NUM_SOCKETS },
  { "iterations",    required_argument, 0, SELECT_ITERATIONS },
  { "threads",       required_argument, 0, SELECT_THREADS },
  { "help",          no_argument,       0, SELECT_HELP },
};

static const unsigned int DecryptMode = 0x80000000U;
enum Method {
  AES128Enc = 1 << 0,
  AES192Enc = 1 << 1,
  AES256Enc = 1 << 2,
  OpenSSL128Enc = 1 << 3,
  OpenSSL192Enc = 1 << 4,
  OpenSSL256Enc = 1 << 5,
  AES128Dec = AES128Enc | DecryptMode,
  AES192Dec = AES192Enc | DecryptMode,
  AES256Dec = AES256Enc | DecryptMode,
  OpenSSL128Dec = OpenSSL128Enc | DecryptMode,
  OpenSSL192Dec = OpenSSL192Enc | DecryptMode,
  OpenSSL256Dec = OpenSSL256Enc | DecryptMode
};

struct BenchmarkResult {
  BenchmarkResult()
    : inBuf(NULL)
    , hThread(0)
    , t(0)
    , ticks(0)
  { /* ... */ }
  ~BenchmarkResult() {
#ifdef WIN32
    if (hThread)
      CloseHandle(hThread);
#endif
  }
  // input fields
  Method method;
  AES_KEY encKey;
  AES_KEY decKey;
  uint8_t* inBuf;
  uint8_t* encBuf;
  uint8_t* decBuf;
  int bufSize;
  int threadNum;
  HANDLE hThread;
  int iterations;
  int numCores;
  CoreBinding coreBinding;
  // output fields
  int64_t t;
  int64_t ticks;
};


// die im Thread laufenden Benchmark-Routine
#if defined(WIN32)
DWORD WINAPI
#elif defined(__GNUC__)
void* 
#endif
  BenchmarkThreadProc(LPVOID lpParameter)
{
  BenchmarkResult* result = (BenchmarkResult*)lpParameter;
  if (result->coreBinding != NoCoreBinding) {
    int core = result->threadNum % result->numCores;
    switch (result->coreBinding) {
    case EvenFirstCoreBinding:
      {
        core = 2 * core + ((core < result->numCores / 2)? 0 : 1 - result->numCores);
        break;
      }
    case OddFirstCoreBinding:
      {
        core = 2 * core + ((core < result->numCores / 2)? 1 : -result->numCores);
        break;
      }
    case LinearCoreBinding:
      // fall-through
    case NoCoreBinding:
      // fall-through
    default:
      // ignore
      break;
    }
#if defined(WIN32)
    DWORD_PTR affinityMask = 1 << (DWORD)core;
    SetThreadAffinityMask(GetCurrentThread(), affinityMask);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#elif defined(__GNUC__)
    // set CPU affinity
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    // set priority
    pthread_attr_t tattr;
    pthread_attr_init(&tattr);
    int policy = 0;
    pthread_attr_getschedpolicy(&tattr, &policy);
    int maxprio = sched_get_priority_max(policy);
    pthread_setschedprio(pthread_self(), maxprio);
    pthread_attr_destroy(&tattr);
#endif
  }
  int64_t tMin = LLONG_MAX;
  int64_t ticksMin = LLONG_MAX;
  for (int i = 0; i < result->iterations; ++i) {
    int64_t t, ticks;
    {
      Stopwatch stopwatch(t, ticks);
      switch (result->method)
      {
      case AES128Enc:
        // fall-through
      case AES256Enc:
        {
          uint8_t* plain = (uint8_t*)result->inBuf + result->threadNum * result->bufSize;
          uint8_t* enc = (uint8_t*)result->encBuf + result->threadNum * result->bufSize;
          AES_CBC_encrypt(plain, enc, AES_CBC_IV, result->bufSize, result->encKey.rd_key, result->encKey.rounds);
          break;
        }
      case AES128Dec:
        // fall-through
      case AES256Dec:
        {
          uint8_t* enc = (uint8_t*)result->encBuf + result->threadNum * result->bufSize;
          uint8_t* dec = (uint8_t*)result->decBuf + result->threadNum * result->bufSize;
          AES_CBC_decrypt(enc, dec, AES_CBC_IV, result->bufSize, result->decKey.rd_key, result->decKey.rounds);
          break;
        }
      case OpenSSL128Enc:
        // fall-through
      case OpenSSL256Enc:
        {
          uint8_t* plain = (uint8_t*)result->inBuf + result->threadNum * result->bufSize;
          uint8_t* enc = (uint8_t*)result->encBuf + result->threadNum * result->bufSize;
          AES_CBC_encrypt_OpenSSL(plain, enc, result->bufSize, &result->encKey, AES_CBC_IV, AES_ENCRYPT);
          break;
        }
      case OpenSSL128Dec:
        // fall-through
      case OpenSSL256Dec:
        {
          uint8_t* plain = (uint8_t*)result->inBuf + result->threadNum * result->bufSize;
          uint8_t* enc = (uint8_t*)result->encBuf + result->threadNum * result->bufSize;
          AES_CBC_encrypt_OpenSSL(plain, enc, result->bufSize, &result->encKey, AES_CBC_IV, AES_DECRYPT);
          break;
        }
      }
    }
    if (t < tMin)
      tMin = t;
    if (ticks < ticksMin)
      ticksMin = ticks;
  }
  result->t = tMin;
  result->ticks = ticksMin;
  return EXIT_SUCCESS;
}


void runBenchmark(int numThreads, const char* strMethod, const Method method) {
  HANDLE* hThread = new HANDLE[numThreads];
  BenchmarkResult* pResult = new BenchmarkResult[numThreads];
  int numCores = CPUFeatures::instance().getNumCores();
  std::cout.setf(std::ios_base::left, std::ios_base::adjustfield);
  std::cout << "  " << std::setfill(' ') << std::setw(18) << strMethod << "  Generieren ...";
  int64_t t = LLONG_MAX;
  int64_t ticks = LLONG_MAX;
#if defined(__GNUC__)
  Stopwatch stopwatch(t, ticks);
#endif
  std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b";
  std::cout << ((method & DecryptMode)? "Entschluesselung" : "Verschluesselung") << " ...";

  for (int i = 0; i < numThreads; ++i) {
    pResult[i].threadNum = i;
    pResult[i].inBuf = gInBuf;
    pResult[i].encBuf = gEncBuf;
    pResult[i].decBuf = gDecBuf;
    pResult[i].bufSize = gBufSize;
    pResult[i].iterations = gIterations;
    pResult[i].numCores = numCores;
    pResult[i].coreBinding = gCoreBinding;
#if defined(WIN32)
    pResult[i].hThread = CreateThread(NULL, 0, BenchmarkThreadProc, (LPVOID)&pResult[i], CREATE_SUSPENDED, NULL);
#elif defined(__GNUC__)
    pthread_create(&pResult[i].hThread, NULL, BenchmarkThreadProc, (void*)&pResult[i]);
#endif
    pResult[i].method = method;
    switch (method) {
    case AES128Enc:
    case OpenSSL128Enc:
      AES_set_encrypt_key(AES128_TEST_KEY, 128, &pResult[i].encKey);
      break;
    case AES192Enc:
    case OpenSSL192Enc:
      AES_set_encrypt_key(AES192_TEST_KEY, 192, &pResult[i].encKey);
      break;
    case AES256Enc:
    case OpenSSL256Enc:
      AES_set_encrypt_key(AES256_TEST_KEY, 256, &pResult[i].encKey);
      break;
    case AES128Dec:
    case OpenSSL128Dec:
      AES_set_decrypt_key(AES128_TEST_KEY, 128, &pResult[i].decKey);
      break;
    case AES192Dec:
    case OpenSSL192Dec:
      AES_set_decrypt_key(AES192_TEST_KEY, 192, &pResult[i].decKey);
      break;
    case AES256Dec:
    case OpenSSL256Dec:
      AES_set_decrypt_key(AES256_TEST_KEY, 256, &pResult[i].decKey);
      break;
    }
    hThread[i] = pResult[i].hThread; // hThread[] wird von WaitForMultipleObjects() benötigt
  }

#if defined(WIN32)
  {
    Stopwatch stopwatch(t, ticks);
    for (int i = 0; i < numThreads; ++i)
      ResumeThread(hThread[i]);
    WaitForMultipleObjects(numThreads, hThread, TRUE, INFINITE);
  }
#elif defined(__GNUC__)
  for (int i = 0; i < numThreads; ++i)
    pthread_join(hThread[i], 0);
  stopwatch.stop();
#endif

  std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";
  // Ergebnisse sammeln
  int64_t tMin = pResult[0].t;
  for (int i = 1; i < numThreads; ++i)
    tMin += pResult[i].t;
  tMin /= numThreads;

  std::cout.setf(std::ios_base::right, std::ios_base::adjustfield);
  std::cout << std::setfill(' ') << std::setw(8) << std::dec << (1000*tMin/Stopwatch::RESOLUTION) << " ms  " 
    << std::fixed << std::setprecision(2) << std::setw(8)
    << (float)gBufSize*gIterations/1024/1024/((float)t/Stopwatch::RESOLUTION)*numThreads << " MB/s"
    << std::setw(8) << (float)pResult[0].ticks / gBufSize
    << std::endl;

  delete [] pResult;
  delete [] hThread;
}


void usage(void) {
  std::cout << "Aufruf: aes [Optionen]" << std::endl
    << std::endl
    << "Optionen:" << std::endl
    << "  -n N" << std::endl
    << "     N MByte große Blocks generieren (Vorgabe: " << DEFAULT_BUF_SIZE << ")" << std::endl
    << std::endl
    << "  (--iterations|-i) N" << std::endl
    << "     Generieren N Mal wiederholen (Vorgabe: " << DEFAULT_ITERATIONS << ")" << std::endl
    << std::endl
    << "  (--core-binding|-b) (linear|evenfirst|oddfirst|none|auto)" << std::endl
    << "     Modus, nach dem Threads an logische CPU-Kerne gebunden werden" << std::endl
    << "     (Vorgabe: none)" << std::endl
    << std::endl
    << "  (--quiet|-q)" << std::endl
    << "     Keine Informationen ausgeben" << std::endl
    << std::endl
    << "  (--threads|-t) N" << std::endl
    << "     Zufallszahlen in N Threads parallel generieren (Vorgabe: " << DEFAULT_NUM_THREADS << ")" << std::endl
    << "     Mehrfachnennungen möglich." << std::endl
    << std::endl
    << "  (--help|-h|-?)" << std::endl
    << "     Diese Hilfe anzeigen" << std::endl
    << std::endl;
}


void disclaimer(void) {
  std::cout << "aes - Experimente mit AES-NI." << std::endl
    << "Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag" << std::endl
    << "Alle Rechte vorbehalten." << std::endl
    << std::endl
    << "Diese Software wurde zu Lehr- und Demonstrationszwecken erstellt." << std::endl
    << "Alle Ausgaben ohne Gewähr." << std::endl
    << std::endl;
}


int main(int argc, char* argv[]) {
#if defined(WIN32)
  gThreadPriority = GetThreadPriority(GetCurrentThread());
  SecureZeroMemory(gNumThreads+1, sizeof(gNumThreads[0]) * (MAX_NUM_THREADS-1));
#elif defined(__GNUC__)
  bzero(gNumThreads+1, sizeof(gNumThreads[0]) * (MAX_NUM_THREADS-1));
  // TODO
#endif
  for (;;) {
    int option_index = 0;
    int c = getopt_long(argc, argv, "vh?n:t:i:b:s:", long_options, &option_index);
    if (c == -1)
      break;
    switch (c)
    {
    case SELECT_ITERATIONS:
      // fall-through
    case 'i':
      if (optarg == NULL) {
        usage();
        return EXIT_FAILURE;
      }
      gIterations = atoi(optarg);
      if (gIterations <= 0)
        gIterations = DEFAULT_ITERATIONS;
      break;
    case 'n':
      if (optarg == NULL) {
        usage();
        return EXIT_FAILURE;
      }
      gBufSize = atoi(optarg);
      if (gBufSize <= 0)
        gBufSize = DEFAULT_BUF_SIZE;
      break;
    case SELECT_NUM_SOCKETS:
      // fall-through
    case 's':
      if (optarg == NULL) {
        usage();
        return EXIT_FAILURE;
      }
      gNumSockets = atoi(optarg);
      if (gNumSockets <= 0)
        gNumSockets = 1;
      break;
    case SELECT_THREADS:
      // fall-through
    case 't':
      if (optarg == NULL) {
        usage();
        return EXIT_FAILURE;
      }
      if (gThreadIterations < MAX_NUM_THREADS) {
        int numThreads = atoi(optarg);
        if (numThreads <= 0)
          numThreads = 1;
        if (numThreads > gMaxNumThreads)
          gMaxNumThreads = numThreads;
        gNumThreads[gThreadIterations++] = numThreads;
      }
      break;
    case 'v':
      ++gVerbose;
      break;
    case 'h':
      // fall-through
    case '?':
      // fall-through
    case SELECT_HELP:
      disclaimer();
      usage();
      return EXIT_SUCCESS;
      break;
    case SELECT_CORE_BINDING:
      // fall-through
    case 'b':
      if (optarg == NULL) {
        usage();
        return EXIT_FAILURE;
      }
      if (strcmp(optarg, "linear") == 0) {
        gCoreBinding = LinearCoreBinding;
      }
      else if (strcmp(optarg, "evenfirst") == 0) {
        gCoreBinding = EvenFirstCoreBinding;
      }
      else if (strcmp(optarg, "oddfirst") == 0) {
        gCoreBinding = OddFirstCoreBinding;
      }
      else if (strcmp(optarg, "none") == 0) {
        gCoreBinding = NoCoreBinding;
      }
      else if (strcmp(optarg, "auto") == 0) {
        gCoreBinding = AutomaticCoreBinding;
      }
      else {
        usage();
        return EXIT_FAILURE;
      }
      break;
    default:
      usage();
      return EXIT_FAILURE;
    }
  }

  if (gVerbose > 1) {
    static const char* B[2] = { " NO", "YES" };
    std::cout.setf(std::ios_base::right, std::ios_base::adjustfield);

    const std::vector<LogicalProcessorData>& cpu = CPUFeatures::instance().logical_cpu_data;
    std::cout << ">>>\tAPIC\tHT\t#" << std::endl;
    for (std::vector<LogicalProcessorData>::const_iterator i = cpu.begin(); i != cpu.end(); ++i) {
      std::cout << ">>>\t"
        << i->localApicId << "\t" 
        << i->HTT << "\t" 
        << i->logicalProcessorCount << std::endl;
    }

    std::cout
      << ">>> # Logical cores (by system call): "
      << std::setw(3) << CPUFeatures::instance().logical_cores_from_system << std::endl
      << ">>> # Logical cores  : "
      << std::setw(3) << CPUFeatures::instance().cores << std::endl
      << ">>> # Threads/core   : "
      << std::setw(3) << CPUFeatures::instance().threads_per_package << std::endl
      << ">>> CPU vendor       : "
      << CPUFeatures::instance().vendor << std::endl
      << ">>> Multi-Threading  : "
      << B[CPUFeatures::instance().htt_supported] << std::endl
      << ">>> Hyper-Threading  : "
      << B[CPUFeatures::instance().ht_supported] << std::endl
      << ">>> CPU type         : "
      << std::setw(3) << CPUFeatures::instance().cpu_type << std::endl
      << ">>> CPU family       : "
      << std::setw(3) << CPUFeatures::instance().cpu_family << std::endl
      << ">>> CPU ext family   : "
      << std::setw(3) << CPUFeatures::instance().cpu_ext_family << std::endl
      << ">>> CPU model        : "
      << std::setw(3) << CPUFeatures::instance().cpu_model << std::endl
      << ">>> CPU ext model    : "
      << std::setw(3) << CPUFeatures::instance().cpu_ext_model << std::endl
      << ">>> CPU stepping     : "
      << std::setw(3) << CPUFeatures::instance().cpu_stepping << std::endl
      << ">>> CLFLUSH line size: "
      << std::setw(3) << CPUFeatures::instance().clflush_linesize << std::endl
      << ">>> MMX              : "
      << B[CPUFeatures::instance().mmx_supported] << std::endl
      << ">>> SSE              : "
      << B[CPUFeatures::instance().sse_supported] << std::endl
      << ">>> SSE2             : " 
      << B[CPUFeatures::instance().sse2_supported] << std::endl
      << ">>> SSE3             : " 
      << B[CPUFeatures::instance().sse3_supported] << std::endl
      << ">>> SSSE3            : "
      << B[CPUFeatures::instance().ssse3_supported] << std::endl
      << ">>> SSE4.1           : " 
      << B[CPUFeatures::instance().sse41_supported] << std::endl
      << ">>> SSE4.2           : " 
      << B[CPUFeatures::instance().sse42_supported] << std::endl
      << ">>> MONITOR/WAIT     : " 
      << B[CPUFeatures::instance().monitor_wait_supported] << std::endl
      << ">>> VMX              : " 
      << B[CPUFeatures::instance().vmx_supported] << std::endl
      << ">>> F16C             : " 
      << B[CPUFeatures::instance().f16c_supported] << std::endl
      << ">>> AVX              : " 
      << B[CPUFeatures::instance().avx_supported] << std::endl
      << ">>> FMA              : " 
      << B[CPUFeatures::instance().fma_supported] << std::endl
      << ">>> POPCNT           : " 
      << B[CPUFeatures::instance().popcnt_supported] << std::endl
      << ">>> RDRAND           : " 
      << B[CPUFeatures::instance().rdrand_supported] << std::endl
      << ">>> AES              : " 
      << B[CPUFeatures::instance().aes_supported] << std::endl
      << std::endl;
  }

  if (gVerbose == 3)
    return EXIT_SUCCESS;

  if (gCoreBinding == AutomaticCoreBinding) {
    gCoreBinding = (CPUFeatures::instance().ht_supported)
      ? EvenFirstCoreBinding
      : NoCoreBinding;
  }

  if (gVerbose > 1)
    std::cout << "Bindung der Threads an logische Kerne: "
    << CoreBindingString[gCoreBinding] << std::endl;

  if (!CPUFeatures::instance().isAESSupported()) {
    std::cout
      << "//////////////////////////////////////////////////////" << std::endl
      << "/// Die CPU unterstützt die AES-NI nicht.          ///" << std::endl
      << "/// Die betreffenden Benchmarks entfallen daher.   ///" << std::endl
      << "//////////////////////////////////////////////////////" << std::endl;
  }

  // Speicherblöcke mit Zufallszahlen belegen
  MersenneTwister gen;
  gen.seed();
  if (gVerbose > 0)
    std::cout << std::endl << "Generieren von " << gMaxNumThreads << "x" << gBufSize << " MByte ..." << std::endl;
  gBufSize *= 1024*1024;
  try {
    gInBuf  = new uint8_t[gMaxNumThreads * gBufSize];
    gEncBuf = new uint8_t[gMaxNumThreads * gBufSize];
    gDecBuf = new uint8_t[gMaxNumThreads * gBufSize];
  }
  catch(...) {
    std::cerr << "FEHLER: nicht genug freier Speicher!" << std::endl;
    return EXIT_FAILURE;
  }
  if (gInBuf == NULL || gEncBuf == NULL || gDecBuf == NULL) {
    std::cerr << "FEHLER beim Allozieren des Speichers!" << std::endl;
    return EXIT_FAILURE;
  }
  uint32_t* rn = reinterpret_cast<uint32_t*>(gInBuf);
  const uint32_t* const rne = rn + gMaxNumThreads * gBufSize / sizeof(uint32_t);
  while (rn < rne)
    gen.next(*rn++);

#if defined(WIN32)
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif
  bool correct = false;
  std::cout << "Ver- und Entschluesselung (" << gIterations << "x" << (gBufSize/1024/1024) << " MByte) ..." << std::endl;
  for (int i = 0; i <= gThreadIterations && gNumThreads[i] > 0 ; ++i) {
    const int numThreads = gNumThreads[i];
    std::cout << std::endl
      << "... in " << numThreads << " Thread" << (numThreads == 1? "" : "s") << ":" << std::endl
      << std::endl
      << "  Methode                t/Block      Durchsatz  Zyklen" << std::endl
      << "  -----------------------------------------------------" << std::endl;
    if (CPUFeatures::instance().isAESSupported()) {
      runBenchmark(numThreads, "AES128", AES128Enc);
      runBenchmark(numThreads, "AES128", AES128Dec);
      correct = memcmp(gInBuf, gDecBuf, gBufSize) == 0;
      std::cout << "  " << (correct? "OK." : ">>>FAIL<<<") << std::endl;
#if defined(USE_AES_192)
      runBenchmark(numThreads, "AES192", AES192Enc);
      runBenchmark(numThreads, "AES192", AES192Dec);
      correct = memcmp(gInBuf, gDecBuf, gBufSize) == 0;
      std::cout << "  " << (correct? "OK." : ">>>FAIL<<<") << std::endl;
#endif
      runBenchmark(numThreads, "AES256", AES256Enc);
      runBenchmark(numThreads, "AES256", AES256Dec);
      correct = memcmp(gInBuf, gDecBuf, gBufSize) == 0;
      std::cout << "  " << (correct? "OK." : ">>>FAIL<<<") << std::endl;

      runBenchmark(numThreads, "AES128 (OpenSSL)", OpenSSL128Enc);
      runBenchmark(numThreads, "AES128 (OpenSSL)", OpenSSL128Dec);
      correct = memcmp(gInBuf, gDecBuf, gBufSize) == 0;
      std::cout << "  " << (correct? "OK." : ">>>FAIL<<<") << std::endl;

      runBenchmark(numThreads, "AES256 (OpenSSL)", OpenSSL128Enc);
      runBenchmark(numThreads, "AES256 (OpenSSL)", OpenSSL128Dec);
      correct = memcmp(gInBuf, gDecBuf, gBufSize) == 0;
      std::cout << "  " << (correct? "OK." : ">>>FAIL<<<") << std::endl;

      runBenchmark(numThreads, "AES128 (OpenSSL)", OpenSSL128Enc);
      runBenchmark(numThreads, "AES128", AES128Dec);
      correct = memcmp(gInBuf, gDecBuf, gBufSize) == 0;
      std::cout << "  " << (correct? "OK." : ">>>FAIL<<<") << std::endl;

      runBenchmark(numThreads, "AES256 (OpenSSL)", OpenSSL128Enc);
      runBenchmark(numThreads, "AES256", AES128Dec);
      correct = memcmp(gInBuf, gDecBuf, gBufSize) == 0;
      std::cout << "  " << (correct? "OK." : ">>>FAIL<<<") << std::endl;
    }
  }

  delete [] gInBuf;
  delete [] gEncBuf;
  delete [] gDecBuf;

  return (correct)? EXIT_SUCCESS : EXIT_FAILURE;
}
