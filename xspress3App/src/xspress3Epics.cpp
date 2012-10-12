#include <iostream>
#include <string>
#include <stdexcept>

//Epics headers
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsExport.h>
#include <epicsString.h>
#include <iocsh.h>
#include <drvSup.h>
#include <registryFunction.h>


//Xspress3 API header
//extern "C" {
#include "xspress3.h"
//}

#include "xspress3Epics.h"

using std::cout;
using std::endl;

//Definitions of static class data members
const epicsUInt32 Xspress3::logFlow_ = 1;
const epicsUInt32 Xspress3::logError_ = 2;
const epicsInt32 Xspress3::ctrlDisable_ = 0;
const epicsInt32 Xspress3::ctrlEnable_ = 1;


//C Function prototypes to tie in with EPICS
static void xsp3StatusTaskC(void *drvPvt);
static void xsp3DataTaskC(void *drvPvt);


/**
 * Constructor for Xspress3::Xspress3. 
 * This must be called in the Epics IOC startup file.
 * @param portName The Asyn port name to use
 * @param maxChannels The number of channels to use (eg. 8)
 */
Xspress3::Xspress3(const char *portName, int numChannels, const char *baseIP, int maxFrames, int maxBuffers, size_t maxMemory, int debug)
  : asynNDArrayDriver(portName,
		      numChannels, /* maxAddr - channels use different param lists*/ 
		      NUM_DRIVER_PARAMS,
		      maxBuffers,
		      maxMemory,
		      asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat32ArrayMask | asynFloat64ArrayMask | asynDrvUserMask | asynOctetMask, /* Interface mask */
		      asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat32ArrayMask | asynFloat64ArrayMask | asynOctetMask,  /* Interrupt mask */
		      ASYN_CANBLOCK | ASYN_MULTIDEVICE, /* asynFlags.*/
		      1, /* Autoconnect */
		      0, /* Default priority */
		      0), /* Default stack size*/
    debug_(debug), numChannels_(numChannels)
{
  int status = asynSuccess;
  const char *functionName = "Xspress3::Xspress3";

  strncpy(baseIP_, baseIP, 16);
  baseIP_[16] = '\0';

  log(logFlow_, "Start of constructor", functionName); 

  //Create the epicsEvent for signaling to the status task when parameters should have changed.
  //This will cause it to do a poll immediately, rather than wait for the poll time period.
  statusEvent_ = epicsEventMustCreate(epicsEventEmpty);
  if (!statusEvent_) {
    printf("%s:%s epicsEventCreate failure for status event\n", driverName, functionName);
    return;
  }
  startEvent_ = epicsEventMustCreate(epicsEventEmpty);
  if (!startEvent_) {
    printf("%s:%s epicsEventCreate failure for start event\n", driverName, functionName);
    return;
  }
  stopEvent_ = epicsEventMustCreate(epicsEventEmpty);
  if (!stopEvent_) {
    printf("%s:%s epicsEventCreate failure for stop event\n", driverName, functionName);
    return;
  }

  //Add the params to the paramLib 
  //createParam adds the parameters to all param lists automatically (using maxAddr).
  createParam(xsp3ResetParamString,         asynParamInt32,       &xsp3ResetParam);
  createParam(xsp3EraseParamString,         asynParamInt32,       &xsp3EraseParam);
  createParam(xsp3StartParamString,         asynParamInt32,       &xsp3StartParam);
  createParam(xsp3StopParamString,          asynParamInt32,       &xsp3StopParam);
  createParam(xsp3StatusParamString,        asynParamOctet,       &xsp3StatusParam);
  createParam(xsp3NumChannelsParamString,   asynParamInt32,       &xsp3NumChannelsParam);
  createParam(xsp3MaxNumChannelsParamString,asynParamInt32,       &xsp3MaxNumChannelsParam);
  createParam(xsp3MaxSpectraParamString,asynParamInt32,       &xsp3MaxSpectraParam);
  createParam(xsp3MaxFramesParamString,asynParamInt32,       &xsp3MaxFramesParam);
  createParam(xsp3FrameCountParamString,asynParamInt32,       &xsp3FrameCountParam);
  createParam(xsp3FrameCountTotalParamString,asynParamInt32,       &xsp3FrameCountTotalParam);
  createParam(xsp3TriggerModeParamString,   asynParamInt32,       &xsp3TriggerModeParam);
  createParam(xsp3FixedTimeParamString,   asynParamInt32,       &xsp3FixedTimeParam);
  createParam(xsp3NumFramesParamString,     asynParamInt32,       &xsp3NumFramesParam);
  createParam(xsp3NumCardsParamString,      asynParamInt32,       &xsp3NumCardsParam);
  createParam(xsp3ConfigPathParamString,    asynParamOctet,       &xsp3ConfigPathParam);
  createParam(xsp3ConnectParamString,      asynParamInt32,       &xsp3ConnectParam);
  createParam(xsp3ConnectedParamString,      asynParamInt32,       &xsp3ConnectedParam);
  createParam(xsp3DisconnectParamString,      asynParamInt32,       &xsp3DisconnectParam);
  createParam(xsp3SaveSettingsParamString,      asynParamInt32,       &xsp3SaveSettingsParam);
  createParam(xsp3RestoreSettingsParamString,      asynParamInt32,       &xsp3RestoreSettingsParam);
  //These params will use different param lists based on asyn address
  createParam(xsp3ChanMcaParamString,       asynParamInt32Array,  &xsp3ChanMcaParam);
  createParam(xsp3ChanMcaCorrParamString,   asynParamFloat64Array,&xsp3ChanMcaCorrParam);
  createParam(xsp3ChanSca0ParamString,      asynParamInt32,       &xsp3ChanSca0Param);
  createParam(xsp3ChanSca1ParamString,      asynParamInt32,       &xsp3ChanSca1Param);
  createParam(xsp3ChanSca2ParamString,      asynParamInt32,       &xsp3ChanSca2Param);
  createParam(xsp3ChanSca3ParamString,      asynParamInt32,       &xsp3ChanSca3Param);
  createParam(xsp3ChanSca4ParamString,      asynParamInt32,       &xsp3ChanSca4Param);
  createParam(xsp3ChanSca5ParamString,      asynParamInt32,       &xsp3ChanSca5Param);
  createParam(xsp3ChanSca6ParamString,      asynParamInt32,       &xsp3ChanSca6Param);
  createParam(xsp3ChanSca7ParamString,      asynParamInt32,       &xsp3ChanSca7Param);
  createParam(xsp3ChanSca5CorrParamString,  asynParamFloat64,     &xsp3ChanSca5CorrParam);
  createParam(xsp3ChanSca6CorrParamString,  asynParamFloat64,     &xsp3ChanSca6CorrParam);
  createParam(xsp3ChanSca0ArrayParamString,asynParamInt32Array,  &xsp3ChanSca0ArrayParam);
  createParam(xsp3ChanSca1ArrayParamString,asynParamInt32Array,  &xsp3ChanSca1ArrayParam);
  createParam(xsp3ChanSca2ArrayParamString,asynParamInt32Array,  &xsp3ChanSca2ArrayParam);
  createParam(xsp3ChanSca3ArrayParamString,asynParamInt32Array,  &xsp3ChanSca3ArrayParam);
  createParam(xsp3ChanSca4ArrayParamString,asynParamInt32Array,  &xsp3ChanSca4ArrayParam);
  createParam(xsp3ChanSca5ArrayParamString,asynParamInt32Array,  &xsp3ChanSca5ArrayParam);
  createParam(xsp3ChanSca6ArrayParamString,asynParamInt32Array,  &xsp3ChanSca6ArrayParam);
  createParam(xsp3ChanSca7ArrayParamString,asynParamInt32Array,  &xsp3ChanSca7ArrayParam);
  createParam(xsp3ChanSca5CorrArrayParamString,asynParamFloat64Array,  &xsp3ChanSca5CorrArrayParam);
  createParam(xsp3ChanSca6CorrArrayParamString,asynParamFloat64Array,  &xsp3ChanSca6CorrArrayParam);
  createParam(xsp3ChanSca4ThresholdParamString,   asynParamInt32,       &xsp3ChanSca4ThresholdParam);
  createParam(xsp3ChanSca5HlmParamString,   asynParamInt32,       &xsp3ChanSca5HlmParam);
  createParam(xsp3ChanSca6HlmParamString,   asynParamInt32,       &xsp3ChanSca6HlmParam);
  createParam(xsp3ChanSca5LlmParamString,   asynParamInt32,       &xsp3ChanSca5LlmParam);
  createParam(xsp3ChanSca6LlmParamString,   asynParamInt32,       &xsp3ChanSca6LlmParam);
  //These control data update rates
  createParam(xsp3CtrlDataParamString,         asynParamInt32,       &xsp3CtrlDataParam);
  createParam(xsp3CtrlMcaParamString,         asynParamInt32,       &xsp3CtrlMcaParam);
  createParam(xsp3CtrlScaParamString,         asynParamInt32,       &xsp3CtrlScaParam);
  createParam(xsp3CtrlDataPeriodParamString,         asynParamInt32,       &xsp3CtrlDataPeriodParam);
  createParam(xsp3CtrlMcaPeriodParamString,         asynParamInt32,       &xsp3CtrlMcaPeriodParam);
  createParam(xsp3CtrlScaPeriodParamString,         asynParamInt32,       &xsp3CtrlScaPeriodParam);
  
 //Initialize non static data members
  acquiring_ = 0;

  pollingPeriod_ = 0.5; //seconds
  fastPollingPeriod_ = 0.1; //seconds

  /* Create the thread that updates the Xspress3 status */
  status = (epicsThreadCreate("Xsp3StatusTask",
                              epicsThreadPriorityMedium,
                              epicsThreadGetStackSize(epicsThreadStackMedium),
                              (EPICSTHREADFUNC)xsp3StatusTaskC,
                              this) == NULL);
  if (status) {
    printf("%s:%s epicsThreadCreate failure for status task\n",driverName, functionName);    
    return;
  }


    /* Create the thread that readouts the data */
  status = (epicsThreadCreate("GeDataTask",
                              epicsThreadPriorityMedium,
                              epicsThreadGetStackSize(epicsThreadStackMedium),
                              (EPICSTHREADFUNC)xsp3DataTaskC,
                              this) == NULL);
  if (status) {
    printf("%s:%s epicsThreadCreate failure for data task\n",
           driverName, functionName);
    return;
  }

  //Set any paramLib parameters that need passing up to device support
  status = setIntegerParam(xsp3NumChannelsParam, numChannels_);
  status |= setIntegerParam(xsp3MaxNumChannelsParam, numChannels_);
  status |= setIntegerParam(xsp3TriggerModeParam, 0);
  status |= setIntegerParam(xsp3FixedTimeParam, 0);
  status |= setIntegerParam(xsp3NumFramesParam, 0);
  status != setIntegerParam(xsp3MaxFramesParam, maxFrames);
  status |= setIntegerParam(xsp3NumCardsParam, 0);
  status |= setIntegerParam(xsp3FrameCountParam, 0);
  for (int chan=0; chan<numChannels_; chan++) {
    status |= setIntegerParam(chan, xsp3ChanSca4ThresholdParam, 0);
    status |= setIntegerParam(chan, xsp3ChanSca5HlmParam, 0);
    status |= setIntegerParam(chan, xsp3ChanSca6HlmParam, 0);
    status |= setIntegerParam(chan, xsp3ChanSca5LlmParam, 0);
    status |= setIntegerParam(chan, xsp3ChanSca6LlmParam, 0);
  }
  status |= eraseSCA();

  status |= setStringParam(xsp3StatusParam, "Init. System disconnected.");

  //Init asynNDArray params
  //NDArraySizeX
  //NDArraySizeY
  //NDArraySize
    
  //Do we need to do a status read now? - no because we don't connect at startup.
  //statusRead();
  
  callParamCallbacks();

  if (status) {
    printf("%s:%s Unable to set driver parameters in constructor.\n", driverName, functionName);
  }

  log(logFlow_, "End of constructor", functionName); 

}

