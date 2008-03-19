/**
 * HopfieldDemo.cc
 *
 * Emulates a hopfield network using OpenCog dynamics
 *
 * Author: Joel Pitt
 * Creation: Wed Mar 12 11:14:22 GMT+12 2008
 */

#include <sstream>
#include <math.h>

#include <mt19937ar.h>
#include <Logger.h>
#include <SimpleTruthValue.h>

#include "HopfieldDemo.h"

using namespace opencog;
using namespace std;

#define HDEMO_DEFAULT_WIDTH 3
#define HDEMO_DEFAULT_HEIGHT 3
#define HDEMO_DEFAULT_LINKS 25
#define HDEMO_DEFAULT_PERCEPT_STIM 10
#define HDEMO_DEFAULT_SPREAD_THRESHOLD 4

int main(int argc, char *argv[])
{
    int patternArray[] = { 0, 1, 0, 1, 1, 1, 0, 1, 0 };
    int patternArray2[] = { 0, 1, 0, 1, 1, 1, 0, 0, 0 };
    std::vector<int> pattern1(patternArray, patternArray + 9);
    std::vector<int> pattern2(patternArray2, patternArray2 + 9);

    HopfieldDemo hDemo;
    MAIN_LOGGER.setPrintToStdoutFlag(true);
    MAIN_LOGGER.log(Util::Logger::INFO,"Init HopfieldDemo");

    hDemo.init(HDEMO_DEFAULT_WIDTH, HDEMO_DEFAULT_HEIGHT, HDEMO_DEFAULT_LINKS);

    hDemo.unitTestServerLoop(5);
    MAIN_LOGGER.log(Util::Logger::INFO,"Ran server for 5 loops");

    hDemo.encodePattern(pattern1);
    hDemo.unitTestServerLoop(10);
    MAIN_LOGGER.log(Util::Logger::INFO,"Encoded pattern and ran server for 5 loops");
    hDemo.retrievePattern(pattern2,1);
    MAIN_LOGGER.log(Util::Logger::INFO,"Updated Atom table for retrieval");
}

template <class T>
inline string to_string (const T& t)
{
    stringstream ss;
    ss << t;
    return ss.str();
}

HopfieldDemo::HopfieldDemo()
{
    perceptStimUnit = HDEMO_DEFAULT_PERCEPT_STIM;
    spreadThreshold = HDEMO_DEFAULT_SPREAD_THRESHOLD;
    rng = new Util::MT19937RandGen(time(NULL));

    agent = new ImportanceUpdatingAgent();
    agent->getLogger()->enable();
    agent->getLogger()->setLevel(Util::Logger::FINE);
    agent->getLogger()->setPrintToStdoutFlag(true);
    plugInMindAgent(agent, 1);

    MAIN_LOGGER.log(Util::Logger::INFO,"Plugged in ImportanceUpdatingAgent");
}

HopfieldDemo::~HopfieldDemo()
{
    delete agent;
    delete rng;

}

void HopfieldDemo::init(int width, int height, int numLinks)
{
    AtomSpace* atomSpace = getAtomSpace();
    string nodeName = "Hopfield_";
    this->width = width;
    this->height = height;

    // Create nodes
    for (int i=0; i < width; i++) {
	for (int j=0; j < height; j++) {
	    nodeName = "Hopfield_";
	    nodeName += to_string(i) + "_" + to_string(j);
	    cout << nodeName << endl;
	    Handle h = atomSpace->addNode(CONCEPT_NODE, nodeName.c_str());
	    hGrid.push_back(h);
	}
    }
    
    // If only 1 node, don't try and connect it
    if (hGrid.size() < 2) return;

    // Link nodes randomly with numLinks links
    while (numLinks > 0) {
	int source, target;
	HandleSeq outgoing;

	source = rng->randint(hGrid.size());
	target = source;
	while (target == source) target = rng->randint(hGrid.size());

	outgoing.push_back(hGrid[target]);
	outgoing.push_back(hGrid[source]);
	atomSpace->addLink(HEBBIAN_LINK, outgoing);

	numLinks--;
    }


}

void HopfieldDemo::encodePattern(std::vector<int> pattern)
{
    std::vector<Handle>::iterator it = hGrid.begin();
    std::vector<int>::iterator p_it = pattern.begin();
    
    while (it!=hGrid.end() && p_it != pattern.end()) {
	Handle h = (*it);
	int value = (*p_it);
	getAtomSpace()->stimulateAtom(h,perceptStimUnit * value);
	it++; p_it++;
    }

}

