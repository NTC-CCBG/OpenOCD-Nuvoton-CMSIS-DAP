/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef OPENOCD_LOADERS_FLASH_NCT_ESIO_NCT6694D_FLASH_H
#define OPENOCD_LOADERS_FLASH_NCT_ESIO_NCT6694D_FLASH_H

/* Define one bit mask */
#define BIT0	(0x00000001)	///< Bit 0 mask of an 32 bit integer
#define BIT1	(0x00000002)	///< Bit 1 mask of an 32 bit integer
#define BIT2	(0x00000004)	///< Bit 2 mask of an 32 bit integer
#define BIT3	(0x00000008)	///< Bit 3 mask of an 32 bit integer
#define BIT4	(0x00000010)	///< Bit 4 mask of an 32 bit integer
#define BIT5	(0x00000020)	///< Bit 5 mask of an 32 bit integer
#define BIT6	(0x00000040)	///< Bit 6 mask of an 32 bit integer
#define BIT7	(0x00000080)	///< Bit 7 mask of an 32 bit integer
#define BIT8	(0x00000100)	///< Bit 8 mask of an 32 bit integer
#define BIT9	(0x00000200)	///< Bit 9 mask of an 32 bit integer
#define BIT10	(0x00000400)	///< Bit 10 mask of an 32 bit integer
#define BIT11	(0x00000800)	///< Bit 11 mask of an 32 bit integer
#define BIT12	(0x00001000)	///< Bit 12 mask of an 32 bit integer
#define BIT13	(0x00002000)	///< Bit 13 mask of an 32 bit integer
#define BIT14	(0x00004000)	///< Bit 14 mask of an 32 bit integer
#define BIT15	(0x00008000)	///< Bit 15 mask of an 32 bit integer
#define BIT16	(0x00010000)	///< Bit 16 mask of an 32 bit integer
#define BIT17	(0x00020000)	///< Bit 17 mask of an 32 bit integer
#define BIT18	(0x00040000)	///< Bit 18 mask of an 32 bit integer
#define BIT19	(0x00080000)	///< Bit 19 mask of an 32 bit integer
#define BIT20	(0x00100000)	///< Bit 20 mask of an 32 bit integer
#define BIT21	(0x00200000)	///< Bit 21 mask of an 32 bit integer
#define BIT22	(0x00400000)	///< Bit 22 mask of an 32 bit integer
#define BIT23	(0x00800000)	///< Bit 23 mask of an 32 bit integer
#define BIT24	(0x01000000)	///< Bit 24 mask of an 32 bit integer
#define BIT25	(0x02000000)	///< Bit 25 mask of an 32 bit integer
#define BIT26	(0x04000000)	///< Bit 26 mask of an 32 bit integer
#define BIT27	(0x08000000)	///< Bit 27 mask of an 32 bit integer
#define BIT28	(0x10000000)	///< Bit 28 mask of an 32 bit integer
#define BIT29	(0x20000000)	///< Bit 29 mask of an 32 bit integer
#define BIT30	(0x40000000)	///< Bit 30 mask of an 32 bit integer
#define BIT31	(0x80000000)	///< Bit 31 mask of an 32 bit integer

#define RegSetBit(Reg, BitMsk)	{(Reg) |= (BitMsk);}	// Set register bit
#define RegClrBit(Reg, BitMsk)	{(Reg) &= ~(BitMsk);}	// Clear register bit

enum flash_algo_command {
	FLASH_ALGO_CMD_INIT = 0,
	FLASH_ALGO_CMD_ERASE,
	FLASH_ALGO_CMD_PROGRAM,
	FLASH_ALGO_CMD_READ,
};

enum flash_algo_state {
	FLASH_ALGO_STATE_IDLE = 0,
	FLASH_ALGO_STATE_BUSY,
};

enum flash_port {
	FLASH_PORT_PVT = 0,
	FLASH_PORT_SHD,
	FLASH_PORT_BKP,
	FLASH_PORT_INTERNAL = 0xFF,
};

enum flash_mode {
	FLASH_MODE_SINGLE,
	FLASH_MODE_DUAL,
	FLASH_MODE_QUAD,
};

