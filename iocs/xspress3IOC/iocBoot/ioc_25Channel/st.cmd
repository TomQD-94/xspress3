< envPaths
errlogInit(20000)

# PREFIX
epicsEnvSet("PREFIX", "XSP3_25Chan:")

# Number of xspress3 channels
epicsEnvSet("NUM_CHANNELS",  "25")   

# Number of xspress3 cards and IP ADDR
epicsEnvSet("XSP3CARDS", "13")
epicsEnvSet("XSP3ADDR",  "192.168.0.1")

# Max Number of Frames for data collection
epicsEnvSet("MAXFRAMES", "16384")
epicsEnvSet("MAXDRIVERFRAMES", "16384")

# Number of Energy bins
epicsEnvSet("NUM_BINS",  "4096")

< ../common/DefineXSP3Driver.cmd

#####################
#Define Channels
#Channel 1
epicsEnvSet("CHAN",  "1")
epicsEnvSet("CHM1",  "0")
< ../common/DefineSCAROI.cmd

#Channel 2
epicsEnvSet("CHAN",  "2")
epicsEnvSet("CHM1",  "1")
< ../common/DefineSCAROI.cmd

#Channel 3
epicsEnvSet("CHAN",  "3")
epicsEnvSet("CHM1",  "2")
< ../common/DefineSCAROI.cmd

#Channel 4
epicsEnvSet("CHAN",  "4")
epicsEnvSet("CHM1",  "3")
< ../common/DefineSCAROI.cmd

#Channel 5
epicsEnvSet("CHAN",  "5")
epicsEnvSet("CHM1",  "4")
< ../common/DefineSCAROI.cmd

#Channel 6
epicsEnvSet("CHAN",  "6")
epicsEnvSet("CHM1",  "5")
< ../common/DefineSCAROI.cmd

#Channel 7
epicsEnvSet("CHAN",  "7")
epicsEnvSet("CHM1",  "6")
< ../common/DefineSCAROI.cmd

#Channel 8
epicsEnvSet("CHAN",  "8")
epicsEnvSet("CHM1",  "7")
< ../common/DefineSCAROI.cmd

#Channel 9
epicsEnvSet("CHAN",  "9")
epicsEnvSet("CHM1",  "8")
< ../common/DefineSCAROI.cmd

#Channel 10
epicsEnvSet("CHAN",  "10")
epicsEnvSet("CHM1",  "9")
< ../common/DefineSCAROI.cmd

#Channel 11
epicsEnvSet("CHAN",  "11")
epicsEnvSet("CHM1",  "10")
< ../common/DefineSCAROI.cmd

#Channel 12
epicsEnvSet("CHAN",  "12")
epicsEnvSet("CHM1",  "11")
< ../common/DefineSCAROI.cmd

#Channel 13
epicsEnvSet("CHAN",  "13")
epicsEnvSet("CHM1",  "12")
< ../common/DefineSCAROI.cmd

#Channel 14
epicsEnvSet("CHAN",  "14")
epicsEnvSet("CHM1",  "13")
< ../common/DefineSCAROI.cmd

#Channel 15
epicsEnvSet("CHAN",  "15")
epicsEnvSet("CHM1",  "14")
< ../common/DefineSCAROI.cmd

#Channel 16
epicsEnvSet("CHAN",  "16")
epicsEnvSet("CHM1",  "15")
< ../common/DefineSCAROI.cmd

#Channel 17
epicsEnvSet("CHAN",  "17")
epicsEnvSet("CHM1",  "16")
< ../common/DefineSCAROI.cmd

#Channel 18
epicsEnvSet("CHAN",  "18")
epicsEnvSet("CHM1",  "17")
< ../common/DefineSCAROI.cmd

#Channel 19
epicsEnvSet("CHAN",  "19")
epicsEnvSet("CHM1",  "18")
< ../common/DefineSCAROI.cmd

#Channel 20
epicsEnvSet("CHAN",  "20")
epicsEnvSet("CHM1",  "19")
< ../common/DefineSCAROI.cmd

#Channel 21
epicsEnvSet("CHAN",  "21")
epicsEnvSet("CHM1",  "20")
< ../common/DefineSCAROI.cmd

#Channel 22
epicsEnvSet("CHAN",  "22")
epicsEnvSet("CHM1",  "21")
< ../common/DefineSCAROI.cmd

#Channel 23
epicsEnvSet("CHAN",  "23")
epicsEnvSet("CHM1",  "22")
< ../common/DefineSCAROI.cmd

#Channel 24
epicsEnvSet("CHAN",  "24")
epicsEnvSet("CHM1",  "23")
< ../common/DefineSCAROI.cmd

#Channel 25
epicsEnvSet("CHAN",  "25")
epicsEnvSet("CHM1",  "24")
< ../common/DefineSCAROI.cmd

#####################

dbLoadRecords("xspress3Deadtime_25Channel.template",   "P=$(PREFIX)")

< ../common/AutoSave.cmd

###############################
# start IOC
iocInit

# setup startup values
#Configure and connect to Xspress3
dbpf("$(PREFIX)det1:CONFIG_PATH", "/etc/epics/xspress3/settings/25channel")

< ../common/SetMainValues.cmd  

#####################
#Set Channels
#Channel 1
epicsEnvSet("CHAN",  "1")
epicsEnvSet("CHM1",  "0")
< ../common/SetChannelValues.cmd

#Channel 2
epicsEnvSet("CHAN",  "2")
epicsEnvSet("CHM1",  "1")
< ../common/SetChannelValues.cmd

