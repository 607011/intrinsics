// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#if defined(WIN32)
#include <Windows.h>
#endif

#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits.h>
#include <getopt.h>

#include "mersenne_twister.h"
#include "marsaglia.h"
#include "mcg.h"
#include "circ.h"

#include "rdrand.h"
#include "sharedutil.h"
#include "stopwatch.h"

#if defined(__GNUC__)
#include <strings.h>
#include <pthread.h>
typedef pthread_t HANDLE;
typedef unsigned int DWORD;
typedef void* LPVOID;
#endif

static const int DEFAULT_ITERATIONS = 12;
static const int DEFAULT_RNGBUF_SIZE = 256;
static const int DEFAULT_NUM_THREADS = 1;
static const int MAX_NUM_THREADS = 256;

int gIterations = DEFAULT_ITERATIONS;
int gRngBufSize = DEFAULT_RNGBUF_SIZE;
int gNumThreads[MAX_NUM_THREADS] = { DEFAULT_NUM_THREADS };
int gThreadIterations = 0;
int gThreadPriority;
bool gBindToCore = true;
bool gDoAppend = false;
bool gDoWrite = true;
bool gDoWriteToMemory = true;

enum _long_options {
  SELECT_HELP = 0x1,
  SELECT_NO_WRITE,
  SELECT_NO_WRITE_TO_MEM,
  SELECT_NO_BIND_TO_CORE,
  SELECT_ITERATIONS,
  SELECT_THREADS,
  SELECT_APPEND
};
static struct option long_options[] = {
  { "append",               no_argument,       0, SELECT_APPEND },
  { "no-write",             no_argument,       0, SELECT_NO_WRITE },
  { "no-write-to-memory",   no_argument,       0, SELECT_NO_WRITE_TO_MEM },
  { "no-bind-to-core",      no_argument,       0, SELECT_NO_BIND_TO_CORE },
  { "iterations",           required_argument, 0, SELECT_ITERATIONS },
  { "threads",              required_argument, 0, SELECT_THREADS },
  { "help",                 no_argument,       0, SELECT_HELP },
};


struct BenchmarkResult {
  BenchmarkResult()
    : rngBuf(NULL)
    , hThread(0)
    , t(0)
    , ticks(0)
  { /* ... */ }
  ~BenchmarkResult() {
#if defined(WIN32)
    if (hThread)
      CloseHandle(hThread);
#endif
    if (rngBuf != NULL)
      delete [] rngBuf;
  }
  // input fields
  bool writeToMemory;
  uint8_t* rngBuf;
  int rngBufSize;
  int num;
  HANDLE hThread;
  int iterations;
  DWORD numCores;
  // output fields
  int64_t t;
  int64_t ticks;
  uint64_t exceeded;
  uint64_t invalid;
};


// die im Thread laufenden Benchmark-Routine
template <class GEN>
#if defined(WIN32)
DWORD WINAPI 
#elif defined(__GNUC__)
void*
#endif
  BenchmarkThreadProc(LPVOID lpParameter)
{
  BenchmarkResult* result = (BenchmarkResult*)lpParameter;
  if (gBindToCore) {
#if defined(WIN32)
    SetThreadAffinityMask(GetCurrentThread(), 1<<(result->num % result->numCores));
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#elif defined(__GNUC__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(result->num, &cpuset);
    sched_setaffinity(pthread_self(), sizeof(cpu_set_t), &cpuset);
    // TODO: set priority
#endif
  }
  GEN gen;
  gen.seed();
  int64_t tMin = LLONG_MAX;
  int64_t ticksMin = LLONG_MAX;
  // result->iterations Zufallszahlenblöcke generieren
  for (int i = 0; i < result->iterations; ++i) {
    int64_t t = 0, ticks = 0;
    if (result->writeToMemory) { // einen Block Zufallszahlen generieren
      Stopwatch stopwatch(t, ticks);
      typename GEN::result_t* rn = (typename GEN::result_t*)result->rngBuf;
      const typename GEN::result_t* rne = rn + result->rngBufSize / GEN::result_size();
      while (rn < rne)
        gen.next(*rn++);
    }
    else {
      Stopwatch stopwatch(t, ticks);
      int i = result->rngBufSize / GEN::result_size();
      while (--i)
        gen();
    }
    if (t < tMin)
      tMin = t;
    if (ticks < ticksMin)
      ticksMin = ticks;
    result->invalid = gen.invalid();
    result->exceeded = gen.limitExceeded();
  }
  result->t = tMin;
  result->ticks = ticksMin;
  return EXIT_SUCCESS;
}


