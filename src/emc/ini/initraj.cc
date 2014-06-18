/********************************************************************
* Description: initraj.cc
*   INI file initialization for trajectory level
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author:
* License: GPL Version 2
* System: Linux
*    
* Copyright (c) 2004 All rights reserved.
*
* Last change:
********************************************************************/

#include <stdio.h>		// NULL
#include <stdlib.h>		// atol()
#include <string.h>		// strlen()
#include <ctype.h>		// isspace()

#include "emc.hh"
#include "emcpos.h"             // EmcPose
#include "rcs_print.hh"
#include "posemath.h"		// PM_POSE, PM_RPY
#include "emcIniFile.hh"
#include "initraj.hh"		// these decls
#include "emcglb.h"		/*! \todo TRAVERSE_RATE (FIXME) */
#include "inihal.hh"

extern value_inihal_data old_inihal_data;

static int loadKins(EmcIniFile *trajInifile)
{
    trajInifile->EnableExceptions(EmcIniFile::ERR_CONVERSION);

    try {
	int joints = 0;
	trajInifile->Find(&joints, "JOINTS", "KINS");

        if (0 != emcTrajSetJoints(joints)) {
            if (EMC_DEBUG & EMC_DEBUG_CONFIG) {
                rcs_print("bad return value from emcTrajSetJoints\n");
            }
            return -1;
        }
    }
   
    catch (EmcIniFile::Exception &e) {
        e.Print();
        return -1;
    }
    
    return 0;
}


/*
  loadTraj()

  Loads ini file params for traj

  AXES <int>                    number of axes in system
  LINEAR_UNITS <float>          units per mm
  ANGULAR_UNITS <float>         units per degree
  CYCLE_TIME <float>            cycle time for traj calculations
  DEFAULT_VELOCITY <float>      default velocity
  MAX_VELOCITY <float>          max velocity
  MAX_ACCELERATION <float>      max acceleration
  DEFAULT_ACCELERATION <float>  default acceleration
  HOME <float> ...              world coords of home, in X Y Z R P W

  calls:

  emcTrajSetJoints(int joints);
  emcTrajSetUnits(double linearUnits, double angularUnits);
  emcTrajSetCycleTime(double cycleTime);
  emcTrajSetVelocity(double vel);
  emcTrajSetAcceleration(double acc);
  emcTrajSetMaxVelocity(double vel);
  emcTrajSetMaxAcceleration(double acc);
  emcTrajSetHome(EmcPose home);
  */

