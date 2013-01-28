// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#include <Windows.h>
#include <immintrin.h>
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

static const int DEFAULT_CHUNK_COUNT = 10;
static const int DEFAULT_CHUNK_SIZE = 32*1024*1024;

int chunkCount = DEFAULT_CHUNK_COUNT;
int chunkSize = DEFAULT_CHUNK_SIZE;
BYTE* rngBuf = NULL;
MersenneTwister mt;
MCG mcg;
MultiplyWithCarry mwc;
bool doAppend = false;
bool noWrite = false;
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


void runBenchmark(void(*f)(int), const char* name, const char* outputFilename, int bytesPerIter = 0) {
    std::ofstream fs;
    if (outputFilename != NULL && !noWrite)
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
	std::cout << name << " ";
	for (int i = 0; i < chunkCount; ++i) {
		std::cout << '.' << std::flush;
		__int64 t, ticks;
		{
			Stopwatch stopwatch(t, ticks);
			(*f)(chunkSize);
		}
		if (t < tMin)
			tMin = t;
		if (ticks < ticksMin)
			ticksMin = ticks;
		if (bytesPerIter > 0 && fs.is_open()) {
			fs.write((char*)rngBuf, chunkSize);
			std::cout << "\b+" << std::flush;
		}
	}
	std::cout << ' ' << std::setfill(' ') << std::setw(5) << tMin << " ms, " 
		<< std::fixed << std::setw(8) << std::setprecision(2) << (float)chunkSize/1024/1024/(1e-3*tMin) << " Mbyte/s"
		<< std::endl;
	fs.close();
}


void dummyTest(int n) {
	volatile int dummy = 0;
	for (int i = 0; i < n; ++i)
		++dummy;
}


template <typename T>
void mtTest(int n) {
	T* rn = (T*)rngBuf;
	const T* rne = rn + n / sizeof(T);
	while (rn < rne)
		*rn++ = mt();
}


template <typename T>
void mcgTest(int n) {
	T* rn = rngBuf;
	const T* rne = rn + n / sizeof(T);
	while (rn < rne)
		*rn++ = mcg();
}


template <typename T>
void mwcTest(int n) {
	T* rn = (T*)rngBuf;
	const T* rne = rn + n / sizeof(T);
	while (rn < rne)
		*rn++ = mwc();
}


template <typename T>
void rdrand16Test(int n) {
	T* rn = (T*)rngBuf;
	const T* rne = rn + n / sizeof(T);
	while (rn < rne)
		_rdrand16_step(rn++);
}


template <typename T>
void rdrand32Test(int n) {
	T* rn = (T*)rngBuf;
	const T* rne = rn + n / sizeof(T);
	while (rn < rne)
		_rdrand32_step(rn++);
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


int main(int argc, char* argv[]) {
	threadPriority = GetThreadPriority(GetCurrentThread());
    for (;;) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "h?n:s:", long_options, &option_index);
        if (c == -1)
            break;
        switch (c)
        {
        case SELECT_APPEND:
            doAppend = true;
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
			noWrite = true;
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
	rngBuf = new BYTE[chunkSize];

	runBenchmark(&dummyTest, "empty loop       ", NULL);

	mwc.seed(GetTickCount());
	runBenchmark(&mwcTest<unsigned int>,  "Marsaglia (MWC)  ", "mwc.dat");

	mcg.seed(GetTickCount());
	runBenchmark(&mcgTest<unsigned char>, "MCG              ", "mcg.dat");

	mt.seed(GetTickCount());
	runBenchmark(&mtTest<unsigned int>,   "Mersenne-Twister ", "mt.dat");

	delete [] rngBuf;
	SetThreadPriority(GetCurrentThread(), threadPriority);
	return EXIT_SUCCESS;
}
