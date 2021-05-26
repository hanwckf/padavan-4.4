/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * Author: TH <tsunghan_tsai@richtek.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef PD_CORE_H_
#define PD_CORE_H_

#include "tcpci_timer.h"
#include "tcpci_event.h"
#include "pd_dbg_info.h"
#include "tcpci_config.h"

#ifdef CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID
#define CONFIG_PD_DISCOVER_CABLE_ID
#endif /* CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID */

#ifdef CONFIG_USB_PD_DFP_READY_DISCOVER_ID
#undef CONFIG_PD_DISCOVER_CABLE_ID
#define CONFIG_PD_DISCOVER_CABLE_ID
#endif /* CONFIG_USB_PD_DFP_READY_DISCOVER_ID */

#ifdef CONFIG_USB_PD_ALT_MODE
#define CONFIG_USB_PD_MODE_OPERATION
#endif	/* CONFIG_USB_PD_ALT_MODE */

#ifdef CONFIG_USB_PD_ALT_MODE_RTDC
#undef CONFIG_USB_PD_MODE_OPERATION
#define CONFIG_USB_PD_MODE_OPERATION
#endif	/* CONFIG_USB_PD_ALT_MODE_RTDC */

#define PD_SOP_NR	3

/* Default retry count for transmitting */
#define PD_RETRY_COUNT 3

#if PD_RETRY_COUNT > 3
#error "PD_RETRY_COUNT Max = 3"
#endif

/* --- PD data message helpers --- */
#define PDO_MAX_OBJECTS   7
#define PDO_MODES (PDO_MAX_OBJECTS - 1)

/* PDO : Power Data Object */
/*
 * 1. The vSafe5V Fixed Supply Object shall always be the first object.
 * 2. The remaining Fixed Supply Objects,
 *    if present, shall be sent in voltage order; lowest to highest.
 * 3. The Battery Supply Objects,
 *    if present shall be sent in Minimum Voltage order; lowest to highest.
 * 4. The Variable Supply (non battery) Objects,
 *    if present, shall be sent in Minimum Voltage order; lowest to highest.
 */
#define PDO_TYPE_FIXED    (0 << 30)
#define PDO_TYPE_BATTERY  (1 << 30)
#define PDO_TYPE_VARIABLE (2 << 30)
#define PDO_TYPE_MASK     (3 << 30)

#define PDO_FIXED_DUAL_ROLE (1 << 29) /* Dual role device */
#define PDO_FIXED_SUSPEND   (1 << 28) /* USB Suspend supported (SRC)*/
#define PDO_FIXED_HIGH_CAP	(1 << 28) /* Higher Capability (SNK )*/
#define PDO_FIXED_EXTERNAL  (1 << 27) /* Externally powered */
#define PDO_FIXED_COMM_CAP  (1 << 26) /* USB Communications Capable */
#define PDO_FIXED_DATA_SWAP (1 << 25) /* Data role swap command supported */

#define PDO_FIXED_PEAK_CURR(i) \
	((i & 0x03) << 20) /* [21..20] Peak current */
#define PDO_FIXED_VOLT(mv)  \
	((((mv)/50) & 0x3fff) << 10) /* Voltage in 50mV units */
#define PDO_FIXED_CURR(ma)  \
	((((ma)/10) & 0x3fff) << 0)  /* Max current in 10mA units */

#define PDO_TYPE(raw)	(raw & PDO_TYPE_MASK)

#define PDO_FIXED_EXTRACT_VOLT_RAW(raw)	(((raw) >> 10) & 0x3ff)
#define PDO_FIXED_EXTRACT_CURR_RAW(raw)	(((raw) >> 0) & 0x3ff)
#define PDO_FIXED_EXTRACT_VOLT(raw)	(PDO_FIXED_EXTRACT_VOLT_RAW(raw) * 50)
#define PDO_FIXED_EXTRACT_CURR(raw)	(PDO_FIXED_EXTRACT_CURR_RAW(raw) * 10)
#define PDO_FIXED_RESET_CURR(raw, ma)	\
	((raw & ~0x3ff) | PDO_FIXED_CURR(ma))

#define PDO_FIXED(mv, ma, flags) (PDO_FIXED_VOLT(mv) |\
				  PDO_FIXED_CURR(ma) | (flags))

#define PDO_VAR_MAX_VOLT(mv) ((((mv) / 50) & 0x3FF) << 20)
#define PDO_VAR_MIN_VOLT(mv) ((((mv) / 50) & 0x3FF) << 10)
#define PDO_VAR_OP_CURR(ma)  ((((ma) / 10) & 0x3FF) << 0)

#define PDO_VAR_EXTRACT_MAX_VOLT_RAW(raw)	(((raw) >> 20) & 0x3ff)
#define PDO_VAR_EXTRACT_MIN_VOLT_RAW(raw)	(((raw) >> 10) & 0x3ff)
#define PDO_VAR_EXTRACT_CURR_RAW(raw)		(((raw) >> 0) & 0x3ff)

#define PDO_VAR_EXTRACT_MAX_VOLT(raw)	(PDO_VAR_EXTRACT_MAX_VOLT_RAW(raw) * 50)
#define PDO_VAR_EXTRACT_MIN_VOLT(raw)	(PDO_VAR_EXTRACT_MIN_VOLT_RAW(raw) * 50)
#define PDO_VAR_EXTRACT_CURR(raw)	(PDO_VAR_EXTRACT_CURR_RAW(raw) * 10)

#define PDO_VAR_RESET_CURR(raw, ma)	\
	((raw & ~0x3ff) | PDO_VAR_OP_CURR(ma))

#define PDO_VAR(min_mv, max_mv, op_ma) \
				(PDO_VAR_MIN_VOLT(min_mv) | \
				 PDO_VAR_MAX_VOLT(max_mv) | \
				 PDO_VAR_OP_CURR(op_ma)   | \
				 PDO_TYPE_VARIABLE)

#define PDO_BATT_MAX_VOLT(mv) ((((mv) / 50) & 0x3FF) << 20)
#define PDO_BATT_MIN_VOLT(mv) ((((mv) / 50) & 0x3FF) << 10)
#define PDO_BATT_OP_POWER(mw) ((((mw) / 250) & 0x3FF) << 0)

#define PDO_BATT_EXTRACT_MAX_VOLT_RAW(raw)	(((raw) >> 20) & 0x3ff)
#define PDO_BATT_EXTRACT_MIN_VOLT_RAW(raw)	(((raw) >> 10) & 0x3ff)
#define PDO_BATT_EXTRACT_OP_POWER_RAW(raw)	(((raw) >> 0) & 0x3ff)

#define PDO_BATT_EXTRACT_MAX_VOLT(raw)	\
	(PDO_BATT_EXTRACT_MAX_VOLT_RAW(raw) * 50)
#define PDO_BATT_EXTRACT_MIN_VOLT(raw)	\
	(PDO_BATT_EXTRACT_MIN_VOLT_RAW(raw) * 50)
#define PDO_BATT_EXTRACT_OP_POWER(raw)	\
	(PDO_BATT_EXTRACT_OP_POWER_RAW(raw) * 250)

#define PDO_BATT(min_mv, max_mv, op_mw) \
				(PDO_BATT_MIN_VOLT(min_mv) | \
				 PDO_BATT_MAX_VOLT(max_mv) | \
				 PDO_BATT_OP_POWER(op_mw) | \
				 PDO_TYPE_BATTERY)