enum flash_command {
	FLASH_CMD_WRITE_STATUS_REG		= 0x01,
	FLASH_CMD_PAGE_PROGRAM			= 0x02,
	FLASH_CMD_READ_DATA			= 0x03,
	FLASH_CMD_WRITE_DISABLE			= 0x04,
	FLASH_CMD_READ_STATUS_REG		= 0x05,
	FLASH_CMD_WRITE_ENABLE			= 0x06,
	FLASH_CMD_4BYTE_PROGRAM			= 0x12,
	FLASH_CMD_4BYTE_READ			= 0x13,
	FLASH_CMD_SECTOR_ERASE			= 0x20,
	FLASH_CMD_4BYTE_SECTOR_ERASE		= 0x21,
	FLASH_CMD_READ_STATUS2_REG		= 0x35,
	FLASH_CMD_ENTER_4BYTE_MODE		= 0xB7,
	FLASH_CMD_EXIT_4BYTE_MODE		= 0xE9,
	FLASH_CMD_READ_JEDEC_ID			= 0x9F,
};

/* Flash loader parameters */
struct __attribute__((__packed__)) nct_esio_flash_params {
	volatile uint8_t state;		/* Flash loader state */
	volatile uint8_t cmd;		/* Flash loader command */
	volatile uint8_t type;		/* Flash type */
	volatile uint8_t adr_4b;	/* 4-byte address mode */
	volatile uint32_t version;	/* Flash loader version */
	volatile uint32_t addr; 	/* Address in flash */
	volatile uint32_t len;  	/* Number of bytes */
	volatile uint32_t reserved; /* Reserved */
};

/*==============================================================================================================*/
/* SPI Master Mode (SPIM) Registers																				*/
/*==============================================================================================================*/
typedef struct {
	volatile unsigned long CTL0;		/*!< [0x00]	Control and Status Register 0				*/
	volatile unsigned long CTL1;		/*!< [0x04]	Control Register 1							*/
	volatile unsigned long RESERVED1[1];
	volatile unsigned long RXCLKDLY;	/*!< [0x0C]	RX Clock Delay Control Register				*/
	volatile unsigned long RX[4];		/*!< [0x10]	Data Receive Register						*/
	volatile unsigned long TX[4];		/*!< [0x20]	Data Transmit Register						*/
	volatile unsigned long SRAMADDR;	/*!< [0x30]	SRAM Memory Address Register				*/
	volatile unsigned long DMACNT;		/*!< [0x34]	DMA Transfer Byte Count Register			*/
	volatile unsigned long FADDR;		/*!< [0x38]	SPI Flash Address Register					*/
	volatile unsigned long KEY1;		/*!< [0x3C]	Cipher Key1 Register						*/
	volatile unsigned long KEY2;		/*!< [0x40]	Cipher Key2 Register						*/
	volatile unsigned long DMMCTL;		/*!< [0x44]	Direct Memory Mapping Mode Control Register	*/
	volatile unsigned long CTL2;		/*!< [0x48]	Control Register 2							*/
	volatile unsigned long VERSION;		/*!< [0x4C]	SPIM Version Control Register				*/
} SPIM_T;

#define SPIM	((SPIM_T *)	(0x40017000))

/*----------------------*/
/* SPIM_CTL0 fields	*/
/*----------------------*/
#define SPIM_CTL0_CMDCODE_Pos	(24)
#define SPIM_CTL0_CMDCODE_Msk	(0xFF << SPIM_CTL0_CMDCODE_Pos)

#define SPIM_CTL0_OPMODE_Pos	(22)
#define SPIM_CTL0_OPMODE_Msk	(0x3 << SPIM_CTL0_OPMODE_Pos)

#define SPIM_CTL0_BITMODE_Pos	(20)
#define SPIM_CTL0_BITMODE_Msk	(0x3 << SPIM_CTL0_BITMODE_Pos)

#define SPIM_CTL0_QDIODIR_Pos	(15)
#define SPIM_CTL0_QDIODIR_Msk	(0x1 << SPIM_CTL0_QDIODIR_Pos)

#define SPIM_CTL0_BURSTNUM_Pos	(13)
#define SPIM_CTL0_BURSTNUM_Msk	(0x3 << SPIM_CTL0_BURSTNUM_Pos)

#define SPIM_CTL0_DWIDTH_Pos	(8)
#define SPIM_CTL0_DWIDTH_Msk	(0x1F << SPIM_CTL0_DWIDTH_Pos)

#define SPIM_CTL0_CIPHOFF_Pos	(0)
#define SPIM_CTL0_CIPHOFF_Msk	(0x1 << SPIM_CTL0_CIPHOFF_Pos)

/*----------------------*/
/* SPIM_CTL1 fields	*/
/*----------------------*/
#define SPIM_CTL1_DIVIDER_Pos	(16)
#define SPIM_CTL1_DIVIDER_Msk	(0xFF << SPIM_CTL1_DIVIDER_Pos)

#define SPIM_CTL1_SS_Pos	(4)
#define SPIM_CTL1_SS_Msk	(0x1 << SPIM_CTL1_SS_Pos)

