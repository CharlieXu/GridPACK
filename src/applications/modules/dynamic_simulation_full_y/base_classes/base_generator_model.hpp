/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   base_generator_model.hpp
 * @author Bruce Palmer
 * @Last modified:   May 18, 2015
 * 
 * @brief  
 * 
 * 
 */

#ifndef _base_generator_model_h_
#define _base_generator_model_h_

#include "boost/smart_ptr/shared_ptr.hpp"
#include "gridpack/include/gridpack.hpp"
#include "base_governor_model.hpp"
#include "base_exciter_model.hpp"
#include "base_relay_model.hpp" //renke add

namespace gridpack {
namespace dynamic_simulation {
class BaseGeneratorModel
{
  public:
    /**
     * Basic constructor
     */
    BaseGeneratorModel();

    /**
     * Basic destructor
     */
    virtual ~BaseGeneratorModel();

    /**
     * Load parameters from DataCollection object into generator model
     * @param data collection of generator parameters from input files
     * @param index of generator on bus
     * TODO: might want to move this functionality to BaseGeneratorModel
     */
    virtual void load(boost::shared_ptr<gridpack::component::DataCollection>
        data, int idx);

    /**
     * Initialize generator model before calculation
     * @param mag voltage magnitude
     * @param ang voltage angle
     * @param ts time step
     */
    virtual void init(double mag, double ang, double ts);

    /**
     * Return contribution to Norton current
     * @return contribution to Norton vector
     */
    virtual gridpack::ComplexType INorton();

    /**
     * Return Norton impedence
     * @return value of Norton impedence
     */
    virtual gridpack::ComplexType NortonImpedence();

    /**
     * Predict new state variables for time step
     * @param flag initial step if true
     */
    virtual void predictor_currentInjection(bool flag);

    /**
     * Correct state variables for time step
     * @param flag initial step if true
     */
    virtual void corrector_currentInjection(bool flag);

    /**
     * Predict new state variables for time step
     * @param t_inc time step increment
     * @param flag initial step if true
     */
    virtual void predictor(double t_inc, bool flag);

    /**
     * Correct state variables for time step
     * @param t_inc time step increment
     * @param flag initial step if true
     */
    virtual void corrector(double t_inc, bool flag);

    /**
     * Set voltage on each generator
     */
    virtual void setVoltage(gridpack::ComplexType voltage);

    /**
     * Write output from generators to a string.
     * @param string (output) string with information to be printed out
     * @param bufsize size of string buffer in bytes
     * @param signal an optional character string to signal to this
     * routine what about kind of information to write
     * @return true if bus is contributing string to output, false otherwise
     */
    virtual bool serialWrite(char *string, const int bufsize,
        const char *signal);

    /** 
     * Get the value of the field voltage parameter
     * @return value of field voltage
     */
    virtual double getFieldVoltage();

    virtual double getAngle();

    /**
     * Write out generator state
     * @param signal character string used to determine behavior
     * @param string buffer that contains output
     */
    virtual void write(const char* signal, char* string);

    void setGovernor(boost::shared_ptr<BaseGovernorModel> &p_governor);

    void setExciter(boost::shared_ptr<BaseExciterModel> &p_exciter);
	
    void AddRelay(boost::shared_ptr<BaseRelayModel> &p_relay);  // renke add, add relay
    void ClearRelay();  // renke add, clear relay vector

    boost::shared_ptr<BaseGovernorModel> getGovernor();

    boost::shared_ptr<BaseExciterModel> getExciter();
	
    boost::shared_ptr<BaseRelayModel> getRelay( int iRelay); //renke add
    void getRelayNumber( int &nrelay ); //renke add

    void setWatch(bool flag);

    bool getWatch();
	bool getGenStatus(); //return the bolean indicate whether the gen is tripped by a relay
	void SetGenServiceStatus (bool sta); // set the sta to be false if gen is tripped by a relay

  private:

    bool p_hasExciter;
    bool p_hasGovernor;
    boost::shared_ptr<BaseGovernorModel> p_governor;
    boost::shared_ptr<BaseExciterModel> p_exciter;
    bool p_watch;
	bool bStatus;
    std::vector< boost::shared_ptr<BaseRelayModel> > vp_relay;  //renke add, relay vector

};
}  // dynamic_simulation
}  // gridpack
#endif
