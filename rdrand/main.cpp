// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#include <Windows.h>
#include <stdio.h>
#include <getopt.h>
#include <fstream>
#include <iostream>
#include <iomanip>

#include "stopwatch.h"
#include "abstract_random_number_generator.h"
#include "mersenne_twister.h"
#include "marsaglia.h"
#include "mcg.h"
#include "rdrand.h"

static const int DEFAULT_CHUNK_COUNT = 10;
static const int DEFAULT_CHUNK_SIZE = 32*1024*1024;

int chunkCount = DEFAULT_CHUNK_COUNT;
int chunkSize = DEFAULT_CHUNK_SIZE;
BYTE* rngBuf = NULL;
MersenneTwister mt;
MCG mcg;
MultiplyWithCarry mwc;
DummyGenerator dummy;
RdRand16 rdrand16;
RdRand32 rdrand32;
bool doAppend = false;
bool doWrite = true;
int verbose = 0;
int threadPriority = THREAD_PRIORITY_NORMAL;


enum _long_options {
    SELECT_HELP = 0x1,
    SELECT_NO_WRITE,
    SELECT_APPEND};
static struct option long_options[] = {
    { "append",               no_argument, 0, SELECT_APPEND },
    { "no-write",             no_argument, 0, SELECT_NO_WRITE },
    { "help",                 no_argument, 0, SELECT_HELP },
};


inline int roundup(int x, int bits) {
	return ((x)>>(bits)<<(bits)) + (1<<bits);
}


template <typename T>
void runBenchmark(AbstractRandomNumberGenerator<T>& gen, const char* name, const char* outputFilename) {
    std::ofstream fs;
    if (outputFilename != NULL && doWrite)
    {
        fs.open(outputFilename, (doAppend
            ? (std::ios::binary | std::ios::out | std::ios::app | std::ios::ate)
            : (std::ios::binary | std::ios::out)));
        if (!fs.is_open() || fs.fail())
        {
            std::cerr << "FEHLER: Öffnen von " << outputFilename << " fehlgeschlagen." << std::endl;
            exit(EXIT_FAILURE);
        }
    }

	__int64 tMin = MAXLONGLONG;
	__int64 ticksMin = MAXLONGLONG;
	std::cout.setf(std::ios_base::left, std::ios_base::adjustfield);
	std::cout << std::setfill('_') << std::setw(18) << name << " ";
	for (int i = 0; i < chunkCount; ++i) {
		std::cout << '.' << std::flush;
		__int64 t, ticks;
		{
			Stopwatch stopwatch(t, ticks);
			T* rn = (T*)rngBuf;
			const T* rne = rn + chunkSize / sizeof(T);
			while (rn < rne)
				gen.next(*rn++);
		}
		if (t < tMin)
			tMin = t;
		if (ticks < ticksMin)
			ticksMin = ticks;
		if (fs.is_open()) {
			fs.write((char*)rngBuf, chunkSize);
			std::cout << "\b+" << std::flush;
		}
	}
	std::cout.setf(std::ios_base::right, std::ios_base::adjustfield);
	std::cout << ' ' << std::setfill(' ') << std::setw(5) << tMin << " ms, " 
		<< std::fixed << std::setw(8) << std::setprecision(2) << (float)chunkSize/1024/1024/(1e-3*tMin) << " Mbyte/s"
		<< std::endl;
	fs.close();
}


void usage(void) {
    std::cout << "Aufruf: intrinsics [Optionen]" << std::endl
        << std::endl
        << "Optionen:" << std::endl
        << "  -s N" << std::endl
		<< "     N Durchläufe pro Benchmark (Vorgabe: " << DEFAULT_CHUNK_COUNT << ")" << std::endl
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


const char* B[2] = { "false", "true" };


int main(int argc, char* argv[]) {
	threadPriority = GetThreadPriority(GetCurrentThread());

	for (;;) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "vh?n:s:", long_options, &option_index);
        if (c == -1)
            break;
        switch (c)
        {
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
            return 0;
            break;
		case SELECT_NO_WRITE:
			doWrite = false;
			break;
        case 's':
            if (optarg == NULL)
            {
                usage();
                exit(EXIT_FAILURE);
            }
            chunkSize = atoi(optarg);
            if (chunkSize <= 0)
                chunkSize = DEFAULT_CHUNK_SIZE;
            break;
        case 'n':
            if (optarg == NULL)
            {
                usage();
                exit(EXIT_FAILURE);
            }
            chunkCount = atoi(optarg);
            if (chunkCount <= 0)
                chunkCount = DEFAULT_CHUNK_COUNT;
            break;
        default:
			usage();
			return EXIT_FAILURE;
		}
	}

	int cpureg[4] = { 0, 0, 0, 0 };
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
		std::cout << "[[ SSE3   : " << B[sse3_supported] << std::endl;
		std::cout << "[[ SSE4.1 : " << B[sse41_supported] << std::endl;
		std::cout << "[[ SSE4.2 : " << B[sse42_supported] << std::endl;
		std::cout << "[[ POPCNT : " << B[popcnt_supported] << std::endl;
		std::cout << "[[ AVX    : " << B[avx_supported] << std::endl;
		std::cout << "[[ AES    : " << B[aes_supported] << std::endl;
		std::cout << "[[ 1<<29  : " << B[b29_supported] << std::endl;
		std::cout << "[[ RDRAND : " << B[rdrand_supported] << std::endl;
		std::cout << "[[ 1<<31  : " << B[b31_supported] << std::endl;
	}

	chunkSize = roundup(chunkSize, 2);
	rngBuf = new BYTE[chunkSize];
	
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS); 
	// warmup to turbo mode
	runBenchmark<unsigned char>(dummy, "empty loop", NULL);

	mwc.seed(GetTickCount());
	runBenchmark<unsigned int>(mwc, "Marsaglia (MWC)", "mwc.dat");

	mcg.seed(GetTickCount());
	runBenchmark<unsigned char>(mcg, "MCG", "mcg.dat");

	mt.seed(GetTickCount());
	runBenchmark<unsigned int>(mt, "Mersenne-Twister", "mt.dat");

	if (rdrand_supported) {
		runBenchmark<unsigned short>(rdrand16, "_rdrand16_step", "mt.dat");
		runBenchmark<unsigned int>(rdrand32, "_rdrand32_step", "mt.dat");
	}

	delete [] rngBuf;
	SetThreadPriority(GetCurrentThread(), threadPriority);
	return EXIT_SUCCESS;
}