#define SPIM_CTL1_CDINVAL_Pos	(3)
#define SPIM_CTL1_CDINVAL_Msk	(0x1 << SPIM_CTL1_CDINVAL_Pos)

#define SPIM_CTL1_SPIMEN_Pos	(0)
#define SPIM_CTL1_SPIMEN_Msk	(0x1 << SPIM_CTL1_SPIMEN_Pos)

#define SPIM_MODE_NORMAL	0
#define SPIM_MODE_DMA_WRITE	1
#define SPIM_BITMODE_SINGLE	0

#define SPIM_CTL0_NORMAL(bit_mode, output, bitwidth, len) \
	((SPIM_MODE_NORMAL << SPIM_CTL0_OPMODE_Pos) | \
	((bit_mode) << SPIM_CTL0_BITMODE_Pos) | \
	((output) << SPIM_CTL0_QDIODIR_Pos) | \
	((bitwidth-1) << SPIM_CTL0_DWIDTH_Pos) | \
	(((len) - 1) << SPIM_CTL0_BURSTNUM_Pos) | \
	SPIM_CTL0_CIPHOFF_Msk);

/*==============================================================================================================*/
/* HIGH-FREQUENCY CLOCK GENERATOR (HFCG)																		*/
/*==============================================================================================================*/
typedef struct {
	volatile uint8_t CTRL;			/*!< [0x00]	HFCG Control				*/
	volatile uint8_t RESERVED1[1];
	volatile uint8_t ML;			/*!< [0x02]	HFCG M Low Byte Value		*/
	volatile uint8_t RESERVED2[1];
	volatile uint8_t MH;			/*!< [0x04]	HFCG M High Byte Value		*/
	volatile uint8_t RESERVED3[1];
	volatile uint8_t N;				/*!< [0x06]	HFCG N Value				*/
	volatile uint8_t RESERVED4[1];
	volatile uint8_t P;				/*!< [0x08]	HFCG Prescaler				*/
	volatile uint8_t RESERVED5[7];
	volatile uint8_t BCD;			/*!< [0x10]	HFCG Bus Clock Dividers		*/
	volatile uint8_t RESERVED6[1];
	volatile uint8_t BCD1;			/*!< [0x12]	HFCG Bus Clock Dividers 1	*/
	volatile uint8_t RESERVED7[1];
	volatile uint8_t BCD2;			/*!< [0x14]	HFCG Bus Clock Dividers 2	*/
} HFCG_T;

#define HFCG	((HFCG_T *)	(0x400B5000))

