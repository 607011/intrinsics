// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#define _CRT_RAND_S

#include <Windows.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <getopt.h>

#include "stopwatch.h"
#include "abstract_random_number_generator.h"
#include "mersenne_twister.h"
#include "marsaglia.h"
#include "mcg.h"
#include "circ.h"
#include "rdrand.h"
#include "util.h"

static const int DEFAULT_ITERATIONS = 10;
static const int DEFAULT_CHUNK_SIZE = 32*1024*1024;
static const int DEFAULT_NUM_THREADS = 1;

int iterations = DEFAULT_ITERATIONS;
int chunkSize = DEFAULT_CHUNK_SIZE;

MersenneTwister mt;
MCG mcg;
MultiplyWithCarry mwc;
DummyGenerator dummy;
RdRand16 rdrand16;
RdRand32 rdrand32;
#if defined(_M_X64)
RdRand64 rdrand64;
#endif

int numThreads = DEFAULT_NUM_THREADS;
bool doAppend = false;
bool doWrite = true;
int verbose = 0;
int threadPriority = THREAD_PRIORITY_NORMAL;


enum _long_options {
    SELECT_HELP = 0x1,
    SELECT_NO_WRITE,
    SELECT_ITERATIONS,
    SELECT_THREADS,
    SELECT_APPEND};
static struct option long_options[] = {
    { "append",               no_argument,       0, SELECT_APPEND },
    { "no-write",             no_argument,       0, SELECT_NO_WRITE },
    { "iterations",           required_argument, 0, SELECT_ITERATIONS },
    { "threads",              required_argument, 0, SELECT_THREADS },
    { "help",                 no_argument,       0, SELECT_HELP },
};

static const char* B[2] = { "false", "true" };


template <typename T>
struct BenchmarkResult {
	__int64 t;
	__int64 ticks;
	int elementSize;
	AbstractRandomNumberGenerator<T>* gen;
	LPVOID rngBuf;
	int num;
	HANDLE hThread;
};