/* RDO : Request Data Object */
#define RDO_OBJ_POS(n)             (((n) & 0x7) << 28)
#define RDO_POS(rdo)               (((rdo) >> 28) & 0x7)
#define RDO_GIVE_BACK              (1 << 27)
#define RDO_CAP_MISMATCH           (1 << 26)
#define RDO_COMM_CAP               (1 << 25)
#define RDO_NO_SUSPEND             (1 << 24)
#define RDO_FIXED_VAR_OP_CURR(ma)  ((((ma) / 10) & 0x3FF) << 10)
#define RDO_FIXED_VAR_MAX_CURR(ma) ((((ma) / 10) & 0x3FF) << 0)

#define RDO_FIXED_VAR_EXTRACT_OP_CURR(raw)	(((raw >> 10 & 0x3ff)) * 10)
#define RDO_FIXED_VAR_EXTRACT_MAX_CURR(raw)	(((raw >> 0 & 0x3ff)) * 10)

#define RDO_BATT_OP_POWER(mw)      ((((mw) / 250) & 0x3FF) << 10)
#define RDO_BATT_MAX_POWER(mw)     ((((mw) / 250) & 0x3FF) << 0)

#define RDO_BATT_EXTRACT_OP_POWER(raw)	(((raw >> 10 & 0x3ff)) * 250)
#define RDO_BATT_EXTRACT_MAX_POWER(raw)	(((raw >> 0 & 0x3ff)) * 250)

#define RDO_FIXED(n, op_ma, max_ma, flags) \
				(RDO_OBJ_POS(n) | (flags) | \
				RDO_FIXED_VAR_OP_CURR(op_ma) | \
				RDO_FIXED_VAR_MAX_CURR(max_ma))


#define RDO_BATT(n, op_mw, max_mw, flags) \
				(RDO_OBJ_POS(n) | (flags) | \
				RDO_BATT_OP_POWER(op_mw) | \
				RDO_BATT_MAX_POWER(max_mw))

/* BDO : BIST Data Object */
#define BDO_MODE_RECV       (0 << 28)
#define BDO_MODE_TRANSMIT   (1 << 28)
#define BDO_MODE_COUNTERS   (2 << 28)
#define BDO_MODE_CARRIER0   (3 << 28)
#define BDO_MODE_CARRIER1   (4 << 28)
#define BDO_MODE_CARRIER2   (5 << 28)
#define BDO_MODE_CARRIER3   (6 << 28)
#define BDO_MODE_EYE        (7 << 28)
#define BDO_MODE_TEST_DATA	(8 << 28)

#define BDO_MODE(obj)		(obj & (0xf << 28))
#define BDO(mode, cnt)      ((mode) | ((cnt) & 0xFFFF))

#define SVID_DISCOVERY_MAX 16

/* Protocol revision */
#define PD_REV10 0
#define PD_REV20 1

/* build message header */

#define PD_HEADER_SOP(msg_type, prole, drole, id, cnt) \
		((msg_type) | (PD_REV20 << 6) | \
		 ((drole) << 5) | ((prole) << 8) | \
		 ((id) << 9) | ((cnt) << 12))

#define PD_HEADER_SOP_PRIME(msg_type, cable_plug, id, cnt) \
		((msg_type) | (PD_REV20 << 6) | \
		 ((cable_plug) << 8) | \
		 ((id) << 9) | ((cnt) << 12))

#define PD_HEADER_CNT(header)  (((header) >> 12) & 7)
#define PD_HEADER_TYPE(header) ((header) & 0xF)
#define PD_HEADER_ID(header)   (((header) >> 9) & 7)
#define PD_HEADER_PR(header)	(((header) >> 8) & 1)
#define PD_HEADER_DR(header)	(((header) >> 5) & 1)





/*
 * VDO : Vendor Defined Message Object
 * VDM object is minimum of VDM header + 6 additional data objects.
 */

/*
 * VDM header
 * ----------
 * <31:16>  :: SVID
 * <15>     :: VDM type ( 1b == structured, 0b == unstructured )
 * <14:13>  :: Structured VDM version (can only be 00 == 1.0 currently)
 * <12:11>  :: reserved
 * <10:8>   :: object position (1-7 valid ... used for enter/exit mode only)
 * <7:6>    :: command type (SVDM only?)
 * <5>      :: reserved (SVDM), command type (UVDM)
 * <4:0>    :: command
 */
#define VDO_MAX_SIZE		(7)
#define VDO_MAX_DATA_SIZE	(VDO_MAX_SIZE-1)
#define VDO_MAX_SVID_SIZE	(VDO_MAX_DATA_SIZE*2)

#define VDO(vid, type, custom)				\
	(((vid) << 16) |				\
	 ((type) << 15) |				\
	 ((custom) & 0x7FFF))

#define VDO_S(svid, cmd_type, cmd, obj)	\
	VDO(svid, 1, VDO_CMDT(cmd_type) | VDO_OPOS(obj) | cmd)

#define VDO_SVDM_TYPE     (1 << 15)
#define VDO_SVDM_VERS(x)  (x << 13)
#define VDO_OPOS(x)       (x << 8)
#define VDO_CMDT(x)       (x << 6)
#define VDO_OPOS_MASK     VDO_OPOS(0x7)
#define VDO_CMDT_MASK     VDO_CMDT(0x3)

#define CMDT_INIT     0
#define CMDT_RSP_ACK  1
#define CMDT_RSP_NAK  2
#define CMDT_RSP_BUSY 3


/* reserved for SVDM ... for Google UVDM */
#define VDO_SRC_INITIATOR (0 << 5)
#define VDO_SRC_RESPONDER (1 << 5)

#define CMD_DISCOVER_IDENT  1
#define CMD_DISCOVER_SVID   2
#define CMD_DISCOVER_MODES  3
#define CMD_ENTER_MODE      4
#define CMD_EXIT_MODE       5
#define CMD_ATTENTION       6
#define CMD_DP_STATUS      16
#define CMD_DP_CONFIG      17

#define VDO_CMD_VENDOR(x)    (((10 + (x)) & 0x1f))

/* ChromeOS specific commands */
#define VDO_CMD_VERSION      VDO_CMD_VENDOR(0)
#define VDO_CMD_SEND_INFO    VDO_CMD_VENDOR(1)
#define VDO_CMD_READ_INFO    VDO_CMD_VENDOR(2)
#define VDO_CMD_REBOOT       VDO_CMD_VENDOR(5)
#define VDO_CMD_FLASH_ERASE  VDO_CMD_VENDOR(6)
#define VDO_CMD_FLASH_WRITE  VDO_CMD_VENDOR(7)
#define VDO_CMD_ERASE_SIG    VDO_CMD_VENDOR(8)
#define VDO_CMD_PING_ENABLE  VDO_CMD_VENDOR(10)
#define VDO_CMD_CURRENT      VDO_CMD_VENDOR(11)
#define VDO_CMD_FLIP         VDO_CMD_VENDOR(12)
#define VDO_CMD_GET_LOG      VDO_CMD_VENDOR(13)
#define VDO_CMD_CCD_EN       VDO_CMD_VENDOR(14)

#define PD_VDO_VID(vdo)  ((vdo) >> 16)
#define PD_VDO_SVDM(vdo) (((vdo) >> 15) & 1)
#define PD_VDO_OPOS(vdo) (((vdo) >> 8) & 0x7)
#define PD_VDO_CMD(vdo)  ((vdo) & 0x1f)
#define PD_VDO_CMDT(vdo) (((vdo) >> 6) & 0x3)


/*
 * SVDM Identity request -> response
 *
 * Request is simply properly formatted SVDM header
 *
 * Response is 4 data objects:
 * [0] :: SVDM header
 * [1] :: Identitiy header
 * [2] :: Cert Stat VDO
 * [3] :: (Product | Cable) VDO
 * [4] :: AMA VDO
 *
 */