/*==============================================================================================================*/
/* System Configuration Core Registers																			*/
/*==============================================================================================================*/
typedef struct {
	volatile uint8_t DEVCNT;			/*!< [0x00]	Device Control						*/
	volatile const uint8_t STRPST;		/*!< [0x01]	Straps Status						*/
	volatile uint8_t RSTCTL;			/*!< [0x02]	Reset Control and Status			*/
	volatile uint8_t DEV_CTL2;			/*!< [0x03]	Device Control 2					*/
	volatile uint8_t DEV_CTL3;			/*!< [0x04]	Device Control 3					*/
	volatile uint8_t RESERVED1[1];
	volatile uint8_t DEV_CTL4;			/*!< [0x06]	Device Control 4					*/
	volatile uint8_t RESERVED2[4];
	volatile uint8_t DEVALT10;			/*!< [0x0B]	Device Alternate Function 10		*/
	volatile uint8_t DEVALT11;			/*!< [0x0C]	Device Alternate Function 11		*/
	volatile const uint8_t DEVALT12;	/*!< [0x0D]	Device Alternate Function 12		*/
	volatile uint8_t RESERVED3[2];
	volatile uint8_t DEVALT0;			/*!< [0x10]	Device Alternate Function 0			*/
	volatile uint8_t DEVALT1;			/*!< [0x11]	Device Alternate Function 1			*/
	volatile uint8_t DEVALT2;			/*!< [0x12]	Device Alternate Function 2			*/
	volatile uint8_t DEVALT3;			/*!< [0x13]	Device Alternate Function 3			*/
	volatile uint8_t DEVALT4;			/*!< [0x14]	Device Alternate Function 4			*/
	volatile uint8_t DEVALT5;			/*!< [0x15]	Device Alternate Function 5			*/
	volatile uint8_t DEVALT6;			/*!< [0x16]	Device Alternate Function 6			*/
	volatile uint8_t DEVALT7;			/*!< [0x17]	Device Alternate Function 7			*/
	volatile uint8_t DEVALT8;			/*!< [0x18]	Device Alternate Function 8			*/
	volatile uint8_t DEVALT9;			/*!< [0x19]	Device Alternate Function 9			*/
	volatile uint8_t DEVALTA;			/*!< [0x1A]	Device Alternate Function A			*/
	volatile uint8_t DEVALTB;			/*!< [0x1B]	Device Alternate Function B			*/
	volatile uint8_t DEVALTC;			/*!< [0x1C]	Device Alternate Function C			*/
	volatile uint8_t RESERVED6[1];
	volatile uint8_t DEVALTE;			/*!< [0x1E]	Device Alternate Function E			*/
	volatile uint8_t RESERVED7[3];
	volatile uint8_t DBGCTRL;			/*!< [0x22]	Device Alternate Function E			*/
	volatile uint8_t RESERVED8[1];
	volatile uint8_t DEVALTCX;			/*!< [0x24]	Device Alternate Function CX		*/
	volatile uint8_t RESERVED9[3];
	volatile uint8_t DEVPU0;			/*!< [0x28]	Device Pull-Up Enable 0				*/
	volatile uint8_t DEVPD1;			/*!< [0x29]	Device Pull-Down Enable 1			*/
	volatile uint8_t RESERVED10[1];
	volatile uint8_t LV_CTL1;			/*!< [0x2B]	Low-Voltage Pins Control 1 Register	*/
	volatile uint8_t RESERVED11[2];
	volatile uint8_t DEVALT2E;			/*!< [0x2E]	Device Alternate Function 2E		*/
	volatile uint8_t RESERVED12[1];
	volatile uint8_t DEVALT30;			/*!< [0x30]	Device Alternate Function 30		*/
	volatile uint8_t DEVALT31;			/*!< [0x31]	Device Alternate Function 31		*/
	volatile uint8_t DEVALT32;			/*!< [0x32]	Device Alternate Function 32		*/
	volatile uint8_t DEVALT33;			/*!< [0x33]	Device Alternate Function 33		*/
	volatile uint8_t DEVALT34;			/*!< [0x34]	Device Alternate Function 34		*/
	volatile uint8_t M4DIS;				/*!< [0x35]	Cortex-M4 function Disable			*/
	volatile uint8_t RESERVED13[22];
	volatile uint8_t EMCPHY_CTL;		/*!< [0x4C]	EMC Management Interface Control	*/
	volatile uint8_t EMC_CTL;			/*!< [0x4D]	EMC Control							*/
	volatile uint8_t RESERVED14[2];
	volatile uint8_t DEVALT50;			/*!< [0x50]	ACPI_HW_SW_ctl_0					*/
	volatile uint8_t DEVALT51;			/*!< [0x51]	ACPI_HW_SW_ctl_1					*/
	volatile uint8_t DEVALT52;			/*!< [0x52]	ACPI_HW_SW_ctl_2					*/
	volatile uint8_t DEVALT53;			/*!< [0x53]	ACPI_HW_SW_val_0					*/
	volatile uint8_t DEVALT54;			/*!< [0x54]	ACPI_HW_SW_val_1					*/
	volatile uint8_t DEVALT55;			/*!< [0x55]	ACPI_HW_SW_val_2					*/
	volatile uint8_t RESERVED15[3];
	volatile uint8_t DEVALT59;			/*!< [0x59]	ACPI_HW_OD_SEL_0					*/
	volatile uint8_t DEVALT5A;			/*!< [0x5A]	ACPI_HW_OD_SEL_1					*/
	volatile uint8_t DEVALT5B;			/*!< [0x5B]	ACPI_HW_OD_SEL_2					*/
	volatile uint8_t DEVALT5C;			/*!< [0x5C]	ACPI_GPIO_SEL_0						*/
	volatile uint8_t DEVALT5D;			/*!< [0x5D]	ACPI_GPIO_SEL_1						*/
	volatile uint8_t DEVALT5E;			/*!< [0x5E]	ACPI_GPIO_SEL_2						*/
	volatile uint8_t DEVALT5F;			/*!< [0x5F]	ACPI_GPIO_SEL_3						*/
	volatile uint8_t RESERVED16[2];
	volatile uint8_t DEVALT62;			/*!< [0x62]	Device Alternate Function 62		*/
	volatile uint8_t RESERVED17[3];
	volatile uint8_t DEVALT66;			/*!< [0x66]	Device Alternate Function 66		*/
	volatile uint8_t DEVALT67;			/*!< [0x67]	Device Alternate Function 67		*/
	volatile uint8_t RESERVED18[1];
	volatile uint8_t DEVALT69;			/*!< [0x69]	Device Alternate Function 69		*/
	volatile uint8_t RESERVED19[1];
	volatile uint8_t DEVALT6B;			/*!< [0x6B]	Device Alternate Function 6B		*/
	volatile uint8_t DEVALT6C;			/*!< [0x6C]	Device Alternate Function 6C		*/
	volatile uint8_t DEVALT6D;			/*!< [0x6D]	Device Alternate Function 6D		*/
	volatile uint8_t LV_CTL4;			/*!< [0x6E]	Low-Voltage Control 4				*/
	volatile uint8_t RESERVED20[4];
	volatile uint8_t DEVPU2;			/*!< [0x73]	Device Pull-Up Enable 2				*/
	volatile uint8_t RESERVED21[2];
	volatile uint8_t DBGFRZEN1;			/*!< [0x76]	Debug Freeze Enable 1				*/
	volatile uint8_t DBGFRZEN2;			/*!< [0x77]	Debug Freeze Enable 2				*/
	volatile uint8_t DBGFRZEN3;			/*!< [0x78]	Debug Freeze Enable 3				*/
	volatile uint8_t DBGFRZEN4;			/*!< [0x79]	Debug Freeze Enable 4				*/
} SCFG_T;

