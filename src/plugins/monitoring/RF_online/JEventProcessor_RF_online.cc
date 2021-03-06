// $Id$
//
//    File: JEventProcessor_RF_online.cc
// Created: Wed Nov  8 11:58:09 EST 2015
// Creator: pmatt (on Linux stan.jlab.org 2.6.32-279.11.1.el6.x86_64 x86_64)

#include "JEventProcessor_RF_online.h"

// Routine used to create our JEventProcessor
extern "C"
{
	void InitPlugin(JApplication *app)
	{
		InitJANAPlugin(app);
		app->AddProcessor(new JEventProcessor_RF_online());
	}
}

/*
	TI-TIME UNIFORMITY CHECK:
	1) Confirm that the copies of the global system clock present on each ROC are consistent with each other: "ROCTIs" directory

	TDC->TIME CHECK:
	1) Confirm that the tdc->time conversion is accurate for each system: "DeltaT_RF_FirstTime" directory
		- TOF & TAGH RF signals read out 1/128 times: expect first delta-t peak at 128*1000/499 ns
		- PSC & FDC RF signals read out 1/64 times: expect first delta-t peak at 64*1000/499 ns

	RF TIME CALIBRATION:
	1) Confirm that the RF signal is arriving at approximately 499 MHz: "RF_SignalPeriod" directory
	2) Study the resolution of each source of the RF signal: "DeltaT_RF_Itself" directory. Check in these constants. 
	3) Do a rough alignment of each RF system with respect to the TOF signal: "AbsoluteDeltaT_RF_OtherRFs" directory & FitMacro_RF_CoarseOffsets.C Macro. Check in these constants.
		- This is important in case the RF signal period is not exactly 499 MHz. If it's slightly wrong, add the time offsets are huge, then propagating across several micro-s will result in a many-ps error in the time. 
	4) Study the resolution when taking the difference between RF times at different sources: "DeltaT_RF_OtherRFs" directory. 
		- If these resolutions are much larger than the individual system uncertainties, then do not average the system times together.
		- This is the default behavior of the DRFTime factory (for the signals in the data stream, only uses the system with the best time resolution)
	5) Do a detailed alignment of each RF system with respect to the TOF signal: "AverageDeltaT_RF_OtherRFs" directory. Update these constants (and the uncertainties). 

	CONFIRM ALIGNMENT TO TAGGER HODOSCOPE:
	1) Once the tagger hodoscope has been aligned to the RF, confirm the alignment: "DeltaT_RF_TAGH" directory. 
      - If it is out-of-line, realign the tagger to the RF (outside the scope of this plugin)
*/