#define VDO_INDEX_HDR     0
#define VDO_INDEX_IDH     1
#define VDO_INDEX_CSTAT   2
#define VDO_INDEX_CABLE   4
#define VDO_INDEX_PRODUCT 3
#define VDO_INDEX_AMA     4
#define VDO_I(name) VDO_INDEX_##name

/*
 * SVDM Identity Header
 * --------------------
 * <31>     :: data capable as a USB host
 * <30>     :: data capable as a USB device
 * <29:27>  :: product type
 * <26>     :: modal operation supported (1b == yes)
 * <25:16>  :: SBZ
 * <15:0>   :: USB-IF assigned VID for this cable vendor
 */
#define IDH_PTYPE_UNDEF  0
#define IDH_PTYPE_HUB    1
#define IDH_PTYPE_PERIPH 2
#define IDH_PTYPE_PCABLE 3
#define IDH_PTYPE_ACABLE 4
#define IDH_PTYPE_AMA    5

#define VDO_IDH(usbh, usbd, ptype, is_modal, vid)		\
	((usbh) << 31 | (usbd) << 30 | ((ptype) & 0x7) << 27	\
	 | (is_modal) << 26 | ((vid) & 0xffff))

#define PD_IDH_PTYPE(vdo) (((vdo) >> 27) & 0x7)
#define PD_IDH_VID(vdo)   ((vdo) & 0xffff)

#define PD_IDH_MODAL_SUPPORT	(1<<26)

/*
 * Cert Stat VDO
 * -------------
 * <31:20> : SBZ
 * <19:0>  : USB-IF assigned TID for this cable
 */
#define VDO_CSTAT(tid)    ((tid) & 0xfffff)
#define PD_CSTAT_TID(vdo) ((vdo) & 0xfffff)

/*
 * Product VDO
 * -----------
 * <31:16> : USB Product ID
 * <15:0>  : USB bcdDevice
 */
#define VDO_PRODUCT(pid, bcd) (((pid) & 0xffff) << 16 | ((bcd) & 0xffff))
#define PD_PRODUCT_PID(vdo) (((vdo) >> 16) & 0xffff)

/*
 * Cable VDO
 * ---------
 * <31:28> :: Cable HW version
 * <27:24> :: Cable FW version
 * <23:20> :: SBZ
 * <19:18> :: type-C to Type-A/B/C (00b == A, 01 == B, 10 == C)
 * <17>    :: Type-C to Plug/Receptacle (0b == plug, 1b == receptacle)
 * <16:13> :: cable latency (0001 == <10ns(~1m length))
 * <12:11> :: cable termination type (11b == both ends active VCONN req)
 * <10>    :: SSTX1 Directionality support (0b == fixed, 1b == cfgable)
 * <9>     :: SSTX2 Directionality support
 * <8>     :: SSRX1 Directionality support
 * <7>     :: SSRX2 Directionality support
 * <6:5>   :: Vbus current handling capability
 * <4>     :: Vbus through cable (0b == no, 1b == yes)
 * <3>     :: SOP" controller present? (0b == no, 1b == yes)
 * <2:0>   :: USB SS Signaling support
 */
#define CABLE_ATYPE 0
#define CABLE_BTYPE 1
#define CABLE_CTYPE 2
#define CABLE_PLUG       0
#define CABLE_RECEPTACLE 1
#define CABLE_CURR_1A5   0
#define CABLE_CURR_3A    1
#define CABLE_CURR_5A    2
#define CABLE_USBSS_U2_ONLY  0
#define CABLE_USBSS_U31_GEN1 1
#define CABLE_USBSS_U31_GEN2 2
#define VDO_CABLE(hw, fw, cbl, gdr, lat, term, tx1d,\
			tx2d, rx1d, rx2d, cur, vps, sopp, usbss) \
	(((hw) & 0x7) << 28 | ((fw) & 0x7) << 24 | ((cbl) & 0x3) << 18	\
	 | (gdr) << 17 | ((lat) & 0x7) << 13 | ((term) & 0x3) << 11	\
	 | (tx1d) << 10 | (tx2d) << 9 | (rx1d) << 8 | (rx2d) << 7	\
	 | ((cur) & 0x3) << 5 | (vps) << 4 | (sopp) << 3		\
	 | ((usbss) & 0x7))

#define PD_VDO_CABLE_CURR(x)	(((x) >> 5) & 0x03)

/*
 * AMA VDO
 * ---------
 * <31:28> :: Cable HW version
 * <27:24> :: Cable FW version
 * <23:12> :: SBZ
 * <11>    :: SSTX1 Directionality support (0b == fixed, 1b == cfgable)
 * <10>    :: SSTX2 Directionality support
 * <9>     :: SSRX1 Directionality support
 * <8>     :: SSRX2 Directionality support
 * <7:5>   :: Vconn power
 * <4>     :: Vconn power required
 * <3>     :: Vbus power required
 * <2:0>   :: USB SS Signaling support
 */
#define VDO_AMA(hw, fw, tx1d, tx2d, rx1d, rx2d, vcpwr, vcr, vbr, usbss) \
	(((hw) & 0x7) << 28 | ((fw) & 0x7) << 24			\
	 | (tx1d) << 11 | (tx2d) << 10 | (rx1d) << 9 | (rx2d) << 8	\
	 | ((vcpwr) & 0x3) << 5 | (vcr) << 4 | (vbr) << 3		\
	 | ((usbss) & 0x7))

#define PD_VDO_AMA_VCONN_REQ(vdo) (((vdo) >> 4) & 1)
#define PD_VDO_AMA_VBUS_REQ(vdo)  (((vdo) >> 3) & 1)

#define AMA_VCONN_PWR_1W   0
#define AMA_VCONN_PWR_1W5  1
#define AMA_VCONN_PWR_2W   2
#define AMA_VCONN_PWR_3W   3
#define AMA_VCONN_PWR_4W   4
#define AMA_VCONN_PWR_5W   5
#define AMA_VCONN_PWR_6W   6
#define AMA_USBSS_U2_ONLY  0
#define AMA_USBSS_U31_GEN1 1
#define AMA_USBSS_U31_GEN2 2
#define AMA_USBSS_BBONLY   3

/*
 * SVDM Discover SVIDs request -> response
 *
 * Request is properly formatted VDM Header with discover SVIDs command.
 * Response is a set of SVIDs of all all supported SVIDs with all zero's to
 * mark the end of SVIDs.  If more than 12 SVIDs are supported command SHOULD be
 * repeated.
 */
#define VDO_SVID(svid0, svid1) (((svid0) & 0xffff) << 16 | ((svid1) & 0xffff))
#define PD_VDO_SVID_SVID0(vdo) ((vdo) >> 16)
#define PD_VDO_SVID_SVID1(vdo) ((vdo) & 0xffff)

/*
 * Google modes capabilities
 * <31:8> : reserved
 * <7:0>  : mode
 */
#define VDO_MODE_GOOGLE(mode) (mode & 0xff)

#define MODE_GOOGLE_FU 1 /* Firmware Update mode */

/*
 * Mode Capabilities
 *
 * Number of VDOs supplied is SID dependent (but <= 6 VDOS?)
 */
#define VDO_MODE_CNT_DISPLAYPORT 1

