// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#if defined(WIN32)
#include <Windows.h>
#include <nmmintrin.h>
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
#include "sharedutil.h"
#include "crc32.h"

#include "crcutil-fast/crc32c_sse4.h"
#include "crcutil-fast/generic_crc.h"
#include "crcutil-fast/protected_crc.h"
#include "crcutil-fast/rolling_crc.h"

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
static const int DEFAULT_RNGBUF_SIZE = 128;
static const int DEFAULT_NUM_THREADS = 1;
static const int MAX_NUM_THREADS = 256;

enum CoreBinding {
  NoCoreBinding,
  LinearCoreBinding,
  EvenFirstCoreBinding,
  OddFirstCoreBinding,
  AutomaticCoreBinding
};

int gIterations = DEFAULT_ITERATIONS;
uint8_t* gRngBuf = NULL;
int gRngBufSize = DEFAULT_RNGBUF_SIZE;
int gNumThreads[MAX_NUM_THREADS] = { DEFAULT_NUM_THREADS };
int gMaxNumThreads = 1;
int gThreadIterations = 0;
int gThreadPriority;
int gNumSockets = 1;
int gVerbose = 0;
CoreBinding gCoreBinding = EvenFirstCoreBinding;

struct CrcResult {
  int nThreads;
  std::string method;
  uint32_t crc;
};

std::vector<CrcResult> gCrcResults;

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

enum Method {
  Intrinsic8,
  Intrinsic16,
  Intrinsic32,
#if defined(_M_X64) || defined(__x86_64__)
  Intrinsic64,
#endif
  Boost,
  DefaultNaive,
  DefaultOptimized,
  Crc32cSSE4,
};