Xspress3::~Xspress3() 
{
  cout << "Destructor called." << endl;
}


/**
 * Function to connect to the Xspress3 by calling xsp3_config and 
 * some other setup functions.
 */
asynStatus Xspress3::connect(void)
{
  asynStatus status = asynSuccess;
  int xsp3_num_cards = 0;
  int xsp3_num_tf = 0;
  char configPath[256];
  const char *functionName = "Xspress3::connect";

  log(logFlow_, "Calling connect", functionName);

  getIntegerParam(xsp3NumCardsParam, &xsp3_num_cards);
  getIntegerParam(xsp3NumFramesParam, &xsp3_num_tf);
  getStringParam(xsp3ConfigPathParam, static_cast<int>(sizeof(configPath)), configPath);

  //Set up the xspress3 system (might have to make all this callable at runtime, using PVs to set the parameters)
  log(logFlow_, "Setting up xsp3 system...", functionName);
  cout << "baseIP_ is: " << baseIP_ << endl;
  cout << "Num cards: " << xsp3_num_cards << endl;
  cout << "Num frames: " << xsp3_num_tf << endl;
  cout << "Num channels: " << numChannels_ << endl;
  cout << "Config Path: " << configPath << endl;
  xsp3_handle_ = xsp3_config(xsp3_num_cards, xsp3_num_tf, const_cast<char *>(baseIP_), -1, NULL, numChannels_, 1, NULL, debug_, 0);
  if (xsp3_handle_ < 0) {
    checkStatus(xsp3_handle_, "xsp3_config", functionName);
    status = asynError;
  } else {
    
    int xsp3_status = 0;

    //Set up clocks on each card
    for (int i=0; i<xsp3_num_cards; i++) {
      xsp3_status = xsp3_clocks_setup(xsp3_handle_, i, XSP3_CLK_SRC_INT, XSP3_CLK_FLAGS_MASTER, 0);
      if (xsp3_status != XSP3_OK) {
	checkStatus(xsp3_status, "xsp3_clocks_setup", functionName);
	status = asynError;
      }
    }
    
    //Restore settings from a file
    cout << "Calling xsp3_restore_settings with path: " << configPath << endl;
    xsp3_status = xsp3_restore_settings(xsp3_handle_, configPath, 0);
    if (xsp3_status != XSP3_OK) {
      checkStatus(xsp3_status, "xsp3_restore_settings", functionName);
      status = asynError;
    }

    //Can we do xsp3_format_run here? For normal user operation all the arguments seem to be set to zero.
    for (int chan=0; chan<numChannels_; chan++) {
      xsp3_status = xsp3_format_run(xsp3_handle_, chan, 0, 0, 0, 0, 0, 12);
      if (xsp3_status != XSP3_OK) {
	checkStatus(xsp3_status, "xsp3_format_run", functionName);
	status = asynError;
      }
    }

    if (status == asynSuccess) {
      log(logFlow_, "Finished setting up xsp3.", functionName);
      setStringParam(xsp3StatusParam, "System Connected");
      setIntegerParam(xsp3ConnectedParam, 1);
    } else {
      log(logFlow_, "ERROR: failed to connect to xspress3.", functionName);
      setStringParam(xsp3StatusParam, "ERROR: failed to connect");
      setIntegerParam(xsp3ConnectedParam, 0);
    }
  }

  return status;
}