#define SCFG	((SCFG_T *)	(0x400C3000))

/*==============================================================================================================*/
/* FLASH INTERFACE UNIT (FIU)																					*/
/*==============================================================================================================*/
typedef struct {
	volatile uint8_t CFG;				/*!< [0x00]	FIU Configuration								*/
	volatile uint8_t BURST_CFG;			/*!< [0x01]	Burst Configuration								*/
	volatile uint8_t RESP_CFG;			/*!< [0x02]	FIU Response Configuration						*/
	volatile uint8_t Reserved1[17];
	volatile uint8_t SPI_FL_CFG;		/*!< [0x14]	SPI Flash Configuration							*/
	volatile uint8_t Reserved2[1];
	volatile uint8_t UMA_CODE;			/*!< [0x16]	UMA Code Byte									*/
	volatile uint8_t UMA_AB0;			/*!< [0x17]	UMA Address Byte 0								*/
	volatile uint8_t UMA_AB1;			/*!< [0x18]	UMA Address Byte 1								*/
	volatile uint8_t UMA_AB2;			/*!< [0x19]	UMA Address Byte 2								*/
	volatile uint8_t UMA_DB0;			/*!< [0x1A]	UMA Data Byte 0									*/
	volatile uint8_t UMA_DB1;			/*!< [0x1B]	UMA Data Byte 1									*/
	volatile uint8_t UMA_DB2;			/*!< [0x1C]	UMA Data Byte 2									*/
	volatile uint8_t UMA_DB3;			/*!< [0x1D]	UMA Data Byte 3									*/
	volatile uint8_t UMA_CTS;			/*!< [0x1E]	UMA Control and Status							*/
	volatile uint8_t UMA_ECTS;			/*!< [0x1F]	UMA Extended  Control and Status				*/
	volatile const uint32_t UMA_DB0_3;	/*!< [0x20]	UMA Data Bytes 0-3								*/
	volatile uint8_t Reserved3[2];
 	volatile uint8_t CRCCON;			/*!< [0x26]	CRC Control Register							*/
	volatile uint8_t CRCENT;			/*!< [0x27]	CRC Entry Register								*/
	volatile uint32_t CRCRSLT;			/*!< [0x28]	CRC Initialization and Result Register			*/
	volatile uint8_t Reserved4[2];
	volatile uint8_t RD_CMD_BKP;		/*!< [0x2E]	FIU Read Command (6694)							*/
	volatile uint8_t Reserved5;
	volatile uint8_t RD_CMD_PVT;		/*!< [0x30]	FIU Read Command (6694)							*/
	volatile uint8_t RD_CMD_SHD;		/*!< [0x31]	FIU Read Command (6694)							*/
	volatile uint8_t DMM_CYC;			/*!< [0x32]	FIU Dummy Cycles								*/
	volatile uint8_t EXT_CFG;			/*!< [0x33]	FIU Extended Configuration						*/
	volatile uint32_t UMA_AB0_3;		/*!< [0x34]	UMA Address Bytes 0-3							*/
	volatile uint8_t Reserved6[4];
	volatile uint8_t SET_CMD_EN;		/*!< [0x3C]	Set command enable in 4 Byte address mode		*/
	volatile uint8_t SHD_4B_EN;			/*!< [0x3D]	4 Byte address mode Enable						*/
	volatile uint8_t Reserved7[3];
	volatile uint8_t MI_CNT_THRSH;		/*!< [0x41]	Master Inactive Counter Threshold				*/
	volatile uint8_t MSR_STS;			/*!< [0x42]	FIU Master Status								*/
	volatile uint8_t MSR_IE_CFG;		/*!< [0x43]	FIU Master Interrupt Enable and Configuration	*/
	volatile uint8_t Q_P_EN;			/*!< [0x44]	FIU Master Interrupt Enable and Configuration	*/
	volatile uint8_t Reserved8[3];
	volatile uint8_t EXT_DB_CFG;		/*!< [0x48]	Quad Program Enable								*/
	volatile uint8_t DIRECT_WR_CFG;		/*!< [0x49]	SPI Direct Write Configuration					*/
	volatile uint8_t Reserved9[6];
	volatile uint8_t UMA_EXT_DB[16];	/*!< [0x50]	Extended Data Buffer							*/
} FIU_T;

