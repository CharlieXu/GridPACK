// -------------------------------------------------------------
/**
 * @file   pf_components.cpp
 * @author Bruce Palmer
 * @date   2013-09-23 07:01:53 d3g096
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------

#include <vector>
#include <iostream.h>

#include "boost/smart_ptr/shared_ptr.hpp"
#include "gridpack/utilities/complex.hpp"
#include "gridpack/component/base_component.hpp"
#include "gridpack/component/data_collection.hpp"
#include "gridpack/applications/powerflow/pf_components.hpp"
#include "gridpack/parser/dictionary.hpp"

/**
 *  Simple constructor
 */
gridpack::powerflow::PFBus::PFBus(void)
{
  p_shunt_gs = 0.0;
  p_shunt_bs = 0.0;
  p_mode = YBus;
  setReferenceBus(false);
}

/**
 *  Simple destructor
 */
gridpack::powerflow::PFBus::~PFBus(void)
{
}

/**
 *  Return size of matrix block contributed by the component
 *  @param isize, jsize: number of rows and columns of matrix block
 *  @return: false if network component does not contribute matrix element
 */
bool gridpack::powerflow::PFBus::matrixDiagSize(int *isize, int *jsize) const
{
  if (p_mode == Jacobian && getReferenceBus()) {
    *isize = 2;
    *jsize = 2;
    return true;
  } else {
    *isize = 2;
    *jsize = 2;
  }
  return true;
}

/**
 * Return the values of the matrix block. The values are
 * returned in row-major order.
 * @param values: pointer to matrix block values
 * @return: false if network component does not contribute matrix element
 */
bool gridpack::powerflow::PFBus::matrixDiagValues(ComplexType *values)
{
  if (p_mode == YBus) {
    gridpack::ComplexType ret(p_ybusr,p_ybusi);
    values[0] = p_ybusr;
    values[1] = p_ybusi;
    values[2] = -p_ybusi;
    values[3] = p_ybusr;
    return true;
  } else if (p_mode == Jacobian) {
    if (!getReferenceBus()) {
      /*      double branch_values[4];
      // TODO: More stuff here
      std::vector<boost::shared_ptr<BaseComponent> > branches;
      getNeighborBranches(branches);
      int size = branches.size();
      int i;
      values[0] = 0.0;
      values[1] = 0.0;
      values[2] = 2.0*p_v*p_ybusr;
      values[3] = -2.0*p_v*p_ybusi;
      //printf("p_v = %f\n", p_v);
      for (i=0; i<size; i++) {
      (dynamic_cast<gridpack::powerflow::PFBranch*>(branches[i].get()))->
      getJacobian(this, branch_values);
      values[0] -= p_v*branch_values[0];
      values[1] += p_v*(-branch_values[1]); //correct diagonal value 
      values[2] += branch_values[2];
      values[3] += branch_values[3];
      } */ // use simple equations below to get diagonal values. YChen 9/29/2013
      values[0] = -p_Qinj - p_ybusi * p_v *p_v; 
      values[1] = p_Pinj - p_ybusr * p_v *p_v; 
      values[2] = p_Pinj / p_v + p_ybusr * p_v; 
      values[3] = p_Qinj / p_v - p_ybusi * p_v; 
      // Fix up matrix elements if bus is PV bus
      if (p_isPV) {
        values[1] = 0.0;
        values[2] = 0.0;
        values[3] = 1.0;
      }
    } else {
      values[0] = 1.0;
      values[1] = 0.0;
      values[2] = 0.0;
      values[3] = 1.0;
      return true;
    }
  }
}

/**
 * Return the size of the block that this component contributes to the
 * vector
 * @param size: size of vector block
 * @return: false if component does not contribute to vector
 */
bool gridpack::powerflow::PFBus::vectorSize(int *size) const
{
  if (p_mode == Jacobian && getReferenceBus()) {
    *size = 2;
    return true;
  } else {
    *size = 2;
  }
  return true;
}

