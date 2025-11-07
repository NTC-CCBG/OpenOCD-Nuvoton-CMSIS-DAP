/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef OPENOCD_LOADERS_FLASH_NCT_ESIO_NCT6692D_FLASH_H
#define OPENOCD_LOADERS_FLASH_NCT_ESIO_NCT6692D_FLASH_H

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

#define RegSetBit(Reg, BitMsk)	{(Reg) |= (BitMsk);}					// Set register bit
#define RegClrBit(Reg, BitMsk)	{(Reg) &= ~(BitMsk);}					// Clear register bit
#define RegRepBit(Reg, BitMsk, BitMskVal) {(Reg) = ((Reg) & ~(BitMsk)) | (BitMskVal);}	// Replace register bit

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
	FLASH_CMD_4BYTE_DUAL_IO_READ		= 0xBC,
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
/* FLASH INTERFACE UNIT	(FIU)											*/
/*==============================================================================================================*/
/**
	@addtogroup FIU FLASH INTERFACE UNIT (FIU)
	Memory Mapped Structure for FIU Controller
@{ */

typedef struct {
	volatile uint8_t CFG;			/*!< [0x00]	FIU Configuration				*/
	volatile uint8_t BURST_CFG;		/*!< [0x01]	Burst Configuration				*/
	volatile uint8_t RESP_CFG;		/*!< [0x02]	FIU Response Configuration			*/
	volatile uint8_t Reserved1[17];
	volatile uint8_t SPI_FL_CFG;		/*!< [0x14]	SPI Flash Configuration				*/
	volatile uint8_t Reserved2[1];
	volatile uint8_t UMA_CODE;		/*!< [0x16]	UMA Code Byte					*/
	volatile uint8_t UMA_AB0;		/*!< [0x17]	UMA Address Byte 0				*/
	volatile uint8_t UMA_AB1;		/*!< [0x18]	UMA Address Byte 1				*/
	volatile uint8_t UMA_AB2;		/*!< [0x19]	UMA Address Byte 2				*/
	volatile uint8_t UMA_DB0;		/*!< [0x1A]	UMA Data Byte 0					*/
	volatile uint8_t UMA_DB1;		/*!< [0x1B]	UMA Data Byte 1					*/
	volatile uint8_t UMA_DB2;		/*!< [0x1C]	UMA Data Byte 2					*/
	volatile uint8_t UMA_DB3;		/*!< [0x1D]	UMA Data Byte 3					*/
	volatile uint8_t UMA_CTS;		/*!< [0x1E]	UMA Control and Status				*/
	volatile uint8_t UMA_ECTS;		/*!< [0x1F]	UMA Extended  Control and Status		*/
	volatile const uint32_t UMA_DB0_3;	/*!< [0x20]	UMA Data Bytes 0-3				*/
	volatile uint8_t Reserved3[2];
	volatile uint8_t CRCCON;		/*!< [0x26]	CRC Control Register				*/
	volatile uint8_t CRCENT;		/*!< [0x27]	CRC Entry Register				*/
	volatile uint32_t CRCRSLT;		/*!< [0x28]	CRC Initialization and Result Register		*/
	volatile uint8_t Reserved4[4];
	volatile uint8_t RD_CMD;		/*!< [0x30]	FIU Read Command				*/
	volatile uint8_t Reserved5[1];
	volatile uint8_t DMM_CYC;		/*!< [0x32]	FIU Dummy Cycles				*/
	volatile uint8_t EXT_CFG;		/*!< [0x33]	FIU Extended Configuration			*/
	volatile uint32_t UMA_AB0_3;		/*!< [0x34]	UMA Address Bytes 0-3				*/
	volatile uint8_t Reserved6[9];
	volatile uint8_t MI_CNT_THRSH;		/*!< [0x41]	Master Inactive Counter Threshold		*/
	volatile uint8_t MSR_STS;		/*!< [0x42]	FIU Master Status				*/
	volatile uint8_t MSR_IE_CFG;		/*!< [0x43]	FIU Master Interrupt Enable and Configuration	*/
} FIU_T;
#define FIU_BASE_ADDR(mdl)	(0x40020000 + ((mdl) * 0x1000L))	/*!< FLASH INTERFACE UNIT (FIU) */
#define FIU0			((FIU_T *)	FIU_BASE_ADDR(0))

// UMA
#define FIU_UMA_BLOCK(fiu)                  {RegSetBit((fiu)->MSR_IE_CFG, FIU_MSR_IE_CGF_UMA_BLOCK_Msk);}
#define FIU_UMA_UNBLOCK(fiu)                {RegClrBit((fiu)->MSR_IE_CFG, FIU_MSR_IE_CGF_UMA_BLOCK_Msk);}