static int loadTraj(EmcIniFile *trajInifile)
{
    const char *inistring;
    EmcLinearUnits linearUnits;
    EmcAngularUnits angularUnits;
    double vel;
    double acc;
    int t;
    int len;
    double d;

    trajInifile->EnableExceptions(EmcIniFile::ERR_CONVERSION);

    try{
	int axes = 0;
	int axismask = 0;
	const char *coord = trajInifile->Find("COORDINATES", "TRAJ");
	if(coord) {
	    if(strchr(coord, 'x') || strchr(coord, 'X')) axismask |= 1;
	    if(strchr(coord, 'y') || strchr(coord, 'Y')) axismask |= 2;
	    if(strchr(coord, 'z') || strchr(coord, 'Z')) axismask |= 4;
	    if(strchr(coord, 'a') || strchr(coord, 'A')) axismask |= 8;
	    if(strchr(coord, 'b') || strchr(coord, 'B')) axismask |= 16;
	    if(strchr(coord, 'c') || strchr(coord, 'C')) axismask |= 32;
	    if(strchr(coord, 'u') || strchr(coord, 'U')) axismask |= 64;
	    if(strchr(coord, 'v') || strchr(coord, 'V')) axismask |= 128;
	    if(strchr(coord, 'w') || strchr(coord, 'W')) axismask |= 256;
	} else {
	    axismask = 1 | 2 | 4;		// default: XYZ machine
	}
	trajInifile->Find(&axes, "AXES", "TRAJ");
        if (0 != emcTrajSetAxes(axes, axismask)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                rcs_print("bad return value from emcTrajSetAxes\n");
            }
            return -1;
        }

        linearUnits = 0;
        trajInifile->FindLinearUnits(&linearUnits, "LINEAR_UNITS", "TRAJ");
        angularUnits = 0;
        trajInifile->FindAngularUnits(&angularUnits, "ANGULAR_UNITS", "TRAJ");
        if (0 != emcTrajSetUnits(linearUnits, angularUnits)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                rcs_print("bad return value from emcTrajSetUnits\n");
            }
            return -1;
        }

        vel = 1.0;
        trajInifile->Find(&vel, "DEFAULT_LINEAR_VELOCITY", "TRAJ");
        // set the corresponding global
        traj_default_velocity = vel;
        old_inihal_data.traj_default_velocity = vel;

        // and set dynamic value
        if (0 != emcTrajSetVelocity(0, vel)) { //default velocity on startup 0
            if (emc_debug & EMC_DEBUG_CONFIG) {
                rcs_print("bad return value from emcTrajSetVelocity\n");
            }
            return -1;
        }

        vel = 1e99; // by default, use AXIS limit
        trajInifile->Find(&vel, "MAX_LINEAR_VELOCITY", "TRAJ");
        // set the corresponding global
        traj_max_velocity = vel;
        old_inihal_data.traj_max_velocity = vel;

        // and set dynamic value
        if (0 != emcTrajSetMaxVelocity(vel)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                rcs_print("bad return value from emcTrajSetMaxVelocity\n");
            }
            return -1;
        }

        acc = 1e99; // let the axis values apply
        trajInifile->Find(&acc, "DEFAULT_LINEAR_ACCEL", "TRAJ");
        if (0 != emcTrajSetAcceleration(acc)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                rcs_print("bad return value from emcTrajSetAcceleration\n");
            }
            return -1;
        }
        old_inihal_data.traj_default_acceleration = acc;

        acc = 1e99; // let the axis values apply
        trajInifile->Find(&acc, "MAX_LINEAR_ACCEL", "TRAJ");
        if (0 != emcTrajSetMaxAcceleration(acc)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                rcs_print("bad return value from emcTrajSetMaxAcceleration\n");
            }
            return -1;
        }
        old_inihal_data.traj_max_acceleration = acc;

        int arcBlendEnable = 1;
        int arcBlendFallbackEnable = 0;
        int arcBlendOptDepth = 50;
        double arcBlendGapCycles = 4;
        double arcBlendRampFreq = 100.0;

        trajInifile->Find(&arcBlendEnable, "ARC_BLEND_ENABLE", "TRAJ");
        trajInifile->Find(&arcBlendFallbackEnable, "ARC_BLEND_FALLBACK_ENABLE", "TRAJ");
        trajInifile->Find(&arcBlendOptDepth, "ARC_BLEND_OPTIMIZATION_DEPTH", "TRAJ");
        trajInifile->Find(&arcBlendGapCycles, "ARC_BLEND_GAP_CYCLES", "TRAJ");
        trajInifile->Find(&arcBlendRampFreq, "ARC_BLEND_RAMP_FREQ", "TRAJ");

        if (0 != emcSetupArcBlends(arcBlendEnable, arcBlendFallbackEnable,
                    arcBlendOptDepth, arcBlendGapCycles, arcBlendRampFreq)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                rcs_print("bad return value from emcSetupArcBlends\n");
            }
            return -1;
        } 

        double maxFeedScale = 1.0;
        trajInifile->Find(&maxFeedScale, "MAX_FEED_OVERRIDE", "DISPLAY");

        if (0 != emcSetMaxFeedOverride(maxFeedScale)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                rcs_print("bad return value from emcSetMaxFeedOverride\n");
            }
            return -1;
        } 
    }

    catch(EmcIniFile::Exception &e){
        e.Print();
        return -1;
    }

    return 0;
}

/*
  iniTraj(const char *filename)

  Loads ini file parameters for trajectory, from [TRAJ] section
 */
int iniTraj(const char *filename)
{
    EmcIniFile trajInifile;

    if (trajInifile.Open(filename) == false) {
	return -1;
    }
    // load trajectory values
    if (0 != loadKins(&trajInifile)) {
	return -1;
    }
    // load trajectory values
    if (0 != loadTraj(&trajInifile)) {
	return -1;
    }

    return 0;
}