/**
 * Return the values of the vector block
 * @param values: pointer to vector values
 * @return: false if network component does not contribute
 *        vector element
 */
bool gridpack::powerflow::PFBus::vectorValues(ComplexType *values)
{
  if (p_mode == S_Cal)  {
    double pi = 4.0*atan(1.0);
    values[0] = p_v * cos(p_a);
    values[1] = p_v * sin(p_a);
    printf ("p_v = %f, p_a = %f\n", p_v, p_a*pi/180.0);
    return true;
  }
  if (p_mode == RHS) {
    if (!getReferenceBus()) {
      std::vector<boost::shared_ptr<BaseComponent> > branches;
      getNeighborBranches(branches);
      int size = branches.size();
      int i;
      double P, Q, p, q;
      P = 0.0;
      Q = 0.0;
      for (i=0; i<size; i++) {
        gridpack::powerflow::PFBranch *branch
          = dynamic_cast<gridpack::powerflow::PFBranch*>(branches[i].get());
        branch->getPQ(this, &p, &q);
        P += p;
        Q += q;
        //printf("i=%d:p=%f, q=%f, P=%f, Q=%f\n", i,p,q,P,Q);
      }
      //printf("p_P0=%f,p_Q0=%f\n\n", p_P0,p_Q0);
      // Also add bus i's own Pi, Qi
      P += p_v*p_v*p_ybusr;
      Q += p_v*p_v*(-p_ybusi);
      p_Pinj = P;
      p_Qinj = Q;
      //printf("p = %f, q = %f\n", p_voltage*p_voltage*p_ybusr, p_voltage*p_voltage*(-p_ybusi));
      P -= p_P0;
      Q -= p_Q0;
      values[0] = P;
      if (!p_isPV) {
        values[1] = Q;
      } else {
        values[1] = 0.0;
      }
      return true;
    } else {
      values[0] = 0.0;
      values[1] = 0.0;
    }
  }
  /*  if (p_mode == Jacobian) {
    std::vector<boost::shared_ptr<BaseComponent> > branches;
    getNeighborBranches(branches);
    int size = branches.size();
    int i;
    double P, Q, p, q;
    P = 0.0;
    Q = 0.0;
    for (i=0; i<size; i++) {
      gridpack::powerflow::PFBranch *branch
        = dynamic_cast<gridpack::powerflow::PFBranch*>(branches[i].get());
      branch->getPQ(this, &p, &q);
      P += p;
      Q += q;
    }
    P -= p_P0;
    Q -= p_Q0;
    values[0] = P;
    values[1] = Q;
    return true;
  } */
}

/**
 * Set the internal values of the voltage magnitude and phase angle. Need this
 * function to push values from vectors back onto buses 
 * @param values array containing voltage magnitude and angle
 */
void gridpack::powerflow::PFBus::setValues(gridpack::ComplexType *values)
{
  p_a -= real(values[0]);
  p_v -= real(values[1]);
//  *p_vAng_ptr = *p_vAng_ptr - real(values[0]);
//  *p_vMag_ptr = *p_vMag_ptr - real(values[1]);
  *p_vAng_ptr = p_a;
  *p_vMag_ptr = p_v;
  printf("da: %12.6f dv: %12.6f  p_a: %12.6f p_v: %12.6f\n",
      real(values[0]),real(values[1]),p_a,p_v);
}

/**
 * Return the size of the buffer used in data exchanges on the network.
 * For this problem, the voltage magnitude and phase angle need to be exchanged
 * @return size of buffer
 */
int gridpack::powerflow::PFBus::getXCBufSize(void)
{
  return 2*sizeof(double);
}

/**
 * Assign pointers for voltage magnitude and phase angle
 */
