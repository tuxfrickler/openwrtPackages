#define LINUX
#define MY_DEVICE_ID  "Vid_04d8&Pid_003c"
#define VENDOR_ID  0x04d8
#define PRODUCT_ID 0x003c

#define HID_CONFIGURATION 1
const static int ENDPOINT_INT_IN   = 0x81;
const static int ENDPOINT_INT_OUT  = 0x01;


const static int TIMEOUT           = 1000;  /* timeout in ms */

// HID Class-Specific Requests values. See section 7.2 of the HID specifications
#define HID_GET_REPORT                0x01
#define HID_GET_IDLE                  0x02
#define HID_GET_PROTOCOL              0x03
#define HID_SET_REPORT                0x09
#define HID_SET_IDLE                  0x0A
#define HID_SET_PROTOCOL              0x0B
#define HID_REPORT_TYPE_INPUT         0x01
#define HID_REPORT_TYPE_OUTPUT        0x02
#define HID_REPORT_TYPE_FEATURE       0x03



//*********************** BOOTLOADER COMMANDS ******************************
#define QUERY_DEVICE				0x02
#define UNLOCK_CONFIG				0x03
#define ERASE_DEVICE				0x04
#define PROGRAM_DEVICE				0x05
#define PROGRAM_COMPLETE			0x06
#define GET_DATA					0x07
#define RESET_DEVICE				0x08
#define GET_ENCRYPTED_FF			0xFF
//**************************************************************************

//*********************** QUERY RESULTS ************************************
#define QUERY_IDLE					0xFF
#define QUERY_RUNNING				0x00
#define QUERY_SUCCESS				0x01
#define QUERY_WRITE_FILE_FAILED		0x02
#define QUERY_READ_FILE_FAILED		0x03
#define QUERY_MALLOC_FAILED			0x04
//**************************************************************************

//*********************** PROGRAMMING RESULTS ******************************
#define PROGRAM_IDLE				0xFF
#define PROGRAM_RUNNING				0x00
#define PROGRAM_SUCCESS				0x01
#define PROGRAM_WRITE_FILE_FAILED	0x02
#define PROGRAM_READ_FILE_FAILED	0x03
#define PROGRAM_RUNNING_ERASE		0x05
#define PROGRAM_RUNNING_PROGRAM		0x06
//**************************************************************************

//*********************** ERASE RESULTS ************************************
#define ERASE_IDLE					0xFF
#define ERASE_RUNNING				0x00
#define ERASE_SUCCESS				0x01
#define ERASE_WRITE_FILE_FAILED		0x02
#define ERASE_READ_FILE_FAILED		0x03
#define ERASE_VERIFY_FAILURE		0x04
#define ERASE_POST_QUERY_FAILURE	0x05
#define ERASE_POST_QUERY_RUNNING	0x06
#define ERASE_POST_QUERY_SUCCESS	0x07
//**************************************************************************

//*********************** VERIFY RESULTS ***********************************
#define VERIFY_IDLE					0xFF
#define VERIFY_RUNNING				0x00
#define VERIFY_SUCCESS				0x01
#define VERIFY_WRITE_FILE_FAILED	0x02
#define VERIFY_READ_FILE_FAILED		0x03
#define VERIFY_MISMATCH_FAILURE		0x04
//**************************************************************************

//*********************** READ RESULTS *************************************
#define READ_IDLE					0xFF
#define READ_RUNNING				0x00
#define READ_SUCCESS				0x01
#define READ_READ_FILE_FAILED		0x02
#define READ_WRITE_FILE_FAILED		0x03
//**************************************************************************

//*********************** UNLOCK CONFIG RESULTS ****************************
#define UNLOCK_CONFIG_IDLE			0xFF
#define UNLOCK_CONFIG_RUNNING		0x00
#define UNLOCK_CONFIG_SUCCESS		0x01
#define UNLOCK_CONFIG_FAILURE		0x02
//**************************************************************************

//*********************** BOOTLOADER STATES ********************************
#define BOOTLOADER_IDLE				0xFF
#define BOOTLOADER_QUERY			0x00
#define BOOTLOADER_PROGRAM			0x01
#define BOOTLOADER_ERASE			0x02
#define BOOTLOADER_VERIFY			0x03
#define BOOTLOADER_READ				0x04
#define BOOTLOADER_UNLOCK_CONFIG	0x05
#define BOOTLOADER_RESET			0x06
//**************************************************************************

//*********************** RESET RESULTS ************************************
#define RESET_IDLE					0xFF
#define RESET_RUNNING				0x00
#define RESET_SUCCESS				0x01
#define RESET_WRITE_FILE_FAILED		0x02
//**************************************************************************

//*********************** MEMORY REGION TYPES ******************************
#define MEMORY_REGION_PROGRAM_MEM	0x01
#define MEMORY_REGION_EEDATA		0x02
#define MEMORY_REGION_CONFIG		0x03
#define MEMORY_REGION_END			0xFF
//**************************************************************************

//*********************** HEX FILE CONSTANTS *******************************
#define HEX_FILE_EXTENDED_LINEAR_ADDRESS 0x04
#define HEX_FILE_EOF 0x01
#define HEX_FILE_DATA 0x00

//This is the number of bytes per line of the 
#define HEX_FILE_BYTES_PER_LINE 16
//**************************************************************************

//*********************** Device Family Definitions ************************
#define DEVICE_FAMILY_PIC18		1
#define DEVICE_FAMILY_PIC24		2
#define DEVICE_FAMILY_PIC32		3
//**************************************************************************

#define ERROR_SUCCESS 1

#define PIC24_RESET_REMAP_OFFSET 0x1400
#define MAX_DATA_REGIONS 6

typedef unsigned int DWORD;
typedef unsigned short WORD;

#pragma pack(1)
typedef struct _MEMORY_REGION
{
	unsigned char Type;
	DWORD Address;
	DWORD Size;
}MEMORY_REGION;

typedef union _BOOTLOADER_COMMAND
{
	struct
	{
		unsigned char Command;
		unsigned char Pad[63];
	}EnterBootloader;
	struct
	{
		unsigned char Command;
		unsigned char Pad[63];
	}QueryDevice;
	
	struct
	{
		unsigned char Command;
		unsigned char BytesPerPacket;
		unsigned char DeviceFamily;
		MEMORY_REGION MemoryRegions[MAX_DATA_REGIONS];
		unsigned char Pad[8];
	}QueryResults;
	
	struct
	{
		unsigned char Command;
		unsigned char Setting;
		unsigned char Pad[62];
	}UnlockConfig;
	struct
	{
		unsigned char Command;
		unsigned char Pad[63];
	}EraseDevice;
	struct
	{
		unsigned char Command;
		DWORD Address;
		unsigned char BytesPerPacket;
		unsigned char Data[58];
	}ProgramDevice;
	struct
	{
		unsigned char Command;
		unsigned char Pad[63];
	}ProgramComplete;
	struct
	{
		unsigned char Command;
		DWORD Address;
		unsigned char BytesPerPacket;
		unsigned char Pad[58];
	}GetData;
	struct
	{
		unsigned char Command;
		DWORD Address;
		unsigned char BytesPerPacket;
		unsigned char Data[58];
	}GetDataResults;
	struct
	{
		unsigned char Command;
		unsigned char Pad[63];
	}ResetDevice;
	struct
	{
		unsigned char Command;
		unsigned char blockSize;
		unsigned char Data[63];
	}GetEncryptedFFResults;
	struct
	{
		unsigned char Data[64];
	}PacketData;
	unsigned char RawData[65];
} BOOTLOADER_COMMAND;

#pragma pack()