/**
 * Disconnect from the xspress3 system. 
 * This simply calls xsp3_close().
 */
asynStatus Xspress3::disconnect(void)
{
  asynStatus status = asynSuccess;
  int xsp3_status = 0;
  const char *functionName = "Xspress3::disconnect";

  log(logFlow_, "Calling disconnect. This calls xsp3_close().", functionName);

  if ((status = checkConnected()) == asynSuccess) {
    xsp3_status = xsp3_close(xsp3_handle_);
    if (xsp3_status != XSP3_OK) {
      checkStatus(xsp3_status, "xsp3_close", functionName);
      status = asynError;
    }
    setStringParam(xsp3StatusParam, "System disconnected.");
    setIntegerParam(xsp3ConnectedParam, 0);
  }
  
  return status;
}

/**
 * Check the connected status.
 * @return asynSuccess if connected, or asynError if disconnected.
 */
asynStatus Xspress3::checkConnected(void) 
{
  int xsp3_connected = 0;
  const char *functionName = "Xspress3::checkConnected";

  log(logFlow_, "Checking connected status.", functionName);

  getIntegerParam(xsp3ConnectedParam, &xsp3_connected);
  if (xsp3_connected != 1) {
    log(logFlow_, "ERROR: We are not connected.", functionName);
    setStringParam(xsp3StatusParam, "ERROR: Not Connected");
    return asynError;
  }

  return asynSuccess;
}


/**
 * Save the system settings for the xspress3 system. 
 * This simply calls xsp3_save_settings().
 */
asynStatus Xspress3::saveSettings(void)
{
  asynStatus status = asynSuccess;
  int xsp3_status = 0;
  char configPath[256];
  int connected = 0;
  const char *functionName = "Xspress3::saveSettings";

  log(logFlow_, "Calling saveSettings. This calls xsp3_save_settings().", functionName);

  getIntegerParam(xsp3ConnectedParam, &connected);
  getStringParam(xsp3ConfigPathParam, static_cast<int>(sizeof(configPath)), configPath);

  if ((configPath == NULL) || (connected != 1)) {
    log(logError_, "ERROR: Trying to call xsp3_save_settings, but system not setup correctly.", functionName);
    status = asynError;
  } else {
    xsp3_status = xsp3_save_settings(xsp3_handle_, configPath);
    if (xsp3_status != XSP3_OK) {
      checkStatus(xsp3_status, "xsp3_save_settings", functionName);
      status = asynError;
    }
  }
  
  return status;
}

/**
 * Restore the system settings for the xspress3 system. 
 * This simply calls xsp3_restore_settings().
 */
asynStatus Xspress3::restoreSettings(void)
{
  asynStatus status = asynSuccess;
  int xsp3_status = 0;
  char configPath[256];
  int connected = 0;
  const char *functionName = "Xspress3::restoreSettings";

  log(logFlow_, "Calling saveSettings. This calls xsp3_restore_settings().", functionName);

  getIntegerParam(xsp3ConnectedParam, &connected);
  getStringParam(xsp3ConfigPathParam, static_cast<int>(sizeof(configPath)), configPath);

  if ((configPath == NULL) || (connected != 1)) {
    log(logError_, "ERROR: Trying to call xsp3_restore_settings, but system not setup correctly.", functionName);
    status = asynError;
  } else {
    xsp3_status = xsp3_restore_settings(xsp3_handle_, configPath, 0);
    if (xsp3_status != XSP3_OK) {
      checkStatus(xsp3_status, "xsp3_restore_settings", functionName);
      status = asynError;
    }
  }
  
  return status;
}



/**
 * Wrapper for asynPrint and local debug prints. If the debug_ data member
 * is on, then it simply prints the message. Otherwise if uses asynPrint statements.
 * @param mask Use either logFlow_ or logError_
 * @param msg The message to print
 * @param function The name of the calling function
 */
void Xspress3::log(epicsUInt32 mask, const char *msg, const char *function)
{
  if (debug_ == 1) {
    cout << function << " " << msg << endl;
  } else {
    if (mask == logFlow_) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s %s.\n", function, msg);
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s %s.\n", function, msg);
    }
  }
}

/**
 * Funtion to log an error if any of the Xsp3 functions return an error.
 * The function also take a pointer to the name of the function.
 * @param status The int status returned from an xsp3 function.
 * @param function The name of the called xsp3 function.
 * @param parentFunction The name of the parent function.
 */