void gridpack::powerflow::PFBus::setXCBuf(void *buf)
{
  p_vAng_ptr = (double*)buf;
  p_vMag_ptr = p_vAng_ptr+1;
  // Note: we are assuming that the load function has been called BEFORE
  // the factory setExchange method, so p_a and p_v are set with their initial
  // values.
  *p_vAng_ptr = p_a;
  *p_vMag_ptr = p_v;
}

void gridpack::powerflow::PFBus::setYBus(void)
{
  gridpack::ComplexType ret(0.0,0.0);
  std::vector<boost::shared_ptr<BaseComponent> > branches;
  getNeighborBranches(branches);
  int size = branches.size();
  int i;
  // HACK: Need to cast pointer, is there a better way?
  for (i=0; i<size; i++) {
    gridpack::powerflow::PFBranch *branch
      = dynamic_cast<gridpack::powerflow::PFBranch*>(branches[i].get());
    ret -= branch->getAdmittance();
    //ret += branch->getTransformer(this);
    //ret -= branch->getTransformer(this); //YChen 9-5-2013: Doesn't work for tap ratio not equal to 1.0 cases yet. 
    ret += branch->getShunt(this);
  }
  if (p_shunt) {
    gridpack::ComplexType shunt(p_shunt_gs,p_shunt_bs);
    ret += shunt;
  }
  p_ybusr = real(ret);
  p_ybusi = imag(ret);
}

gridpack::ComplexType gridpack::powerflow::PFBus::getYBus(void)
{
  gridpack::ComplexType ret(p_ybusr,p_ybusi);
  return ret;
}
  
/**
 * Load values stored in DataCollection object into PFBus object. The
 * DataCollection object will have been filled when the network was created
 * from an external configuration file
 * @param data: DataCollection object contain parameters relevant to this
 *       bus that were read in when network was initialized
 */
void gridpack::powerflow::PFBus::load(
    const boost::shared_ptr<gridpack::component::DataCollection> &data)
{
  //p_shunt = p_shunt && data->getValue(CASE_SBASE, &p_sbase);
  //p_shunt = p_shunt && data->getValue(BUS_BASEKV, &p_sbase);
  p_sbase = 100.0;
  //YChen p_sbase is set to 100.0. It needs to be read from parser (the 2nd number in the first line)
  //printf("CASE_SBASE=%f\n",p_sbase);

  data->getValue(BUS_VOLTAGE_ANG, &p_angle);
  data->getValue(BUS_VOLTAGE_MAG, &p_voltage); 
  p_v = p_voltage;
  printf("P_V: %f\n",p_v);
  double pi = 4.0*atan(1.0);
  p_angle = p_angle*pi/180.0;
  p_a = p_angle;
  p_shunt = true;
  p_shunt = p_shunt && data->getValue(BUS_SHUNT_GL, &p_shunt_gs);
  p_shunt = p_shunt && data->getValue(BUS_SHUNT_BL, &p_shunt_bs);
  p_shunt_gs /= p_sbase;
  p_shunt_bs /= p_sbase;
  // TODO: Need to get values of P0 and Q0 from Network Configuration file
  //printf("p_shunt_gs=%f,p_shunt_bs=%f\n",p_shunt_gs,p_shunt_bs);
  // Check to see if bus is reference bus
  int itype;
  data->getValue(BUS_TYPE, &itype);
  if (itype == 3) {
    setReferenceBus(true);
  }

  // if BUS_TYPE = 2 then bus is a PV bus
  p_isPV = false;
  if (itype == 2) p_isPV = true;

  // added p_pg,p_qg,p_pl,p_ql,p_sbase;
  p_load = true;
  p_load = p_load && data->getValue(LOAD_PL, &p_pl);
  p_load = p_load && data->getValue(LOAD_QL, &p_ql);
  //printf("p_pl=%f,p_ql=%f\n",p_pl,p_ql);
  bool lgen;
  int i, ngen, gstatus;
  double pg, qg;
  if (data->getValue(GENERATOR_NUMBER, &ngen)) {
    for (i=0; i<ngen; i++) {
      lgen = true;
      lgen = lgen && data->getValue(GENERATOR_PG, &pg,i);
      lgen = lgen && data->getValue(GENERATOR_QG, &qg,i);
      //printf("ng=%d,pg=%f,qg=%f\n",i,pg,qg);
      lgen = lgen && data->getValue(GENERATOR_STAT, &gstatus,i);
      if (lgen) {
        p_pg.push_back(pg);
        p_qg.push_back(qg);
        p_gstatus.push_back(gstatus);
      }
    }
  }
  //printf("p_gstatus = %d\n", p_gstatus);
  //p_gstatus = 1;

}


