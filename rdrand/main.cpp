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

static const int DEFAULT_ITERATIONS = 12;
static const int DEFAULT_RNGBUF_SIZE = 256;
static const int DEFAULT_NUM_THREADS = 1;

int gIterations = DEFAULT_ITERATIONS;
int gRngBufSize = DEFAULT_RNGBUF_SIZE;
int gNumThreads = DEFAULT_NUM_THREADS;
int gVerbose = 0;
int gThreadPriority = THREAD_PRIORITY_NORMAL;
bool gDoAppend = false;
bool gDoWrite = true;


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


template <typename T>
struct BenchmarkResult {
	__int64 t;
	__int64 ticks;
	AbstractRandomNumberGenerator<T>* gen;
	LPVOID rngBuf;
	int num;
	HANDLE hThread;
	int rngBufSize;
	int iterations;
};


template <class GEN>
DWORD WINAPI BenchmarkThreadProc(LPVOID lpParameter)
{
	BenchmarkResult<GEN::result_t>* result = (BenchmarkResult<GEN::result_t>*)lpParameter;
	assert(GetCurrentThread() == result->hThread);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	GEN gen;
	gen.seed(GEN::makeSeed());
	GEN::result_t* rngBuf = new GEN::result_t[result->rngBufSize / GEN::result_size()];
	result->rngBuf = (LPVOID)rngBuf;
	__int64 tMin = MAXLONGLONG;
	__int64 ticksMin = MAXLONGLONG;
	for (int i = 0; i < result->iterations; ++i) {
		{
			Stopwatch stopwatch(result->t, result->ticks);
			GEN::result_t* rn = (GEN::result_t*)rngBuf;
			const GEN::result_t* rne = rn + result->rngBufSize / GEN::result_size();
			while (rn < rne)
				gen.next(*rn++);
		}
		if (result->t < tMin)
			tMin = result->t;
		if (result->ticks < ticksMin)
			ticksMin = result->ticks;
	}
	result->t = tMin;
	result->ticks = ticksMin;
	return EXIT_SUCCESS;
}


template <class GEN>
void runBenchmark(const char* outputFilename) {
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

	HANDLE* hThread = new HANDLE[gNumThreads];
	BenchmarkResult<GEN::result_t>* pResult = new BenchmarkResult<GEN::result_t>[gNumThreads];
	for (int i = 0; i < gNumThreads; ++i) {
		pResult[i].t = 0;
		pResult[i].ticks = 0;
		pResult[i].rngBuf = NULL;
		pResult[i].num = i;
		pResult[i].rngBufSize = gRngBufSize;
		pResult[i].iterations = gIterations;
		pResult[i].hThread = hThread[i] = CreateThread(NULL, 0, BenchmarkThreadProc<GEN>, (LPVOID)&pResult[i], CREATE_SUSPENDED, NULL);
	}

	std::cout.setf(std::ios_base::left, std::ios_base::adjustfield);
	std::cout << std::setfill(' ') << std::setw(18) << GEN::name() << ' ';

	__int64 t = MAXLONGLONG;
	__int64 ticks = MAXLONGLONG;
	{
		for (int i = 0; i < gNumThreads; ++i)
			ResumeThread(hThread[i]);
		Stopwatch stopwatch(t, ticks);
		WaitForMultipleObjects(gNumThreads, hThread, TRUE, INFINITE);
	}

	if (fs.is_open() && pResult[0].rngBuf != NULL) {
		std::cout << "writing ..." << std::flush << "\b\b\b\b\b\b\b\b\b\b\b";
		fs.write((char*)pResult[0].rngBuf, gRngBufSize);
		fs.close();
	}

	__int64 tMin = 0;
	for (int i = 0; i < gNumThreads; ++i) {
		tMin += pResult[i].t;
		delete [] pResult[i].rngBuf;
		CloseHandle(hThread[i]);
	}
	tMin /= gNumThreads;

	std::cout.setf(std::ios_base::right, std::ios_base::adjustfield);
	std::cout << std::setfill(' ') << std::setw(5) << tMin << " ms, " 
		<< std::fixed << std::setw(8) << std::setprecision(2) << (float)gRngBufSize*gIterations/1024/1024/(1e-3*t)*gNumThreads << " Mbyte/s"
		<< std::endl;

	delete [] pResult;
	delete [] hThread;
}


void usage(void) {
	std::cout << "Aufruf: intrinsics [Optionen]" << std::endl
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
	gThreadPriority = GetThreadPriority(GetCurrentThread());

	for (;;) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "vh?n:t:i:", long_options, &option_index);
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
			gNumThreads = atoi(optarg);
			if (gNumThreads <= 0)
				gNumThreads = 1;
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
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	evaluateCPUFeatures();
	if (gVerbose > 0)
		std::cout << "Generieren von " << gIterations << "x" << gRngBufSize << " MByte in " << gNumThreads << " Threads ..." << std::endl;
	gRngBufSize *= 1024*1024;

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	// warmup to turbo mode
	runBenchmark<DummyByteGenerator>("null.dat");
	runBenchmark<DummyIntGenerator>("null.dat");

	// run software PRNG benchmarks
	runBenchmark<CircularBytes>("circular.dat");
	runBenchmark<MultiplyWithCarry>("mwc.dat");
	runBenchmark<MCG>("mcg.dat");
	runBenchmark<MersenneTwister>("mt.dat");

	// run Ivy Bridge RNG benchmarks
	if (isRdRandSupported()) {
		runBenchmark<RdRand16>("rdrand16.dat");
		runBenchmark<RdRand32>("rdrand32.dat");
#if defined(_M_X64)
		runBenchmark<RdRand64>("rdrand64.dat");
#endif
	}

	SetThreadPriority(GetCurrentThread(), gThreadPriority);
	return EXIT_SUCCESS;
}
