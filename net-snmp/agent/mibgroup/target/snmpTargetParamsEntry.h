
/* This file was generated by mib2c and is intended for use as a mib module
   for the ucd-snmp snmpd agent. Edited by Michael Baer

   last changed 2/2/99.
*/

#ifndef _MIBGROUP_SNMPTARGETPARAMSENTRY_H
#define _MIBGROUP_SNMPTARGETPARAMSENTRY_H

/* we use header_generic and checkmib from the util_funcs module */

config_require(util_funcs)

/* Magic number definitions: */

#define   SNMPTARGETPARAMSMPMODEL        1
#define   SNMPTARGETPARAMSSECURITYMODEL  2
#define   SNMPTARGETPARAMSSECURITYNAME   3
#define   SNMPTARGETPARAMSSECURITYLEVEL  4
#define   SNMPTARGETPARAMSSTORAGETYPE    5
#define   SNMPTARGETPARAMSROWSTATUS      6

#define   SNMPTARGETPARAMSMPMODELCOLUMN        2
#define   SNMPTARGETPARAMSSECURITYMODELCOLUMN  3
#define   SNMPTARGETPARAMSSECURITYNAMECOLUMN   4
#define   SNMPTARGETPARAMSSECURITYLEVELCOLUMN  5
#define   SNMPTARGETPARAMSSTORAGETYPECOLUMN    6
#define   SNMPTARGETPARAMSROWSTATUSCOLUMN      7

/* structure definitions */

struct targetParamTable_struct {
  char   *paramName;
  int    mpModel;
  int    secModel;
  char   *secName;
  int    secLevel;
  int    storageType;
  int    rowStatus;
  struct targetParamTable_struct *next;
  time_t updateTime;
};

/* utility functions */
struct targetParamTable_struct *get_paramEntry(char *name);
void snmpTargetParamTable_add(struct targetParamTable_struct *newEntry);
struct targetParamTable_struct *snmpTargetParamTable_create(void);

/* function definitions */

void          init_snmpTargetParamsEntry(void);
int           store_snmpTargetParamsEntry(int majorID, int minorID,
                                          void *serverarg, void *clientarg);
extern FindVarMethod var_snmpTargetParamsEntry;

void snmpd_parse_config_targetParams(const char *, char *);

WriteMethod write_snmpTargetParamsMPModel;
WriteMethod write_snmpTargetParamsSecModel;
WriteMethod write_snmpTargetParamsSecName;
WriteMethod write_snmpTargetParamsSecLevel;
WriteMethod write_snmpTargetParamsStorageType;
WriteMethod write_snmpTargetParamsRowStatus;


#endif /* _MIBGROUP_SNMPTARGETPARAMSENTRY_H */