/**
 * Set the mode to control what matrices and vectors are built when using
 * the mapper
 * @param mode: enumerated constant for different modes
 */
void gridpack::powerflow::PFBus::setMode(int mode)
{
  p_mode = mode;
}

/**
 * Return the value of the voltage magnitude on this bus
 * @return: voltage magnitude
 */
double gridpack::powerflow::PFBus::getVoltage()
{
  return *p_vMag_ptr;
}

/**
 * Return whether or not the bus is a PV bus (V held fixed in powerflow
 * equations)
 * @return true if bus is PV bus
 */
bool gridpack::powerflow::PFBus::isPV(void)
{
  return p_isPV;
}

/**
 * Return the value of the phase angle on this bus
 * @return: phase angle
 */
double gridpack::powerflow::PFBus::getPhase()
{
  return *p_vAng_ptr;
}

/**
 * setGBus
 */
void gridpack::powerflow::PFBus::setGBus(void)
{
  //if (p_gstatus == 1) 
}

/**
 * setSBus
 BUS = (CG*(GEN(ON,PG) + J*GEN(ON,QG)-(PD+J*QD))/BASEMVA
 */
void gridpack::powerflow::PFBus::setSBus(void)
{
  // need to update later to consider multiple generators located at the same bus 
  // Chen 8_27_2013 (DONE, 9/29/2013)
#if 1
  // TODO: Need to fix this so that is works for more than 1 generator per bus
  int i;
  double pg, qg;
  pg = 0.0;
  qg = 0.0;
  bool usegen = false;
  for (i=0; i<p_gstatus.size(); i++) {
    if (p_gstatus[i] == 1) {
      pg += p_pg[i];
      qg += p_qg[i];
      usegen = true;
    }
  }
  if (p_gstatus.size() > 0 && usegen) {
    gridpack::ComplexType sBus((pg - p_pl) / p_sbase, (qg - p_ql) / p_sbase);
    //p_sbusr = real(sBus);
    //p_sbusr = real(sBus);
    p_P0 = real(sBus);
    p_Q0 = imag(sBus);
    //printf("p_P0=%f, p_Q0=%f\n",p_P0,p_Q0);
  } else {
    gridpack::ComplexType sBus((- p_pl) / p_sbase, (- p_ql) / p_sbase);
    p_P0 = real(sBus);
    p_Q0 = imag(sBus);
  } 
  //printf("p_P0=%f, p_Q0=%f\n",p_P0,p_Q0);
#endif
}

/**
 * Write output from buses to standard out
 * @param string (output) string with information to be printed out
 * @param signal an optional character string to signal to this
 * routine what about kind of information to write
 * @return true if bus is contributing string to output, false otherwise
 */
bool gridpack::powerflow::PFBus::serialWrite(char *string, char *signal)
{
  double pi = 4.0*atan(1.0);
  double angle = p_a*180.0/pi;
  sprintf(string, "     %6d      %12.6f         %12.6f\n",
      getOriginalIndex(),angle,p_v);
  return true;
}

/**
 *  Simple constructor
 */