#Channel 3
epicsEnvSet("CHAN",  "3")
epicsEnvSet("CHM1",  "2")
< ../common/SetChannelValues.cmd

#Channel 4
epicsEnvSet("CHAN",  "4")
epicsEnvSet("CHM1",  "3")
< ../common/SetChannelValues.cmd

#Channel 5
epicsEnvSet("CHAN",  "5")
epicsEnvSet("CHM1",  "4")
< ../common/SetChannelValues.cmd

#Channel 6
epicsEnvSet("CHAN",  "6")
epicsEnvSet("CHM1",  "5")
< ../common/SetChannelValues.cmd

#Channel 7
epicsEnvSet("CHAN",  "7")
epicsEnvSet("CHM1",  "6")
< ../common/SetChannelValues.cmd

#Channel 8
epicsEnvSet("CHAN",  "8")
epicsEnvSet("CHM1",  "7")
< ../common/SetChannelValues.cmd

#Channel 9
epicsEnvSet("CHAN",  "9")
epicsEnvSet("CHM1",  "8")
< ../common/SetChannelValues.cmd

#Channel 10
epicsEnvSet("CHAN",  "10")
epicsEnvSet("CHM1",  "9")
< ../common/SetChannelValues.cmd

#Channel 11
epicsEnvSet("CHAN",  "11")
epicsEnvSet("CHM1",  "10")
< ../common/SetChannelValues.cmd

#Channel 12
epicsEnvSet("CHAN",  "12")
epicsEnvSet("CHM1",  "11")
< ../common/SetChannelValues.cmd

#Channel 13
epicsEnvSet("CHAN",  "13")
epicsEnvSet("CHM1",  "12")
< ../common/SetChannelValues.cmd

#Channel 14
epicsEnvSet("CHAN",  "14")
epicsEnvSet("CHM1",  "13")
< ../common/SetChannelValues.cmd

#Channel 15
epicsEnvSet("CHAN",  "15")
epicsEnvSet("CHM1",  "14")
< ../common/SetChannelValues.cmd

#Channel 16
epicsEnvSet("CHAN",  "16")
epicsEnvSet("CHM1",  "15")
< ../common/SetChannelValues.cmd

#Channel 17
epicsEnvSet("CHAN",  "17")
epicsEnvSet("CHM1",  "16")
< ../common/SetChannelValues.cmd

#Channel 18
epicsEnvSet("CHAN",  "18")
epicsEnvSet("CHM1",  "17")
< ../common/SetChannelValues.cmd

#Channel 19
epicsEnvSet("CHAN",  "19")
epicsEnvSet("CHM1",  "18")
< ../common/SetChannelValues.cmd

#Channel 20
epicsEnvSet("CHAN",  "20")
epicsEnvSet("CHM1",  "19")
< ../common/SetChannelValues.cmd

#Channel 21
epicsEnvSet("CHAN",  "21")
epicsEnvSet("CHM1",  "20")
< ../common/SetChannelValues.cmd

#Channel 22
epicsEnvSet("CHAN",  "22")
epicsEnvSet("CHM1",  "21")
< ../common/SetChannelValues.cmd

#Channel 23
epicsEnvSet("CHAN",  "23")
epicsEnvSet("CHM1",  "22")
< ../common/SetChannelValues.cmd

#Channel 24
epicsEnvSet("CHAN",  "24")
epicsEnvSet("CHM1",  "23")
< ../common/SetChannelValues.cmd

#Channel 25
epicsEnvSet("CHAN",  "25")
epicsEnvSet("CHM1",  "24")
< ../common/SetChannelValues.cmd

#####################
# save settings every thirty seconds
create_monitor_set("auto_settings.req",30,"P=$(PREFIX)")
epicsThreadSleep(5.)
# Set default event widths for deadtime correction
# note: these may be tuned for each detector:
# dbpf("$(PREFIX)C1:EventWidth",    "6")
# dbpf("$(PREFIX)C2:EventWidth",    "6")
# dbpf("$(PREFIX)C3:EventWidth",    "6")
# dbpf("$(PREFIX)C4:EventWidth",    "6")
# dbpf("$(PREFIX)C5:EventWidth",    "6")
# dbpf("$(PREFIX)C6:EventWidth",    "6")
# dbpf("$(PREFIX)C7:EventWidth",    "6")
# dbpf("$(PREFIX)C8:EventWidth",    "6")
# dbpf("$(PREFIX)C9:EventWidth",    "6")
# dbpf("$(PREFIX)C10:EventWidth",    "6")
# dbpf("$(PREFIX)C11:EventWidth",    "6")
# dbpf("$(PREFIX)C12:EventWidth",    "6")
# dbpf("$(PREFIX)C13:EventWidth",    "6")
# dbpf("$(PREFIX)C14:EventWidth",    "6")
# dbpf("$(PREFIX)C15:EventWidth",    "6")
# dbpf("$(PREFIX)C16:EventWidth",    "6")
# dbpf("$(PREFIX)C17:EventWidth",    "6")
# dbpf("$(PREFIX)C18:EventWidth",    "6")
# dbpf("$(PREFIX)C19:EventWidth",    "6")
# dbpf("$(PREFIX)C20:EventWidth",    "6")
# dbpf("$(PREFIX)C21:EventWidth",    "6")
# dbpf("$(PREFIX)C22:EventWidth",    "6")
# dbpf("$(PREFIX)C23:EventWidth",    "6")
# dbpf("$(PREFIX)C24:EventWidth",    "6")
# dbpf("$(PREFIX)C25:EventWidth",    "6")
# Xspress3 is now ready to use!