/*--------------------------*/
/* UMA_CTS fields          */
/*--------------------------*/
#define FIU_UMA_CTS_EXEC_DONE_Pos		(7)
#define FIU_UMA_CTS_EXEC_DONE_Msk		(0x1 << FIU_UMA_CTS_EXEC_DONE_Pos)

#define FIU_UMA_CTS_DEV_NUM_Pos			(6)
#define FIU_UMA_CTS_DEV_NUM_Msk			(0x1 << FIU_UMA_CTS_DEV_NUM_Pos)

#define FIU_UMA_CTS_RD_WR_Pos                       (5)
#define FIU_UMA_CTS_RD_WR_Msk                       (0x1 << FIU_UMA_CTS_RD_WR_Pos)

#define FIU_UMA_CTS_C_SIZE_Pos                      (4)
#define FIU_UMA_CTS_C_SIZE_Msk                      (0x1 << FIU_UMA_CTS_C_SIZE_Pos)

#define FIU_UMA_CTS_A_SIZE_Pos                      (3)
#define FIU_UMA_CTS_A_SIZE_Msk                      (0x1 << FIU_UMA_CTS_A_SIZE_Pos)

/*--------------------------*/
/* UMA_ECTS fields          */
/*--------------------------*/
#define FIU_UMA_ECTS_UMA_ADDR_SIZE_Pos		(4)
#define FIU_UMA_ECTS_UMA_ADDR_SIZE_Msk		(0x7 << FIU_UMA_ECTS_UMA_ADDR_SIZE_Pos)

#define FIU_UMA_ECTS_SW_CS1_Pos			(1)
#define FIU_UMA_ECTS_SW_CS1_Msk			(0x1 << FIU_UMA_ECTS_SW_CS1_Pos)

#define FIU_UMA_ECTS_SW_CS0_Pos			(0)
#define FIU_UMA_ECTS_SW_CS0_Msk			(0x1 << FIU_UMA_ECTS_SW_CS0_Pos)

/*--------------------------*/
/* FIU_EXT_CFG fields       */
/*--------------------------*/
#define FIU_EXT_CFG_FL_SIZE_EX_Pos		(3)
#define FIU_EXT_CFG_FL_SIZE_EX_Msk		(0x3 << FIU_EXT_CFG_FL_SIZE_EX_Pos)

#define FIU_EXT_CFG_SET_DMM_EN_Pos		(2)
#define FIU_EXT_CFG_SET_DMM_EN_Msk		(0x1 << FIU_EXT_CFG_SET_DMM_EN_Pos)

#define FIU_EXT_CFG_SET_CMD_EN_Pos		(1)
#define FIU_EXT_CFG_SET_CMD_EN_Msk		(0x1 << FIU_EXT_CFG_SET_CMD_EN_Pos)

#define FIU_EXT_CFG_FOUR_BADDR_Pos		(0)
#define FIU_EXT_CFG_FOUR_BADDR_Msk		(0x1 << FIU_EXT_CFG_FOUR_BADDR_Pos)

/*--------------------------*/
/* FIU_MSR_IE_CFG fields    */
/*--------------------------*/
#define FIU_MSR_IE_CGF_UMA_BLOCK_Pos		(3)
#define FIU_MSR_IE_CGF_UMA_BLOCK_Msk		(0x1 << FIU_MSR_IE_CGF_UMA_BLOCK_Pos)

#define FIU_RD_MODE_FAST_RD_DUAL_IO(fiu)	{RegRepBit((fiu)->SPI_FL_CFG, (BIT7|BIT6), (BIT7|BIT6));}

/*==============================================================================================================*/
/* System Configuration Core Registers										*/
/*==============================================================================================================*/
/**
	@addtogroup SCFG SYSTEM CONFIGURATION CORE REGISTERS (SCFG)
	Memory Mapped Structure for SCFG Controller
@{ */