template <class GEN>
void runBenchmark(const char* outputFilename, const int numThreads) {
  std::ofstream fs;
  if (outputFilename != NULL && gDoWrite) {
    fs.open(outputFilename, gDoAppend
      ? (std::ios::binary | std::ios::out | std::ios::app | std::ios::ate)
      : (std::ios::binary | std::ios::out));
    if (!fs.is_open() || fs.fail()) {
      std::cerr << "FEHLER: Öffnen von " << outputFilename << " fehlgeschlagen." << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  int64_t t = LLONG_MAX;
  int64_t ticks = LLONG_MAX;
#if defined(__GNUC__)
  Stopwatch stopwatch(t, ticks);
#endif

  // Threads zum Leben erwecken
  HANDLE* hThread = new HANDLE[numThreads];
  BenchmarkResult* pResult = new BenchmarkResult[numThreads];
  const DWORD numCores = getNumCores();
  for (int i = 0; i < numThreads; ++i) {
    pResult[i].num = i;
    pResult[i].rngBufSize = gRngBufSize;
    pResult[i].iterations = gIterations;
    pResult[i].numCores = numCores;
    pResult[i].rngBuf = NULL;
    pResult[i].writeToMemory = gDoWriteToMemory;
    try {
      if (gDoWriteToMemory)
        pResult[i].rngBuf = new uint8_t[gRngBufSize];
    }
    catch (...) {
      std::cerr << "FEHLER: nicht genug freier Speicher!" << std::endl;
      exit(EXIT_FAILURE);
    }
    if (pResult[i].rngBuf == NULL && gDoWriteToMemory) {
      std::cerr << "FEHLER beim Allozieren des Speichers!" << std::endl;
      exit(EXIT_FAILURE);
    } 

#if defined(WIN32)
    pResult[i].hThread = CreateThread(NULL, 0,
      BenchmarkThreadProc<GEN>,
      (LPVOID)&pResult[i],
      CREATE_SUSPENDED, NULL);
#elif defined(__GNUC__)
    pthread_create(&pResult[i].hThread, NULL,
      BenchmarkThreadProc<GEN>,
      (void*)&pResult[i]);
#endif

    hThread[i] = pResult[i].hThread; // hThread[] wird von WaitForMultipleObjects() benötigt
  }

  std::cout.setf(std::ios_base::left, std::ios_base::adjustfield);
  std::cout << "  " << std::setfill(' ') << std::setw(18) << GEN::name() << " ";
  // Threads starten, auf Ende warten, Zeit stoppen

  std::cout << "Generieren ...";

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
  std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b";

  // ggf. Zufallszahlenbuffer in Datei schreiben
  if (fs.is_open()) {
    for (int i = 0; i < numThreads; ++i) {
      std::cout << "Schreiben ..." << std::flush;
      if (pResult[i].rngBuf != NULL)
        fs.write((const char*)pResult[i].rngBuf, gRngBufSize);
      std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b";
    }
    fs.close();
  }

  // Ergebnisse sammeln
  int64_t tMin = 0;
  int64_t invalidSum = 0;
  int64_t exceededSum = 0;
  for (int i = 0; i < numThreads; ++i) {
    tMin += pResult[i].t;
    invalidSum += pResult[i].invalid;
    exceededSum += pResult[i].exceeded;
  }
  tMin /= numThreads;

  std::cout.setf(std::ios_base::right, std::ios_base::adjustfield);
  std::cout << std::setfill(' ') << std::setw(5) << tMin << " ms, " 
    << std::fixed << std::setw(8) << std::setprecision(2) << (float)gRngBufSize*gIterations/1024/1024/(1e-3*t)*numThreads << " MByte/s";

  if (invalidSum > 0)
    std::cout << "(invalid: " << invalidSum << ", exceeded: " << exceededSum << ")";
  std::cout << std::endl;

  delete [] pResult;
  delete [] hThread;
}


void usage(void) {
  std::cout << "Aufruf: rdrand.exe [Optionen]" << std::endl
    << std::endl
    << "Optionen:" << std::endl
    << "  -n N" << std::endl
    << "     N MByte Zufallsbytes generieren (Vorgabe: " << DEFAULT_RNGBUF_SIZE << ")" << std::endl
    << std::endl
    << "  --iterations N" << std::endl
    << "  -i N" << std::endl
    << "     Generieren N Mal wiederholen (Vorgabe: " << DEFAULT_ITERATIONS << ")" << std::endl
    << std::endl
    << "  --append" << std::endl
    << "     Zufallszahlen an bestehende Dateien anhängen" << std::endl
    << std::endl
    << "  --no-write" << std::endl
    << "     Zufallszahlen nur erzeugen, nicht in Datei schreiben" << std::endl
    << std::endl
    << "  --no-write-to-memory" << std::endl
    << "     Zufallszahlen nur erzeugen, nicht in Speicher ablegen" << std::endl
    << "     (impliziert --no-write)" << std::endl
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
  std::cout << "rdrand - Experimente mit (Pseudo-)Zufallszahlengeneratoren." << std::endl
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
#endif
  for (;;) {
    int option_index = 0;
    int c = getopt_long(argc, argv, "vh?n:t:i:", long_options, &option_index);
    if (c == -1)
      break;
    switch (c)
    {
    case SELECT_ITERATIONS:
      // fall-through
    case 'i':
      if (optarg == NULL) {
        usage();
        exit(EXIT_FAILURE);
      }
      gIterations = atoi(optarg);
      if (gIterations <= 0)
        gIterations = DEFAULT_ITERATIONS;
      break;
    case 'n':
      if (optarg == NULL) {
        usage();
        exit(EXIT_FAILURE);
      }
      gRngBufSize = atoi(optarg);
      if (gRngBufSize <= 0)
        gRngBufSize = DEFAULT_RNGBUF_SIZE;
      break;
    case SELECT_NO_WRITE_TO_MEM:
      gDoWriteToMemory = false;
      break;
    case SELECT_THREADS:
      // fall-through
    case 't':
      if (optarg == NULL) {
        usage();
        exit(EXIT_FAILURE);
      }
      if (gThreadIterations < MAX_NUM_THREADS) {
        int numThreads = atoi(optarg);
        if (numThreads <= 0)
          numThreads = 1;
        gNumThreads[gThreadIterations++] = numThreads;
      }
      break;
    case SELECT_APPEND:
      gDoAppend = true;
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
    case SELECT_NO_WRITE:
      gDoWrite = false;
      break;
    case SELECT_NO_BIND_TO_CORE:
      gBindToCore = false;
      break;
    default:
      usage();
      return EXIT_FAILURE;
    }
  }

  if (!gDoWriteToMemory)
    gDoWrite = false;

  evaluateCPUFeatures();

  if (!isRdRandSupported()) {
    std::cout
      << "//////////////////////////////////////////////////////////" << std::endl
      << "/// Die CPU unterstuetzt die RDRAND-Instruktion nicht. ///" << std::endl
      << "/// Die _rdrandxx_step-Benchmarks entfallen daher.     ///" << std::endl
      << "//////////////////////////////////////////////////////////" << std::endl;
  }

  if (gVerbose > 0)
    std::cout << std::endl << "Generieren von " << gIterations << "x" << gRngBufSize << " MByte ..." << std::endl;
  gRngBufSize *= 1024*1024;

#if defined(WIN32)
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif

  for (int i = 0; i <= gThreadIterations && gNumThreads[i] > 0 ; ++i) {
    const int numThreads = gNumThreads[i];
    if (gVerbose > 0)
      std::cout << std::endl << "... in " << numThreads << " Thread" << (numThreads == 1? "" : "s") << ":" << std::endl;

    runBenchmark<DummyByteGenerator>(NULL, numThreads);
    // runBenchmark<DummyUIntGenerator>(NULL, numThreads);

    // software PRNG benchmarks
    // runBenchmark<CircularBytes>("circular.dat", numThreads);
    runBenchmark<MultiplyWithCarry>("mwc.dat", numThreads);
    runBenchmark<MCG>("mcg.dat", numThreads);
    runBenchmark<MersenneTwister>("mt.dat", numThreads);

    // Ivy Bridge RNG benchmarks
    if (isRdRandSupported()) {
      runBenchmark<RdRand16>("rdrand16.dat", numThreads);
      runBenchmark<RdRand32>("rdrand32.dat", numThreads);
#if defined(_M_X64) || defined(__x86_64__)
      runBenchmark<RdRand64>("rdrand64.dat", numThreads);
#endif
    }

  }

  return EXIT_SUCCESS;
}