/*
 * DisplayPort modes capabilities
 * -------------------------------
 * <31:24> : SBZ
 * <23:16> : UFP_D pin assignment supported
 * <15:8>  : DFP_D pin assignment supported
 * <7>     : USB 2.0 signaling (0b=yes, 1b=no)
 * <6>     : Plug | Receptacle (0b == plug, 1b == receptacle)
 * <5:2>   : xxx1: Supports DPv1.3, xx1x Supports USB Gen 2 signaling
 *           Other bits are reserved.
 * <1:0>   : signal direction ( 00b=rsv, 01b=sink, 10b=src 11b=both )
 */
#define VDO_MODE_DP(snkp, srcp, usb, gdr, sign, sdir)			\
	(((snkp) & 0xff) << 16 | ((srcp) & 0xff) << 8			\
	 | ((usb) & 1) << 7 | ((gdr) & 1) << 6 | ((sign) & 0xF) << 2	\
	 | ((sdir) & 0x3))
#define PD_DP_PIN_CAPS(x) ((((x) >> 6) & 0x1) ? (((x) >> 16) & 0x3f)	\
			   : (((x) >> 8) & 0x3f))

#define MODE_DP_PIN_A 0x01
#define MODE_DP_PIN_B 0x02
#define MODE_DP_PIN_C 0x04
#define MODE_DP_PIN_D 0x08
#define MODE_DP_PIN_E 0x10
#define MODE_DP_PIN_F 0x20

/* Pin configs B/D/F support multi-function */
#define MODE_DP_PIN_MF_MASK 0x2a
/* Pin configs A/B support BR2 signaling levels */
#define MODE_DP_PIN_BR2_MASK 0x3
/* Pin configs C/D/E/F support DP signaling levels */
#define MODE_DP_PIN_DP_MASK 0x3c

#define MODE_DP_V13  0x1
#define MODE_DP_GEN2 0x2

#define MODE_DP_SNK  0x1
#define MODE_DP_SRC  0x2
#define MODE_DP_BOTH 0x3

#define MODE_DP_PORT_CAP(raw)		(raw & 0x03)
#define MODE_DP_SIGNAL_SUPPORT(raw)	((raw>>2) & 0x0f)
#define MODE_DP_RECEPT(mode)	((mode >> 6) & 0x01)

#define MODE_DP_PIN_DFP(mode)	((mode >> 8) & 0xff)
#define MODE_DP_PIN_UFP(mode)	((mode >> 16) & 0xff)

#define PD_DP_DFP_D_PIN_CAPS(x)	(MODE_DP_RECEPT(x) ? \
		MODE_DP_PIN_DFP(x) : MODE_DP_PIN_UFP(x))

#define PD_DP_UFP_D_PIN_CAPS(x)	(MODE_DP_RECEPT(x) ? \
		MODE_DP_PIN_UFP(x) : MODE_DP_PIN_DFP(x))

/*
 * DisplayPort Status VDO
 * ----------------------
 * <31:9> : SBZ
 * <8>    : IRQ_HPD : 1 == irq arrived since last message otherwise 0.
 * <7>    : HPD state : 0 = HPD_LOW, 1 == HPD_HIGH
 * <6>    : Exit DP Alt mode: 0 == maintain, 1 == exit
 * <5>    : USB config : 0 == maintain current, 1 == switch to USB from DP
 * <4>    : Multi-function preference : 0 == no pref, 1 == MF preferred.
 * <3>    : enabled : is DPout on/off.
 * <2>    : power low : 0 == normal or LPM disabled, 1 == DP disabled for LPM
 * <1:0>  : connect status : 00b ==  no (DFP|UFP)_D is connected or disabled.
 *          01b == DFP_D connected, 10b == UFP_D connected, 11b == both.
 */

#define VDO_DP_STATUS(irq, lvl, amode, usbc, mf, en, lp, conn)		\
	(((irq) & 1) << 8 | ((lvl) & 1) << 7 | ((amode) & 1) << 6	\
	 | ((usbc) & 1) << 5 | ((mf) & 1) << 4 | ((en) & 1) << 3	\
	 | ((lp) & 1) << 2 | ((conn & 0x3) << 0))

#define PD_VDO_DPSTS_HPD_IRQ(x) (((x) >> 8) & 1)
#define PD_VDO_DPSTS_HPD_LVL(x) (((x) >> 7) & 1)
#define PD_VDO_DPSTS_MF_PREF(x) (((x) >> 4) & 1)

#define PD_VDO_DPSTS_CONNECT(x)	(((x) >> 0) & 0x03)

#define DPSTS_DISCONNECT		0

#define DPSTS_DFP_D_CONNECTED	(1 << 0)
#define DPSTS_UFP_D_CONNECTED	(1 << 1)
#define DPSTS_BOTH_CONNECTED	(DPSTS_DFP_D_CONNECTED | DPSTS_UFP_D_CONNECTED)

/* UFP_U only */
#define DPSTS_DP_ENABLED		(1<<3)
#define DPSTS_DP_MF_PREF		(1<<4)
#define DPSTS_DP_USB_CONFIG		(1<<5)
#define DPSTS_DP_EXIT_ALT_MODE	(1<<6)

/* UFP_D only */
#define DPSTS_DP_HPD_STATUS		(1<<7)
#define DPSTS_DP_HPD_IRQ		(1<<8)

/* Per DisplayPort Spec v1.3 Section 3.3 */
#define HPD_USTREAM_DEBOUNCE_LVL (2*MSEC)
#define HPD_USTREAM_DEBOUNCE_IRQ (250)
#define HPD_DSTREAM_DEBOUNCE_IRQ (750)  /* between 500-1000us */

/*
 * DisplayPort Configure VDO
 * -------------------------
 * <31:24> : SBZ
 * <23:16> : SBZ
 * <15:8>  : Pin assignment requested.  Choose one from mode caps.
 * <7:6>   : SBZ
 * <5:2>   : signalling : 1h == DP v1.3, 2h == Gen 2
 *           Oh is only for USB, remaining values are reserved
 * <1:0>   : cfg : 00 == USB, 01 == DFP_D, 10 == UFP_D, 11 == reserved
 */

#define DP_CONFIG_USB				0
#define DP_CONFIG_DFP_D				1
#define DP_CONFIG_UFP_D				2

#define VDO_DP_CFG(pin, sig, cfg) \
	(((pin) & 0xff) << 8 | ((sig) & 0xf) << 2 | ((cfg) & 0x3))

#if 0
#define VDO_DP_DFP_CFG(pin, sig) \
	(((pin) & 0xff) << 8 | ((sig) & 0xf) << 2 | (DP_CONFIG_DFP_D))

#define VDO_DP_UFP_CFG(pin, sig) \
	(((pin) & 0xff) << 16 | ((sig) & 0xf) << 2 | (DP_CONFIG_UFP_D))
#endif

#define VDO_DP_DFP_CFG(pin, sig) VDO_DP_CFG(pin, sig, DP_CONFIG_DFP_D)
#define VDO_DP_UFP_CFG(pin, sig) VDO_DP_CFG(pin, sig, DP_CONFIG_UFP_D)

#define PD_DP_CFG_USB(x)	((x & 0x3) == DP_CONFIG_USB)
#define PD_DP_CFG_DFP_D(x) ((x & 0x3) == DP_CONFIG_DFP_D)
#define PD_DP_CFG_UFP_D(x) ((x & 0x3) == DP_CONFIG_UFP_D)
#define PD_DP_CFG_DPON(x) (PD_DP_CFG_DFP_D(x) | PD_DP_CFG_UFP_D(x))

#define DP_SIG_DPV13	(0x01)
#define DP_SIG_GEN2	(0x02)

