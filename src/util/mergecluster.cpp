//
// Created by mad on 5/9/15.
//

#include "mergecluster.h"
#include <list>
#include <Debug.h>
#include "DBReader.h"
#include "DBWriter.h"
#include "Parameters.h"


void mergeClusteringResults(std::string seqDB, std::string outDB, std::list<std::string> cluSteps)
{
    // open the sequence database
    // it will serve as the reference for sequence indexes
    std::string seqDBIndex = seqDB + ".index";
    DBReader* dbr = new DBReader(seqDB.c_str(), seqDBIndex.c_str());
    dbr->open(DBReader::NOSORT);

    // init the structure for cluster merging
    // it has the size of all possible cluster (sequence amount)
    std::list<int>** mergedClustering = new std::list<int>*[dbr->getSize()];
    Debug(Debug::WARNING) << "List amount "<< dbr->getSize() << "\n";
    for (size_t i = 0; i < dbr->getSize(); i++){
        mergedClustering[i] = new std::list<int>();
    }

    // read the clustering from the first clustering step
    std::string firstCluStep = cluSteps.front();
    cluSteps.pop_front();
    std::string firstCluStepIndex = firstCluStep + ".index";
    DBReader* cluStepDbr = new DBReader(firstCluStep.c_str(), firstCluStepIndex.c_str());
    cluStepDbr->open(DBReader::NOSORT);

    for (size_t i = 0; i < cluStepDbr->getSize(); i++){
        size_t cluId = dbr->getId(cluStepDbr->getDbKey(i));
        std::stringstream lineSs (cluStepDbr->getData(i));
        std::string val;
        // go through the sequences in the cluster and add them to the initial clustering
        while (std::getline(lineSs, val)){
            size_t seqId = dbr->getId(val.c_str());
            mergedClustering[cluId]->push_back(seqId);
        }
    }
    cluStepDbr->close();
    delete cluStepDbr;
    Debug(Debug::WARNING) << "Clustering step 1...\n";

    // merge later clustering steps into the initial clustering step
    int cnt = 2;
    while(!cluSteps.empty()){
        // open the next clustering database
        std::string cluStep = cluSteps.front();
        std::string cluStepIndex = cluStep + ".index";
        cluSteps.pop_front();

        cluStepDbr = new DBReader(cluStep.c_str(), cluStepIndex.c_str());
        cluStepDbr->open(DBReader::NOSORT);

        // go through the clusters and merge them into the clusters from the previous clustering step
        for (size_t i = 0; i < cluStepDbr->getSize(); i++){
            size_t cluId = dbr->getId(cluStepDbr->getDbKey(i));
            char* cluData = cluStepDbr->getData(i);
            std::stringstream lineSs(cluData);
            std::string val;
            // go through the sequences in the cluster and add them and their clusters to the cluster of cluId
            // afterwards, delete the added cluster from the clustering
            while (std::getline(lineSs, val, '\n')){
                size_t seqId = dbr->getId(val.c_str());
                if(seqId != cluId) // to avoid copies of the same cluster list
                    mergedClustering[cluId]->splice(mergedClustering[cluId]->end(), *mergedClustering[seqId]);
            }

        }
        cluStepDbr->close();
        delete cluStepDbr;
        Debug(Debug::WARNING) << "Clustering step " << cnt << "...\n";
        cnt++;
    }

    Debug(Debug::WARNING) << "Writing the results...\n";

    std::string outDBIndex = outDB + ".index";
    DBWriter* dbw = new DBWriter(outDB.c_str(), outDBIndex.c_str());
    dbw->open();

    size_t BUFFER_SIZE = 1000000;
    char* outBuffer = new char[BUFFER_SIZE];
    // go through all sequences in the database
    for (size_t i = 0; i < dbr->getSize(); i++){

        // no cluster for this representative
        if (mergedClustering[i]->size() == 0)
            continue;

        // representative
        char* dbKey = dbr->getDbKey(i);

        std::stringstream res;
        for(std::list<int>::iterator it = mergedClustering[i]->begin(); it != mergedClustering[i]->end(); ++it){
            res << dbr->getDbKey(*it) << "\n";
        }

        std::string cluResultsOutString = res.str();
        const char* cluResultsOutData = cluResultsOutString.c_str();
        if (BUFFER_SIZE < strlen(cluResultsOutData)){
            std::cerr << "Tried to process the clustering list for the query " << dbKey << " , the length of the list = " << mergedClustering[i]->size() << "\n";
            std::cerr << "Output buffer size < clustering result size! (" << BUFFER_SIZE << " < " << cluResultsOutString.length() << ")\nIncrease buffer size or reconsider your parameters -> output buffer is already huge ;-)\n";
            continue;
        }
        memcpy(outBuffer, cluResultsOutData, cluResultsOutString.length()*sizeof(char));
        dbw->write(outBuffer, cluResultsOutString.length(), dbKey);
    }
    dbw->close();
    delete dbw;
    delete[] outBuffer;

    // delete the clustering data structure
    for (unsigned int i = 0; i < dbr->getSize(); i++){
        delete mergedClustering[i];
    }
    delete[] mergedClustering;
    Debug(Debug::WARNING) << "...done.\n";

}

int mergecluster(int argc,const char **argv)
{

    std::string usage;
    usage.append("Combines n cluster steps to a final clustering\n");
    usage.append("USAGE: <sequenceDB> <outDB> <cluDB1> ... <cluDBn>\n");
    usage.append("\nDesigned and implemented by Maria Hauser, Martin Steinegger <martin.steinegger@campus.lmu.de>.\n");

    Parameters par;
    par.parseParameters(argc, argv, usage, par.onlyverbosity, 4);
    std::list<std::string> clusterings;
    for(int i = 3; i < argc; i++){
        clusterings.push_back(std::string(argv[i]));
    }
    mergeClusteringResults(par.db1, par.db2, clusterings);

    return 0;
}