void Xspress3::checkStatus(int status, const char *function, const char *parentFunction)
{

  if (status == XSP3_OK) {
    log(logFlow_, "XSP3_OK", function);
  } else if (status == XSP3_ERROR) {
    log(logError_, "XSP3_ERROR", function);
  } else if (status == XSP3_INVALID_PATH) {
    log(logError_, "XSP3_INVALID_PATH", function);
  } else if (status == XSP3_ILLEGAL_CARD) {
    log(logError_, "XSP3_ILLEGAL_CARD", function);
  } else if (status == XSP3_ILLEGAL_SUBPATH) {
    log(logError_, "XSP3_ILLEGAL_SUBPATH", function);
  } else if (status == XSP3_INVALID_DMA_STREAM) {
    log(logError_, "XSP3_INVALID_DMA_STREAM", function);
  } else if (status == XSP3_RANGE_CHECK) {
    log(logError_, "XSP3_RANGE_CHECK", function);
  } else if (status == XSP3_INVALID_SCOPE_MOD) {
    log(logError_, "XSP3_INVALID_SCOPE_MOD", function);
  } else {
    log(logError_, "Unknown XSP3 error code", function);
  }

  if (status != XSP3_OK) {
    log(logError_, "ERROR calling XSP3 function.", parentFunction);
    log(logError_, xsp3_get_error_message(), parentFunction);
  }
 
}

/**
 * Wrapper for xsp3_set_window that does some error checking.
 * @param channel The asyn address, mapping to xsp3 channel.
 * @param sca The SCA number
 * @param llm The low limit
 * @param hlm The high limit
 * @return asynStatus
 */
asynStatus Xspress3::setWindow(int channel, int sca, int llm, int hlm)
{
  asynStatus status = asynSuccess;
  int xsp3_status = 0;
  const char *functionName = "Xspress3::setWindow";

  if ((status = checkConnected()) == asynSuccess) {
    if (llm > hlm) {
      log(logError_, "ERROR: SCA low limit is higher than high limit.", functionName);
      status = asynError;
    } else {
      xsp3_status = xsp3_set_window(xsp3_handle_, channel, sca, llm, hlm); 
      if (xsp3_status != XSP3_OK) {
	checkStatus(xsp3_status, "xsp3_set_window", functionName);
	status = asynError;
      }
    }
  }
  
  return status;
}


/**
 * Call xsp3_histogram_clear, and clear scalar data.
 * @return asynStatus
 */
asynStatus Xspress3::erase(void)
{
  asynStatus status = asynSuccess;
  int xsp3_status = 0;
  int xsp3_time_frames = 0;
  int xsp3_num_channels = 0;
  const char *functionName = "Xspress3::erase";

  if ((status = checkConnected()) == asynSuccess) {
    log(logFlow_, "Erase MCA data, clear SCAs", functionName);

    status = eraseSCA();

    getIntegerParam(xsp3NumFramesParam, &xsp3_time_frames);
    getIntegerParam(xsp3NumChannelsParam, &xsp3_num_channels);
    xsp3_status = xsp3_histogram_clear(xsp3_handle_, 0, xsp3_num_channels, 0, xsp3_time_frames);
    if (xsp3_status != XSP3_OK) {
      checkStatus(xsp3_status, "xsp3_histogram_clear", functionName);
      status = asynError;
    } else {
      if (status == asynSuccess) {
	setStringParam(xsp3StatusParam, "Erased Data");
      } else {
	setStringParam(xsp3StatusParam, "Problem Erasing Data");
      }
    }
  }
  
  return status;
}

/**
 * Function to clear the SCA data.
 */
asynStatus Xspress3::eraseSCA(void)
{
  int status = asynSuccess;
  int xsp3_num_channels = 0;
  const char *functionName = "Xspress3::eraseSCA";

  log(logFlow_, "Clear SCA data", functionName);

  getIntegerParam(xsp3NumChannelsParam, &xsp3_num_channels);

  for (int chan=0; chan<xsp3_num_channels; chan++) {
    status |= setIntegerParam(chan, xsp3ChanSca0Param, 0);
    status |= setIntegerParam(chan, xsp3ChanSca1Param, 0);
    status |= setIntegerParam(chan, xsp3ChanSca2Param, 0);
    status |= setIntegerParam(chan, xsp3ChanSca3Param, 0);
    status |= setIntegerParam(chan, xsp3ChanSca4Param, 0);
    status |= setIntegerParam(chan, xsp3ChanSca5Param, 0);
    status |= setIntegerParam(chan, xsp3ChanSca6Param, 0);
    status |= setIntegerParam(chan, xsp3ChanSca7Param, 0);
    status |= setDoubleParam(chan,  xsp3ChanSca5CorrParam, 0);
    status |= setDoubleParam(chan,  xsp3ChanSca6CorrParam, 0);
  }

  if (status != asynSuccess) {
    log(logError_, "ERROR clearing SCA data", functionName);
  }

  return static_cast<asynStatus>(status);
}





/** Report status of the driver.
  * Prints details about the detector in us if details>0.
  * It then calls the ADDriver::report() method.
  * \param[in] fp File pointed passed by caller where the output is written to.
  * \param[in] details Controls the level of detail in the report. */
void Xspress3::report(FILE *fp, int details)
{
 
  fprintf(fp, "Xspress3::report.\n");

  fprintf(fp, "Xspress3 port=%s\n", this->portName);
  if (details > 0) { 
    fprintf(fp, "Xspress3 driver details...\n");
  }

  fprintf(fp, "Xspress3 finished.\n");
  
  //Call the base class method
  asynNDArrayDriver::report(fp, details);

}


/**
 * Reimplementing this function from asynNDArrayDriver to deal with integer values.
 */ 