#define FIU0	((FIU_T *)	(0x40020000))

//++-----------------------------------------++//

#define FIU_UMA_CTS_DEV_PRI_MSK	0
#define FIU_UMA_CTS_DEV_SHR_MSK	FIU_UMA_CTS_DEV_NUM_Msk

//++-----------------------------------------++//

#define FIU_Set_4B_CMD_EN(fiu, cmd)	{(fiu == FLASH_PORT_PVT)?(FIU0->SET_CMD_EN |= FIU_SET_PVT_CMD_EN_Msk): \
					((fiu == FLASH_PORT_BKP)?(FIU0->SET_CMD_EN |= FIU_SET_BKP_CMD_EN_Msk):\
					(FIU0->SET_CMD_EN |= FIU_SET_SHD_CMD_EN_Msk)); \
					FIU0->EXT_CFG |= (FIU_EXT_CFG_FOUR_BADDR_Msk); \
					(fiu == FLASH_PORT_PVT)?(FIU0->RD_CMD_PVT = cmd):\
					((fiu == FLASH_PORT_BKP)?(FIU0->RD_CMD_BKP = cmd):(FIU0->RD_CMD_SHD = cmd));}
#define FIU_Clr_4B_CMD_EN(fiu)		{(fiu == FLASH_PORT_PVT)?(FIU0->SET_CMD_EN &= ~FIU_SET_PVT_CMD_EN_Msk):\
					(fiu == FLASH_PORT_BKP)?(FIU0->SET_CMD_EN &= ~FIU_SET_BKP_CMD_EN_Msk):\
					(FIU0->SET_CMD_EN &= ~FIU_SET_SHD_CMD_EN_Msk);\
					FIU0->EXT_CFG &= ~(FIU_EXT_CFG_SET_CMD_EN_Msk | FIU_EXT_CFG_FOUR_BADDR_Msk); }

//++-----------------------------------------++//

#define FIU_Quad_Mode_Enable	{ FIU0->RESP_CFG |= FIU_RESP_CFG_QUAD_EN_Msk;}
#define FIU_Quad_Mode_Disable	{ FIU0->RESP_CFG &= ~FIU_RESP_CFG_QUAD_EN_Msk;}

//++-----------------------------------------++//

#define SET_FIU_CLK_DIV(div)    {HFCG->BCD1 = ((HFCG->BCD1&(~HFCG_BCD1_FIUDIV_Msk))|((div&0x03)<<HFCG_BCD1_FIUDIV_Pos));}

//++-----------------------------------------++//

#define BASE_FIU_PVT	0x60000000
#define BASE_FIU_SHD	0x70000000
#define BASE_FIU_BKP	0x80000000
#define BASE_FIU(n)	(0x60000000+(0x10000000*n))

// FIU pin confiugration
#define SET_FIU_SHD_TRI_STATE	{ SCFG->DEVCNT |= SCFG_DEVCNT_SHD_SPI_TRIS_Msk; }
#define SET_FIU_SHD_ACTIVE		{ SCFG->DEVCNT &= ~SCFG_DEVCNT_SHD_SPI_TRIS_Msk; }

#define ENABLE_FIU_SHD_QUAD		{ SCFG->DEVALTC |= SCFG_DEVALTC_SHD_SPI_QUAD_Msk; }
#define DISABLE_FIU_SHD_QUAD	{ SCFG->DEVALTC &= ~SCFG_DEVALTC_SHD_SPI_QUAD_Msk; }

#define ENABLE_FIU_SHD_SPI		{ SCFG->DEVALTC |= SCFG_DEVALTC_SHD_SPI_Msk; }
#define DISABLE_FIU_SHD_SPI		{ SCFG->DEVALTC &= ~SCFG_DEVALTC_SHD_SPI_Msk; }

#define ENABLE_FIU_PVT_CS		{ SCFG->DEVALT0 |= SCFG_DEVALT0_FIU_PVT_CS_SL_Msk; }
#define DISABLE_FIU_PVT_CS		{ SCFG->DEVALT0 &= ~SCFG_DEVALT0_FIU_PVT_CS_SL_Msk; }

