// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#include <Windows.h>
#include <iostream>
#include <iomanip>

#include <util.h>
#include <getopt.h>
#include <mersenne_twister.h>
#include <stopwatch.h>
#include <boost/crc.hpp>

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
	Intrinsic32,
	Boost
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
	}
	// input fields
	Method method;
	unsigned int* rngBuf;
	int rngBufSize;
	int num;
	HANDLE hThread;
	int iterations;
	DWORD numCores;
	// output fields
	__int64 t;
	__int64 ticks;
	unsigned int crc;
};


// die im Thread laufenden Benchmark-Routine
DWORD WINAPI BenchmarkThreadProc(LPVOID lpParameter)
{
	BenchmarkResult* result = (BenchmarkResult*)lpParameter;
	if (gBindToCore)
		SetThreadAffinityMask(GetCurrentThread(), 1<<(result->num % result->numCores));
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	__int64 tMin = MAXLONGLONG;
	__int64 ticksMin = MAXLONGLONG;
	unsigned int crc;
	for (int i = 0; i < result->iterations; ++i) {
		__int64 t, ticks;
		crc = 0x00000000U;
		{
			Stopwatch stopwatch(t, ticks);
			unsigned int* rn = result->rngBuf + result->num * result->rngBufSize / sizeof(unsigned int);
			switch (result->method) 
			{
			case Intrinsic32:
				{
					const unsigned int* rne = rn + result->rngBufSize / sizeof(unsigned int);
					while (rn < rne)
						crc = _mm_crc32_u32(crc, *rn++);
					break;
				}
			case Boost:
				{
					boost::crc_optimal<32, 0x1EDC6F41 , 0, 0, true, true> crcResult;
					crcResult.process_bytes(rn, result->rngBufSize);
					crc = crcResult.checksum();
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
	result->crc = crc;
	return EXIT_SUCCESS;
}


void runBenchmark(const int numThreads, const char* strMethod, const Method method) {
	// Threads zum Leben erwecken
	HANDLE* hThread = new HANDLE[numThreads];
	BenchmarkResult* pResult = new BenchmarkResult[numThreads];
	const DWORD numCores = getNumCores();
	std::cout.setf(std::ios_base::left, std::ios_base::adjustfield);
    std::cout << "  " << std::setfill(' ') << std::setw(18) << strMethod << ": Generieren ..." << std::flush;
	for (int i = 0; i < numThreads; ++i) {
		pResult[i].num = i;
		pResult[i].rngBuf = gRngBuf;
		pResult[i].rngBufSize = gRngBufSize;
		pResult[i].iterations = gIterations;
		pResult[i].numCores = numCores;
		pResult[i].hThread = CreateThread(NULL, 0, BenchmarkThreadProc, (LPVOID)&pResult[i], CREATE_SUSPENDED, NULL);
		pResult[i].method = method;
		hThread[i] = pResult[i].hThread; // hThread[] wird von WaitForMultipleObjects() ben�tigt
	}
	std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b";
	std::cout << "Calculating CRC ...";

	__int64 t = MAXLONGLONG;
	__int64 ticks = MAXLONGLONG;
	{
		Stopwatch stopwatch(t, ticks);
		for (int i = 0; i < numThreads; ++i)
			ResumeThread(hThread[i]);
		WaitForMultipleObjects(numThreads, hThread, TRUE, INFINITE);
	}
	std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";

	// Ergebnisse sammeln
	__int64 tMin = 0;
	for (int i = 0; i < numThreads; ++i)
		tMin += pResult[i].t;
	tMin /= numThreads;

	std::cout.setf(std::ios_base::right, std::ios_base::adjustfield);
	std::cout << "0x" << std::setfill('0') << std::hex << std::setw(8) << pResult[0].crc
		<< std::setfill(' ') << std::setw(6) << std::dec << tMin << " ms, " 
		<< std::fixed << std::setprecision(2) << std::setw(8) << (float)gRngBufSize*gIterations/1024/1024/(1e-3*t)*numThreads << " MByte/s";

	std::cout << std::endl;

	delete [] pResult;
	delete [] hThread;
}


void usage(void) {
	std::cout << "Aufruf: crc.exe [Optionen]" << std::endl
		<< std::endl
		<< "Optionen:" << std::endl
		<< "  -n N" << std::endl
		<< "     N MByte gro�e Blocks generieren (Vorgabe: " << DEFAULT_RNGBUF_SIZE << ")" << std::endl
		<< std::endl
		<< "  --iterations N" << std::endl
		<< "  -i N" << std::endl
		<< "     Generieren N Mal wiederholen (Vorgabe: " << DEFAULT_ITERATIONS << ")" << std::endl
		<< std::endl
		<< "  --quiet" << std::endl
		<< "  -q" << std::endl
		<< "     Keine Informationen ausgeben" << std::endl
		<< std::endl
		<< "  --threads N" << std::endl
		<< "  -t N" << std::endl
		<< "     Zufallszahlen in N Threads parallel generieren (Vorgabe: " << DEFAULT_NUM_THREADS << ")" << std::endl
		<< "     Mehrfachnennungen m�glich." << std::endl
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
		<< "Alle Ausgaben ohne Gew�hr." << std::endl
		<< std::endl;
}


int main(int argc, char* argv[]) {
	if (!isCRCSupported()) {
		std::cerr << "ERROR: CRC not supported." << std::endl;
		return EXIT_FAILURE;
	}
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
		case SELECT_NO_BIND_TO_CORE:
			gBindToCore = false;
			break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	evaluateCPUFeatures();

	// Speicherbl�cke mit Zufallszahlen belegen
	MersenneTwister gen;
	gen.seed();
	if (gVerbose > 0)
		std::cout << std::endl << "Generating " << gMaxNumThreads << "x" << gRngBufSize << " MByte ..." << std::endl;
	gRngBufSize *= 1024*1024;
	try {
		gRngBuf = new unsigned int[gMaxNumThreads * gRngBufSize / sizeof(unsigned int)];
	}
	catch(...)
	{
		std::cerr << "ERROR: not enough memory." << std::endl;
		return EXIT_FAILURE;
	}
	if (gRngBuf == NULL) {
		std::cerr << "ERROR: cannot allocate memory." << std::endl;
		return EXIT_FAILURE;
	}
	unsigned int* rn = gRngBuf;
	const unsigned int* rne = rn + gMaxNumThreads * gRngBufSize / sizeof(unsigned int);
	while (rn < rne)
		gen.next(*rn++);

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	std::cout << "Checksumming (" << gIterations << " iterations) ..." << std::endl;
	for (int i = 0; i <= gThreadIterations && gNumThreads[i] > 0 ; ++i) {
		const int numThreads = gNumThreads[i];
		if (gVerbose > 0)
			std::cout << std::endl << "... in " << numThreads << " Thread" << (numThreads == 1? "" : "s") << ":" << std::endl;
		runBenchmark(numThreads, "_mm_crc32_u32", Intrinsic32);
		runBenchmark(numThreads, "boost::crc",    Boost);
	}

	delete [] gRngBuf;

	return EXIT_SUCCESS;
}