asynStatus Xspress3::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  int function = pasynUser->reason;
  int addr = 0;
  int xsp3_status = 0;
  int xsp3_num_cards = 0;
  int xsp3_sca_lim = 0;
  int xsp3_time_frames = 0;
  asynStatus status = asynSuccess;
  int xsp3_num_channels = 0;
  const char *functionName = "Xspress3::writeInt32";
  
  log(logFlow_, "Calling writeInt32", functionName);
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s asynUser->reason: &d. value: %d\n", functionName, function, value);

  //Read address (channel number).
  status = getAddress(pasynUser, &addr); 
  if (status!=asynSuccess) {
    return(status);
  }

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Asyn addr: &d.\n", functionName, addr);

  cout << "VAL: " << value << "  ADDR: " << addr << endl;

  if (function == xsp3ResetParam) {
    log(logFlow_, "System reset", functionName);
    if ((status = checkConnected()) == asynSuccess) {
      //What do we do here?
    }
  } 
  else if (function == xsp3EraseParam) {
    status = erase();
  } 
  else if (function == xsp3StartParam) {
    /////////////////////////////////////////////temp
    epicsEventSignal(this->startEvent_);
    /////////////////////////////////////////////////
    if ((status = checkConnected()) == asynSuccess) {

      //Only allow if we are not collecting data

      log(logFlow_, "Start collecting data", functionName);
      getIntegerParam(xsp3NumCardsParam, &xsp3_num_cards);
      for (int card=0; card<xsp3_num_cards; card++) {
	xsp3_status = xsp3_histogram_start(xsp3_handle_, card);
	if (xsp3_status != XSP3_OK) {
	  checkStatus(xsp3_status, "xsp3_histogram_start", functionName);
	  status = asynError;
	}
      }
      if (status == asynSuccess) {
	epicsEventSignal(this->startEvent_);
	setStringParam(xsp3StatusParam, "Acquiring Data");
      }
    }
  } 
  else if (function == xsp3StopParam) {
    //////////////////////////////////////temp
    epicsEventSignal(this->stopEvent_);
    ///////////////////////////////////////////
    if ((status = checkConnected()) == asynSuccess) {
      
      //Need to check we are already collecting data here, and if so do not send a stop event.

      log(logFlow_, "Stop collecting data", functionName);
      getIntegerParam(xsp3NumCardsParam, &xsp3_num_cards);
      for (int card=0; card<xsp3_num_cards; card++) {
	xsp3_status = xsp3_histogram_stop(xsp3_handle_, card);
	if (xsp3_status != XSP3_OK) {
	  checkStatus(xsp3_status, "xsp3_histogram_stop", functionName);
	  status = asynError;
	  setStringParam(xsp3StatusParam, "Stopped Acquiring");
	}
      }
      if (status == asynSuccess) {
	epicsEventSignal(this->stopEvent_);
      }
    }
  }
  else if (function == xsp3NumChannelsParam) {
   log(logFlow_, "Set number of channels", functionName);
   cout << "channels: " << value << endl;
   getIntegerParam(xsp3MaxNumChannelsParam, &xsp3_num_channels);
   if ((value > xsp3_num_channels) || (value < 1)) {
     log(logError_, "ERROR: num channels out of range.", functionName);
     status = asynError;
   }
  }
  else if (function == xsp3TriggerModeParam) {
    if ((status = checkConnected()) == asynSuccess) {
      log(logFlow_, "Set trigger mode", functionName);
      cout << "VAL: " << value << endl;
      getIntegerParam(xsp3NumCardsParam, &xsp3_num_cards);
      for (int card=0; card<xsp3_num_cards; card++) {
	xsp3_status = xsp3_set_glob_timeA(xsp3_handle_, card, value);
	if (xsp3_status != XSP3_OK) {
	  checkStatus(xsp3_status, "xsp3_set_glob_timeA", functionName);
	  status = asynError;
	}
      }
    }
  } 
  else if (function == xsp3FixedTimeParam) {
    if ((status = checkConnected()) == asynSuccess) {
      log(logFlow_, "Set the fixed time register", functionName);
      cout << "VAL: " << value << endl;
      getIntegerParam(xsp3NumCardsParam, &xsp3_num_cards);
      for (int card=0; card<xsp3_num_cards; card++) {
	xsp3_status = xsp3_set_glob_timeFixed(xsp3_handle_, card, value);
	if (xsp3_status != XSP3_OK) {
	  checkStatus(xsp3_status, "xsp3_set_glob_timeFixed", functionName);
	  status = asynError;
	}
      }
    }
  } 
  else if (function == xsp3NumFramesParam) {
    log(logFlow_, "Set the number of frames", functionName);
  } 
  else if (function == xsp3NumCardsParam) {
    log(logFlow_, "Set the number of cards", functionName);
  } 
  else if (function == xsp3ConnectParam) {
    log(logFlow_, "Run the connect function.", functionName);
    status = connect();
  }
  else if (function == xsp3DisconnectParam) {
    log(logFlow_, "Run the Disconnect function.", functionName);
    status = disconnect();
  }
  else if (function == xsp3SaveSettingsParam) {
    log(logFlow_, "Run the saveSettings function.", functionName);
    status = saveSettings();
  }
  else if (function == xsp3RestoreSettingsParam) {
    log(logFlow_, "Run the restoreSettings function.", functionName);
    status = restoreSettings();
  }
  else if (function == xsp3ChanSca4ThresholdParam) {
    if ((status = checkConnected()) == asynSuccess) {
      log(logFlow_, "Set the SCA4 threshold register", functionName);
      xsp3_status = xsp3_set_good_thres(xsp3_handle_, addr, value);
      if (xsp3_status != XSP3_OK) {
	checkStatus(xsp3_status, "xsp3_set_good_thres", functionName);
	status = asynError;
      }
    }
  } 
  else if (function == xsp3ChanSca5HlmParam) {
    log(logFlow_, "Setting SCA5 high limit.", functionName);
    getIntegerParam(addr, xsp3ChanSca5LlmParam, &xsp3_sca_lim);
    cout << "Low lim is already: " << xsp3_sca_lim << endl;
    status = setWindow(addr, 0, xsp3_sca_lim, value); 
  } 
  else if (function == xsp3ChanSca6HlmParam) {
    log(logFlow_, "Setting SCA6 high limit.", functionName);
    getIntegerParam(addr, xsp3ChanSca6LlmParam, &xsp3_sca_lim);
    cout << "Low lim is already: " << xsp3_sca_lim << endl;
    status = setWindow(addr, 1, xsp3_sca_lim, value); 
  } 
  else if (function == xsp3ChanSca5LlmParam) {
    log(logFlow_, "Setting SCA5 low limit", functionName);
    getIntegerParam(addr, xsp3ChanSca5HlmParam, &xsp3_sca_lim);
    cout << "High lim is already: " << xsp3_sca_lim << endl;
    status = setWindow(addr, 0, value, xsp3_sca_lim); 
  } 
  else if (function == xsp3ChanSca6LlmParam) {
    log(logFlow_, "Setting SCA6 low limit", functionName); 
    getIntegerParam(addr, xsp3ChanSca6HlmParam, &xsp3_sca_lim);
    cout << "High lim is already: " << xsp3_sca_lim << endl;
    status = setWindow(addr, 1, value, xsp3_sca_lim);
  } 
  else if (function == xsp3CtrlDataParam) {
    if (value == ctrlDisable_) {
      log(logFlow_, "Disabling all live data update.", functionName);
      setIntegerParam(xsp3CtrlMcaParam, 0);
      setIntegerParam(xsp3CtrlScaParam, 0);
    } else if (value == ctrlEnable_) {
      log(logFlow_, "Enabling live data update.", functionName);
    }
  } 
  else if (function == xsp3CtrlMcaParam) {
    if (value == ctrlDisable_) {
      log(logFlow_, "Disabling MCA data update.", functionName);
    } else if (value == ctrlEnable_) {
      log(logFlow_, "Enabling MCA data update.", functionName);
      setIntegerParam(xsp3CtrlDataParam, 1);
    }
  } 
  else if (function == xsp3CtrlScaParam) {
    if (value == ctrlDisable_) {
      log(logFlow_, "Disabling SCA data update.", functionName);
    } else if (value == ctrlEnable_) {
      log(logFlow_, "Enabling SCA data update.", functionName);
      setIntegerParam(xsp3CtrlDataParam, 1);
    }
  } 
  else if (function == xsp3CtrlDataPeriodParam) {
    log(logFlow_, "Setting scalar data update rate.", functionName);
  }
  else if (function == xsp3CtrlMcaPeriodParam) {
    log(logFlow_, "Setting MCA data update rate.", functionName);
  }
  else if (function == xsp3CtrlScaPeriodParam) {
    log(logFlow_, "Setting SCA data update rate.", functionName);
  }
  else {
    log(logError_, "No matching parameter.", functionName);
  }

  if (status != asynSuccess) {
    return asynError;
  }

  //Set in param lib so the user sees a readback straight away. We might overwrite this in the 
  //status task, depending on the parameter.
  status = (asynStatus) setIntegerParam(addr, function, value);
  if (status!=asynSuccess) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
	      "%s Error setting parameter. Asyn addr: &d, asynUser->reason: &d.\n", 
	      functionName, addr, function);
    return(status);
  }

  //Do callbacks so higher layers see any changes 
  callParamCallbacks(addr);

  return status;
}

