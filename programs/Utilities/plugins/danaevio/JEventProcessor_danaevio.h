// $Id$
//
//    File: JEventProcessor_danaevio.h
// Created: Mon Mar 15 09:08:37 EDT 2010
// Creator: wolin (on Linux stan.jlab.org 2.6.18-164.el5 x86_64)
//

#ifndef _JEventProcessor_danaevio_
#define _JEventProcessor_danaevio_


#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>
#include <JANA/JFactory.h>

#include <evioFileChannel.hxx>


using namespace std;
using namespace jana;
using namespace evio;


//----------------------------------------------------------------------------


class JEventProcessor_danaevio : public JEventProcessor {

 public:
  JOBJECT_PUBLIC(JEventProcessor_danaevio);
  const char* className(void) { return "JEventProcessor_danaevio";}

  JEventProcessor_danaevio();
  ~JEventProcessor_danaevio();


 private:
  jerror_t evnt(JEventLoop *eventLoop, int eventnumber);
  jerror_t fini();
  
  evioFileChannel *chan;
  

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
};


#endif // _JEventProcessor_danaevio_
