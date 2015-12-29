#ifndef PREFILTERING_H
#define PREFILTERING_H

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string>
#include <time.h>
#include <sys/time.h>
#include <stddef.h>
#include <stdbool.h>

#include "DBReader.h"
#include "DBWriter.h"
#include "SubstitutionMatrix.h"
#include "Sequence.h"
#include "NucleotideMatrix.h"
#include "Debug.h"
#include "Log.h"
#include "Util.h"
#include "Parameters.h"

#include "ExtendedSubstitutionMatrix.h"
#include "ReducedMatrix.h"
#include "SubstitutionMatrixWithoutX.h"

#include "KmerGenerator.h"
#include "QueryTemplateMatcher.h"
#include "SequenceLookup.h"

#ifdef OPENMP
#include <omp.h>
#endif

class Prefiltering {
public:
    Prefiltering(std::string queryDB,
                 std::string queryDBIndex,
                 std::string targetDB,
                 std::string targetDBIndex,
                 std::string outDB,
                 std::string outDBIndex,
                 Parameters par);

    ~Prefiltering();
    void run(size_t dbFrom, size_t dbSize, int splitMode, std::string resultDB, std::string resultDBIndex);
    void run(int mpi_rank, int mpi_num_procs);
    void run ();
    void closeReader();
    void mergeOutput(std::vector<std::pair<std::string, std::string> > filenames);
    IndexTable *getIndexTable(int split, size_t dbFrom, size_t dbSize); // needed for index lookup

    static IndexTable *generateIndexTable(DBReader<unsigned int>*dbr, Sequence *seq, int alphabetSize, int kmerSize,
                                          size_t dbFrom, size_t dbTo, int searchMode, bool diagonalScoring);

    static void fillDatabase(DBReader<unsigned int>* dbr, Sequence* seq, IndexTable * indexTable,
                             size_t dbFrom, size_t dbTo);


    // get substituion matrix
    static BaseMatrix *getSubstitutionMatrix(std::string scoringMatrixFile, int alphabetSize, float bitFactor,
                                             bool ignoreX);


private:
    static const size_t BUFFER_SIZE = 1000000;

    int threads;

    DBReader<unsigned int>* qdbr;
    DBReader<unsigned int>* tdbr;
    DBReader<unsigned int>* tidxdbr;


    Sequence** qseq;
    int* notEmpty;

    std::list<int>** reslens;
    BaseMatrix* subMat;
    ExtendedSubstitutionMatrix* _2merSubMatrix;
    ExtendedSubstitutionMatrix* _3merSubMatrix;

    std::string outDB;
    std::string outDBIndex;
    // parameter
    int kmerSize;
    const int kmerScore;
    bool spacedKmer;
    const int sensitivity;
    size_t maxResListLen;
    int alphabetSize;
    const float zscoreThr;
    const size_t maxSeqLen;
    const int querySeqType;
    const int targetSeqType;
    bool templateDBIsIndex;
    bool mask;
    bool diagonalScoring;
    bool aaBiasCorrection;
    short kmerThr;
    double kmerMatchProb;
    int split;
    int splitMode;
    int searchMode;
    bool sameQTDB;

    /* Set the k-mer similarity threshold that regulates the length of k-mer lists for each k-mer in the query sequence.
     * As a result, the prefilter always has roughly the same speed for different k-mer and alphabet sizes.
     */
    std::pair<short, double> setKmerThreshold(IndexTable *indexTable, DBReader<unsigned int> *qdbr, DBReader<unsigned int> *tdbr,
                                              int targetKmerMatchProb, const int kmerScore);
    // write prefiltering to ffindex database
    int writePrefilterOutput(DBWriter *dbWriter, int thread_idx, size_t id,
                             std::pair<hit_t *, size_t> prefResults, size_t seqIdOffset);
    // init QueryTemplateMatcher
    QueryTemplateMatcher **createQueryTemplateMatcher(BaseMatrix *m, IndexTable *indexTable,
                                                      unsigned int *seqLens, short kmerThr,
                                                      double kmerMatchProb, int kmerSize,
                                                      size_t effectiveKmerSize, size_t dbSize,
                                                      bool aaBiasCorrection, bool diagonalScoring,
                                                      unsigned int maxSeqLen, int searchMode,
                                                      size_t maxHitsPerQuery);


    void printStatistics(statistics_t &stats, size_t empty);

    statistics_t computeStatisticForKmerThreshold(IndexTable *indexTable, size_t querySetSize, unsigned int *querySeqsIds, bool reverseQuery, const size_t kmerThrMid);

    void mergeFiles(std::vector<std::pair<std::string, std::string>> splitFiles, int mode);
};

#endif