gridpack::powerflow::PFBranch::PFBranch(void)
{
  p_reactance = 0.0;
  p_resistance = 0.0;
  //p_tap_ratio = 1.0;
  p_phase_shift = 0.0;
  p_charging = 0.0;
  p_shunt_admt_g1 = 0.0;
  p_shunt_admt_b1 = 0.0;
  p_shunt_admt_g2 = 0.0;
  p_shunt_admt_b2 = 0.0;
  p_mode = YBus;
}

/**
 *  Simple destructor
 */
gridpack::powerflow::PFBranch::~PFBranch(void)
{
}

/**
 * Return size of off-diagonal matrix block contributed by the component
 * for the forward/reverse directions
 * @param isize, jsize: number of rows and columns of matrix block
 * @return: false if network component does not contribute matrix element
 */
bool gridpack::powerflow::PFBranch::matrixForwardSize(int *isize, int *jsize) const
{
  if (p_mode == Jacobian) {
    gridpack::powerflow::PFBus *bus1
      = dynamic_cast<gridpack::powerflow::PFBus*>(getBus1().get());
    gridpack::powerflow::PFBus *bus2
      = dynamic_cast<gridpack::powerflow::PFBus*>(getBus2().get());
    bool ok = !bus1->getReferenceBus();
    ok = ok & !bus2->getReferenceBus();
    if (ok) {
      *isize = 2;
      *jsize = 2;
      return true;
    } else {
/*      *isize = 0;
      *jsize = 0;
      return false;
*/    *isize = 2;
      *jsize = 2;
      return true; 
    }
  } else {
    *isize = 2;
    *jsize = 2;
    return true;
  }
}
bool gridpack::powerflow::PFBranch::matrixReverseSize(int *isize, int *jsize) const
{
  if (p_mode == Jacobian) {
    gridpack::powerflow::PFBus *bus1
      = dynamic_cast<gridpack::powerflow::PFBus*>(getBus1().get());
    gridpack::powerflow::PFBus *bus2
      = dynamic_cast<gridpack::powerflow::PFBus*>(getBus2().get());
    bool ok = !bus1->getReferenceBus();
    ok = ok & !bus2->getReferenceBus();
    if (ok) {
      *isize = 2;
      *jsize = 2;
      return true;
    } else {
/*      *isize = 0;
      *jsize = 0;
      return false;
*/    *isize = 2;
      *jsize = 2;
      return true; 
    }
  } else {
    *isize = 2;
    *jsize = 2;
    return true;
  }
}

/**
 * Return the values of the off-diagonal matrix block. The values are
 * returned in row-major order
 * @param values: pointer to matrix block values
 * @return: false if network component does not contribute matrix element
 */
bool gridpack::powerflow::PFBranch::matrixForwardValues(ComplexType *values)
{
  if (p_mode == Jacobian) {
    gridpack::powerflow::PFBus *bus1
      = dynamic_cast<gridpack::powerflow::PFBus*>(getBus1().get());
    gridpack::powerflow::PFBus *bus2
      = dynamic_cast<gridpack::powerflow::PFBus*>(getBus2().get());
    bool ok = !bus1->getReferenceBus();
    ok = ok & !bus2->getReferenceBus();
    if (ok) {
      double t11, t12, t21, t22;
      double cs = cos(p_theta);
      double sn = sin(p_theta);
      values[0] = (p_ybusr*sn - p_ybusi*cs);
      values[1] = (p_ybusr*cs + p_ybusi*sn);
      values[2] = (p_ybusr*cs + p_ybusi*sn);
      values[3] = (p_ybusr*sn - p_ybusi*cs);
      values[0] *= ((bus1->getVoltage())*(bus2->getVoltage()));
      values[1] *= -((bus1->getVoltage())*(bus2->getVoltage()));
      values[2] *= bus1->getVoltage();
      values[3] *= bus1->getVoltage();
      // fix up matrix if one or both buses at the end of the branch is a PV bus
      bool bus1PV = bus1->isPV();
      bool bus2PV = bus2->isPV();
      if (bus1PV & bus2PV) {
        values[1] = 0.0;
        values[2] = 0.0;
        values[3] = 0.0;
      } else if (bus1PV) {
        values[1] = 0.0;
        values[3] = 0.0;
      } else if (bus2PV) {
        values[2] = 0.0;
        values[3] = 0.0;
      }
      return true;
    } else {
      return false;
    }
  } else {
    values[0] = p_ybusr;
    values[1] = p_ybusi;
    values[2] = -p_ybusi;
    values[3] = p_ybusr;
    return true;
  }
}