struct BenchmarkResult {
  BenchmarkResult()
    : rngBuf(NULL)
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
  uint8_t* rngBuf;
  int rngBufSize;
  int num;
  HANDLE hThread;
  int iterations;
  int numCores;
  CoreBinding coreBinding;
  // output fields
  int64_t t;
  int64_t ticks;
  uint32_t crc;
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
    int core = result->num % result->numCores;
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
  uint32_t crc = 0;
#if defined(_M_X64) || defined(__x86_64__)
  uint64_t crc64 = 0;
#endif
  for (int i = 0; i < result->iterations; ++i) {
    int64_t t, ticks;
    crc = 0;
#if defined(_M_X64) || defined(__x86_64__)
    crc64 = 0;
#endif
    {
      Stopwatch stopwatch(t, ticks);
      switch (result->method)
      {
      case Intrinsic8:
        {
          uint8_t* rn = (uint8_t*)result->rngBuf + result->num * result->rngBufSize / sizeof(uint8_t);
          const uint8_t* const rne = (uint8_t*)rn + result->rngBufSize / sizeof(uint8_t);
          while (rn < rne)
            crc = _mm_crc32_u8(crc, *rn++);
          break;
        }
      case Intrinsic16:
        {
          uint16_t* rn = (uint16_t*)result->rngBuf + result->num * result->rngBufSize / sizeof(uint16_t);
          const uint16_t* const rne = (uint16_t*)rn + result->rngBufSize / sizeof(uint16_t);
          while (rn < rne)
            crc = _mm_crc32_u16(crc, *rn++);
          break;
        }
      case Intrinsic32:
        {
          uint32_t* rn = (uint32_t*)result->rngBuf + result->num * result->rngBufSize / sizeof(uint32_t);
          const uint32_t* const rne = rn + result->rngBufSize / sizeof(uint32_t);
          while (rn < rne)
            crc = _mm_crc32_u32(crc, *rn++);
          break;
        }
#if defined(_M_X64) || defined(__x86_64__)
      case Intrinsic64:
        {
          uint64_t* rn = (uint64_t*)result->rngBuf + result->num * result->rngBufSize / sizeof(uint64_t);
          const uint64_t* const rne = rn + result->rngBufSize / sizeof(uint64_t);
          while (rn < rne)
            crc64 = _mm_crc32_u64(crc64, *rn++);
          break;
        }
#endif
      case Boost:
        {
          uint32_t* rn = (uint32_t*)result->rngBuf + result->num * result->rngBufSize / sizeof(uint32_t);
          boost::crc_optimal<32U, 0x1edc6f41U, 0U, 0U, true, true> crcBoost;
          crcBoost.process_bytes(rn, result->rngBufSize);
          crc = crcBoost.checksum();
          break;
        }
      case DefaultNaive:
        {
          uint8_t* rn = (uint8_t*)result->rngBuf + result->num * result->rngBufSize / sizeof(uint8_t);
          CRC32Naive<0U, 0x1edc6f41U, true> crc32c;
          crc = crc32c.process(rn, result->rngBufSize);
          break;
        }
      case DefaultOptimized:
        {
          uint8_t* rn = (uint8_t*)result->rngBuf + result->num * result->rngBufSize / sizeof(uint8_t);
          CRC32Optimized<0U, 0x1edc6f41U, true> crc32c;
          crc = crc32c.process(rn, result->rngBufSize);
          break;
        }
      case Crc32cSSE4:
        {
          crcutil::Crc32cSSE4 crc32(false);
          crc = (uint32_t)crc32.CrcDefault((uint8_t*)result->rngBuf + result->num * result->rngBufSize, result->rngBufSize, 0U);
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
#if defined(_M_X64) || defined(__x86_64__)
  result->crc = (result->method == Intrinsic64)? (uint32_t)(crc64 & 0xffffffffU) : crc;
#else
  result->crc = crc;
#endif
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
  for (int i = 0; i < numThreads; ++i) {
    pResult[i].num = i;
    pResult[i].rngBuf = gRngBuf;
    pResult[i].rngBufSize = gRngBufSize;
    pResult[i].iterations = gIterations;
    pResult[i].numCores = numCores;
    pResult[i].coreBinding = gCoreBinding;
#if defined(WIN32)
    pResult[i].hThread = CreateThread(NULL, 0, BenchmarkThreadProc, (LPVOID)&pResult[i], CREATE_SUSPENDED, NULL);
#elif defined(__GNUC__)
    pthread_create(&pResult[i].hThread, NULL, BenchmarkThreadProc, (void*)&pResult[i]);
#endif
    pResult[i].method = method;
    hThread[i] = pResult[i].hThread; // hThread[] wird von WaitForMultipleObjects() benötigt
  }
  std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b";
  std::cout << "Berechnen des CRC ...";

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

  // Ergebnisse sammeln
  CrcResult result = { numThreads, strMethod, pResult[0].crc };
  gCrcResults.push_back(result);
  int64_t tMin = pResult[0].t;
  for (int i = 1; i < numThreads; ++i)
    tMin += pResult[i].t;
  tMin /= numThreads;

  std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";
  std::cout.setf(std::ios_base::right, std::ios_base::adjustfield);
  std::cout << "0x" << std::setfill('0') << std::hex << std::setw(8) << pResult[0].crc
    << std::setfill(' ') << std::setw(10) << std::dec << tMin << " ms  " 
    << std::fixed << std::setprecision(2) << std::setw(8)
    << (float)gRngBufSize*gIterations/1024/1024/(1e-3*t)*numThreads << " MB/s"
    << std::setw(8) << (float)pResult[0].ticks / gRngBufSize
    << std::endl;

  delete [] pResult;
  delete [] hThread;
}


void usage(void) {
  std::cout << "Aufruf: crc [Optionen]" << std::endl
    << std::endl
    << "Optionen:" << std::endl
    << "  -n N" << std::endl
    << "     N MByte große Blocks generieren (Vorgabe: " << DEFAULT_RNGBUF_SIZE << ")" << std::endl
    << std::endl
    << "  (--iterations|-i) N" << std::endl
    << "     Generieren N Mal wiederholen (Vorgabe: " << DEFAULT_ITERATIONS << ")" << std::endl
    << std::endl
    << "  (--core-binding|-b) (linear|evenfirst|oddfirst|none|auto)" << std::endl
    << "     Modus, nach dem Threads Prozessorkerne gebunden werden" << std::endl
    << "     (Vorgabe: auto)" << std::endl
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
  std::cout << "crc - Experimente mit dem Maschinenbefehl CRC." << std::endl
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
      gRngBufSize = atoi(optarg);
      if (gRngBufSize <= 0)
        gRngBufSize = DEFAULT_RNGBUF_SIZE;
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

    std::cout
      << ">>> # Cores          : "
      << std::setw(3) << CPUFeatures::instance().cores << std::endl
      << ">>> # Threads/core   : "
      << std::setw(3) << CPUFeatures::instance().threads_per_package << std::endl
      << std::endl;

#if defined(WIN32)
    int numaNodeCount = 0, processorCoreCount = 0,
      logicalProcessorCount = 0, processorPackageCount = 0;
    CPUFeatures::instance().count(numaNodeCount, processorCoreCount,
				  logicalProcessorCount, processorPackageCount);
    std::cout
      << std::setfill(' ')
      << "GetLogicalProcessorInformation()" << std::endl
      << "================================" << std::endl
      << ">>> numaNodeCount         : "
      << std::setw(3) << numaNodeCount << std::endl
      << ">>> processorCoreCount    : "
      << std::setw(3) << processorCoreCount << std::endl
      << ">>> logicalProcessorCount : "
      << std::setw(3) << logicalProcessorCount << std::endl
      << ">>> processorPackageCount : "
      << std::setw(3) << processorPackageCount << std::endl
      << std::endl;
#endif

    std::cout
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
    // TODO
  }

  if (!CPUFeatures::instance().isCRCSupported()) {
    std::cout
      << "//////////////////////////////////////////////////////" << std::endl
      << "/// Die CPU unterstützt die CRC-Instruktion nicht. ///" << std::endl
      << "/// Die _mm_crc32_uxx-Benchmarks entfallen daher.  ///" << std::endl
      << "//////////////////////////////////////////////////////" << std::endl;
  }

  // Speicherblöcke mit Zufallszahlen belegen
  MersenneTwister gen;
  gen.seed();
  if (gVerbose > 0)
    std::cout << std::endl << "Generieren von " << gMaxNumThreads << "x" << gRngBufSize << " MByte ..." << std::endl;
  gRngBufSize *= 1024*1024;
  try {
    gRngBuf = new uint8_t[gMaxNumThreads * gRngBufSize];
  }
  catch(...) {
    std::cerr << "FEHLER: nicht genug freier Speicher!" << std::endl;
    return EXIT_FAILURE;
  }
  if (gRngBuf == NULL) {
    std::cerr << "FEHLER beim Allozieren des Speichers!" << std::endl;
    return EXIT_FAILURE;
  }
  uint32_t* rn = reinterpret_cast<uint32_t*>(gRngBuf);
  const uint32_t* const rne = rn + gMaxNumThreads * gRngBufSize / sizeof(uint32_t);
  while (rn < rne)
    gen.next(*rn++);

#if defined(WIN32)
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif
  std::cout << "Bilden der Pruefsummen (" << gIterations << "x" << (gRngBufSize/1024/1024) << " MByte) ..." << std::endl;
  for (int i = 0; i <= gThreadIterations && gNumThreads[i] > 0 ; ++i) {
    const int numThreads = gNumThreads[i];
    std::cout << std::endl
      << "... in " << numThreads << " Thread" << (numThreads == 1? "" : "s") << ":" << std::endl
      << std::endl
      << "  Methode             CRC             t/Block      Durchsatz  Zyklen" << std::endl
      << "  ------------------------------------------------------------------" << std::endl;
    if (CPUFeatures::instance().isCRCSupported()) {
      runBenchmark(numThreads, "_mm_crc32_u8", Intrinsic8);
      runBenchmark(numThreads, "_mm_crc32_u16", Intrinsic16);
      runBenchmark(numThreads, "_mm_crc32_u32", Intrinsic32);
#if defined(_M_X64) || defined(__x86_64__)
      runBenchmark(numThreads, "_mm_crc32_u64", Intrinsic64);
#endif
    }
    runBenchmark(numThreads, "boost::crc", Boost);
    runBenchmark(numThreads, "naive", DefaultNaive);
    runBenchmark(numThreads, "optimized", DefaultOptimized);
    runBenchmark(numThreads, "Crc32cSSE4", Crc32cSSE4);
  }

  if (gVerbose > 1) 
    std::cout << std::endl;
  bool correct = true;
  const uint32_t crc = gCrcResults.begin()->crc;
  for (std::vector<CrcResult>::const_iterator i = gCrcResults.begin(); i != gCrcResults.end() && correct; ++i) {
    correct = (i->crc == crc);
    if (gVerbose > 1)
      std::cout << "Pruefen des Ergebnisses von Methode '" << i->method
      << "' in " << i->nThreads << " Threads ..."
      << ((correct)? "OK" : "FEHLER") << std::endl;
  }

  if (gVerbose > 0) {
    std::cout << std::endl;
    if (correct)
      std::cout << "OK." << std::endl;
    else 
      std::cerr << "FEHLER: unterschiedliche CRC-Ergebnisse!" << std::endl;
  }

  delete [] gRngBuf;

  return (correct)? EXIT_SUCCESS : EXIT_FAILURE;
}