typedef struct {
	volatile uint8_t DEVCNT;		/*!< [0x00]	Device Control					*/
	volatile const uint8_t STRPST;		/*!< [0x01]	Straps Status					*/
	volatile uint8_t RSTCTL;		/*!< [0x02]	Reset Control and Status			*/
	volatile uint8_t DEV_CTL2;		/*!< [0x03]	Device Control 2				*/
	volatile uint8_t DEV_CTL3;		/*!< [0x04]	Device Control 3				*/
	volatile uint8_t RESERVED1[1];
	volatile uint8_t DEV_CTL4;		/*!< [0x06]	Device Control 4				*/
	volatile uint8_t RESERVED2[4];
	volatile uint8_t DEVALT10;		/*!< [0x0B]	Device Alternate Function 10			*/
	volatile uint8_t DEVALT11;		/*!< [0x0C]	Device Alternate Function 11			*/
	volatile uint8_t DEVALT12;		/*!< [0x0D]	Device Alternate Function 12			*/
	volatile uint8_t RESERVED3[2];
	volatile uint8_t DEVALT0;		/*!< [0x10]	Device Alternate Function 0			*/
	volatile uint8_t DEVALT1;		/*!< [0x11]	Device Alternate Function 1			*/
	volatile uint8_t DEVALT2;		/*!< [0x12]	Device Alternate Function 2			*/
	volatile uint8_t DEVALT3;		/*!< [0x13]	Device Alternate Function 3			*/
	volatile uint8_t DEVALT4;		/*!< [0x14]	Device Alternate Function 4			*/
	volatile uint8_t DEVALT5;		/*!< [0x15]	Device Alternate Function 5			*/
	volatile uint8_t DEVALT6;		/*!< [0x16]	Device Alternate Function 6			*/
	volatile uint8_t DEVALT7;		/*!< [0x17]	Device Alternate Function 7			*/
	volatile uint8_t DEVALT8;		/*!< [0x18]	Device Alternate Function 8			*/
	volatile uint8_t DEVALT9;		/*!< [0x19]	Device Alternate Function 9			*/
	volatile uint8_t DEVALTA;		/*!< [0x1A]	Device Alternate Function A			*/
	volatile uint8_t DEVALTB;		/*!< [0x1B]	Device Alternate Function B			*/
	volatile uint8_t DEVALTC;		/*!< [0x1C]	Device Alternate Function C			*/
	volatile uint8_t DEVALTD;		/*!< [0x1D]	Device Alternate Function D			*/
	volatile uint8_t DEVALTE;		/*!< [0x1E]	Device Alternate Function E			*/
	volatile uint8_t RESERVED6[3];
	volatile uint8_t DBGCTRL;		/*!< [0x22]	Device Alternate Function E			*/
	volatile uint8_t RESERVED7[5];
	volatile uint8_t DEVPU0;		/*!< [0x28]	Device Pull-Up Enable 0				*/
	volatile uint8_t DEVPU1;		/*!< [0x29]	Device Pull-Up Enable 1				*/
	volatile uint8_t RESERVED8[76];
	volatile uint8_t DBGFRZEN1;		/*!< [0x76]	Debug Freeze Enable 1				*/
	volatile uint8_t DBGFRZEN2;		/*!< [0x77]	Debug Freeze Enable 2				*/
	volatile uint8_t DBGFRZEN3;		/*!< [0x78]	Debug Freeze Enable 3				*/
	volatile uint8_t DBGFRZEN4;		/*!< [0x79]	Debug Freeze Enable 4				*/
} SCFG_T;
#define SCFG_BASE_ADDR		(0x400C3000)	/*!< SYSTEM CONFIGURATION */
#define SCFG			((SCFG_T *)	SCFG_BASE_ADDR)

#define M8(addr)		(*((volatile unsigned char  *) (addr)))
#define M16(addr)		(*((volatile unsigned short *) (addr)))
#define M32(addr)		(*((volatile unsigned long *) (addr)))

#define GDMA_BASE_ADDR		(0x40011000)	/*!< General DMA CONTROLLER */

#define GDMA_CTL0		M32(GDMA_BASE_ADDR + 0x00)
#define GDMA_SRCB0		M32(GDMA_BASE_ADDR + 0x04)
#define GDMA_DSTB0		M32(GDMA_BASE_ADDR + 0x08)
#define GDMA_TCNT0		M32(GDMA_BASE_ADDR + 0x0C)
#define GDMA_CSRC0		M32(GDMA_BASE_ADDR + 0x10)
#define GDMA_CDST0		M32(GDMA_BASE_ADDR + 0x14)
#define GDMA_CTCNT0		M32(GDMA_BASE_ADDR + 0x18)

#define GDMA_CTL1		M32(GDMA_BASE_ADDR + 0x20)
#define GDMA_SRCB1		M32(GDMA_BASE_ADDR + 0x24)
#define GDMA_DSTB1		M32(GDMA_BASE_ADDR + 0x28)
#define GDMA_TCNT1		M32(GDMA_BASE_ADDR + 0x2C)
#define GDMA_CSRC1		M32(GDMA_BASE_ADDR + 0x30)
#define GDMA_CDST1		M32(GDMA_BASE_ADDR + 0x34)
#define GDMA_CTCNT1		M32(GDMA_BASE_ADDR + 0x38)

#endif /* OPENOCD_LOADERS_FLASH_NCT_ESIO_NCT6692D_FLASH_H */