bool gridpack::powerflow::PFBranch::matrixReverseValues(ComplexType *values)
{
  if (p_mode == Jacobian) {
    gridpack::powerflow::PFBus *bus1
      = dynamic_cast<gridpack::powerflow::PFBus*>(getBus1().get());
    gridpack::powerflow::PFBus *bus2
      = dynamic_cast<gridpack::powerflow::PFBus*>(getBus2().get());
    bool ok = !bus1->getReferenceBus();
    ok = ok & !bus2->getReferenceBus();
    if (ok) {
      double t11, t12, t21, t22;
      double cs = cos(-p_theta);
      double sn = sin(-p_theta);
      values[0] = (p_ybusr*sn - p_ybusi*cs);
      values[1] = (p_ybusr*cs + p_ybusi*sn);
      values[2] = (p_ybusr*cs + p_ybusi*sn);
      values[3] = (p_ybusr*sn - p_ybusi*cs);
      values[0] *= ((bus1->getVoltage())*(bus2->getVoltage()));
      values[1] *= -((bus1->getVoltage())*(bus2->getVoltage()));
      values[2] *= bus2->getVoltage();
      values[3] *= bus2->getVoltage();
      // fix up matrix if one or both buses at the end of the branch is a PV bus
      bool bus1PV = bus1->isPV();
      bool bus2PV = bus2->isPV();
      if (bus1PV & bus2PV) {
        values[1] = 0.0;
        values[2] = 0.0;
        values[3] = 0.0;
      } else if (bus1PV) {
        values[2] = 0.0;
        values[3] = 0.0;
      } else if (bus2PV) {
        values[1] = 0.0;
        values[3] = 0.0;
      }
      return true;
    } else {
      return false;
    }
  } else {
    values[0] = p_ybusr;
    values[1] = p_ybusi;
    values[2] = -p_ybusi;
    values[3] = p_ybusr;
    return true;
  }
}

// Calculate contributions to the admittance matrix from the branches
void gridpack::powerflow::PFBranch::setYBus(void)
{
  gridpack::ComplexType ret(p_resistance,p_reactance);
  ret = -1.0/ret;
  gridpack::ComplexType a(cos(p_phase_shift),sin(p_phase_shift));
  a = p_tap_ratio*a;
  //if (a == static_cast<ComplexType>(0.0)) {
  //printf("p_tap_ratio = %f\n", p_tap_ratio);
  //if (p_tap_ratio != 0.0) {
  /*if (p_xform) {
    //ret = ret - ret/conj(a);
    ret = - ret/conj(a); // YChen 9-5-2013: This is for the from end, need to have another one for to end. Equation is ret = - ret/a;
  }*/ 
  //printf("imag(ret)=%lf\n", imag(ret));
  p_ybusr = real(ret);
  p_ybusi = imag(ret);
  // Not really a contribution to the admittance matrix but might as well
  // calculate phase angle difference between buses at each end of branch
  gridpack::powerflow::PFBus *bus1 =
    dynamic_cast<gridpack::powerflow::PFBus*>(getBus1().get());
  gridpack::powerflow::PFBus *bus2 =
    dynamic_cast<gridpack::powerflow::PFBus*>(getBus2().get());
  //p_theta = bus1->getPhase() - bus2->getPhase();
  double pi = 4.0*atan(1.0);
  p_theta = (bus1->getPhase() - bus2->getPhase());
  //printf("p_phase_shift: %12.6f\n",p_phase_shift);
  //printf("p_theta: %12.6f\n",p_theta);
  //printf("p_tap_ratio: %12.6f\n",p_tap_ratio);

}