#define DP_PIN_ASSIGN_SUPPORT_A		(1 << 0)
#define DP_PIN_ASSIGN_SUPPORT_B		(1 << 1)
#define DP_PIN_ASSIGN_SUPPORT_C		(1 << 2)
#define DP_PIN_ASSIGN_SUPPORT_D		(1 << 3)
#define DP_PIN_ASSIGN_SUPPORT_E		(1 << 4)
#define DP_PIN_ASSIGN_SUPPORT_F		(1 << 5)

/*
 * Get the pin assignment mask
 * for backward compatibility, if it is null,
 * get the former sink pin assignment we used to be in <23:16>.
 */

#define PD_DP_CFG_PIN(x) (((x) >> 8) & 0xff)

/*
 * ChromeOS specific PD device Hardware IDs. Used to identify unique
 * products and used in VDO_INFO. Note this field is 10 bits.
 */
#define USB_PD_HW_DEV_ID_RESERVED    0
#define USB_PD_HW_DEV_ID_ZINGER      1
#define USB_PD_HW_DEV_ID_MINIMUFFIN  2
#define USB_PD_HW_DEV_ID_DINGDONG    3
#define USB_PD_HW_DEV_ID_HOHO        4
#define USB_PD_HW_DEV_ID_HONEYBUNS   5

/*
 * ChromeOS specific VDO_CMD_READ_INFO responds with device info including:
 * RW Hash: First 20 bytes of SHA-256 of RW (20 bytes)
 * HW Device ID: unique descriptor for each ChromeOS model (2 bytes)
 *               top 6 bits are minor revision, bottom 10 bits are major
 * SW Debug Version: Software version useful for debugging (15 bits)
 * IS RW: True if currently in RW, False otherwise (1 bit)
 */
#define VDO_INFO(id, id_minor, ver, is_rw) ((id_minor) << 26 \
				  | ((id) & 0x3ff) << 16 \
				  | ((ver) & 0x7fff) << 1 \
				  | ((is_rw) & 1))
#define VDO_INFO_HW_DEV_ID(x)    ((x) >> 16)
#define VDO_INFO_SW_DBG_VER(x)   (((x) >> 1) & 0x7fff)
#define VDO_INFO_IS_RW(x)        ((x) & 1)

#define HW_DEV_ID_MAJ(x) (x & 0x3ff)
#define HW_DEV_ID_MIN(x) ((x) >> 10)

/* USB-IF SIDs */
#define USB_SID_PD		0xff00	/* power delivery */
#define USB_SID_DISPLAYPORT	0xff01	/* display port */
#define USB_SID_RICHTEK	0x29cf  /* demo uvdm */
#define USB_SID_DIRECTCHARGE	0x29cf  /* direct charge */

/* DPM Flags */

#define DPM_FLAGS_PARTNER_DR_POWER		(1<<0)
#define DPM_FLAGS_PARTNER_DR_DATA		(1<<1)
#define DPM_FLAGS_PARTNER_EXTPOWER		(1<<2)
#define DPM_FLAGS_PARTNER_USB_COMM		(1<<3)
#define DPM_FLAGS_PARTNER_USB_SUSPEND	(1<<4)
#define DPM_FLAGS_PARTNER_HIGH_CAP		(1<<5)

#define DPM_FLAGS_PARTNER_MISMATCH		(1<<7)
#define DPM_FLAGS_PARTNER_GIVE_BACK		(1<<8)
#define DPM_FLAGS_PARTNER_NO_SUSPEND	(1<<9)

#define DPM_FLAGS_RESET_PARTNER_MASK	\
	(DPM_FLAGS_PARTNER_DR_POWER | DPM_FLAGS_PARTNER_DR_DATA|\
	DPM_FLAGS_PARTNER_EXTPOWER | DPM_FLAGS_PARTNER_USB_COMM)

#define DPM_FLAGS_CHECK_DC_MODE			(1<<20)
#define DPM_FLAGS_CHECK_UFP_SVID		(1<<21)
#define DPM_FLAGS_CHECK_EXT_POWER		(1<<22)
#define DPM_FLAGS_CHECK_DP_MODE			(1<<23)
#define DPM_FLAGS_CHECK_SINK_CAP		(1<<24)
#define DPM_FLAGS_CHECK_SOURCE_CAP		(1<<25)
#define DPM_FLAGS_CHECK_UFP_ID			(1<<26)
#define DPM_FLAGS_CHECK_CABLE_ID		(1<<27)
#define DPM_FLAGS_CHECK_CABLE_ID_DFP		(1<<28)
#define DPM_FLAGS_CHECK_PR_ROLE			(1<<29)
#define DPM_FLAGS_CHECK_DR_ROLE			(1<<30)

/* DPM_CAPS */

#define DPM_CAP_LOCAL_DR_POWER			(1<<0)
#define DPM_CAP_LOCAL_DR_DATA			(1<<1)
#define DPM_CAP_LOCAL_EXT_POWER			(1<<2)
#define DPM_CAP_LOCAL_USB_COMM			(1<<3)
#define DPM_CAP_LOCAL_USB_SUSPEND		(1<<4)
#define DPM_CAP_LOCAL_HIGH_CAP			(1<<5)
#define DPM_CAP_LOCAL_GIVE_BACK			(1<<6)
#define DPM_CAP_LOCAL_NO_SUSPEND		(1<<7)
#define DPM_CAP_LOCAL_VCONN_SUPPLY		(1<<8)

#define DPM_CAP_ATTEMP_ENTER_DC_MODE		(1<<11)
#define DPM_CAP_ATTEMP_DISCOVER_CABLE_DFP	(1<<12)
#define DPM_CAP_ATTEMP_ENTER_DP_MODE		(1<<13)
#define DPM_CAP_ATTEMP_DISCOVER_CABLE		(1<<14)
#define DPM_CAP_ATTEMP_DISCOVER_ID		(1<<15)

enum dpm_cap_pr_check_prefer {
	DPM_CAP_PR_CHECK_DISABLE = 0,
	DPM_CAP_PR_CHECK_PREFER_SNK = 1,
	DPM_CAP_PR_CHECK_PREFER_SRC = 2,
};

#define DPM_CAP_PR_CHECK_PROP(cap)			((cap & 0x03) << 16)
#define DPM_CAP_EXTRACT_PR_CHECK(raw)		((raw >> 16) & 0x03)
#define DPM_CAP_PR_SWAP_REJECT_AS_SRC		(1<<18)
#define DPM_CAP_PR_SWAP_REJECT_AS_SNK		(1<<19)
#define DPM_CAP_PR_SWAP_CHECK_GP_SRC		(1<<20)
#define DPM_CAP_PR_SWAP_CHECK_GP_SNK		(1<<21)
#define DPM_CAP_PR_SWAP_CHECK_GOOD_POWER	\
	(DPM_CAP_PR_SWAP_CHECK_GP_SRC | DPM_CAP_PR_SWAP_CHECK_GP_SNK)

enum dpm_cap_dr_check_prefer {
	DPM_CAP_DR_CHECK_DISABLE = 0,
	DPM_CAP_DR_CHECK_PREFER_UFP = 1,
	DPM_CAP_DR_CHECK_PREFER_DFP = 2,
};

#define DPM_CAP_DR_CHECK_PROP(cap)		((cap & 0x03) << 22)
#define DPM_CAP_EXTRACT_DR_CHECK(raw)		((raw >> 22) & 0x03)
#define DPM_CAP_DR_SWAP_REJECT_AS_DFP		(1<<24)
#define DPM_CAP_DR_SWAP_REJECT_AS_UFP		(1<<25)

#define DPM_CAP_DP_PREFER_MF				(1<<29)
#define DPM_CAP_SNK_PREFER_LOW_VOLTAGE		(1<<30)
#define DPM_CAP_SNK_IGNORE_MISMATCH_CURRENT	(1<<31)