std::vector<int> HopfieldDemo::retrievePattern(std::vector<int> partialPattern, int numCycles)
{
    std::string logString;
    
    logString += "Initialising " + to_string(numCycles) + " cycle pattern retrieval process.";
    MAIN_LOGGER.log(Util::Logger::INFO, logString.c_str());

    while (numCycles > 0) {
	encodePattern(partialPattern);
	updateAtomTableForRetrieval();

	numCycles--;
	MAIN_LOGGER.log(Util::Logger::INFO, "Cycles left %d",numCycles);
    }
    
    logString = "Imprinted pattern: \n" + patternToString(partialPattern); 
    MAIN_LOGGER.log(Util::Logger::INFO, logString.c_str());
    
    MAIN_LOGGER.log(Util::Logger::INFO, "Pattern retrieval process ended.");
    
    std::vector<int> a;
    return a;
}

std::string HopfieldDemo::patternToString(std::vector<int> p)
{
    std::stringstream ss;
    int col = 0;

    std::vector<int>::iterator it = p.begin();

    while(it != p.end()) {
	ss << *it << " ";
	col++;
	if (col == width) {
	    ss << endl;
	    col = 0;
	}
	it++;
    }
    return ss.str();
}

std::vector<int> HopfieldDemo::updateAtomTableForRetrieval()
{
//   run ImportanceUpdatingAgent once without updating links

    bool oldLinksFlag = agent->getUpdateLinksFlag();
    agent->setUpdateLinksFlag(false);
    
    MAIN_LOGGER.log(Util::Logger::INFO, "===updateAtomTableForRetreival===");
    MAIN_LOGGER.log(Util::Logger::INFO, "Running Importance updating agent");
    agent->run(this);

    MAIN_LOGGER.log(Util::Logger::INFO, "Spreading Importance");
    spreadImportance();

    agent->setUpdateLinksFlag(oldLinksFlag);

    std::vector<int> a;
    return a;
}


void HopfieldDemo::spreadImportance()
{
// TODO: This should be a mind agent, convert later...

    AttentionValue::sti_t current;

    // Find all nodes with sti > spread threshold
    AtomSpace *a;
    std::vector<Handle> atoms;
    std::vector<Handle>::iterator hi;
    std::back_insert_iterator< std::vector<Handle> > out_hi(atoms);
    
    a = getAtomSpace();
    MAIN_LOGGER.log(Util::Logger::INFO, "Getting handle set");
    a->getHandleSet(out_hi,NODE,true);
    MAIN_LOGGER.log(Util::Logger::INFO, "Got handle set");

    hi = atoms.begin();
    while (hi != atoms.end()) {
	Handle h = *hi;
	MAIN_LOGGER.log(Util::Logger::INFO, "Spreading importance for atom %s", TLB::getAtom(h)->toString().c_str());

	current = a->getSTI(h);
	/* spread if STI > spread threshold */
	if (current > spreadThreshold )
	    // spread fraction of importance to nodes it's linked to
	    spreadAtomImportance(h);
	
	hi++;
    }

    

}

void HopfieldDemo::spreadAtomImportance(Handle h)
{
    HandleEntry *neighbours, *links, *he;
    AtomSpace *a;
    int maxTransferAmount, totalRelatedness, totalTransferred;
    AttentionValue::sti_t importanceSpreadingQuantum = 5;
    float importanceSpreadingFactor = 0.4;

    totalRelatedness = totalTransferred = 0;

    a = getAtomSpace();
    
    neighbours = TLB::getAtom(h)->getNeighbors(true,true,HEBBIAN_LINK);
    links = TLB::getAtom(h)->getIncomingSet()->clone();
    links = HandleEntry::filterSet(links, HEBBIAN_LINK, true);
    
    maxTransferAmount = (int) (a->getSTI(h) * importanceSpreadingFactor);

    // sum total relatedness
    he = links;
    while (he) {
	totalRelatedness += abs(a->getSTI(he->handle));
	he = he->next;
    }

    if (totalRelatedness > 0) {
	// Find order of links based on their STI
	// links = orderBySTI (handleEntry)?

	he = links;
	while (he) {
	    double transferWeight, transferAmount;
	    std::vector<Handle> targets;
	    Handle h = he->handle;
	    const TruthValue &linkTV = a->getTV(h);
	    // TODO: Transfer weight should be combination of TV strength and
	    // confidence...
	    transferWeight = linkTV.toFloat();
	    targets = TLB::getAtom(h)->getOutgoingSet();

	    // amount to spread dependent on weight and quantum - needs to be
	    // divided by (targets->size - 1)
	    transferAmount = transferWeight * importanceSpreadingQuantum;

	    if (totalTransferred + transferAmount < maxTransferAmount) {
		std::vector<Handle>::iterator t;
		t = targets.begin();
		while (t != targets.end()) {
		    Handle target_h = *t;


		    t++;
		}
		// Then for each target of link (except source)...
		bool doTransfer = true; 
		if (a->getSTI(h) >= a->getAttentionalFocusBoundary() && \
		    a->getSTI(h) - transferAmount < a->getAttentionalFocusBoundary()) {

		}

		
	    }


	    he = he->next;
	}
    }

}
