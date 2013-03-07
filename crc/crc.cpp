// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

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
#include "mersenne_twister.h"
#include "stopwatch.h"
#include "sharedutil.h"
#include "crc32.h"

#if defined(__GNUC__)
typedef unsigned int DWORD;
typedef pthread_t HANDLE;
#include <strings.h>
#endif

static const int DEFAULT_ITERATIONS = 16;
static const int DEFAULT_RNGBUF_SIZE = 128;
static const int DEFAULT_NUM_THREADS = 1;
static const int MAX_NUM_THREADS = 256;

int gIterations = DEFAULT_ITERATIONS;
unsigned int* gRngBuf = NULL;
int gRngBufSize = DEFAULT_RNGBUF_SIZE;
int gNumThreads[MAX_NUM_THREADS] = { DEFAULT_NUM_THREADS };
int gMaxNumThreads = 1;
int gThreadIterations = 0;
int gThreadPriority;
bool gBindToCore = true;

struct CrcResult {
  int nThreads;
  std::string method;
  uint32_t crc;
};

std::vector<CrcResult> gCrcResults;

enum _long_options {
	SELECT_HELP = 0x1,
	SELECT_NO_BIND_TO_CORE,
	SELECT_ITERATIONS,
	SELECT_THREADS,
	SELECT_APPEND};
static struct option long_options[] = {
	{ "no-bind-to-core",      no_argument,       0, SELECT_NO_BIND_TO_CORE },
	{ "iterations",           required_argument, 0, SELECT_ITERATIONS },
	{ "threads",              required_argument, 0, SELECT_THREADS },
	{ "help",                 no_argument,       0, SELECT_HELP },
};


enum Method {
	Intrinsic8,
	Intrinsic16,
	Intrinsic32,
#if defined(_M_X64) || defined(__x86_64__)
	Intrinsic64,
#endif
	Boost,
	DefaultFast
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
  unsigned int* rngBuf;
  int rngBufSize;
  DWORD num;
  HANDLE hThread;
  int iterations;
  DWORD numCores;
  bool bindToCore;
  // output fields
  int64_t t;
  int64_t ticks;
  unsigned int crc;
};


// die im Thread laufenden Benchmark-Routine
#if defined(WIN32)
DWORD WINAPI BenchmarkThreadProc(LPVOID lpParameter)
#elif defined(__GNUC__)
void* BenchmarkThreadProc(void* lpParameter)
#endif
{
  BenchmarkResult* result = (BenchmarkResult*)lpParameter;
  if (result->bindToCore) {
#if defined(WIN32)
    DWORD affinityMask = 1 << (result->num % result->numCores);
    SetThreadAffinityMask(GetCurrentThread(), affinityMask);
#elif defined(__GNUC__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(result->num, &cpuset);
    sched_setaffinity(pthread_self(), sizeof(cpu_set_t), &cpuset); 
#endif
  }
#if defined(WIN32)
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#elif defined(__GNUC__)
  // TODO
#endif
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
	    uint32_t* rn = result->rngBuf + result->num * result->rngBufSize / sizeof(uint32_t);
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
	    uint32_t* rn = result->rngBuf + result->num * result->rngBufSize / sizeof(uint32_t);
	    boost::crc_optimal<32U, 0x1edc6f41U, 0U, 0U, true, true> crcBoost;
	    crcBoost.process_bytes(rn, result->rngBufSize);
	    crc = crcBoost.checksum();
	    break;
	  }
	case DefaultFast:
	  {
	    uint8_t* rn = (uint8_t*)result->rngBuf + result->num * result->rngBufSize / sizeof(uint8_t);
	    CRC32_SSE42 crc32_sse42;
	    crc = crc32_sse42.process(rn, result->rngBufSize);
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


void runBenchmark(const int numThreads, const char* strMethod, const Method method) {
  // Threads zum Leben erwecken
  HANDLE* hThread = new HANDLE[numThreads];
  BenchmarkResult* pResult = new BenchmarkResult[numThreads];
  const DWORD numCores = getNumCores();
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
    pResult[i].bindToCore = gBindToCore;
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
	    << (float)gRngBufSize*gIterations/1024/1024/(1e-3*t)*numThreads << " MB/s";
  
  std::cout << std::endl;
  
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
	    << "  --iterations N" << std::endl
	    << "  -i N" << std::endl
	    << "     Generieren N Mal wiederholen (Vorgabe: " << DEFAULT_ITERATIONS << ")" << std::endl
	    << std::endl
	    << "  --no-bind-to-core" << std::endl
	    << "  -0" << std::endl
	    << "     Threads nicht Prozessorkerne binden" << std::endl
	    << std::endl
	    << "  --quiet" << std::endl
	    << "  -q" << std::endl
	    << "     Keine Informationen ausgeben" << std::endl
	    << std::endl
	    << "  --threads N" << std::endl
	    << "  -t N" << std::endl
	    << "     Zufallszahlen in N Threads parallel generieren (Vorgabe: " << DEFAULT_NUM_THREADS << ")" << std::endl
	    << "     Mehrfachnennungen möglich." << std::endl
	    << std::endl
	    << "  --help" << std::endl
	    << "  -h" << std::endl
	    << "  -?" << std::endl
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
    int c = getopt_long(argc, argv, "vh0?n:t:i:", long_options, &option_index);
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
      case '0':
	// fall-through
      case SELECT_NO_BIND_TO_CORE:
	gBindToCore = false;
	break;
      default:
	usage();
	return EXIT_FAILURE;
      }
  }
  
  evaluateCPUFeatures();
  
  if (!isCRCSupported()) {
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
    gRngBuf = new uint32_t[gMaxNumThreads * gRngBufSize / sizeof(uint32_t)];
  }
  catch(...)
    {
      std::cerr << "FEHLER: nicht genug freier Speicher!" << std::endl;
      return EXIT_FAILURE;
    }
  if (gRngBuf == NULL) {
    std::cerr << "FEHLER beim Allozieren des Speichers!" << std::endl;
    return EXIT_FAILURE;
  }
  uint32_t* rn = gRngBuf;
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
	      << "  Methode             CRC             t/Block      Durchsatz" << std::endl
	      << "  ----------------------------------------------------------" << std::endl;
    if (isCRCSupported()) {
      runBenchmark(numThreads, "_mm_crc32_u8", Intrinsic8);
      runBenchmark(numThreads, "_mm_crc32_u16", Intrinsic16);
      runBenchmark(numThreads, "_mm_crc32_u32", Intrinsic32);
#if defined(_M_X64) || defined(__x86_64__)
      runBenchmark(numThreads, "_mm_crc32_u64", Intrinsic64);
#endif
    }
    runBenchmark(numThreads, "boost::crc",    Boost);
    runBenchmark(numThreads, "default",       DefaultFast);
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