/**
 * Reimplementing this function from asynNDArrayDriver to deal with floating point values.
 */ 
asynStatus Xspress3::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
  int function = pasynUser->reason;
  int addr = 0;
  asynStatus status = asynSuccess;
  const char *functionName = "Xspress3::writeFloat64";
  
  log(logFlow_, "Calling writeFloat64", functionName);
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s asynUser->reason: &d.\n", functionName, function);

  //Read address (channel number).
  status = getAddress(pasynUser, &addr); 
  if (status!=asynSuccess) {
    return(status);
  }

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Asyn addr: &d.\n", functionName, addr);


  //Set in param lib so the user sees a readback straight away. We might overwrite this in the 
  //status task, depending on the parameter.
  status = (asynStatus) setDoubleParam(function, value);

  //Do callbacks so higher layers see any changes 
  callParamCallbacks();

  return status;
}



asynStatus Xspress3::writeOctet(asynUser *pasynUser, const char *value, 
                                    size_t nChars, size_t *nActual)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *functionName = "Xspress3::writeOctet";

    /* Set the parameter in the parameter library. */
    status = (asynStatus)setStringParam(function, (char *)value);

    if (function == xsp3ConfigPathParam) {
        log(logFlow_, "Setting config path param.", functionName);
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_DRIVER_COMMAND) status = asynNDArrayDriver::writeOctet(pasynUser, value, nChars, nActual);
    }
    
    /* Do callbacks so higher layers see any changes */
    status = (asynStatus)callParamCallbacks();
    
    if (status) {
      epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
		    "%s:%s: status=%d, function=%d, value=%s", 
		    driverName, functionName, status, function, value);
    } else {
      asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
		"%s:%s: function=%d, value=%s\n", 
		driverName, functionName, function, value);
    }
    *nActual = nChars;
    return status;
}





/**
 * Status poling task
 */
void Xspress3::statusTask(void)
{
  //asynStatus status = asynSuccess;
  epicsEventWaitStatus eventStatus;
  epicsFloat32 timeout = 0;
  epicsUInt32 forcedFastPolls = 0;

  const char* functionName = "Xspress3::statusTask";

  log(logFlow_, "Started.", functionName);

  
  while(1) {

    //Read timeout for polling freq.
    this->lock();
    if (forcedFastPolls > 0) {
      timeout = fastPollingPeriod_;
      forcedFastPolls--;
    } else {
      timeout = pollingPeriod_;
    }
    this->unlock();

    if (timeout != 0.0) {
      eventStatus = epicsEventWaitWithTimeout(statusEvent_, timeout);
    } else {
      eventStatus = epicsEventWait(statusEvent_);
    }              
    if (eventStatus == (epicsEventWaitOK || epicsEventWaitTimeout)) {
      //We got an event, rather than a timeout.  This is because other software
      //knows that data has arrived, or device should have changed state (parameters changed, etc.).
      //Force a minimum number of fast polls, because the device status
      //might not have changed in the first few polls
      forcedFastPolls = 5;
    }

    this->lock();
    
    if (debug_) {
      //log(logFlow_, "Got status event.", functionName);
    }
    
    /* Call the callbacks to update any changes */
    callParamCallbacks();
    this->unlock();
    
  }
     
 

}

/**
 * Data readout task.
 * Calculate statistics and post waveforms.
 */