#define ENABLE_FIU_BKP_CS		{ SCFG->DEVALT0 |= SCFG_DEVALT0_FIU_BAK_CS_SL_Msk; }
#define DISABLE_FIU_BKP_CS		{ SCFG->DEVALT0 &= ~SCFG_DEVALT0_FIU_BAK_CS_SL_Msk; }

// AMD eSPI
#define FIU_SWITCH_TO_SHARE		{ SCFG->DEV_CTL4 &= ~SCFG_DEV_CTL4_AMD_EN_Msk; }

/*--------------------------*/
/* BURST_CFG fields         */
/*--------------------------*/
#define FIU_BURST_CFG_R_BURST_Pos	(0)
#define FIU_BURST_CFG_R_BURST_Msk	(0x3 << FIU_BURST_CFG_R_BURST_Pos)

/*--------------------------*/
/* RESP_CFG fields          */
/*--------------------------*/
#define FIU_RESP_CFG_QUAD_EN_Pos	(3)
#define FIU_RESP_CFG_QUAD_EN_Msk	(0x1 << FIU_RESP_CFG_QUAD_EN_Pos)

/*--------------------------*/
/* SPI_FL_CFG fields        */
/*--------------------------*/
#define FIU_SPI_FL_CFG_RD_MODE_Pos	(6)
#define FIU_SPI_FL_CFG_RD_MODE_Msk	(0x3 << FIU_SPI_FL_CFG_RD_MODE_Pos)

/*--------------------------*/
/* UMA_CTS fields          */
/*--------------------------*/
#define FIU_UMA_CTS_EXEC_DONE_Pos	(7)
#define FIU_UMA_CTS_EXEC_DONE_Msk	(0x1 << FIU_UMA_CTS_EXEC_DONE_Pos)

#define FIU_UMA_CTS_DEV_NUM_Pos		(6)
#define FIU_UMA_CTS_DEV_NUM_Msk		(0x1 << FIU_UMA_CTS_DEV_NUM_Pos)

#define FIU_UMA_CTS_RD_WR_Pos		(5)
#define FIU_UMA_CTS_RD_WR_Msk		(0x1 << FIU_UMA_CTS_RD_WR_Pos)

#define FIU_UMA_CTS_C_SIZE_Pos		(4)
#define FIU_UMA_CTS_C_SIZE_Msk		(0x1 << FIU_UMA_CTS_C_SIZE_Pos)

#define FIU_UMA_CTS_A_SIZE_Pos		(3)
#define FIU_UMA_CTS_A_SIZE_Msk		(0x1 << FIU_UMA_CTS_A_SIZE_Pos)

/*--------------------------*/
/* UMA_ECTS fields          */
/*--------------------------*/
#define FIU_UMA_ECTS_UMA_ADDR_SIZE_Pos	(4)
#define FIU_UMA_ECTS_UMA_ADDR_SIZE_Msk	(0x7 << FIU_UMA_ECTS_UMA_ADDR_SIZE_Pos)

#define FIU_UMA_ECTS_DEV_NUM_BKP_Pos	(3)
#define FIU_UMA_ECTS_DEV_NUM_BKP_Msk	(0x1 << FIU_UMA_ECTS_DEV_NUM_BKP_Pos)

#define FIU_UMA_ECTS_SW_CS2_Pos		(2)
#define FIU_UMA_ECTS_SW_CS2_Msk		(0x1 << FIU_UMA_ECTS_SW_CS2_Pos)

#define FIU_UMA_ECTS_SW_CS1_Pos		(1)
#define FIU_UMA_ECTS_SW_CS1_Msk		(0x1 << FIU_UMA_ECTS_SW_CS1_Pos)

#define FIU_UMA_ECTS_SW_CS0_Pos		(0)
#define FIU_UMA_ECTS_SW_CS0_Msk		(0x1 << FIU_UMA_ECTS_SW_CS0_Pos)

/*--------------------------*/
/* FIU_EXT_CFG fields       */
/*--------------------------*/
#define FIU_EXT_CFG_SET_CMD_EN_Pos	(1)
#define FIU_EXT_CFG_SET_CMD_EN_Msk	(0x1 << FIU_EXT_CFG_SET_CMD_EN_Pos)

#define FIU_EXT_CFG_FOUR_BADDR_Pos	(0)
#define FIU_EXT_CFG_FOUR_BADDR_Msk	(0x1 << FIU_EXT_CFG_FOUR_BADDR_Pos)

