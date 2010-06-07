// $Id$
//
//    File: DLorentzVector.h
//
// This header file is a replacement of a subset of TLorentzVector intended to 
// be consistent with the SIMDized version DVector3. 
//

#ifndef _DLorentzVector_
#define _DLorentzVector_
#include "DVector3.h"
#include <math.h>
#include <emmintrin.h>
#include <iostream>
using namespace std;

class DLorentzVector{
 public:
  DLorentzVector(){
    mP.SetXYZ(0.,0.,0.);
    mE=0.;
  };
  DLorentzVector(const double x,const double y,const double z,const double t){
    mP.SetXYZ(x,y,z);
    mE=t;
  };
  DLorentzVector(const DVector3 &v, const double t){
    mP=v;
    mE=t;
  };
  ~DLorentzVector(){};
  void SetXYZT(const double x,const double y,const double z,const double t){
    mP.SetXYZ(x,y,z);
    mE=t;
  }
  // Set the 3-momentum or position part of the 4-vector
  void SetVect(const DVector3 &p){
    mP=p;
  }
  // Set the time or energy component
  void SetT(const double t){ mE=t;};
  
  // Routines to get position and time
  double X() const {return mP.x();};
  double Y() const {return mP.y();};
  double Z() const {return mP.z();}; 
  double T() const {return mE;};

  // Routine to get full 3-vector;
  DVector3 Vect() const {return mP;};

  // Routines to get momentum and energy
  double Px() const {return mP.x();};
  double Py() const {return mP.y();};
  double Pz() const {return mP.z();}; 
  double P() const {return mP.Mag();};
  double E() const {return mE;};
  double Energy() const {return mE;};

  // Spherical coordinates of spatial component
  double Rho() const { return mP.Mag();};

  // Kinematical quantities 
  double Beta() const { return P()/E();};
  double Mag2() const {return mE*mE-mP.Mag2();};
  double M() const{
    double mm = Mag2();
    return mm < 0.0 ? -sqrt(-mm) : sqrt(mm);
  }

  void Print() const{
    cout << "DLorentzVector (x,y,z,t)=(" << X() << "," << Y() << "," << Z()
	 << "," << T() << ")" << endl;

  };

 private:
  DVector3 mP;  // momentum or position vector
  double mE;  // Energy or time component
};

// Addition 
inline DLorentzVector operator+(const DLorentzVector &v1,const DLorentzVector &v2){
  return DLorentzVector(v1.Vect()+v2.Vect(),v1.E()+v2.E());
}


#endif // _DLorentzVector_