void Xspress3::dataTask(void)
{
  //asynStatus status = asynSuccess;
  epicsEventWaitStatus eventStatus;
  epicsFloat64 timeout = 0.05;
  int numChannels = 0;
  int acquire = 0;
  int xsp3_status = 0;
  int status = 0;
  int xsp3_num_cards = 0;
  int frame_count = 0;
  int notBusyCount = 0;
  int notBusyChan = 0;
  int notBusyTotalCount = 0;
  int frameCounter = 0;
  int scaArraySize = 0;
  XSP3_DMA_MsgCheckDesc dmaCheck;
  const char* functionName = "Xspress3::dataTask";
  epicsUInt32 *pSCA;
  epicsInt32 *pSCA_DATA[numChannels_][XSP3_SW_NUM_SCALERS];
  
  int simTest = 1;

  log(logFlow_, "Started.", functionName);

   while (1) {

     //SEE Pilatus driver for example for how to avoid start/stop deadlock.
     //May need to use acquire/aborted flags like in pilatus driver.

     //Create array for scalar data (max frame * number of SCAs).
     getIntegerParam(xsp3MaxFramesParam, &scaArraySize);
     cout << "scaArraySize: " << scaArraySize << endl;
     pSCA = static_cast<epicsUInt32*>(calloc(XSP3_SW_NUM_SCALERS*scaArraySize*numChannels_, sizeof(epicsUInt32)));
     //Create array to hold SCA data for the duration of the scan, one per SCA, per channel.
     for (int chan=0; chan<numChannels_; chan++) {
       for (int sca=0; sca<XSP3_SW_NUM_SCALERS; sca++) {
	 pSCA_DATA[chan][sca] = static_cast<epicsInt32*>(calloc(scaArraySize, sizeof(epicsInt32)));
       }
     }
     

     eventStatus = epicsEventWait(startEvent_);          
     if (eventStatus == epicsEventWaitOK) {
        log(logFlow_, "Got start event.", functionName);
	acquire = 1;
	frameCounter = 0;
	setIntegerParam(xsp3FrameCountParam, 0);
	setIntegerParam(xsp3FrameCountTotalParam, 0);
	callParamCallbacks();
      }

     //Get the number of channels actually in use
     getIntegerParam(xsp3NumChannelsParam, &numChannels);

     while (acquire) {

       //Wait for a stop event, with a short timeout.
       eventStatus = epicsEventWaitWithTimeout(stopEvent_, timeout);          
       if (eventStatus == epicsEventWaitOK) {
	 log(logFlow_, "Got stop event.", functionName);
	 acquire = 0;
       }

       //If we have stopped, wait until we are not busy twice, on all channels.
       if (!simTest) {
	 if (acquire == 0) {
	   notBusyCount = 0;
	   notBusyChan = 0;
	   notBusyTotalCount = 0;
	   while (notBusyCount<2) {
	     for (int chan=0; chan<numChannels; chan++) {
	       if ((xsp3_histogram_is_busy(xsp3_handle_, chan)) == 0) {
		 ++notBusyChan;
	       }
	     }
	     cout << "notBusyChan: " << notBusyChan << endl;
	     if (notBusyChan == numChannels) {
	       ++notBusyCount;
	     }
	     cout << "notBusyCount: " << notBusyCount << endl;
	     notBusyChan = 0;
	     notBusyTotalCount++;
	     if (notBusyTotalCount==20) {
	       log(logError_, "ERROR: we polled xsp3_histogram_is_busy 20 times. Giving up.", functionName);
	       setStringParam(xsp3StatusParam, "ERROR: xspress3 did not stop. Giving up.");
	       status = asynError;
	       break;
	     }
	   }
	 }
       }

       //Read how many scaler data frames have been transferred.
       if (!simTest) {
	 getIntegerParam(xsp3NumCardsParam, &xsp3_num_cards);
	 for (int card=0; card<xsp3_num_cards; card++) {
	   xsp3_status = xsp3_dma_check_desc(xsp3_handle_, card, XSP3_DMA_STREAM_MASK_SCALERS, &dmaCheck);
	   if (xsp3_status != XSP3_OK) {
	     checkStatus(xsp3_status, "xsp3_dma_check_desc", functionName);
	     status = asynError;
	   }
	   frame_count = dmaCheck.num_desc;
	 }
       } else {
	 //In sim mode we transfer 10 frames each time
	 frame_count = 10;
       }
       
       //Take the value from the last card for now...
       //Do I need to throttle this based on the scalar update rate timer?
       setIntegerParam(xsp3FrameCountParam, frame_count);
       
       if (frame_count > 0) {

	 getIntegerParam(xsp3FrameCountTotalParam, &frameCounter);
	 frameCounter += frame_count;
	 cout << "frameCounter: " << frameCounter << endl;
	 if (frameCounter >= scaArraySize) {
	   log(logFlow_, "ERROR: max frames reached. Stopping.", functionName);
	   acquire=0;
	   break;
	 }
	 setIntegerParam(xsp3FrameCountTotalParam, frameCounter);

	 epicsUInt32 *pData = NULL;
	 //Readout here, and copy the scalar data into local arrays.
	 //For now read out everything everytime, until I know it's working and I can make it more efficient.
	 if (!simTest) {
	   xsp3_status = xsp3_scaler_read(xsp3_handle_, static_cast<u_int32_t*>(pSCA), 0, 0, 0, XSP3_SW_NUM_SCALERS, numChannels, frameCounter);
	 } else {
	   //Fill the array with dummy data in simTest mode
	   pData = pSCA;
	   for (int i=0; i<(XSP3_SW_NUM_SCALERS*scaArraySize*numChannels); ++i) {
	     //cout << "i: " << i << endl;
	     *(pData++) = i;
	   }
	   if (acquire) {
	     epicsThreadSleep(0.1);
	   }
	 }
	 
	 pData = pSCA;
	 epicsInt32 *pDataArray = NULL;
	 //Now copy the data to the per-channel, per-SCA arrays, to make them available for channel access.
	 for (int frame=0; frame<frameCounter; ++frame) {
	   for (int chan=0; chan<numChannels; ++chan) {
	     for (int sca=0; sca<XSP3_SW_NUM_SCALERS; ++sca) {
	       pDataArray = (static_cast<epicsInt32*>((pSCA_DATA[chan][sca])+frame));
	       *pDataArray = *(pData++);
	     }
	   }
	 }

	 //Debug print first 100 frames
	 /*cout << "Debug print first two frames..." << endl;
	 pDataArray = NULL;
	 for (int chan=0; chan<numChannels; ++chan) {
	   for (int sca=0; sca<XSP3_SW_NUM_SCALERS; ++sca) {
	     pDataArray = static_cast<epicsInt32*>(pSCA_DATA[chan][sca]);
	     for (int i=0; i<102; ++i) {
	       cout << " CHAN: " << chan << "  SCA: " << sca << "  FRAME " << i << ": " << *pDataArray << endl;
	       pDataArray++;
	     }
	   }
	   }*/
	 
	 int scalerUpdate = 0;
	 int scalerArrayUpdate = 0; 
	 getIntegerParam(xsp3CtrlDataParam, &scalerUpdate); 
	 getIntegerParam(xsp3CtrlScaParam, &scalerArrayUpdate); 

	 int offset = frameCounter-1;
	 //Post scalar data if we have enabled it and the timer has expired, or if we have stopped acquiring. 
	 if (scalerUpdate || !acquire) {
	   cout << "Posting scaler data values..." << endl;
	   //Take the most recent scalar values and post the values
	   for (int chan=0; chan<numChannels; ++chan) {
	     cout << frame_count << endl;
	     cout << (static_cast<epicsInt32*>(pSCA_DATA[chan][0])) << endl;
	     cout << ((static_cast<epicsInt32*>(pSCA_DATA[chan][0]))+offset) << endl;
	     cout << *(static_cast<epicsInt32*>(pSCA_DATA[chan][0])) << endl;
	     cout << *((static_cast<epicsInt32*>(pSCA_DATA[chan][0]))+offset) << endl;
	     cout << *((static_cast<epicsInt32*>(pSCA_DATA[chan][1]))+offset) << endl;
	     cout << *((static_cast<epicsInt32*>(pSCA_DATA[chan][2]))+offset) << endl;
	     cout << *((static_cast<epicsInt32*>(pSCA_DATA[chan][3]))+offset) << endl;
	     cout << *((static_cast<epicsInt32*>(pSCA_DATA[chan][4]))+offset) << endl;
	     cout << *((static_cast<epicsInt32*>(pSCA_DATA[chan][5]))+offset) << endl;
	     cout << *((static_cast<epicsInt32*>(pSCA_DATA[chan][6]))+offset) << endl;
	     cout << *((static_cast<epicsInt32*>(pSCA_DATA[chan][7]))+offset) << endl;
	     setIntegerParam(chan, xsp3ChanSca0Param, *((static_cast<epicsInt32*>(pSCA_DATA[chan][0]))+offset));
	     setIntegerParam(chan, xsp3ChanSca1Param, *((static_cast<epicsInt32*>(pSCA_DATA[chan][1]))+offset));
	     setIntegerParam(chan, xsp3ChanSca2Param, *((static_cast<epicsInt32*>(pSCA_DATA[chan][2]))+offset));
	     setIntegerParam(chan, xsp3ChanSca3Param, *((static_cast<epicsInt32*>(pSCA_DATA[chan][3]))+offset));
	     setIntegerParam(chan, xsp3ChanSca4Param, *((static_cast<epicsInt32*>(pSCA_DATA[chan][4]))+offset));
	     setIntegerParam(chan, xsp3ChanSca5Param, *((static_cast<epicsInt32*>(pSCA_DATA[chan][5]))+offset));
	     setIntegerParam(chan, xsp3ChanSca6Param, *((static_cast<epicsInt32*>(pSCA_DATA[chan][6]))+offset));
	     setIntegerParam(chan, xsp3ChanSca7Param, *((static_cast<epicsInt32*>(pSCA_DATA[chan][7]))+offset));
	   }
	   cout << "Done." << endl;
	 }

	 //Post scalar array data if we have enabled it and the timer has expired, or if we have stopped acquiring. 
	 if ((scalerArrayUpdate == 1) || !acquire) {
	   cout << "Posting scaler array data values..." << endl;
	   for (int chan=0; chan<numChannels; ++chan) {
	     doCallbacksInt32Array(static_cast<epicsInt32*>(pSCA_DATA[chan][0]), offset, xsp3ChanSca0ArrayParam, chan);
	     doCallbacksInt32Array(static_cast<epicsInt32*>(pSCA_DATA[chan][1]), offset, xsp3ChanSca1ArrayParam, chan);
	     doCallbacksInt32Array(static_cast<epicsInt32*>(pSCA_DATA[chan][2]), offset, xsp3ChanSca2ArrayParam, chan);
	     doCallbacksInt32Array(static_cast<epicsInt32*>(pSCA_DATA[chan][3]), offset, xsp3ChanSca3ArrayParam, chan);
	     doCallbacksInt32Array(static_cast<epicsInt32*>(pSCA_DATA[chan][4]), offset, xsp3ChanSca4ArrayParam, chan);
	     doCallbacksInt32Array(static_cast<epicsInt32*>(pSCA_DATA[chan][5]), offset, xsp3ChanSca5ArrayParam, chan);
	     doCallbacksInt32Array(static_cast<epicsInt32*>(pSCA_DATA[chan][6]), offset, xsp3ChanSca6ArrayParam, chan);
	     doCallbacksInt32Array(static_cast<epicsInt32*>(pSCA_DATA[chan][7]), offset, xsp3ChanSca7ArrayParam, chan);
	   }
	   cout << "Done." << endl;
	 }

       }  //end of frame_count > 0

       frame_count = 0;

       //Need to call callParamCallbacks for each list (ie. channel, indexed by Asyn address).
       for (int addr=0; addr<maxAddr; addr++) {
	 callParamCallbacks(addr);
       }

     } //end of while(acquire)

       
     //Use the API function get_bins_per_mca for spectra length. (should be 4096?)

     //Create data arrays of the correct size using pArray = this->pNDArrayPool->alloc
     //Set NDArraySize
     //Increment image counters (NDArrayCounter)
     //Set uniqueId and timestamp in pArray
     //Set the scalars as attribute data. Get the list like: this->getAttributes(pArray->pAttributeList);

     //Notes on data format:
     //Save a 2D NDArray for the spectra data (channel * MCA bin)
     //Attach all the scalar data as attributes to this NDArray (seperate attributes per channel, per scalar).

     
     //Call this->unlock(); before doCallbacksGenericPointer(pArray, NDArrayData, 0); (the last param is the address).
     //Then lock() again, and call pArray->release();
     
     //Post arrays to channel access, using doCallbacksFloat64Array. Might have to limit this to a preset freq by 
     //checking the time since last post. Or, not post at all depending on what the user set (until a stop acqusition is done).
     
     //callParamCallbacks();

     

     //Example of how to wait for stop event
     //Check for a stop event with a small timeout?
     //eventStatus = epicsEventWaitWithTimeout(stopEvent_, timeout);          
     //if (eventStatus == epicsEventWaitOK) {
     //  log(logFlow_, "Got stop event.", functionName);
     //}

   }

}


