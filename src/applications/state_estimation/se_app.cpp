/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   se_app.cpp
 * @author Yousu Chen 
 * @date   2/24/2014
 * Last updated: 8/5/2014 
 *
 * @brief
 *
 *
 */
// -------------------------------------------------------------

#include "gridpack/math/matrix.hpp"
#include "gridpack/math/vector.hpp"
#include "gridpack/math/linear_solver.hpp"
#include "gridpack/math/linear_matrix_solver.hpp"
#include "gridpack/applications/state_estimation/se_app.hpp"
#include "gridpack/parser/PTI23_parser.hpp"
#include "gridpack/configuration/configuration.hpp"
#include "gridpack/mapper/bus_vector_map.hpp"
#include "gridpack/mapper/full_map.hpp"
#include "gridpack/serial_io/serial_io.hpp"
#include "gridpack/applications/state_estimation/se_factory.hpp"

// Calling program for state estimation application

/**
 * Basic constructor
 */
gridpack::state_estimation::SEApp::SEApp(void)
{
}

/**
 * Basic destructor
 */
gridpack::state_estimation::SEApp::~SEApp(void)
{
}

/**
 * Get list of measurements from external file
 * @param cursor pointer to contingencies in input deck
 * @return vector of measurements
 */
std::vector<gridpack::state_estimation::Measurement>
  gridpack::state_estimation::SEApp::getMeasurements(
      gridpack::utility::Configuration::ChildCursors measurements)
{
  std::vector<gridpack::state_estimation::Measurement> ret;
  int size = measurements.size();
  int idx;
  for (idx = 0; idx < size; idx++) {
    std::string meas_type;
    measurements[idx]->get("Type", &meas_type);
    double meas_value;
    measurements[idx]->get("Value", &meas_value);
    double meas_deviation;
    measurements[idx]->get("Deviation", &meas_deviation);
    if (meas_type == "VM" || meas_type == "PI" || meas_type == "PJ" || meas_type == "QI" || meas_type == "QJ" || meas_type == "VA") {
      int busid;
      measurements[idx]->get("Bus", &busid);
      gridpack::state_estimation::Measurement measurement;
      measurement.p_type = meas_type;
      measurement.p_busid = busid;
      measurement.p_value = meas_value;
      measurement.p_deviation = meas_deviation;
      //printf("%s %d %f %f\n", measurement.p_type.c_str(), measurement.p_busid, measurement.p_value, measurement.p_deviation);
      ret.push_back(measurement); 
    } else if (meas_type == "PIJ" || meas_type == "PJI" || meas_type == "QIJ" || meas_type == "QJI" || meas_type == "IIJ" || meas_type == "IJI") {
      int fbusid;
      measurements[idx]->get("FromBus", &fbusid);
      int tbusid;
      measurements[idx]->get("ToBus", &tbusid);
      std::string ckt;
      measurements[idx]->get("CKT", &ckt);
      gridpack::state_estimation::Measurement measurement;
      measurement.p_type = meas_type;
      measurement.p_fbusid = fbusid;
      measurement.p_tbusid = tbusid;
      measurement.p_ckt = ckt;
      measurement.p_value = meas_value;
      measurement.p_deviation = meas_deviation;
      //printf("%s %d %d %s %f %f\n", measurement.p_type.c_str(), measurement.p_fbusid, measurement.p_tbusid, measurement.p_ckt.c_str(), measurement.p_value, measurement.p_deviation);
      ret.push_back(measurement);
    } 
  }
  return ret;
}

/**
 * Execute application
 */
void gridpack::state_estimation::SEApp::execute(int argc, char** argv)
{
  gridpack::parallel::Communicator world;
  boost::shared_ptr<SENetwork> network(new SENetwork(world));

  // read configuration file
  gridpack::utility::Configuration *config = gridpack::utility::Configuration::configuration();
  if (argc >= 2 && argv[1] != NULL) {
    char inputfile[256];
    sprintf(inputfile,"%s",argv[1]);
    config->open(inputfile,world);
  } else {
    config->open("input.xml",world);
  }
  gridpack::utility::Configuration::CursorPtr cursor;
  cursor = config->getCursor("Configuration.State_estimation");
  std::string filename;
  if (!cursor->get("networkConfiguration",&filename)) {
     printf("No network configuration specified\n");
     return;
  }

  ///////////////////////////////////////////////////////////////////
  // Read in measurement file
  std::string measurementfile;
  if (!cursor->get("measurementList", &measurementfile)) {
    measurementfile = "IEEE14_meas.xml";
  }
  bool ok = config->open(measurementfile, world);

  // get a list of measurements
  cursor = config->getCursor("Measurements");
  gridpack::utility::Configuration::ChildCursors measurements;
  if (cursor) cursor->children(measurements);
  std::vector<gridpack::state_estimation::Measurement>
    meas = getMeasurements(measurements);

  if (world.rank() == 0) {
    int idx;
    for (idx = 0; idx < meas.size(); idx++) {
      std::string meas_type = meas[idx].p_type;
      if (meas_type == "VM" || meas_type == "PI" || meas_type == "PJ") {
        printf("Type: %s\n", meas[idx].p_type.c_str());
        printf("Bus: %d\n", meas[idx].p_busid);
        printf("Value: %f\n", meas[idx].p_value);
        printf("Deviation: %f\n", meas[idx].p_deviation);
      } else if (meas_type == "PIJ") {
        printf("Type: %s\n", meas[idx].p_type.c_str());
        printf("FromBus: %d\n", meas[idx].p_fbusid);
        printf("ToBus: %d\n", meas[idx].p_tbusid);
        printf("CKT: %s\n", meas[idx].p_ckt.c_str());
        printf("Value: %f\n", meas[idx].p_value);
        printf("Deviation: %f\n", meas[idx].p_deviation);
      }
      printf("\n");
    }
  }   
  ///////////////////////////////////////////////////////////////////

  // load input file
  gridpack::parser::PTI23_parser<SENetwork> parser(network);
  parser.parse(filename.c_str());

  // partition network
  network->partition();

  // Create serial IO object to export data from buses or branches
  gridpack::serial_io::SerialBusIO<SENetwork> busIO(128, network);
  gridpack::serial_io::SerialBranchIO<SENetwork> branchIO(128, network);
  char ioBuf[128];

  // create factory
  gridpack::state_estimation::SEFactory factory(network);
  factory.load();

  // set network components using factory
  factory.setComponents();

  // Set up bus data exchange buffers. Need to decide what data needs to be exchanged
  factory.setExchange();

  // Create bus data exchange
  network->initBusUpdate();

  // set YBus components so that you can create Y matrix  
  factory.setYBus();

  factory.setMode(YBus);
  gridpack::mapper::FullMatrixMap<SENetwork> ybusMap(network);
  boost::shared_ptr<gridpack::math::Matrix> ybus = ybusMap.mapToMatrix();
  branchIO.header("\nybus:\n");
  ybus->print();

  // Start N-R loop


    // Form estimation vector


    // Build measurement equation

  
    // Build gain matrix 


    // Form right hand side vector


    // Solve linear equation

  
    // update values


  // End N-R loop


  // Output 

}