/**
 * Load values stored in DataCollection object into PFBranch object. The
 * DataCollection object will have been filled when the network was created
 * from an external configuration file
 * @param data: DataCollection object contain parameters relevant to this
 *       branch that were read in when network was initialized
 */
void gridpack::powerflow::PFBranch::load(
    const boost::shared_ptr<gridpack::component::DataCollection> &data)
{
  bool ok = true;
  ok = ok && data->getValue(BRANCH_X, &p_reactance);
  ok = ok && data->getValue(BRANCH_R, &p_resistance);
  ok = ok && data->getValue(BRANCH_SHIFT, &p_phase_shift);
  double temp;
  ok = ok && data->getValue(BRANCH_TAP, &temp);
  //printf("temp=%f\n", temp);
  if (temp != 0.0) {
    p_tap_ratio = temp; 
  //if (!ok) {
    p_xform = true;
    //p_xform = p_xform && data->getValue(TRANSFORMER_X1_2, &p_reactance);
    //p_xform = p_xform && data->getValue(TRANSFORMER_R1_2, &p_resistance);
    p_xform = p_xform && data->getValue(BRANCH_X, &p_reactance);
    p_xform = p_xform && data->getValue(BRANCH_R, &p_resistance);
    //printf("p_reactance = %f, p_resistance = %f\n", p_reactance, p_resistance);
  } else {
    p_xform = false;
  }
//  p_xform = p_xform && data->getValue(TRANSFORMER_WINDV1, &p_tap_ratio);
//  p_xform = p_xform && data->getValue(TRANSFORMER_ANG1, &p_phase_shift);
  p_shunt = true;
  p_shunt = p_shunt && data->getValue(BRANCH_B, &p_charging);
  p_shunt = p_shunt && data->getValue(BRANCH_SHUNT_ADMTTNC_G1, &p_shunt_admt_g1);
  p_shunt = p_shunt && data->getValue(BRANCH_SHUNT_ADMTTNC_B1, &p_shunt_admt_b1);
  p_shunt = p_shunt && data->getValue(BRANCH_SHUNT_ADMTTNC_G2, &p_shunt_admt_g2);
  p_shunt = p_shunt && data->getValue(BRANCH_SHUNT_ADMTTNC_B2, &p_shunt_admt_b2);
}

/**
 * Set the mode to control what matrices and vectors are built when using
 * the mapper
 * @param mode: enumerated constant for different modes
 */
void gridpack::powerflow::PFBranch::setMode(int mode)
{
  p_mode = mode;
}

/**
 * Return the complex admittance of the branch
 * @return: complex addmittance of branch
 */
gridpack::ComplexType gridpack::powerflow::PFBranch::getAdmittance(void)
{
  gridpack::ComplexType ret(p_resistance, p_reactance);
  return -1.0/ret;
}

/**
 * Return transformer contribution from the branch to the calling
 * bus
 * @param bus: pointer to the bus making the call
 * @return: contribution to Y matrix from branch
 */
gridpack::ComplexType
gridpack::powerflow::PFBranch::getTransformer(gridpack::powerflow::PFBus *bus)
{
  /*if (p_xform) {
    gridpack::ComplexType ret(p_resistance,p_reactance);
    ret = -1.0/ret;
    // HACK: pointer comparison, maybe could handle this better
    if (bus == getBus1().get()) {
      printf("p_xform is on, p_tap_ratio= %f, bus = %d\n", p_tap_ratio, bus);
      ret = ret/(p_tap_ratio*p_tap_ratio);
    } else if (bus == getBus2().get()) {
      // No further action required
    } else {
      // TODO: Some kind of error
    }
    return ret;
  } else {*/
    printf("p_xform is off\n");
    gridpack::ComplexType ret(0.0,0.0);
    return ret;
  //}
}

