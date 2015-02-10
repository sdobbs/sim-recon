#ifndef _DTPOLHit_factory_
#define _DTPOLHit_factory_

#include <JANA/JFactory.h>
#include "TTAB/DTranslationTable.h"
#include "DTPOLHit.h"


class DTPOLHit_factory:public jana::JFactory<DTPOLHit>{
	public:
		DTPOLHit_factory(){};
		~DTPOLHit_factory(){};

		// overall scale factors
		double a_scale;
		double t_scale;
                double t_base;

		// calibration constants stored by channel
		vector<double>  a_gains;
		vector<double>  a_pedestals;
		vector<double>  adc_time_offsets;
		vector<vector<double> >timewalk_parameters;

		//map<string,double>  propogation_corr_factors;
		//double<string,double>  attenuation_corr_factors;
		
		double DELTA_T_ADC_TDC_MAX;
		double HIT_TIME_WINDOW;
		double ADC_THRESHOLD;
		int    rollover_count;

		// geometry information
		static const int NSECTORS = 32;
		static const int NRINGS   = 32;

		DTPOLHit* FindMatch(int sector, double T);

		const double GetConstant(const vector<double>  &the_table,
					 const int in_sector) const;
		const double GetConstant(const vector<double>  &the_table,
					 const DSCDigiHit *the_digihit) const;
		const double GetConstant(const vector<double>  &the_table,
					 const DTPOLHit *the_hit) const;

	private:
		jerror_t init(void);						// Called once at program start.
		jerror_t brun(jana::JEventLoop *eventLoop, int runnumber);	// Called everytime a new run number is detected.
		jerror_t evnt(jana::JEventLoop *eventLoop, int eventnumber);	// Called every event.
		jerror_t erun(void);						// Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);						// Called after last event of last event source has been processed.


};

#endif // _DTPOLHit_factory_
