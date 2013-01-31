// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#define _CRT_RAND_S

#include <Windows.h>
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
static const int MAX_NUM_THREADS = 256;

int gIterations = DEFAULT_ITERATIONS;
int gRngBufSize = DEFAULT_RNGBUF_SIZE;
int gNumThreads[MAX_NUM_THREADS] = { DEFAULT_NUM_THREADS };
int gThreadIterations = 0;
int gVerbose = 0;
int gThreadPriority;
bool gBindToCore = true;
bool gDoAppend = false;
bool gDoWrite = true;


enum _long_options {
	SELECT_HELP = 0x1,
	SELECT_NO_WRITE,
	SELECT_NO_BIND_TO_CORE,
	SELECT_ITERATIONS,
	SELECT_THREADS,
	SELECT_APPEND};
static struct option long_options[] = {
	{ "append",               no_argument,       0, SELECT_APPEND },
	{ "no-write",             no_argument,       0, SELECT_NO_WRITE },
	{ "no-bind-to-core",      no_argument,       0, SELECT_NO_BIND_TO_CORE },
	{ "iterations",           required_argument, 0, SELECT_ITERATIONS },
	{ "threads",              required_argument, 0, SELECT_THREADS },
	{ "help",                 no_argument,       0, SELECT_HELP },
};


struct BenchmarkResult {
	BenchmarkResult()
		: t(0)
		, ticks(0)
		, rngBuf(NULL)
		, hThread(0)
	{ /* ... */ }
	~BenchmarkResult() {
		if (hThread)
			CloseHandle(hThread);
		if (rngBuf)
			delete [] rngBuf;
	}
	__int64 t;
	__int64 ticks;
	LPVOID rngBuf;
	int num;
	HANDLE hThread;
	int rngBufSize;
	int iterations;
	DWORD numCores;
};


// die im Thread laufenden Benchmark-Routine
template <class GEN>
DWORD WINAPI BenchmarkThreadProc(LPVOID lpParameter)
{
	BenchmarkResult* result = (BenchmarkResult*)lpParameter;
	if (gBindToCore)
		SetThreadAffinityMask(GetCurrentThread(), 1<<(result->num % result->numCores));
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	GEN gen;
	gen.seed(GEN::makeSeed());
	__int64 tMin = MAXLONGLONG;
	__int64 ticksMin = MAXLONGLONG;
	// result->iterations Zufallszahlenblöcke generieren
	for (int i = 0; i < result->iterations; ++i) {
		__int64 t, ticks;
		{ // einen Block Zufallszahlen generieren
			Stopwatch stopwatch(t, ticks);
			GEN::result_t* rn = (GEN::result_t*)result->rngBuf;
			const GEN::result_t* rne = rn + result->rngBufSize / GEN::result_size();
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
void runBenchmark(const char* outputFilename, int numThreads) {
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

	// Threads zum Leben erwecken
	HANDLE* hThread = new HANDLE[numThreads];
	BenchmarkResult* pResult = new BenchmarkResult[numThreads];
	const DWORD numCores = getNumCores();
	for (int i = 0; i < numThreads; ++i) {
		pResult[i].num = i;
		pResult[i].rngBufSize = gRngBufSize;
		pResult[i].iterations = gIterations;
		pResult[i].numCores = numCores;
		pResult[i].rngBuf = new GEN::result_t[gRngBufSize / GEN::result_size()];
		pResult[i].hThread = CreateThread(NULL, 0, BenchmarkThreadProc<GEN>, (LPVOID)&pResult[i], CREATE_SUSPENDED, NULL);
		hThread[i] = pResult[i].hThread; // hThread[] wird von WaitForMultipleObjects() benötigt
	}

	std::cout.setf(std::ios_base::left, std::ios_base::adjustfield);
	std::cout << "  " << std::setfill(' ') << std::setw(18) << GEN::name() << ' ';

	// Threads starten, auf Ende warten, Zeit stoppen
	__int64 t = MAXLONGLONG;
	__int64 ticks = MAXLONGLONG;
	{
		Stopwatch stopwatch(t, ticks);
		for (int i = 0; i < numThreads; ++i)
			ResumeThread(hThread[i]);
		WaitForMultipleObjects(numThreads, hThread, TRUE, INFINITE);
	}

	// ggf. Zufallszahlenbuffer in Datei schreiben
	if (fs.is_open()) {
		for (int i = 0; i < numThreads; ++i) {
			if (pResult[i].rngBuf != NULL) {
				std::cout << "writing ..." << std::flush << "\b\b\b\b\b\b\b\b\b\b\b";
				fs.write((const char*)pResult[i].rngBuf, gRngBufSize);
			}
		}
		fs.close();
	}

	__int64 tMin = 0;
	for (int i = 0; i < numThreads; ++i)
		tMin += pResult[i].t;
	tMin /= numThreads;

	std::cout.setf(std::ios_base::right, std::ios_base::adjustfield);
	std::cout << std::setfill(' ') << std::setw(5) << tMin << " ms, " 
		<< std::fixed << std::setw(8) << std::setprecision(2) << (float)gRngBufSize*gIterations/1024/1024/(1e-3*t)*numThreads << " Mbyte/s"
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
		<< "     Mehrfachnennungen möglich." << std::endl
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
	SecureZeroMemory(gNumThreads+1, sizeof(gNumThreads[0]) * (MAX_NUM_THREADS-1));
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

	evaluateCPUFeatures();

	if (gVerbose > 0)
		std::cout << std::endl << "Generieren von " << gIterations << "x" << gRngBufSize << " MByte ..." << std::endl;
	gRngBufSize *= 1024*1024;

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	for (int i = 0; i <= gThreadIterations && gNumThreads[i] > 0 ; ++i) {
		const int numThreads = gNumThreads[i];
		if (gVerbose > 0)
			std::cout << std::endl << "... in " << numThreads << " Thread" << (numThreads == 1? "" : "s") << ":" << std::endl;
		// Aufwärmen für Turbo-Mode
		runBenchmark<DummyByteGenerator>(NULL, numThreads);
		runBenchmark<DummyIntGenerator>(NULL, numThreads);

		// software PRNG benchmarks
		runBenchmark<CircularBytes>("circular.dat", numThreads);
		runBenchmark<MultiplyWithCarry>("mwc.dat", numThreads);
		runBenchmark<MCG>("mcg.dat", numThreads);
		runBenchmark<MersenneTwister>("mt.dat", numThreads);

		// Ivy Bridge RNG benchmarks
		if (isRdRandSupported()) {
			runBenchmark<RdRand16>("rdrand16.dat", numThreads);
			runBenchmark<RdRand32>("rdrand32.dat", numThreads);
#if defined(_M_X64)
			runBenchmark<RdRand64>("rdrand64.dat", numThreads);
#endif
		}
	}

	return EXIT_SUCCESS;
}