/**
 * Return the contribution to a bus from shunts
 * @param bus: pointer to the bus making the call
 * @return: contribution to Y matrix from shunts associated with branches
 */
gridpack::ComplexType
gridpack::powerflow::PFBranch::getShunt(gridpack::powerflow::PFBus *bus)
{
  double retr, reti;
  if (p_shunt) {
    //retr = 0.5*p_charging;
    //reti = 0.0;
    reti = 0.5*p_charging;
    retr = 0.0;
    // HACK: pointer comparison, maybe could handle this better
    if (bus == getBus1().get()) {
      retr += p_shunt_admt_g1;
      reti += p_shunt_admt_b1;
    } else if (bus == getBus2().get()) {
      retr += p_shunt_admt_g2;
      reti += p_shunt_admt_b2;
    } else {
      // TODO: Some kind of error
    }
  } else {
    retr = 0.0;
    reti = 0.0;
  }
  return gridpack::ComplexType(retr,reti);
}

/**
 * Return the contribution to the Jacobian for the powerflow equations from
 * a branch
 * @param bus: pointer to the bus making the call
 * @param values: an array of 4 doubles that holds return metrix elements
 */
void gridpack::powerflow::PFBranch::getJacobian(gridpack::powerflow::PFBus *bus, double *values)
{
  double v;
  double cs, sn;
  if (bus == getBus1().get()) {
    gridpack::powerflow::PFBus *bus2 =
      dynamic_cast<gridpack::powerflow::PFBus*>(getBus2().get());
    v = bus2->getVoltage();
    cs = cos(p_theta);
    sn = sin(p_theta);
  } else if (bus == getBus2().get()) {
    gridpack::powerflow::PFBus *bus1 =
      dynamic_cast<gridpack::powerflow::PFBus*>(getBus1().get());
    v = bus1->getVoltage();
    cs = cos(-p_theta);
    sn = sin(-p_theta);
  } else {
    // TODO: Some kind of error
  }
  values[0] = v*(p_ybusr*sn - p_ybusi*cs);
  values[1] = -v*(p_ybusr*cs + p_ybusi*sn);
  values[2] = (p_ybusr*cs + p_ybusi*sn);
  values[3] = (p_ybusr*sn - p_ybusi*cs);
}

/**
 * Return contribution to constraints
 * @param p: real part of constraint
 * @param q: imaginary part of constraint
 */
void gridpack::powerflow::PFBranch::getPQ(gridpack::powerflow::PFBus *bus, double *p, double *q)
{
  double cs, sn;
  if (bus == getBus1().get()) {
    cs = cos(p_theta);
    sn = sin(p_theta);
  } else if (bus == getBus2().get()) {
    cs = cos(-p_theta);
    sn = sin(-p_theta);
  } else {
    // TODO: Some kind of error
  }
  gridpack::powerflow::PFBus *bus1 = 
    dynamic_cast<gridpack::powerflow::PFBus*>(getBus1().get());
  double v1 = bus1->getVoltage();
  gridpack::powerflow::PFBus *bus2 =
    dynamic_cast<gridpack::powerflow::PFBus*>(getBus2().get());
  double v2 = bus2->getVoltage();
  //*p = -v1*v2*(p_ybusr*cs-p_ybusi*sn);
  //*q = v1*v2*(p_ybusr*sn+p_ybusi*cs);
  *p = v1*v2*(p_ybusr*cs+p_ybusi*sn);
  *q = v1*v2*(p_ybusr*sn-p_ybusi*cs);
//  printf("v1=%f, v2=%f, cs=%f, sn=%f, p_ybusr=%f, p_ybusi=%f\n", v1,v2,cs,sn,p_ybusr,p_ybusi);
//  printf("*p=%f,*q=%f\n",*p,*q);
}