jerror_t JEventProcessor_RF_online::init(void)
{
	dRFSignalPeriod = 1000.0/499.0; //2.004008016

	//pick a combination such that there is a bin edge at +/- 1.002 and +/- 2.004
	double locDeltaTRangeMax = 1.1; //should be a little more than expected-RF-period / 2
	unsigned int locNumDeltaTBins = 1100;

	dRFSignalSystems.push_back(SYS_FDC);  dRFSignalSystems.push_back(SYS_PSC);
	dRFSignalSystems.push_back(SYS_TAGH);  dRFSignalSystems.push_back(SYS_TOF);

	dRFSamplingRateMap[SYS_TOF] = 1.0/128.0;
	dRFSamplingRateMap[SYS_TAGH] = 1.0/128.0;
	dRFSamplingRateMap[SYS_FDC] = 1.0/64.0;
	dRFSamplingRateMap[SYS_PSC] = 1.0/64.0;

/*
	//SUBTRACT this number to the measured RF time
	dTimeOffsetMap[SYS_PSC] = 0.0;
	dTimeOffsetMap[SYS_TAGH] = 33.93; //from TAGH - PSC
	dTimeOffsetMap[SYS_TOF] = -30.59; //from TOF - PSC
	dTimeOffsetMap[SYS_FDC] = 2347.0; //from FDC - PSC
*/

	string locHistName, locHistTitle;

	japp->RootWriteLock(); //ACQUIRE ROOT LOCK!!
	{
		gDirectory->Cd("/");
		new TDirectoryFile("RF", "RF");
		gDirectory->cd("RF");

		//roc info consistency:
			//compare roc infos: delta-t of each to avg-t-exlcuding-itself //separate histograms for each delta
			//will create histograms in evnt() method
		dROCTIDirectory = new TDirectoryFile("ROCTIs", "ROCTIs");

		//rf times:
			//delta-t to first time
		new TDirectoryFile("DeltaT_RF_FirstTime", "DeltaT_RF_FirstTime");
		gDirectory->cd("DeltaT_RF_FirstTime");
		for(size_t loc_i = 0; loc_i < dRFSignalSystems.size(); ++loc_i)
		{
			DetectorSystem_t locSystem = dRFSignalSystems[loc_i];
			string locSystemName = SystemName(locSystem);

			locHistName = locSystemName + string("RF_FirstTimeDeltaT");
			locHistTitle = string("RF_") + locSystemName + string("_TDC;#Deltat (Subsequent Times - First Time) (ns)");
			dHistMap_RFFirstTimeDeltaT[locSystem] = new TH1I(locHistName.c_str(), locHistTitle.c_str(), 50000, 0.0, 1000.0);
		}

		//# rf signals per event
		gDirectory->cd("..");
		new TDirectoryFile("NumRFSignals", "NumRFSignals");
		gDirectory->cd("NumRFSignals");
		for(size_t loc_i = 0; loc_i < dRFSignalSystems.size(); ++loc_i)
		{
			DetectorSystem_t locSystem = dRFSignalSystems[loc_i];
			string locSystemName = SystemName(locSystem);

			locHistName = string("Num") + locSystemName + string("RFSignals");
			locHistTitle = string("RF_") + locSystemName + string(";Num RF Signals");
			dHistMap_NumSignals[locSystem] = new TH1I(locHistName.c_str(), locHistTitle.c_str(), 20, -0.5, 19.5);
		}

		//rf signal period
		gDirectory->cd("..");
		new TDirectoryFile("RF_SignalPeriod", "RF_SignalPeriod");
		gDirectory->cd("RF_SignalPeriod");
		for(size_t loc_i = 0; loc_i < dRFSignalSystems.size(); ++loc_i)
		{
			DetectorSystem_t locSystem = dRFSignalSystems[loc_i];
			string locSystemName = SystemName(locSystem);

			locHistName = locSystemName + string("RF_SignalPeriod");
			locHistTitle = string("RF_") + locSystemName + string(";RF Signal Period (ns)");
			dHistMap_RFSignalPeriod[locSystem] = new TH1I(locHistName.c_str(), locHistTitle.c_str(), 1200, 2.001, 2.007);
		}

		//RF beam bunch period
		gDirectory->cd("..");
		new TDirectoryFile("RF_BeamBunchPeriod", "RF_BeamBunchPeriod");
		gDirectory->cd("RF_BeamBunchPeriod");
		locHistName = "RFBeamBunchPeriod";
		locHistTitle = ";TAGH #Deltat (Within Same Counter) (ns)";
		dHist_RFBeamBunchPeriod = new TH1I(locHistName.c_str(), locHistTitle.c_str(), 1000, 0.0, 200.0);

		//resolutions: compare each rf to itself
		gDirectory->cd("..");
		new TDirectoryFile("DeltaT_RF_Itself", "DeltaT_RF_Itself");
		gDirectory->cd("DeltaT_RF_Itself");
		for(size_t loc_i = 0; loc_i < dRFSignalSystems.size(); ++loc_i)
		{
			DetectorSystem_t locSystem = dRFSignalSystems[loc_i];
			string locSystemName = SystemName(locSystem);

			locHistName = locSystemName + string("RF_SelfDeltaT");
			locHistTitle = string("RF_") + locSystemName + string(";#Deltat (First Pair) (ns)");
			dHistMap_SelfResolution[locSystem] = new TH1I(locHistName.c_str(), locHistTitle.c_str(), locNumDeltaTBins, -1.0*locDeltaTRangeMax, locDeltaTRangeMax);
		}

		//absolute resolutions: compare each rf to each rf
		gDirectory->cd("..");
		new TDirectoryFile("AbsoluteDeltaT_RF_OtherRFs", "AbsoluteDeltaT_RF_OtherRFs");
		gDirectory->cd("AbsoluteDeltaT_RF_OtherRFs");
		for(size_t loc_i = 0; loc_i < dRFSignalSystems.size(); ++loc_i)
		{
			string locSystemName1 = SystemName(dRFSignalSystems[loc_i]);
			for(size_t loc_j = loc_i + 1; loc_j < dRFSignalSystems.size(); ++loc_j)
			{
				string locSystemName2 = SystemName(dRFSignalSystems[loc_j]);
				locHistName = string("RFDeltaT_") + locSystemName1 + string("_") + locSystemName2;
				locHistTitle = string(";#Deltat (RF_") + locSystemName1 + string(" - RF_") + locSystemName2 + string(")");
				pair<DetectorSystem_t, DetectorSystem_t> locSystemPair(dRFSignalSystems[loc_i], dRFSignalSystems[loc_j]);
				dHistMap_AbsoluteRFRFDeltaTs[locSystemPair] = new TH1I(locHistName.c_str(), locHistTitle.c_str(), 4000000, -4000.0, 4000.0);
			}
		}

		//resolutions: compare each rf to each rf
		gDirectory->cd("..");
		new TDirectoryFile("DeltaT_RF_OtherRFs", "DeltaT_RF_OtherRFs");
		gDirectory->cd("DeltaT_RF_OtherRFs");
		for(size_t loc_i = 0; loc_i < dRFSignalSystems.size(); ++loc_i)
		{
			string locSystemName1 = SystemName(dRFSignalSystems[loc_i]);
			for(size_t loc_j = loc_i + 1; loc_j < dRFSignalSystems.size(); ++loc_j)
			{
				string locSystemName2 = SystemName(dRFSignalSystems[loc_j]);
				locHistName = string("RFDeltaT_") + locSystemName1 + string("_") + locSystemName2;
				locHistTitle = string(";#Deltat (RF_") + locSystemName1 + string(" - RF_") + locSystemName2 + string(")");
				pair<DetectorSystem_t, DetectorSystem_t> locSystemPair(dRFSignalSystems[loc_i], dRFSignalSystems[loc_j]);
				dHistMap_RFRFDeltaTs[locSystemPair] = new TH1I(locHistName.c_str(), locHistTitle.c_str(), locNumDeltaTBins, -1.0*locDeltaTRangeMax, locDeltaTRangeMax);
			}
		}

		//resolutions: compare avg of each rf to avg of each rf
			//calib time offsets
		gDirectory->cd("..");
		new TDirectoryFile("AverageDeltaT_RF_OtherRFs", "AverageDeltaT_RF_OtherRFs");
		gDirectory->cd("AverageDeltaT_RF_OtherRFs");
		for(size_t loc_i = 0; loc_i < dRFSignalSystems.size(); ++loc_i)
		{
			string locSystemName1 = SystemName(dRFSignalSystems[loc_i]);
			for(size_t loc_j = loc_i + 1; loc_j < dRFSignalSystems.size(); ++loc_j)
			{
				string locSystemName2 = SystemName(dRFSignalSystems[loc_j]);
				locHistName = string("RFDeltaT_") + locSystemName1 + string("_") + locSystemName2;
				locHistTitle = string(";#Deltat_{#mu} (RF_") + locSystemName1 + string(" - RF_") + locSystemName2 + string(")");
				pair<DetectorSystem_t, DetectorSystem_t> locSystemPair(dRFSignalSystems[loc_i], dRFSignalSystems[loc_j]);
				dHistMap_AverageRFRFDeltaTs[locSystemPair] = new TH1I(locHistName.c_str(), locHistTitle.c_str(), locNumDeltaTBins, -1.0*locDeltaTRangeMax, locDeltaTRangeMax);
			}
		}

		//compare each rf to tagger hodoscope times
		gDirectory->cd("..");
		new TDirectoryFile("DeltaT_RF_TAGH", "DeltaT_RF_TAGH");
		gDirectory->cd("DeltaT_RF_TAGH");
		for(size_t loc_i = 0; loc_i < dRFSignalSystems.size(); ++loc_i)
		{
			DetectorSystem_t locSystem = dRFSignalSystems[loc_i];
			string locSystemName = SystemName(locSystem);

			locHistName = locSystemName + string("RF_TaggerDeltaT");
			locHistTitle = string(";#Deltat (RF_") + locSystemName + string(" - TAGH) (ns)");
			dHistMap_RFTaggerDeltaT[locSystem] = new TH1I(locHistName.c_str(), locHistTitle.c_str(), locNumDeltaTBins, -1.0*locDeltaTRangeMax, locDeltaTRangeMax);
		}

		gDirectory->cd("../..");
	}
	japp->RootUnLock(); //RELEASE ROOT LOCK!!

	return NOERROR;
}

