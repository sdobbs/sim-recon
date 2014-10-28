// $Id$

#include <JANA/JEventLoop.h>
using namespace jana;

//#include "DPSCTruthHit.h"
#include "DPSDigiHit.h"
#include "DPSHit_factory.h"
#include "DPSCDigiHit.h"
#include "DPSCHit_factory.h"
#include "DPSCTDCDigiHit.h"
#include "DPSCTruthHit.h"
#include "DPSTruthHit.h"

jerror_t PAIR_SPECTROMETER_init(JEventLoop *loop)
{
  /// Create and register Pair Spectrometer data factories
  loop->AddFactory(new JFactory<DPSDigiHit>());
  loop->AddFactory(new DPSHit_factory());
  loop->AddFactory(new JFactory<DPSCDigiHit>());
  loop->AddFactory(new JFactory<DPSCTDCDigiHit>());
  loop->AddFactory(new DPSCHit_factory());
  loop->AddFactory(new JFactory<DPSCHit>("TRUTH"));
  loop->AddFactory(new JFactory<DPSCTruthHit>());
  loop->AddFactory(new JFactory<DPSTruthHit>());
  
  return NOERROR;
}