/* PD counter definitions */
#define PD_MESSAGE_ID_COUNT	7
#define PD_HARD_RESET_COUNT	2
#define PD_CAPS_COUNT			50
#define PD_GET_SNK_CAP_RETRIES	3
#define PD_GET_SRC_CAP_RETRIES	3
#define PD_DISCOVER_ID_COUNT	3	/* max : 20 */

enum {
	PD_WAIT_VBUS_DISABLE = 0,
	PD_WAIT_VBUS_VALID_ONCE = 1,
	PD_WAIT_VBUS_INVALID_ONCE = 2,
	PD_WAIT_VBUS_SAFE0V_ONCE = 3,
	PD_WAIT_VBUS_STABLE_ONCE = 4,
};

struct pd_port_power_caps {
	uint8_t nr;
	uint32_t pdos[7];
};

#define PD_SVID_DATA_NR		2	/* must < 11 */

struct svdm_mode_t {
	uint8_t mode_cnt;
	uint32_t mode_vdo[VDO_MAX_DATA_SIZE];
};

struct svdm_svid_ops_t;
struct svdm_svid_data_t {
	bool exist;
	uint16_t svid;
	uint8_t active_mode;
	struct svdm_mode_t local_mode;
	struct svdm_mode_t remote_mode;
	const struct svdm_svid_ops_t *ops;
};

struct svdm_svid_list_t {
	uint8_t cnt;
	uint16_t svids[VDO_MAX_SVID_SIZE];
};

struct pd_port_t {
	struct tcpc_device *tcpc_dev;
	struct mutex pd_lock;

	/* PD */
	bool explicit_contract;
	bool invalid_contract;
	bool vconn_source;

#ifdef CONFIG_USB_PD_DFP_READY_DISCOVER_ID
	bool vconn_return;
#endif	/* CONFIG_USB_PD_DFP_READY_DISCOVER_ID */

	bool pe_ready;
	bool pd_connected;
	bool pd_prev_connected;
	bool msg_output_lock;

	uint8_t state_machine;
	uint8_t pd_connect_state;

	bool reset_vdm_state;
	uint8_t pe_pd_state;
	uint8_t pe_vdm_state;

	uint8_t pe_state_next;
	uint8_t pe_state_curr;

	uint8_t data_role;
	uint8_t power_role;

	uint8_t cap_counter;
	uint8_t discover_id_counter;
	uint8_t hard_reset_counter;
	uint8_t snk_cap_count;
	uint8_t src_cap_count;
	uint8_t get_snk_cap_count;
	uint8_t get_src_cap_count;

#ifdef CONFIG_USB_PD_RECV_HRESET_COUNTER
	uint8_t recv_hard_reset_count;
#endif	/* CONFIG_USB_PD_RECV_HRESET_COUNTER */

	uint8_t msg_id_rx[PD_SOP_NR];
	uint8_t msg_id_rx_init[PD_SOP_NR];
	uint8_t msg_id_tx[PD_SOP_NR];

	uint32_t last_rdo;
	uint32_t cable_vdos[VDO_MAX_SIZE];
	bool power_cable_present;

	uint8_t id_vdo_nr;
	uint32_t id_vdos[VDO_MAX_DATA_SIZE];

#ifdef CONFIG_USB_PD_KEEP_SVIDS
	struct svdm_svid_list_t remote_svid_list;
#endif	/* CONFIG_USB_PD_KEEP_SVIDS */

	uint8_t svid_data_cnt;
	struct svdm_svid_data_t svid_data[PD_SVID_DATA_NR];

	bool during_swap;	/* pr or dr swap */

/* DPM */
	int request_v;
	int request_i;
	int request_v_new;
	int request_i_new;
	int request_i_op;
	int request_i_max;

	uint8_t local_selected_cap;
	uint8_t remote_selected_cap;
	struct pd_port_power_caps local_src_cap;
	struct pd_port_power_caps local_snk_cap;
	struct pd_port_power_caps local_src_cap_default;
	struct pd_port_power_caps remote_src_cap;
	struct pd_port_power_caps remote_snk_cap;

	uint16_t mode_svid;
	uint8_t mode_obj_pos;
	bool modal_operation;
	bool dpm_ack_immediately;

#ifdef CONFIG_USB_PD_DFP_FLOW_DELAY
	bool dpm_dfp_flow_delay_done;
#endif	/* CONFIG_USB_PD_DFP_FLOW_DELAY */

	uint32_t dpm_flags;
	uint32_t dpm_init_flags;
	uint32_t dpm_caps;
	uint32_t dpm_dfp_retry_cnt;

/* ALT Mode */
#ifdef CONFIG_USB_PD_ALT_MODE

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	uint32_t local_dp_config;
	uint32_t remote_dp_config;
	uint8_t dp_ufp_u_attention;
	uint8_t dp_dfp_u_state;
#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */

	uint32_t dp_status;
	uint8_t dp_ufp_u_state;

	uint8_t dp_first_connected;
	uint8_t dp_second_connected;
#endif	/* CONFIG_USB_PD_ALT_MODE */

#ifdef CONFIG_USB_PD_UVDM
	bool uvdm_wait_resp;
	uint8_t uvdm_cnt;
	uint16_t uvdm_svid;
	uint32_t uvdm_data[VDO_MAX_SIZE];
#endif	/* CONFIG_USB_PD_UVDM */

#ifdef CONFIG_USB_PD_ALT_MODE_RTDC
	uint8_t dc_dfp_state;
	uint32_t dc_pass_code;
#endif	/* CONFIG_USB_PD_ALT_MODE_RTDC */

#ifdef CONFIG_USB_PD_CUSTOM_DBGACC
	bool custom_dbgacc;
#endif	/* CONFIG_USB_PD_CUSTOM_DBGACC */
};

extern int pd_core_init(struct tcpc_device *tcpc_dev);
int pd_alert_vbus_changed(struct pd_port_t *pd_port, int vbus_level);

static inline int pd_is_auto_discover_cable_id(struct pd_port_t *pd_port)
{
	if (pd_port->dpm_flags & DPM_FLAGS_CHECK_CABLE_ID) {

		if (pd_port->discover_id_counter < PD_DISCOVER_ID_COUNT)
			return true;

		pd_port->dpm_flags &= ~DPM_FLAGS_CHECK_CABLE_ID;
		return false;
	}

	return false;
}

static inline int pd_is_support_modal_operation(struct pd_port_t *pd_port)
{
	if (!(pd_port->id_vdos[0] & PD_IDH_MODAL_SUPPORT))
		return false;

	return (pd_port->svid_data_cnt > 0);
}

/* new definitions*/

#define PD_RX_CAP_PE_IDLE				(0)
#define PD_RX_CAP_PE_DISABLE			(TCPC_RX_CAP_HARD_RESET)
#define PD_RX_CAP_PE_STARTUP			(TCPC_RX_CAP_HARD_RESET)
#define PD_RX_CAP_PE_HARDRESET			(0)
#define PD_RX_CAP_PE_SEND_WAIT_CAP	\
	(TCPC_RX_CAP_HARD_RESET|TCPC_RX_CAP_SOP)
#define PD_RX_CAP_PE_DISCOVER_CABLE	\
	(TCPC_RX_CAP_HARD_RESET|TCPC_RX_CAP_SOP_PRIME)
#define PD_RX_CAP_PE_READY_UFP	\
	(TCPC_RX_CAP_HARD_RESET|TCPC_RX_CAP_SOP)