template <class GEN>
DWORD WINAPI BenchmarkThreadProc(LPVOID lpParameter)
{
	GEN gen;
	gen.seed(GEN::makeSeed());
	BenchmarkResult<GEN::result_t>* result = (BenchmarkResult<GEN::result_t>*)lpParameter;
	assert(GetCurrentThread() == result->hThread);
	GEN::result_t* rngBuf = new GEN::result_t[chunkSize];
	result->rngBuf = (LPVOID)rngBuf;
	result->elementSize = GEN::result_size();
	__int64 tMin = MAXLONGLONG;
	__int64 ticksMin = MAXLONGLONG;
	for (int i = 0; i < iterations; ++i) {
		__int64 t, ticks;
		{
			Stopwatch stopwatch(t, ticks);
			GEN::result_t* rn = (GEN::result_t*)rngBuf;
			const GEN::result_t* rne = rn + chunkSize / result->elementSize;
			while (rn < rne)
				gen.next(*rn++);
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


template <class GEN>
void runBenchmark(const char* outputFilename) {
    std::ofstream fs;
    if (outputFilename != NULL && doWrite) {
        fs.open(outputFilename, doAppend
			? (std::ios::binary | std::ios::out | std::ios::app | std::ios::ate)
			: (std::ios::binary | std::ios::out));
        if (!fs.is_open() || fs.fail()) {
            std::cerr << "FEHLER: Öffnen von " << outputFilename << " fehlgeschlagen." << std::endl;
            exit(EXIT_FAILURE);
        }
    }

	HANDLE* hThread = new HANDLE[numThreads];
	BenchmarkResult<GEN::result_t>* pResult = new BenchmarkResult<GEN::result_t>[numThreads];
	for (int i = 0; i < numThreads; ++i) {
		pResult[i].t = 0;
		pResult[i].ticks = 0;
		pResult[i].rngBuf = NULL;
		pResult[i].num = i;
		pResult[i].hThread = hThread[i] = CreateThread(NULL, 0, BenchmarkThreadProc<GEN>, (LPVOID)&pResult[i], CREATE_SUSPENDED, NULL);
	}

	std::cout.setf(std::ios_base::left, std::ios_base::adjustfield);
	std::cout << std::setfill(' ') << std::setw(18) << GEN::name() << " ";

	__int64 t = MAXLONGLONG;
	__int64 ticks = MAXLONGLONG;
	{
		for (int i = 0; i < numThreads; ++i)
			ResumeThread(hThread[i]);
		Stopwatch stopwatch(t, ticks);
		WaitForMultipleObjects(numThreads, hThread, TRUE, INFINITE);
	}

	if (fs.is_open() && pResult[0].rngBuf != NULL) {
		std::cout << "writing ..." << std::flush << "\b\b\b\b\b\b\b\b\b\b\b";
		fs.write((char*)pResult[0].rngBuf, chunkSize);
		fs.close();
	}

	__int64 tMin = 0;
	for (int i = 0; i < numThreads; ++i) {
		tMin += pResult[i].t;
		delete [] pResult[i].rngBuf;
		CloseHandle(hThread[i]);
	}
	tMin /= numThreads;

	std::cout.setf(std::ios_base::right, std::ios_base::adjustfield);
	std::cout << std::setfill(' ') << std::setw(5) << tMin << " ms, " 
		<< std::fixed << std::setw(8) << std::setprecision(2) << (float)chunkSize/1024/1024/(1e-3*t)*numThreads << " Mbyte/s"
		<< std::endl;

	delete [] pResult;
	delete [] hThread;
}


void usage(void) {
    std::cout << "Aufruf: intrinsics [Optionen]" << std::endl
        << std::endl
        << "Optionen:" << std::endl
		<< "  --iterations N" << std::endl
        << "  -i N" << std::endl
		<< "     N Durchläufe pro Benchmark (Vorgabe: " << DEFAULT_ITERATIONS << ")" << std::endl
        << std::endl
        << "  -n N" << std::endl
		<< "     N KByte Zufallsbytes pro Durchlauf generieren (Vorgabe: " << DEFAULT_CHUNK_SIZE << " Byte)" << std::endl
        << std::endl
        << "  --append" << std::endl
        << "     Zufallszahlen an bestehende Dateien anhängen" << std::endl
        << std::endl
        << "  --no-write" << std::endl
        << "     Zufallszahlen nur erzeugen, nicht in Datei schreiben" << std::endl
        << std::endl
        << "  --quiet" << std::endl
        << "  -q" << std::endl
        << "     Keine Informationen ausgeben" << std::endl
        << std::endl
        << "  --threads N" << std::endl
        << "  -t N" << std::endl
        << "     Zufallszahlen in N Threads parallel generieren (Vorgabe: " << DEFAULT_NUM_THREADS << ")" << std::endl
        << std::endl
        << "  --help" << std::endl
        << "  -h" << std::endl
        << "  -?" << std::endl
        << "     Diese Hilfe anzeigen" << std::endl
        << std::endl;
}


void disclaimer(void) {
    std::cout << "intrinsics - Experimente mit (Pseudo-)Zufallszahlengeneratoren." << std::endl
        << "Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag" << std::endl
        << "Alle Rechte vorbehalten." << std::endl
        << std::endl
        << "Diese Software wurde zu Lehr- und Demonstrationszwecken erstellt." << std::endl
        << "Alle Ausgaben ohne Gewähr." << std::endl
        << std::endl;
}


int main(int argc, char* argv[]) {
	threadPriority = GetThreadPriority(GetCurrentThread());

	for (;;) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "vh?n:s:t:", long_options, &option_index);
        if (c == -1)
            break;
        switch (c)
        {
		case SELECT_THREADS:
			// fall-through
		case 't':
            if (optarg == NULL) {
                usage();
                exit(EXIT_FAILURE);
            }
            numThreads = atoi(optarg);
            if (numThreads <= 0)
                numThreads = 1;
            break;
        case SELECT_APPEND:
            doAppend = true;
            break;
		case 'v':
			++verbose;
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
			doWrite = false;
			break;
        case 's':
            if (optarg == NULL) {
                usage();
                exit(EXIT_FAILURE);
            }
            chunkSize = atoi(optarg);
            if (chunkSize <= 0)
                chunkSize = DEFAULT_CHUNK_SIZE;
            break;
		case SELECT_ITERATIONS:
			// fall-through
        case 'i':
            if (optarg == NULL) {
                usage();
                exit(EXIT_FAILURE);
            }
            iterations = atoi(optarg);
            if (iterations <= 0)
                iterations = DEFAULT_ITERATIONS;
            break;
        default:
			usage();
			return EXIT_FAILURE;
		}
	}

	int cpureg[4] = { 0x0, 0x0, 0x0, 0x0 };
	__cpuid(cpureg, 1);
	bool sse3_supported =   (cpureg[2] & (1<<0))  != 0;
	bool sse41_supported =  (cpureg[2] & (1<<19)) != 0;
	bool sse42_supported =  (cpureg[2] & (1<<20)) != 0;
	bool popcnt_supported = (cpureg[2] & (1<<23)) != 0;
	bool aes_supported =    (cpureg[2] & (1<<25)) != 0;
	bool avx_supported =    (cpureg[2] & (1<<28)) != 0;
	bool b29_supported =    (cpureg[2] & (1<<29)) != 0;
	bool rdrand_supported = (cpureg[2] & (1<<30)) != 0;
	bool b31_supported =    (cpureg[2] & (1<<31)) != 0;
	if (verbose > 0) {
		std::cout << ">>> SSE3   : " << B[sse3_supported] << std::endl;
		std::cout << ">>> SSE4.1 : " << B[sse41_supported] << std::endl;
		std::cout << ">>> SSE4.2 : " << B[sse42_supported] << std::endl;
		std::cout << ">>> POPCNT : " << B[popcnt_supported] << std::endl;
		std::cout << ">>> AVX    : " << B[avx_supported] << std::endl;
		std::cout << ">>> AES    : " << B[aes_supported] << std::endl;
		std::cout << ">>> 1<<29  : " << B[b29_supported] << std::endl;
		std::cout << ">>> RDRAND : " << B[rdrand_supported] << std::endl;
		std::cout << ">>> 1<<31  : " << B[b31_supported] << std::endl;
	}

	if (verbose > 0)
		std::cout << "Ausführung in " << numThreads << " Threads ..." << std::endl;

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	// warmup to turbo mode
	runBenchmark<DummyGenerator>("null.dat");
	runBenchmark<CircularBytes>("circular.dat");
	runBenchmark<MultiplyWithCarry>("mwc.dat");
	runBenchmark<MCG>("mcg.dat");
	runBenchmark<MersenneTwister>("mt.dat");

	if (isGenuineIntelCPU() && rdrand_supported) {
		runBenchmark<RdRand16>("rdrand16.dat");
		runBenchmark<RdRand32>("rdrand32.dat");
#if defined(_M_X64)
		runBenchmark<RdRand64>("rdrand64.dat");
#endif
	}

	SetThreadPriority(GetCurrentThread(), threadPriority);
	return EXIT_SUCCESS;
}
