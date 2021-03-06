// $Id$
//
//    File: DAnalysisResults_factory_PreKinFit.h
// Created: Tue Aug  9 14:29:24 EST 2011
// Creator: pmatt (on Linux ifarml6 2.6.18-128.el5 x86_64)
//

#ifndef _DAnalysisResults_factory_PreKinFit_
#define _DAnalysisResults_factory_PreKinFit_

#include <map>
#include <deque>
#include <vector>

#include "TH1D.h"
#include "TH2D.h"
#include "TDirectoryFile.h"

#include "JANA/JFactory.h"
#include "DANA/DApplication.h"

#include "TRACKING/DMCThrown.h"

#include "ANALYSIS/DReaction.h"
#include "ANALYSIS/DCutActions.h"
#include "ANALYSIS/DParticleCombo.h"
#include "ANALYSIS/DAnalysisAction.h"
#include "ANALYSIS/DAnalysisUtilities.h"
#include "ANALYSIS/DAnalysisResults.h"
#include "ANALYSIS/DHistogramActions.h"

using namespace jana;
using namespace std;

class DAnalysisResults_factory_PreKinFit : public jana::JFactory<DAnalysisResults>
{
	public:
		DAnalysisResults_factory_PreKinFit():root_hists_created(false){};
		~DAnalysisResults_factory_PreKinFit(){};
		const char* Tag(void){return "PreKinFit";}

	private:
		jerror_t init(void);						///< Called once at program start.
		jerror_t brun(jana::JEventLoop *locEventLoop, int runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(jana::JEventLoop *locEventLoop, int eventnumber);	///< Called every event.
		jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);						///< Called after last event of last event source has been processed.

		void Get_Reactions(jana::JEventLoop* locEventLoop, vector<const DReaction*>& locReactions) const;

		unsigned int dDebugLevel;
		DApplication* dApplication;
		double dMinThrownMatchFOM;
		const DAnalysisUtilities* dAnalysisUtilities;
		bool root_hists_created;

		map<const DReaction*, bool> dMCReactionExactMatchFlags;
		map<const DReaction*, DCutAction_TrueCombo*> dTrueComboCuts;

		map<const DReaction*, TH1D*> dHistMap_NumParticleCombos;
		map<const DReaction*, TH1D*> dHistMap_NumEventsSurvivedAction_All;
		map<const DReaction*, TH1D*> dHistMap_NumEventsWhereTrueComboSurvivedAction;
		map<const DReaction*, TH2D*> dHistMap_NumCombosSurvivedAction;
		map<const DReaction*, TH1D*> dHistMap_NumCombosSurvivedAction1D;
};

#endif // _DAnalysisResults_factory_PreKinFit_

