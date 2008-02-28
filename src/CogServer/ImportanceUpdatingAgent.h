/**
 * ImportantUpdatingAgent.h
 *
 * $Header$
 *
 * Author: Joel Pitt 
 * Creation: Fri Feb   22 16:07:00 GMT+12 2008
 */

#ifndef IMPORTANCEUPDATINGAGENT_H
#define IMPORTANCEUPDATINGAGENT_H

#include <string>
#include <iostream>
#include <sstream>
#include <AttentionValue.h>

#include <AtomSpace.h>
#include "CogServer.h"
#include "MindAgent.h"

/* Starting values for rent and wage */
#define DEFAULT_ATOM_STI_RENT 10
#define DEFAULT_ATOM_LTI_RENT 10
#define DEFAULT_ATOM_STI_WAGE 10
#define DEFAULT_ATOM_LTI_WAGE 10

namespace opencog {

class CogServer;

class ImportanceUpdatingAgent : MindAgent {
private:

    /* Atom Rent */
    AttentionValue::sti_t STIAtomRent;
    AttentionValue::lti_t LTIAtomRent;

    /* Atom wages (to be weighted by stimulus) */
    AttentionValue::sti_t STIAtomWage;
    AttentionValue::lti_t LTIAtomWage;

    /* Cap on STI/LTI */
    AttentionValue::sti_t STICap;
    AttentionValue::lti_t LTICap;

    /* Update links or not */
    bool updateLinks;

    /* Randomly stimulate atoms? */
    bool noiseOn;
    /* Change of randomly introduced stimulation */
    float noiseOdds;
    /* The default stimulus unit, used by random stimulation */
    stim_t noiseUnit;

    /**
     * Randomly stimulate atoms in the AtomSpace
     *
     * @param AtomSpace to act upon
     */
    void randomStimulation(AtomSpace *a) {};
    
    /* Recent amount of stimulus given per cycle */
    stim_t recentTotalStimulusPerCycle;
    stim_t recentTotalStimulusSinceReset;
    /* Rate of decay (r) for estimating recentAttentionalFocusSize
     * Estimate equal to:
     * r *(attentionalFocusSize + (1-r) recentAttentionalFocusSize */
    float recentTotalStimulusDecay;

    /* Number of atoms within attentionFocusBoundary */
    long attentionalFocusSize;
    long recentAttentionalFocusSize;
    long attentionalFocusNodesSize;
    long recentAttentionalFocusNodesSize;
    /* Rate of decay (r) for estimating recentAttentionalFocusSize
     * Estimate equal to:
     * r *(attentionalFocusSize + (1-r) recentAttentionalFocusSize */
    float attentionalFocusSizeDecay;

    /* for calculate the recent maximum STI value, used in Hebbian learning */
    float maxSTIDecayRate;
    AttentionValue::sti_t recentMaxSTI;

    /* The target lobe STI and LTI values are not only initial values, but
     * also values that are returned to if the values exit their
     * acceptable ranges */
    long targetLobeSTI;
    long targetLobeLTI;
    long acceptableLobeSTIRange[2];
    long acceptableLobeLTIRange[2];
    
    /**
     * Collect STI rent for atoms above attentional focus boundary.
     *
     * @param AtomSpace the MindAgent is working on.
     */
    void collectSTIRent(AtomSpace* a); 
    
    /**
     * Collect STI rent for atoms above attentional focus boundary.
     *
     * @param AtomSpace the MindAgent is working on.
     */
    void collectLTIRent(AtomSpace* a); 

    /**
     * Pay STI wages to atoms that have been useful.
     *
     * @param AtomSpace the MindAgent is working on.
     */
    void paySTIWages(AtomSpace* a);

    /**
     * Pay LTI wages to atoms that have been useful.
     *
     * @param AtomSpace the MindAgent is working on.
     */
    void payLTIWages(AtomSpace* a);

    /**
     * Cap STI values to the maximum to prevent all important atoms.
     *
     * @param AtomSpace the MindAgent is working on.
     * @return Whether any atoms had an STI above the cap.
     */
    bool enforceSTICap(AtomSpace* a);

    /**
     * Cap LTI values to the maximum to prevent all important atoms.
     *
     * @param AtomSpace the MindAgent is working on.
     * @return Whether any atoms had an STI above the cap.
     */
    bool enforceLTICap(AtomSpace* a);

    /**
     * Check whether AtomSpace funds are within limits, and make changes
     * if not.
     *
     * @param AtomSpace to work in.
     */
    void checkAtomSpaceFunds(AtomSpace* a);

    /**
     * Decide whether first value is in the range specified by the 2
     * values in range.
     *
     * @param value to check
     * @param range to compare with
     * @return whether value is within range
     */
    bool inRange(long val, long range[2]) const;

    /**
     * If STI funds are outside of acceptable limits, then adjust
     * rent.
     *
     * @param AtomSpace to work in
     */
    void fixSTIDynamics(AtomSpace* a);

    /**
     * If LTI funds are outside of acceptable limits, then adjust
     * rent.
     *
     * @param AtomSpace to work in
     */
    void fixLTIDynamics(AtomSpace* a);

    /**
     * Update the attentional focus size variables
     * needed for tuning attention dynamics.
     *
     * @param the AtomSpace to work on
     */
    void updateAttentionalFocusSizes(AtomSpace* a);

    /* Has init been run to give iterative variables sensible start points */
    bool initialEstimateMade;

    /**
     * Initialise iterative variables with suitable starting values.
     *
     * @param server is a pointer to the cogserver
     */
    void init(CogServer *server);

    /* Debug */
    bool verbose;

public:
		
    ImportanceUpdatingAgent();

    virtual ~ImportanceUpdatingAgent();
		
    virtual void run(CogServer *server);

    virtual string toString();

}; // class

}  // namespace

#endif // IMPORTANCEUPDATINGAGENT_H
