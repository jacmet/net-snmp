/*
 * Note: this file originally auto-generated by mib2c using
 *  : generic-table-constants.m2c,v 1.2 2004/03/29 23:28:42 rstory Exp $
 *
 * $Id$
 */
#ifndef IFXTABLE_CONSTANTS_H
#define IFXTABLE_CONSTANTS_H

#ifdef __cplusplus
extern          "C" {
#endif


    /*
     * column number definitions for table ifXTable 
     */
#define IFXTABLE_OID              1,3,6,1,2,1,31,1,1
#define COLUMN_IFNAME		1
#define COLUMN_IFINMULTICASTPKTS		2
#define COLUMN_IFINBROADCASTPKTS		3
#define COLUMN_IFOUTMULTICASTPKTS		4
#define COLUMN_IFOUTBROADCASTPKTS		5
#define COLUMN_IFHCINOCTETS		6
#define COLUMN_IFHCINUCASTPKTS		7
#define COLUMN_IFHCINMULTICASTPKTS		8
#define COLUMN_IFHCINBROADCASTPKTS		9
#define COLUMN_IFHCOUTOCTETS		10
#define COLUMN_IFHCOUTUCASTPKTS		11
#define COLUMN_IFHCOUTMULTICASTPKTS		12
#define COLUMN_IFHCOUTBROADCASTPKTS		13
#define COLUMN_IFLINKUPDOWNTRAPENABLE		14
#define COLUMN_IFHIGHSPEED		15
#define COLUMN_IFPROMISCUOUSMODE		16
#define COLUMN_IFCONNECTORPRESENT		17
#define COLUMN_IFALIAS		18
#define COLUMN_IFCOUNTERDISCONTINUITYTIME		19

#define IFXTABLE_MIN_COL		COLUMN_IFNAME
#define IFXTABLE_MAX_COL		COLUMN_IFCOUNTERDISCONTINUITYTIME


    /*
     * NOTES on emus
     * =============
     *
     * Value Mapping
     * -------------
     * If the values for your data type don't exactly match the
     * possible values defined by the mib, you should map them
     * below. For example, a boolean flag (1/0) is usually represented
     * as a TruthValue in a MIB, which maps to the values (1/2).
     *
     */

/*************************************************************************
 *************************************************************************
 *
 * enum definitions for table ifXTable
 *
 *************************************************************************
 *************************************************************************/

/*************************************************************
 * constants for enums for the MIB node
 * ifLinkUpDownTrapEnable (INTEGER / ASN_INTEGER)
 *
 * since a Textual Convention may be referenced more than once in a
 * MIB, protect againt redifinitions of the enum values.
 */
#ifndef ifLinkUpDownTrapEnable_ENUMS
#define ifLinkUpDownTrapEnable_ENUMS

#define IFLINKUPDOWNTRAPENABLE_ENABLED  1
#define IFLINKUPDOWNTRAPENABLE_DISABLED  2


#endif                          /* ifLinkUpDownTrapEnable_ENUMS */

    /*
     * TODO:
     * value mapping (set notes at top of file)
     */
#define INTERNAL_IFLINKUPDOWNTRAPENABLE_ENABLED  1
#define INTERNAL_IFLINKUPDOWNTRAPENABLE_DISABLED  0

/*************************************************************
 * constants for enums for the MIB node
 * ifPromiscuousMode (TruthValue / ASN_INTEGER)
 *
 * since a Textual Convention may be referenced more than once in a
 * MIB, protect againt redifinitions of the enum values.
 */
#ifndef TruthValue_ENUMS
#define TruthValue_ENUMS

#define TRUTHVALUE_TRUE  1
#define TRUTHVALUE_FALSE  2


#endif                          /* TruthValue_ENUMS */

    /*
     * TODO:
     * value mapping (set notes at top of file)
     */
#define INTERNAL_IFPROMISCUOUSMODE_TRUE  1
#define INTERNAL_IFPROMISCUOUSMODE_FALSE  0

/*************************************************************
 * constants for enums for the MIB node
 * ifConnectorPresent (TruthValue / ASN_INTEGER)
 *
 * since a Textual Convention may be referenced more than once in a
 * MIB, protect againt redifinitions of the enum values.
 */
#ifndef TruthValue_ENUMS
#define TruthValue_ENUMS

#define TRUTHVALUE_TRUE  1
#define TRUTHVALUE_FALSE  2


#endif                          /* TruthValue_ENUMS */

    /*
     * TODO:
     * value mapping (set notes at top of file)
     */
#define INTERNAL_IFCONNECTORPRESENT_TRUE  1
#define INTERNAL_IFCONNECTORPRESENT_FALSE  0




#ifdef __cplusplus
};
#endif

#endif                          /* IFXTABLE_OIDS_H */