/*--------------------------*/
/* SET_CMD_EN fields        */
/*--------------------------*/
#define FIU_SET_BKP_CMD_EN_Pos		(6)
#define FIU_SET_BKP_CMD_EN_Msk		(0x1 << FIU_SET_BKP_CMD_EN_Pos)

#define FIU_SET_SHD_CMD_EN_Pos		(5)
#define FIU_SET_SHD_CMD_EN_Msk		(0x1 << FIU_SET_SHD_CMD_EN_Pos)

#define FIU_SET_PVT_CMD_EN_Pos		(4)
#define FIU_SET_PVT_CMD_EN_Msk		(0x1 << FIU_SET_PVT_CMD_EN_Pos)

/*--------------------------*/
/* FIU_MSR_IE_CFG fields    */
/*--------------------------*/
#define FIU_MSR_IE_CGF_UMA_BLOCK_Pos	(3)
#define FIU_MSR_IE_CGF_UMA_BLOCK_Msk	(0x1 << FIU_MSR_IE_CGF_UMA_BLOCK_Pos)

/*--------------------------*/
/* DEVCNT fields            */
/*--------------------------*/
#define SCFG_DEVCNT_SHD_SPI_TRIS_Pos	(6)
#define SCFG_DEVCNT_SHD_SPI_TRIS_Msk	(0x1 << SCFG_DEVCNT_SHD_SPI_TRIS_Pos)

/*--------------------------*/
/* DEVALT0 fields           */
/*--------------------------*/
#define SCFG_DEVALT0_FIU_BAK_CS_SL_Pos	(2)
#define SCFG_DEVALT0_FIU_BAK_CS_SL_Msk	(0x1 << SCFG_DEVALT0_FIU_BAK_CS_SL_Pos)
#define SCFG_DEVALT0_FIU_PVT_CS_SL_Pos	(1)
#define SCFG_DEVALT0_FIU_PVT_CS_SL_Msk	(0x1 << SCFG_DEVALT0_FIU_PVT_CS_SL_Pos)

/*--------------------------*/
/* DEVALTC fields           */
/*--------------------------*/
#define SCFG_DEVALTC_SHD_SPI_Pos		(3)
#define SCFG_DEVALTC_SHD_SPI_Msk		(0x1 << SCFG_DEVALTC_SHD_SPI_Pos)
#define SCFG_DEVALTC_SHD_SPI_QUAD_Pos	(2)
#define SCFG_DEVALTC_SHD_SPI_QUAD_Msk	(0x1 << SCFG_DEVALTC_SHD_SPI_QUAD_Pos)

/*--------------------------*/
/* DEV_CTL4 fields          */
/*--------------------------*/
#define SCFG_DEV_CTL4_AMD_EN_Pos		(2)
#define SCFG_DEV_CTL4_AMD_EN_Msk		(0x1 << SCFG_DEV_CTL4_AMD_EN_Pos)

/*--------------------------*/
/* HFCBCD1 fields           */
/*--------------------------*/
#define HFCG_BCD1_FIUDIV_Pos			(0)
#define HFCG_BCD1_FIUDIV_Msk			(0x3 << HFCG_BCD1_FIUDIV_Pos)

/*--------------------------*/
/* EXT_DB_CFG fields        */
/*--------------------------*/
#define EXT_DB_CFG_EXT_DB_EN_Pos		(5)
#define EXT_DB_CFG_EXT_DB_EN_Msk		(0x1 << EXT_DB_CFG_EXT_DB_EN_Pos)

#define EXT_DB_CFG_D_SIZE_Pos			(0)
#define EXT_DB_CFG_D_SIZE_Msk			(0x1F << EXT_DB_CFG_D_SIZE_Pos)

/* Flash  ID */
#define FLASH_ID_WINBOND				0xEF
#define FLASH_ID_MXIC					0xC2
#define FLASH_ID_GIGADEV				0xC8
#define FLASH_ID_CYPRESS				0x01
#define FLASH_ID_MICRON					0x20
#define FLASH_ID_UNSUPP					0xFF

/* GDMA */
#define GDMA_BASE_ADDR					(0x40011000)
#define M32(addr) (*((volatile unsigned long *) (addr)))

#define GDMA_CTL0		M32(GDMA_BASE_ADDR + 0x00)
#define GDMA_SRCB0		M32(GDMA_BASE_ADDR + 0x04)
#define GDMA_DSTB0		M32(GDMA_BASE_ADDR + 0x08)
#define GDMA_TCNT0		M32(GDMA_BASE_ADDR + 0x0C)

#endif /* OPENOCD_LOADERS_FLASH_NCT_ESIO_NCT6694D_FLASH_H */