//Global C utility functions to tie in with EPICS

static void xsp3StatusTaskC(void *drvPvt)
{
  Xspress3 *pPvt = (Xspress3 *)drvPvt;

  pPvt->statusTask();
}


static void xsp3DataTaskC(void *drvPvt)
{
  Xspress3 *pPvt = (Xspress3 *)drvPvt;
  
  pPvt->dataTask();
}

/*************************************************************************************/
/** The following functions have C linkage, and can be called directly or from iocsh */


/**
 * Config function for IOC shell. It instantiates an instance of the driver.
 * @param portName The Asyn port name to use
 * @param numChannels The number of channels (eg. 8)
 */
extern "C" {

  int xspress3Config(const char *portName, int numChannels, const char *baseIP, int maxFrames, int maxBuffers, size_t maxMemory, int debug)
  {
    asynStatus status = asynSuccess;
    
    /*Instantiate class.*/
    try {
      new Xspress3(portName, numChannels, baseIP, maxFrames, maxBuffers, maxMemory, debug);
    } catch (...) {
      cout << "Unknown exception caught when trying to construct Xspress3." << endl;
      status = asynError;
    }
    
    return(status);
  }
  
  /* Code for iocsh registration */
  
  /* xspress3Config */
  static const iocshArg xspress3ConfigArg0 = {"Port name", iocshArgString};
  static const iocshArg xspress3ConfigArg1 = {"Num Channels", iocshArgInt};
  static const iocshArg xspress3ConfigArg2 = {"Base IP Address", iocshArgString};
  static const iocshArg xspress3ConfigArg3 = {"Max Frames", iocshArgInt};
  static const iocshArg xspress3ConfigArg4 = {"Max Buffers", iocshArgInt};
  static const iocshArg xspress3ConfigArg5 = {"Max Memory", iocshArgInt};
  static const iocshArg xspress3ConfigArg6 = {"Debug", iocshArgInt};
  static const iocshArg * const xspress3ConfigArgs[] =  {&xspress3ConfigArg0,
							 &xspress3ConfigArg1,
							 &xspress3ConfigArg2,
							 &xspress3ConfigArg3,
							 &xspress3ConfigArg4,
							 &xspress3ConfigArg5,
							 &xspress3ConfigArg6};
  
  static const iocshFuncDef configXspress3 = {"xspress3Config", 7, xspress3ConfigArgs};
  static void configXspress3CallFunc(const iocshArgBuf *args)
  {
    xspress3Config(args[0].sval, args[1].ival, args[2].sval, args[3].ival, args[4].ival, args[5].ival, args[6].ival);
  }
  
  static void xspress3Register(void)
  {
    iocshRegister(&configXspress3, configXspress3CallFunc);
  }
  
  epicsExportRegistrar(xspress3Register);

} // extern "C"


/****************************************************************************************/