jerror_t JEventProcessor_RF_online::brun(JEventLoop* locEventLoop, int runnumber)
{
	// This is called whenever the run number changes
	dRFTimeFactory = static_cast<DRFTime_factory*>(locEventLoop->GetFactory("DRFTime"));

	//be sure that DRFTime_factory::init() and brun() are called
	vector<const DRFTime*> locRFTimes;
	locEventLoop->Get(locRFTimes);

	return NOERROR;
}

jerror_t JEventProcessor_RF_online::evnt(JEventLoop* locEventLoop, int eventnumber)
{
	vector<const DCODAROCInfo*> locCODAROCInfos;
	locEventLoop->Get(locCODAROCInfos);

	const DTTabUtilities* locTTabUtilities = NULL;
	locEventLoop->GetSingle(locTTabUtilities);

	vector<const DRFTDCDigiTime*> locRFTDCDigiTimes;
	locEventLoop->Get(locRFTDCDigiTimes);

	vector<const DTAGHHit*> locTAGHHits;
	locEventLoop->Get(locTAGHHits);

	//Convert TDCs to Times
	//Use std::set: times are NOT necessarily in order (for high-resolution mode, times interleaved between different internal channels)
	map<DetectorSystem_t, set<double> > locRFTimes;
	map<DetectorSystem_t, set<double> >::iterator locTDCIterator;
	for(size_t loc_i = 0; loc_i < locRFTDCDigiTimes.size(); ++loc_i)
	{
		DetectorSystem_t locSystem = locRFTDCDigiTimes[loc_i]->dSystem;
		double locRFTime = dRFTimeFactory->Convert_TDCToTime(locRFTDCDigiTimes[loc_i], locTTabUtilities);
		locRFTimes[locSystem].insert(locRFTime);
	}

	//Get Total ROC Info Time
	uint64_t locTotalROCTimeStamp = 0;
	for(size_t loc_i = 0; loc_i < locCODAROCInfos.size(); ++loc_i)
		locTotalROCTimeStamp += locCODAROCInfos[loc_i]->timestamp;

	//Average RF Times
	map<DetectorSystem_t, double> locAverageRFTimes;
	for(locTDCIterator = locRFTimes.begin(); locTDCIterator != locRFTimes.end(); ++locTDCIterator)
	{
		set<double>& locRFTimeSet = locTDCIterator->second;

		double locFirstTime = dRFTimeFactory->Step_TimeToNearInputTime(*(locRFTimeSet.begin()), 0.0, dRFSignalPeriod);
		double locAverageTime = locFirstTime;
		set<double>::iterator locSetIterator = locRFTimeSet.begin();
		for(++locSetIterator; locSetIterator != locRFTimeSet.end(); ++locSetIterator)
			locAverageTime += dRFTimeFactory->Step_TimeToNearInputTime(*locSetIterator, locFirstTime, dRFSignalPeriod);
		locAverageTime /= double(locRFTimeSet.size());
		locAverageRFTimes[locTDCIterator->first] = locAverageTime;
	}

	//organize TAGH hits
	map<int, set<double> > locTAGHHitMap; //double: TAGH tdc times (set allows time sorting)
	for(size_t loc_i = 0; loc_i < locTAGHHits.size(); ++loc_i)
	{
		if(locTAGHHits[loc_i]->has_TDC)
			locTAGHHitMap[locTAGHHits[loc_i]->counter_id].insert(locTAGHHits[loc_i]->time_tdc);
	}

	//collect TAGH delta-t's for RF beam bunch histogram
	map<int, set<double> >::iterator locTAGHIterator = locTAGHHitMap.begin();
	vector<double> locTAGHDeltaTs;
	for(; locTAGHIterator != locTAGHHitMap.end(); ++locTAGHIterator)
	{
		set<double>& locCounterHits = locTAGHIterator->second;
		if(locCounterHits.size() < 2)
			continue;
		set<double>::iterator locSetIterator = locCounterHits.begin();
		set<double>::iterator locPreviousSetIterator = locSetIterator;
		for(++locSetIterator; locSetIterator != locCounterHits.end(); ++locSetIterator, ++locPreviousSetIterator)
			locTAGHDeltaTs.push_back(*locSetIterator - *locPreviousSetIterator);
	}

	//FILL HISTOGRAMS
	japp->RootWriteLock(); //ACQUIRE ROOT LOCK!!
	{
		//num rf signals
		for(size_t loc_i = 0; loc_i < dRFSignalSystems.size(); ++loc_i)
		{
			DetectorSystem_t locSystem = dRFSignalSystems[loc_i];
			locTDCIterator = locRFTimes.find(locSystem);
			if(locTDCIterator == locRFTimes.end())
				dHistMap_NumSignals[locSystem]->Fill(0);
			else
				dHistMap_NumSignals[locSystem]->Fill(locTDCIterator->second.size());
		}

		//RF signal frequency
		for(locTDCIterator = locRFTimes.begin(); locTDCIterator != locRFTimes.end(); ++locTDCIterator)
		{
			set<double>& locRFTimeSet = locTDCIterator->second;
			if(locRFTimeSet.size() < 2)
				continue;
			DetectorSystem_t locSystem = locTDCIterator->first;
			double locRFSignalFrequency = (*(locRFTimeSet.rbegin()) - *(locRFTimeSet.begin()))*dRFSamplingRateMap[locSystem]/(double(locRFTimeSet.size() - 1));
			dHistMap_RFSignalPeriod[locSystem]->Fill(locRFSignalFrequency);
		}

		//roc info consistency:
			//compare roc infos: delta-t of each to avg-t-exlcuding-itself //separate histograms for each delta
		for(size_t loc_i = 0; loc_i < locCODAROCInfos.size(); ++loc_i)
		{
			//double has precision of 14 digits: 4ns per tick, 14 digits = 111 hours, way longer than a run
			double locTotalROCTimeStamp_AllButThis = double(locTotalROCTimeStamp - locCODAROCInfos[loc_i]->timestamp);
			double locDeltaT = 4.0*(double(locCODAROCInfos[loc_i]->timestamp) - locTotalROCTimeStamp_AllButThis/(double(locCODAROCInfos.size() - 1)));

			//if histogram doesn't exist yet, create it
			uint32_t locROCID = locCODAROCInfos[loc_i]->rocid;
			if(dHistMap_ROCInfoDeltaT.find(locROCID) == dHistMap_ROCInfoDeltaT.end())
			{
				dROCTIDirectory->cd();
				ostringstream locROCIDStream;
				locROCIDStream << locROCID;
				string locHistName = string("ROCDeltaTs_ROC") + locROCIDStream.str();
				string locHistTitle = string(";#Deltat (ROC ") + locROCIDStream.str() + string(" - #mu_{Other ROCs}) (ns)");
				dHistMap_ROCInfoDeltaT[locROCID] = new TH1I(locHistName.c_str(), locHistTitle.c_str(), 33, -16.5, 16.5);
				gDirectory->Cd("/");
			}

			//fill
			dHistMap_ROCInfoDeltaT[locROCID]->Fill(locDeltaT);
		}

		//RF beam bunch period
		for(size_t loc_i = 0; loc_i < locTAGHDeltaTs.size(); ++loc_i)
			dHist_RFBeamBunchPeriod->Fill(locTAGHDeltaTs[loc_i]);

		//TDCs: RF / First RF
		for(locTDCIterator = locRFTimes.begin(); locTDCIterator != locRFTimes.end(); ++locTDCIterator)
		{
			set<double>& locRFTimeSet = locTDCIterator->second;
			set<double>::iterator locSetIterator = locRFTimeSet.begin();
			for(++locSetIterator; locSetIterator != locRFTimeSet.end(); ++locSetIterator)
				dHistMap_RFFirstTimeDeltaT[locTDCIterator->first]->Fill(*locSetIterator - *(locRFTimeSet.begin()));
		}

		//TDCs / Self
		for(locTDCIterator = locRFTimes.begin(); locTDCIterator != locRFTimes.end(); ++locTDCIterator)
		{
			set<double>& locRFTimeSet = locTDCIterator->second;
			if(locRFTimeSet.size() < 2)
				continue;
			set<double>::iterator locSetIterator = locRFTimeSet.begin();
			++locSetIterator;
			double locShiftedRFTime = dRFTimeFactory->Step_TimeToNearInputTime(*locSetIterator, *(locRFTimeSet.begin()), dRFSignalPeriod);
			dHistMap_SelfResolution[locTDCIterator->first]->Fill(locShiftedRFTime - *(locRFTimeSet.begin()));
		}

		//RF / TAGH
		for(size_t loc_i = 0; loc_i < locTAGHHits.size(); ++loc_i)
		{
			if(!locTAGHHits[loc_i]->has_TDC)
				continue;

			//TDCs
			for(locTDCIterator = locRFTimes.begin(); locTDCIterator != locRFTimes.end(); ++locTDCIterator)
			{
				set<double>& locRFTimeSet = locTDCIterator->second;
				set<double>::iterator locSetIterator = locRFTimeSet.begin();
				for(; locSetIterator != locRFTimeSet.end(); ++locSetIterator)
				{
					double locShiftedRFTime = dRFTimeFactory->Step_TimeToNearInputTime(*locSetIterator, locTAGHHits[loc_i]->time_tdc, dRFSignalPeriod);
					dHistMap_RFTaggerDeltaT[locTDCIterator->first]->Fill(locShiftedRFTime - locTAGHHits[loc_i]->time_tdc);
				}
			}
		}

		//TDCs / TDCs
		for(size_t loc_i = 0; loc_i < dRFSignalSystems.size(); ++loc_i)
		{
			DetectorSystem_t locSystem = dRFSignalSystems[loc_i];
			locTDCIterator = locRFTimes.find(locSystem);
			if(locTDCIterator == locRFTimes.end())
				continue;
			set<double>& locRFTimeSet = locTDCIterator->second;

			for(size_t loc_j = loc_i + 1; loc_j < dRFSignalSystems.size(); ++loc_j)
			{
				DetectorSystem_t locSystem2 = dRFSignalSystems[loc_j];
				map<DetectorSystem_t, set<double> >::iterator locTDCIterator2 = locRFTimes.find(locSystem2);
				if(locTDCIterator2 == locRFTimes.end())
					continue;

				pair<DetectorSystem_t, DetectorSystem_t> locSystemPair(locSystem, locSystem2);
				if(dHistMap_RFRFDeltaTs.find(locSystemPair) == dHistMap_RFRFDeltaTs.end())
					continue;
				set<double>& locRFTimeSet2 = locTDCIterator2->second;

				double locShiftedRFTime = dRFTimeFactory->Step_TimeToNearInputTime(*(locRFTimeSet.begin()), *(locRFTimeSet2.begin()), dRFSignalPeriod);
				double locDeltaT = locShiftedRFTime - *(locRFTimeSet2.begin());
				dHistMap_RFRFDeltaTs[locSystemPair]->Fill(locDeltaT);
			}
		}

		//Average TDCs / TDCs
		for(size_t loc_i = 0; loc_i < dRFSignalSystems.size(); ++loc_i)
		{
			DetectorSystem_t locSystem = dRFSignalSystems[loc_i];
			locTDCIterator = locRFTimes.find(locSystem);
			if(locTDCIterator == locRFTimes.end())
				continue;

			for(size_t loc_j = loc_i + 1; loc_j < dRFSignalSystems.size(); ++loc_j)
			{
				DetectorSystem_t locSystem2 = dRFSignalSystems[loc_j];
				map<DetectorSystem_t, set<double> >::iterator locTDCIterator2 = locRFTimes.find(locSystem2);
				if(locTDCIterator2 == locRFTimes.end())
					continue;

				pair<DetectorSystem_t, DetectorSystem_t> locSystemPair(locSystem, locSystem2);
				if(dHistMap_AverageRFRFDeltaTs.find(locSystemPair) == dHistMap_AverageRFRFDeltaTs.end())
					continue;

				double locShiftedAverageRFTime = dRFTimeFactory->Step_TimeToNearInputTime(locAverageRFTimes[locTDCIterator->first], locAverageRFTimes[locTDCIterator2->first], dRFSignalPeriod);
				double locDeltaT = locShiftedAverageRFTime - locAverageRFTimes[locTDCIterator2->first];
				dHistMap_AverageRFRFDeltaTs[locSystemPair]->Fill(locDeltaT);
			}
		}

		//Absolute TDCs / TDCs
		for(size_t loc_i = 0; loc_i < dRFSignalSystems.size(); ++loc_i)
		{
			DetectorSystem_t locSystem = dRFSignalSystems[loc_i];
			locTDCIterator = locRFTimes.find(locSystem);
			if(locTDCIterator == locRFTimes.end())
				continue;
			set<double>& locRFTimeSet = locTDCIterator->second;

			for(size_t loc_j = loc_i + 1; loc_j < dRFSignalSystems.size(); ++loc_j)
			{
				DetectorSystem_t locSystem2 = dRFSignalSystems[loc_j];
				map<DetectorSystem_t, set<double> >::iterator locTDCIterator2 = locRFTimes.find(locSystem2);
				if(locTDCIterator2 == locRFTimes.end())
					continue;

				pair<DetectorSystem_t, DetectorSystem_t> locSystemPair(locSystem, locSystem2);
				if(dHistMap_AbsoluteRFRFDeltaTs.find(locSystemPair) == dHistMap_AbsoluteRFRFDeltaTs.end())
					continue;
				set<double>& locRFTimeSet2 = locTDCIterator2->second;

				double locDeltaT = *(locRFTimeSet.begin()) - *(locRFTimeSet2.begin());
				dHistMap_AbsoluteRFRFDeltaTs[locSystemPair]->Fill(locDeltaT);
			}
		}
	}
	japp->RootUnLock(); //RELEASE ROOT LOCK!!

	return NOERROR;
}

jerror_t JEventProcessor_RF_online::erun(void)
{
	// This is called whenever the run number changes, before it is
	// changed to give you a chance to clean up before processing
	// events from the next run number.
	return NOERROR;
}

jerror_t JEventProcessor_RF_online::fini(void)
{
	// Called before program exit after event processing is finished.
	return NOERROR;
}