#ifdef CONFIG_PD_DISCOVER_CABLE_ID
#define PD_RX_CAP_PE_READY_DFP	\
	(TCPC_RX_CAP_HARD_RESET|TCPC_RX_CAP_SOP|TCPC_RX_CAP_SOP_PRIME)
#else
#define PD_RX_CAP_PE_READY_DFP	(TCPC_RX_CAP_HARD_RESET|TCPC_RX_CAP_SOP)
#endif

enum {
	PD_BIST_MODE_DISABLE = 0,
	PD_BIST_MODE_EVENT_PENDING,
	PD_BIST_MODE_TEST_DATA,
};

void pd_reset_svid_data(struct pd_port_t *pd_port);
int pd_reset_protocol_layer(struct pd_port_t *pd_port);

int pd_set_rx_enable(struct pd_port_t *pd_port, uint8_t enable);

int pd_enable_vbus_valid_detection(struct pd_port_t *pd_port, bool wait_valid);
int pd_enable_vbus_safe0v_detection(struct pd_port_t *pd_port);
int pd_enable_vbus_stable_detection(struct pd_port_t *pd_port);

uint32_t pd_reset_pdo_power(uint32_t pdo, uint32_t imax);

void pd_extract_rdo_power(
	uint32_t rdo, uint32_t pdo, uint32_t *op_curr, uint32_t *max_curr);

void pd_extract_pdo_power(uint32_t pdo,
	uint32_t *vmin, uint32_t *vmax, uint32_t *ioper);

uint32_t pd_extract_cable_curr(uint32_t vdo);


int pd_set_data_role(struct pd_port_t *pd_port, uint8_t dr);
int pd_set_power_role(struct pd_port_t *pd_port, uint8_t pr);
int pd_init_role(struct pd_port_t *pd_port, uint8_t pr, uint8_t dr, bool vr);

int pd_set_cc_res(struct pd_port_t *pd_port, int pull);
int pd_set_vconn(struct pd_port_t *pd_port, int enable);
int pd_reset_local_hw(struct pd_port_t *pd_port);

int pd_enable_bist_test_mode(struct pd_port_t *pd_port, bool en);

void pd_lock_msg_output(struct pd_port_t *pd_port);
void pd_unlock_msg_output(struct pd_port_t *pd_port);

int pd_update_connect_state(struct pd_port_t *pd_port, uint8_t state);
void pd_update_dpm_request_state(struct pd_port_t *pd_port, uint8_t state);

/* ---- PD notify TCPC Policy Engine State Changed ---- */

extern void pd_try_put_pe_idle_event(struct pd_port_t *pd_port);
extern void pd_notify_pe_transit_to_default(struct pd_port_t *pd_port);
extern void pd_notify_pe_hard_reset_completed(struct pd_port_t *pd_port);
extern void pd_notify_pe_send_hard_reset(struct pd_port_t *pd_port);
extern void pd_notify_pe_idle(struct pd_port_t *pd_port);
extern void pd_notify_pe_running(struct pd_port_t *pd_port);
extern void pd_notify_pe_wait_vbus_once(struct pd_port_t *pd_port, int wait_evt);
extern void pd_notify_pe_error_recovery(struct pd_port_t *pd_port);
extern void pd_notify_pe_execute_pr_swap(struct pd_port_t *pd_port, bool start_swap);
extern void pd_notify_pe_cancel_pr_swap(struct pd_port_t *pd_port);
extern void pd_notify_pe_reset_protocol(struct pd_port_t *pd_port);
extern void pd_noitfy_pe_bist_mode(struct pd_port_t *pd_port, uint8_t mode);
extern void pd_notify_pe_pr_changed(struct pd_port_t *pd_port);
extern void pd_notify_pe_src_explicit_contract(struct pd_port_t *pd_port);
extern void pd_notify_pe_transmit_msg(struct pd_port_t *pd_port, uint8_t type);
extern void pd_notify_pe_recv_ping_event(struct pd_port_t *pd_port);

#ifdef CONFIG_USB_PD_RECV_HRESET_COUNTER
extern void pd_notify_pe_over_recv_hreset(struct pd_port_t *pd_port);
#endif	/* CONFIG_USB_PD_RECV_HRESET_COUNTER */

/* ---- pd_timer ---- */

static inline void pd_restart_timer(struct pd_port_t *pd_port, uint32_t timer_id)
{
	return tcpc_restart_timer(pd_port->tcpc_dev, timer_id);
}

static inline void pd_enable_timer(struct pd_port_t *pd_port, uint32_t timer_id)
{
	return tcpc_enable_timer(pd_port->tcpc_dev, timer_id);
}

static inline void pd_disable_timer(struct pd_port_t *pd_port, uint32_t timer_id)
{
	return tcpc_disable_timer(pd_port->tcpc_dev, timer_id);
}

static inline void pd_reset_pe_timer(struct pd_port_t *pd_port)
{
	tcpc_reset_pe_timer(pd_port->tcpc_dev);
}

/* ---- pd_event ---- */

static inline void pd_free_pd_event(struct pd_port_t *pd_port, struct pd_event_t *pd_event)
{
	pd_free_event(pd_port->tcpc_dev, pd_event);
}

static inline bool pd_put_pe_event(struct pd_port_t *pd_port, uint8_t pe_event)
{
	struct pd_event_t evt = {
		.event_type = PD_EVT_PE_MSG,
		.msg = pe_event,
		.pd_msg = NULL,
	};

	return pd_put_event(pd_port->tcpc_dev, &evt, false);
}

static inline bool pd_put_dpm_event(struct pd_port_t *pd_port, uint8_t event)
{
	struct pd_event_t evt = {
		.event_type = PD_EVT_DPM_MSG,
		.msg = event,
		.pd_msg = NULL,
	};

	return pd_put_event(pd_port->tcpc_dev, &evt, false);
}

static inline bool pd_put_dpm_pd_request_event(
	struct pd_port_t *pd_port, uint8_t event)
{
	struct pd_event_t evt = {
		.event_type = PD_EVT_DPM_MSG,
		.msg = PD_DPM_PD_REQUEST,
		.msg_sec = event,
		.pd_msg = NULL,
	};

	return pd_put_event(pd_port->tcpc_dev, &evt, false);
}

static inline bool vdm_put_dpm_vdm_request_event(
	struct pd_port_t *pd_port, uint8_t event)
{
	bool ret;
	struct pd_event_t evt = {
		.event_type = PD_EVT_DPM_MSG,
		.msg = PD_DPM_VDM_REQUEST,
		.msg_sec = event,
		.pd_msg = NULL,
	};

	ret = pd_put_vdm_event(pd_port->tcpc_dev, &evt, false);

	if (ret) {
		pd_port->reset_vdm_state = true;
		pd_port->pe_vdm_state = pd_port->pe_pd_state;
	}

	return ret;
}

static inline bool pd_put_dpm_notify_event(struct pd_port_t *pd_port, uint8_t notify)
{
	struct pd_event_t evt = {
		.event_type = PD_EVT_DPM_MSG,
		.msg = PD_DPM_NOTIFIED,
		.msg_sec = notify,
		.pd_msg = NULL,
	};

	return pd_put_event(pd_port->tcpc_dev, &evt, false);
}

static inline bool pd_put_dpm_ack_event(struct pd_port_t *pd_port)
{
	struct pd_event_t evt = {
		.event_type = PD_EVT_DPM_MSG,
		.msg = PD_DPM_ACK,
		.pd_msg = NULL,
	};

	return pd_put_event(pd_port->tcpc_dev, &evt, false);
}

static inline bool pd_put_dpm_nak_event(struct pd_port_t *pd_port, uint8_t notify)
{
	struct pd_event_t evt = {
		.event_type = PD_EVT_DPM_MSG,
		.msg = PD_DPM_NAK,
		.msg_sec = notify,
		.pd_msg = NULL,
	};

	return pd_put_event(pd_port->tcpc_dev, &evt, false);
}

static inline bool vdm_put_hw_event(
	struct tcpc_device *tcpc_dev, uint8_t hw_event)
{
	struct pd_event_t evt = {
		.event_type = PD_EVT_HW_MSG,
		.msg = hw_event,
		.pd_msg = NULL,
	};

	return pd_put_vdm_event(tcpc_dev, &evt, false);
}

static inline bool vdm_put_dpm_event(
	struct pd_port_t *pd_port, uint8_t dpm_event, struct pd_msg_t *pd_msg)
{
	struct pd_event_t evt = {
		.event_type = PD_EVT_DPM_MSG,
		.msg = dpm_event,
		.pd_msg = pd_msg,
	};

	return pd_put_vdm_event(pd_port->tcpc_dev, &evt, false);
}

static inline bool vdm_put_dpm_notified_event(struct pd_port_t *pd_port)
{
	return vdm_put_dpm_event(pd_port, PD_DPM_NOTIFIED, NULL);
}

static inline bool vdm_put_dpm_discover_cable_event(struct pd_port_t *pd_port)
{
	pd_port->pe_vdm_state = pd_port->pe_pd_state;
	return vdm_put_dpm_event(pd_port, PD_DPM_DISCOVER_CABLE_ID, NULL);
}

static inline bool pd_put_hw_event(
	struct tcpc_device *tcpc_dev, uint8_t hw_event)
{
	struct pd_event_t evt = {
		.event_type = PD_EVT_HW_MSG,
		.msg = hw_event,
		.pd_msg = NULL,
	};

	return pd_put_event(tcpc_dev, &evt, false);
}

static inline bool pd_put_cc_attached_event(
		struct tcpc_device *tcpc_dev, uint8_t type)
{
	struct pd_event_t evt = {
		.event_type = PD_EVT_HW_MSG,
		.msg = PD_HW_CC_ATTACHED,
		.msg_sec = type,
		.pd_msg = NULL,
	};

	return pd_put_event(tcpc_dev, &evt, false);
}

/* ---- Handle PD Message ----*/

int pd_handle_soft_reset(struct pd_port_t *pd_port, uint8_t state_machine);

/* ---- Send PD Message ----*/

int pd_send_ctrl_msg(
	struct pd_port_t *pd_port, uint8_t sop_type, uint8_t msg);

int pd_send_data_msg(struct pd_port_t *pd_port,
	uint8_t sop_type, uint8_t msg, uint8_t cnt, uint32_t *payload);

int pd_send_soft_reset(struct pd_port_t *pd_port, uint8_t state_machine);
int pd_send_hard_reset(struct pd_port_t *pd_port);

int pd_send_bist_mode2(struct pd_port_t *pd_port);
int pd_disable_bist_mode2(struct pd_port_t *pd_port);


/* ---- Send / Reply SVDM Command ----*/

/* Auto enable timer if success */
int pd_send_svdm_request(struct pd_port_t *pd_port,
	uint8_t sop_type, uint16_t svid, uint8_t vdm_cmd,
	uint8_t obj_pos, uint8_t cnt, uint32_t *data_obj,
	uint32_t timer_id);

int pd_reply_svdm_request(struct pd_port_t *pd_port,
	struct pd_event_t *pd_event, uint8_t reply, uint8_t cnt, uint32_t *data_obj);

static inline int pd_send_vdm_discover_id(
	struct pd_port_t *pd_port, uint8_t sop_type)
{
	return pd_send_svdm_request(pd_port, sop_type, USB_SID_PD,
		CMD_DISCOVER_IDENT, 0, 0, NULL, PD_TIMER_VDM_RESPONSE);
}

static inline int pd_send_vdm_discover_svids(
	struct pd_port_t *pd_port, uint8_t sop_type)
{
	return pd_send_svdm_request(pd_port, sop_type, USB_SID_PD,
		CMD_DISCOVER_SVID, 0, 0, NULL, PD_TIMER_VDM_RESPONSE);
}

static inline int pd_send_vdm_discover_modes(
	struct pd_port_t *pd_port, uint8_t sop_type, uint16_t svid)
{
	return pd_send_svdm_request(pd_port, sop_type, svid,
		CMD_DISCOVER_MODES, 0, 0, NULL, PD_TIMER_VDM_RESPONSE);
}

static inline int pd_send_vdm_enter_mode(
	struct pd_port_t *pd_port, uint8_t sop_type, uint16_t svid, uint8_t obj_pos)
{
	return pd_send_svdm_request(pd_port, sop_type, svid,
		CMD_ENTER_MODE, obj_pos, 0, NULL, PD_TIMER_VDM_MODE_ENTRY);
}

static inline int pd_send_vdm_exit_mode(
	struct pd_port_t *pd_port, uint8_t sop_type, uint16_t svid, uint8_t obj_pos)
{
	return pd_send_svdm_request(pd_port, sop_type, svid,
		CMD_EXIT_MODE, obj_pos, 0, NULL, PD_TIMER_VDM_MODE_EXIT);
}

static inline int pd_send_vdm_attention(
	struct pd_port_t *pd_port, uint8_t sop_type, uint16_t svid, uint8_t obj_pos)
{
	return pd_send_svdm_request(pd_port, sop_type, svid,
		CMD_ATTENTION, obj_pos, 0, NULL, 0);
}

static inline int pd_send_vdm_dp_attention(struct pd_port_t *pd_port,
	uint8_t sop_type, uint8_t obj_pos, uint32_t dp_status)
{
	return pd_send_svdm_request(pd_port, sop_type, USB_SID_DISPLAYPORT,
		CMD_ATTENTION, obj_pos, 1, &dp_status, 0);
}

static inline int pd_send_vdm_dp_status(struct pd_port_t *pd_port,
	uint8_t sop_type, uint8_t obj_pos, uint8_t cnt, uint32_t *data_obj)
{
	return pd_send_svdm_request(pd_port, sop_type, USB_SID_DISPLAYPORT,
		CMD_DP_STATUS, obj_pos, cnt, data_obj, PD_TIMER_VDM_RESPONSE);
}

static inline int pd_send_vdm_dp_config(struct pd_port_t *pd_port,
	uint8_t sop_type, uint8_t obj_pos, uint8_t cnt, uint32_t *data_obj)
{
	return pd_send_svdm_request(pd_port, sop_type, USB_SID_DISPLAYPORT,
		CMD_DP_CONFIG, obj_pos, cnt, data_obj, PD_TIMER_VDM_RESPONSE);
}

static inline int pd_reply_svdm_request_simply(
	struct pd_port_t *pd_port, struct pd_event_t *pd_event, uint8_t reply)
{
	return pd_reply_svdm_request(pd_port, pd_event, reply, 0, NULL);
}

#ifdef CONFIG_USB_PD_UVDM
static inline int pd_send_uvdm(struct pd_port_t *pd_port, uint8_t sop_type)
{
	return pd_send_data_msg(
			pd_port, sop_type, PD_DATA_VENDOR_DEF,
			pd_port->uvdm_cnt, pd_port->uvdm_data);
}

static inline int pd_reply_uvdm(struct pd_port_t *pd_port, uint8_t sop_type,
	uint8_t cnt, uint32_t *payload)
{
	return pd_send_data_msg(
			pd_port, sop_type, PD_DATA_VENDOR_DEF,
			cnt, payload);
}
#endif	/* CONFIG_USB_PD_UVDM */

#endif /* PD_CORE_H_ */
