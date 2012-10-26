#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "bootloader.h"
#include <time.h>
#include <ctype.h>

#define GNU_SOURCE

#define DEBUGGING

#if defined(DEBUGGING)
#define DEBUG_OUT(s) printf("%s\n", s);
#define DEBUG_PRINT_BUFFER(buffer,size) {std::cout << "Buffer" << std::endl << size << std::endl;}
#else
#define DEBUG_OUT(s)
#define DEBUG_PRINT_BUFFER(a,b)
#endif

#define DISABLE_PRINT()
#define ENABLE_PRINT()
#define PRINT_STATUS(s)

void printError(int);
bool TryToFindHIDDeviceFromVIDPID(void);
unsigned char* getMemoryRegion(unsigned char);
char* HexToString(unsigned long, unsigned char);
unsigned long StringToHex(char *);
bool Query();
void setMemoryRegion(unsigned char, unsigned char*);
bool program();
char* substring(const char*, size_t, size_t);
unsigned long hexToInt( const char *, unsigned long *);
char *_strndup(const char*, size_t);
int stringlen(const char*, size_t );

unsigned char bytesPerAddress;
unsigned char bytesPerPacket;
unsigned char memoryRegionsDetected;
unsigned char QueryThreadResults;

static MEMORY_REGION memoryRegions[MAX_DATA_REGIONS];

unsigned char *pData;
unsigned char *pData0;
unsigned char *pData1;
unsigned char *pData2;
unsigned char *pData3;
unsigned char *pData4;
unsigned char *pData5;

unsigned char encryptionBlockSize;
unsigned char encryptedFF[64];
bool  ckbox_ConfigWordProgramming_restore;

static struct libusb_device_handle *devh = NULL;
signed int r;

int stringlen(const char *s, size_t n)
{
  const char *end = memchr (s, '\0', n);
  return end ? (size_t) (end - s) : n;
}


char* _strndup (const char *s, size_t n)
{
  size_t len = stringlen(s, n);
  char *new = malloc (len + 1);

  if (new == NULL)
    return NULL;

  new[len] = '\0';
  return (char*) memcpy (new, s, len);
}

char* substring(const char* str, size_t begin, size_t len) 
{ 
  if (str == 0 || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin+len)) 
    return 0; 

  return _strndup(str + begin, len); 
} 

void delay_sec( int seconds ) {
    clock_t endwait;
    endwait = clock () + seconds * CLOCKS_PER_SEC;
    while (clock() < endwait) {}
}

unsigned long StringToHex(char* s)
{
    unsigned long returnAddress;
    unsigned long placeMultiplier;
    unsigned char i;
    char c;

    returnAddress = 0;
    placeMultiplier = 1;

    for (i=0;i<strlen(s);i++)
    {
        c = s[strlen(s)-1-i];
        if ((c >= 'A') && (c <= 'F'))
        {
            c = 10 + (c - 'A');
        }
        else if ((c >= 'a') && (c <= 'f'))
        {
            c = 10 + (c - 'a');
        }
        else
        {
            c = c - '0';
        }

        returnAddress += (c * placeMultiplier);
        placeMultiplier *= 16;
    }

    return returnAddress;
}

/* TODO <clear>
unsigned long hexToInt(const char* hexStr) {
    stringstream strm;
    strm << hex << hexStr;
    unsigned long value = 0;
    if (!(strm >> value)) DEBUG_OUT("error in conversion");
    return value;
}
*/

unsigned long hexToInt( const char *str, unsigned long *ival) {
	register unsigned long u;
	register const char *cp;

	cp = str;

	if (*cp == '\0')
	    return 0;

	u = 0;
	while (*cp != '\0') {
		if (!isxdigit((int)*cp))
		    return 0;
		if (u >= 0x10000000)
		    return 0;	/* overflow */
		u <<= 4;
		if (*cp <= '9')		/* very ascii dependent */
		    u += *cp++ - '0';
		else if (*cp >= 'a')
		    u += *cp++ - 'a' + 10;
		else
		    u += *cp++ - 'A' + 10;
	}
	*ival = u;
	return 1;
}

char* HexToString(unsigned long input,unsigned char bytes)
{
    char* returnString;
    char returnArray[9];

    unsigned char i;
    unsigned char c;

    for (i=0;i<9;i++)
    {
        returnArray[i]='0';
    }

    for (i=0;i<bytes*2;i++)
    {
        c = (unsigned char)(input & 0x0000000F);

        if (c <= 9)
        {
            returnArray[7-i]=c+'0';
        }
        else
        {
            returnArray[7-i]=c+'A'-10;
        }

        input >>= 4;
    }
    returnArray[9] = 0;
    returnString = returnArray;
    // TODO <clear> returnString = returnString.substr(8-(bytes*2),bytes*2);
    returnString = substring(returnString, 8-(bytes*2), bytes*2);
    return returnString;
}


void setMemoryRegion(unsigned char region, unsigned char* p)
{
    switch (region)
    {
    case 0:
        pData0 = p;
        break;
    case 1:
        pData1 = p;
        break;
    case 2:
        pData2 = p;
        break;
    case 3:
        pData3 = p;
        break;
    case 4:
        pData4 = p;
        break;
    case 5:
        pData5 = p;
        break;
    default:
        return;
    }
}

unsigned char* getMemoryRegion(unsigned char region) {
    switch (region)
    {
    case 0:
        return pData0;
    case 1:
        return pData1;
    case 2:
        return pData2;
    case 3:
        return pData3;
    case 4:
        return pData4;
    case 5:
        return pData5;
    default:
        return 0;
    }
}


void printError(signed int r) {
    switch (r)
    {
    case LIBUSB_ERROR_TIMEOUT:
        DEBUG_OUT("Operation timed out");
        break;
    case LIBUSB_ERROR_PIPE:
        DEBUG_OUT("Pipe error (control request was not supported by the device)");
        break;
    case LIBUSB_ERROR_NO_DEVICE:
        DEBUG_OUT("No such device (it may have been disconnected)");
        break;
    case LIBUSB_ERROR_NOT_FOUND:
        DEBUG_OUT("Entity not found");
        break;
    case LIBUSB_ERROR_BUSY:
        DEBUG_OUT("Resource busy");
        break;
    case LIBUSB_ERROR_OVERFLOW:
        DEBUG_OUT("Overflow");
        break;
    case LIBUSB_ERROR_IO:
        DEBUG_OUT("Input/output error");
        break;
    case LIBUSB_ERROR_INVALID_PARAM:
        DEBUG_OUT("Invalid parameter");
        break;
    case LIBUSB_ERROR_ACCESS:
        DEBUG_OUT("Access denied (insufficient permissions)");
        break;
    case LIBUSB_ERROR_INTERRUPTED:
        DEBUG_OUT("System call interrupted (perhaps due to signal)");
        break;
    case LIBUSB_ERROR_NO_MEM:
        DEBUG_OUT("Insufficient memory");
        break;
    case LIBUSB_ERROR_NOT_SUPPORTED:
        DEBUG_OUT("Operation not supported or unimplemented on this platform");
        break;

    default:
        DEBUG_OUT("Other USB error");
        char str[10];
        sprintf(str, "%d", r); 
        DEBUG_OUT(str);
        DEBUG_OUT("--");
    }

}


bool TryToFindHIDDeviceFromVIDPID(void) {

    if ((r = libusb_init(NULL))<0)
    {
        DEBUG_OUT("Failed to initialise libusb");
        return false;
    }

    if (!(devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID)))
    {
        DEBUG_OUT("Could not find/open bootloader");
        return false;
    }

#ifdef LINUX
    libusb_detach_kernel_driver(devh, 0);
#endif

    r = libusb_set_configuration(devh, HID_CONFIGURATION);
    if (r < 0) {
        DEBUG_OUT("libusb_set_configuration error:");
        printError(r);
        return false;
    }

    r = libusb_claim_interface(devh, 0);
    if (r < 0) {
        DEBUG_OUT("libusb_claim_interface error:");
        printError(r);
        return false;
    }
    return true;
}//end of TryToFindHIDDeviceFromVIDPID()


bool Query() {
    int BytesWritten = 0;
    DWORD ErrorStatus = ERROR_SUCCESS;
    int BytesReceived = 0;
    unsigned char buff_out[64];
    BOOTLOADER_COMMAND myResponse = {{0}};
    unsigned char i;

    for (int i=0;i<64;i++) {
        buff_out[i]=0;
        myResponse.RawData[i]=0;
    }

    buff_out[0]=QUERY_DEVICE;
    memoryRegionsDetected = 0;

    r=libusb_bulk_transfer(devh, ENDPOINT_INT_OUT, buff_out, 64, &BytesWritten, TIMEOUT);
    if ( r < 0) {
        printError(r);
        ErrorStatus = !ERROR_SUCCESS;
        return false;
    } else {
        ErrorStatus = ERROR_SUCCESS;
    }


    r=libusb_bulk_transfer(devh, ENDPOINT_INT_IN,  myResponse.RawData, 64, &BytesReceived, TIMEOUT);
    if (r < 0) {
        printError(r);
        ErrorStatus = !ERROR_SUCCESS;
        return false;
    } else {
        ErrorStatus = ERROR_SUCCESS;
    }
    
    printError(ErrorStatus);

    for (i=0;i<MAX_DATA_REGIONS;i++)
    {
        if (myResponse.QueryResults.MemoryRegions[i].Type == 0xFF) break;
        memoryRegions[i].Type = myResponse.QueryResults.MemoryRegions[i].Type;
        memoryRegions[i].Address = myResponse.QueryResults.MemoryRegions[i].Address;
        memoryRegions[i].Size = myResponse.QueryResults.MemoryRegions[i].Size;
        memoryRegionsDetected++;
    }


    switch (myResponse.QueryResults.DeviceFamily)
    {
    case DEVICE_FAMILY_PIC18:
        bytesPerAddress = 1;
        ckbox_ConfigWordProgramming_restore = true;
        DEBUG_OUT("Family: PIC18");
        break;
    case DEVICE_FAMILY_PIC24:
        bytesPerAddress = 2;
        ckbox_ConfigWordProgramming_restore = true;
        DEBUG_OUT("Family: PIC24");
        break;
    case DEVICE_FAMILY_PIC32:
        bytesPerAddress = 1;
        ckbox_ConfigWordProgramming_restore = false;
        DEBUG_OUT("Family: PIC32");
        break;
    default:
        break;
    }
    bytesPerPacket = myResponse.QueryResults.BytesPerPacket;

    return true;
}


bool program() {
    BOOTLOADER_COMMAND myCommand = {{0}};
    BOOTLOADER_COMMAND myResponse = {{0}};

    int BytesWritten = 0;
    DWORD ErrorStatus = ERROR_SUCCESS;
    int BytesReceived = 0;

    unsigned char* p;
    DWORD address;
    //unsigned long size;
    unsigned char i,currentByteInAddress,currentMemoryRegion;
    bool configsProgrammed,everythingElseProgrammed;
    bool skipBlock,blockSkipped;

    configsProgrammed = false;
    everythingElseProgrammed = false;


    myCommand.EraseDevice.Command = ERASE_DEVICE;

    r=libusb_bulk_transfer(devh, ENDPOINT_INT_OUT, myCommand.RawData, 64, &BytesWritten, TIMEOUT);
    if ( r < 0) {
        printError(r);
        ErrorStatus = !ERROR_SUCCESS;
        return false;
    } else {
        ErrorStatus = ERROR_SUCCESS;
        DEBUG_OUT("ERASED");
    }

    i=0;
    ErrorStatus = !ERROR_SUCCESS;
    while ((i<30) && (ErrorStatus != ERROR_SUCCESS)) {
        delay_sec(1);

        myCommand.QueryDevice.Command = QUERY_DEVICE;
        r=libusb_bulk_transfer(devh, ENDPOINT_INT_OUT, myCommand.RawData, 64, &BytesWritten, TIMEOUT);
        if ( r < 0) {
            ErrorStatus = !ERROR_SUCCESS;
        } else {
            ErrorStatus = ERROR_SUCCESS;
        }

        r=libusb_bulk_transfer(devh, ENDPOINT_INT_IN,  myResponse.RawData, 64, &BytesReceived, TIMEOUT);
        if (r < 0) {
            ErrorStatus = !ERROR_SUCCESS;
            return false;
        } else {
            ErrorStatus = ERROR_SUCCESS;
        }
        i++;
    }
    if (ErrorStatus != ERROR_SUCCESS) {
        DEBUG_OUT("Stuck after erase");
        return false;
    }

    configsProgrammed = false;

    while ((configsProgrammed == false) || (everythingElseProgrammed == false))
    {
        for (currentMemoryRegion=0;currentMemoryRegion<memoryRegionsDetected;currentMemoryRegion++)
        {
            //If we haven't programmed the configuration words then we want
            //  to do this first.  The problem is that if we have erased the
            //  configuration words and we receive a device reset before we
            //  reprogram the configuration words, then the device may not be
            //  capable of running on the USB any more.  To try to minimize the
            //  possibility of this occurrance, we first search all of the
            //  memory regions and look for any configuration regions and program
            //  these regions first.  This minimizes the time that the configuration
            //  words are left unprogrammed.

            //If the configuration words are not programmed yet
            if (configsProgrammed == false)
            {
                //If the current memory region is not a configuration section
                //  then continue to the top of the for loop and look at the
                //  next memory region.  We don't want to waste time yet looking
                //  at the other memory regions.  We will come back later for
                //  the other regions.
                if (memoryRegions[currentMemoryRegion].Type != MEMORY_REGION_CONFIG)
                {
                    continue;
                }
            }
            else
            {
                //If the configuration words are already programmed then if this
                //  region is a configuration region then we want to continue
                //  back to the top of the for loop and skip over this region.
                //  We don't want to program the configuration regions twice.
                if (memoryRegions[currentMemoryRegion].Type == MEMORY_REGION_CONFIG)
                {
                    continue;
                }
            }

            //Get the address, size, and data for the current memory region
            address = memoryRegions[currentMemoryRegion].Address;
            //TODO <clear> size = memoryRegions[currentMemoryRegion].Size;
            p = getMemoryRegion(currentMemoryRegion);

            //Mark that we intend to skip the first block unless we find a non-0xFF
            //  byte in the packet
            skipBlock = true;

            //Mark that we didn't skip the last block
            blockSkipped = false;

            //indicate that we are at the first byte of the current address
            currentByteInAddress = 1;

            //while the current address is less than the end address
            while (address < (memoryRegions[currentMemoryRegion].Address + memoryRegions[currentMemoryRegion].Size))
            {
                myCommand.ProgramDevice.Command = PROGRAM_DEVICE;
                myCommand.ProgramDevice.Address = address;

                //for as many bytes as we can fit in a packet
                for (i=0;i<bytesPerPacket;i++)
                {
                    unsigned char data;

                    //load up the byte from the allocated memory into the packet
                    data = *p++;
                    myCommand.ProgramDevice.Data[i+(sizeof(myCommand.ProgramDevice.Data)-bytesPerPacket)] = data;

#if !defined(ENCRYPTED_BOOTLOADER)
                    //if the byte wasn't 0xFF
                    if (data != 0xFF)
                    {
                        if (bytesPerAddress == 2)
                        {
                            if ((address%2)!=0)
                            {
                                if (currentByteInAddress == 2)
                                {
                                    //We can skip this block because we don't care about this byte
                                    //  it is byte 4 of a 3 word instruction on PIC24
                                    //myCommand.ProgramDevice.Data[i+(sizeof(myCommand.ProgramDevice.Data)-bytesPerPacket)] = 0;
                                }
                                else
                                {
                                    //Then we can't skip this block of data
                                    skipBlock = false;
                                }
                            }
                            else
                            {
                                //Then we can't skip this block of data
                                skipBlock = false;
                            }
                        }
                        else
                        {
                            //Then we can't skip this block of data
                            skipBlock = false;
                        }
                    }
#else
                    if (data != encryptedFF[i%encryptionBlockSize])
                    {
                        //Then we can't skip this block of data
                        skipBlock = false;
                    }
#endif

                    if (currentByteInAddress == bytesPerAddress)
                    {
                        //If we have written enough bytes per address to be
                        //  at the next address, then increment the address
                        //  variable and reset the count.
                        address++;
                        currentByteInAddress = 1;
                    }
                    else
                    {
                        //If we haven't written enough bytes to fill this
                        //  address then increment the number of bytes that
                        //  we have added for this address
                        currentByteInAddress++;
                    }

                    //If we have reached the end of the memory region, then we
                    //  need to pad the data at the end of the packet instead
                    //  of the front of the packet so we need to shift the data
                    //  to the back of the packet.
                    if (address >= (memoryRegions[currentMemoryRegion].Address + memoryRegions[currentMemoryRegion].Size))
                    {
                        unsigned char n;

                        i++;

                        //for each byte of the packet
                        for (n=0;n<sizeof(myCommand.ProgramDevice.Data);n++)
                        {
                            if (n<i)
                            {
                                //move it from where it is to the the back of the packet thus
                                //  shifting all of the data down
                                myCommand.ProgramDevice.Data[sizeof(myCommand.ProgramDevice.Data)-n-1] = myCommand.ProgramDevice.Data[i+(sizeof(myCommand.ProgramDevice.Data)-bytesPerPacket)-n-1];
                            }
                            else
                            {
                                //set the remaining data values to 0
                                myCommand.ProgramDevice.Data[sizeof(myCommand.ProgramDevice.Data)-n-1] = 0;

                            }
                        }

                        //If this was the last address then break out of the for loop
                        //  that is writing bytes to the packet
                        break;
                    }
                }

                //The number of bytes programmed is still contained in the last loop
                //  index, i.  Copy that number into the packet that is going to the device
                myCommand.ProgramDevice.BytesPerPacket = i;

                //If the block was all 0xFF then we can just skip actually programming
                //  this device.  Otherwise enter the programming sequence
                if (skipBlock == false)
                {
                    //If we skipped one block before this block then we may need
                    //  to send a proramming complete command to the device before
                    //  sending the data for this command.
                    if (blockSkipped == true)
                    {
                        BOOTLOADER_COMMAND cmdProgrammingComplete = {{0}};

                        //Send the programming complete command
                        cmdProgrammingComplete.ProgramComplete.Command = PROGRAM_COMPLETE;
                        r=libusb_bulk_transfer(devh, ENDPOINT_INT_OUT, cmdProgrammingComplete.RawData, 64, &BytesWritten, TIMEOUT);
                        if ( r < 0) {
                            ErrorStatus = !ERROR_SUCCESS;
                            DEBUG_OUT("programming failed");
                            return false;
                        } else {
                            ErrorStatus = ERROR_SUCCESS;
                        }


                        //since we have now indicated that the programming is complete
                        //  then we now mark that we haven't skipped any blocks
                        blockSkipped = false;
                    }

                    //Send the program command to the device
                    r=libusb_bulk_transfer(devh, ENDPOINT_INT_OUT, myCommand.RawData, 64, &BytesWritten, TIMEOUT);
                    if ( r < 0) {
                        ErrorStatus = !ERROR_SUCCESS;
                        DEBUG_OUT("programming failed");
                        return false;
                    } else {
                        ErrorStatus = ERROR_SUCCESS;
                    }


                    //initially mark that we are skipping the block.  We will
                    //  set this back to false on the first byte we find that is
                    //  not 0xFF.
                    skipBlock = true;
                }
                else
                {
                    //If we are skipping this block then mark that we have skipped
                    //  a block and initially mark that we will be skipping the
                    //  next block.  We will set skipBlock to false if we find
                    //  a byte that is non-0xFF in the next packet
                    blockSkipped = true;
                    skipBlock = true;
                }
            } //while

            //Now that we are done with all of the addresses in this memory region,
            //  before we move on we need to send a programming complete command to
            //  the device.
            myCommand.ProgramComplete.Command = PROGRAM_COMPLETE;
            r=libusb_bulk_transfer(devh, ENDPOINT_INT_OUT, myCommand.RawData, 64, &BytesWritten, TIMEOUT);
            if ( r < 0) {
                ErrorStatus = !ERROR_SUCCESS;
                DEBUG_OUT("programming failed");
                return false;
            } else {
                ErrorStatus = ERROR_SUCCESS;
            }
        }//for each memory region


        if (configsProgrammed == false)
        {
            //If the configuration bits haven't been programmed yet then the first
            //  pass through the for loop that just completed will have programmed
            //  just the configuration bits so mark them as complete.
            configsProgrammed = true;
        }
        else
        {
            //If the configuration bits were already programmed then this loop must
            //  have programmed all of the other memory regions.  Mark everything
            //  else as being complete.
            everythingElseProgrammed = true;
        }
    }//while

    myCommand.ResetDevice.Command = RESET_DEVICE;
    
    r=libusb_bulk_transfer(devh, ENDPOINT_INT_OUT, myCommand.RawData, 64, &BytesWritten, TIMEOUT);
            if ( r < 0) {
                //ErrorStatus = !ERROR_SUCCESS;
                DEBUG_OUT("programming failed");
                return false;
            } else {
                DEBUG_OUT("device reset ok");
                //ErrorStatus = ERROR_SUCCESS;
      }

    DEBUG_OUT("PROGRAMMED");
    return true;
}



int main(int argc, char **argv) {
    FILE *f;
    char line[4096];
    char *p;
    int i,j;
    unsigned long recordLength, addressField, recordType, checksum;
    unsigned checksumCalculated;
    unsigned long extendedAddress;

    char* str;
    char* dataPayload;

    if (argc < 2) {
        DEBUG_OUT("No filename specified");
        return -2;
    }
    
    memoryRegionsDetected=0;
    bytesPerAddress =0;
    //memoryRegions = (MEMORY_REGION*) malloc(MAX_DATA_REGIONS * sizeof(MEMORY_REGION));  // MEMORY_REGION[MAX_DATA_REGIONS];
    //TODO memoryRegions = new MEMORY_REGION[MAX_DATA_REGIONS];

    for (i=0;i<MAX_DATA_REGIONS;i++)
    {
        setMemoryRegion(i,0);
    }

    if (TryToFindHIDDeviceFromVIDPID()) {
        DEBUG_OUT("Device found");
    } else {
        return -1;
    }
    
    if (!strncmp(argv[1], "--reset", sizeof("--reset") -1)){
      DEBUG_OUT("reset");
      
      BOOTLOADER_COMMAND myCommand = {{0}};
      int BytesWritten = 0;
      
      myCommand.ResetDevice.Command = RESET_DEVICE;
    
      int res = libusb_bulk_transfer(devh, ENDPOINT_INT_OUT, myCommand.RawData, 64, &BytesWritten, TIMEOUT);
            if ( res < 0) {
                //ErrorStatus = !ERROR_SUCCESS;
                DEBUG_OUT("programming failed");
                return false;
            } else {
                DEBUG_OUT("device reset ok");
                //ErrorStatus = ERROR_SUCCESS;
      }
      exit(1);
      
    }
    

    if (!Query()) {
        return -1;
    }

    for (i=0;i<memoryRegionsDetected;i++)
    {
        unsigned long size;

        size = (memoryRegions[i].Size + 1) * bytesPerAddress;
        pData = (unsigned char*) malloc(size);
        setMemoryRegion(i,pData);

        //If the malloc failed
        if (pData == 0)
        {
            DEBUG_OUT("Malloc failed");
            return -13;
        }
    }

    if ((f = fopen(argv[1], "r"))  == NULL) {
        DEBUG_OUT("cannot open file");
        return -3;
    }

    while (NULL != fgets(line, 4096, f)) {
        p = strchr(line, ' ');
        if (p != NULL) p[0]=0;
        p = strchr(line, '\n');
        if (p != NULL) p[0]=0;
        p = strchr(line, '\r');
        if (p != NULL) p[0]=0;
        if (strlen(line) == 0) continue;
        if (line[0] != ':') {
            DEBUG_OUT("no leading : ?!");
            continue;
        }

        p=line+1;
        str = p;
        //TODO <clear> recordLength = hexToInt(str.substr(0,2));
        hexToInt(substring(str, 0, 2), &recordLength);
        //TODO <clear> addressField = hexToInt(str.substr(2,4));
        hexToInt(substring(str, 2, 4), &addressField);
        //TODO <clear> recordType   = hexToInt(str.substr(6,2));
        hexToInt(substring(str, 6, 2), &recordType);
        //TODO <clear> dataPayload  = str.substr(8,recordLength*2);
        dataPayload = substring(str, 8, recordLength * 2);
        //TODO <clear> checksum     = hexToInt(str.substr((recordLength*2)+8,2));
        hexToInt(substring(str, (recordLength*2) + 8, 2), &checksum);
        checksumCalculated = 0;
        for (j=0;j<(recordLength+4);j++) {
            unsigned long tmp = 0;
            //TODO <clear>
            //checksumCalculated += hexToInt(str.substr(j*2,2));
            hexToInt(substring(str, j*2, 2), &tmp);
            checksumCalculated += tmp;
        }
        checksumCalculated = (~checksumCalculated) + 1;
        if ((checksumCalculated & 0x000000FF) != checksum) {
            DEBUG_OUT("ERROR: Checksum error");
            fclose(f);
            return -99;
        }

        switch (recordType) {
        case HEX_FILE_EXTENDED_LINEAR_ADDRESS:
            //if this record is an extended address record then
            //  save off the extended address value so we can later
            //  add it to each of the address fields that we read
            hexToInt(dataPayload, &extendedAddress);
            break;
        case HEX_FILE_EOF:
            // TODO <clear> hexFileEOF = true;
            break;
        case HEX_FILE_DATA:
        {
            unsigned long totalAddress;

            //The total address is the extended address plus the current address field.
            totalAddress = (extendedAddress << 16) + addressField;

            //for each of the valid memory regions we got from the query
            //  command
            for (i=0;i<memoryRegionsDetected;i++) {
                pData = getMemoryRegion(i);
                //If the total address read from the hex file falls within
                //  the valid memory range found in the query results
                if ((totalAddress >= (memoryRegions[i].Address * bytesPerAddress)) && (totalAddress < ((memoryRegions[i].Address + memoryRegions[i].Size) * bytesPerAddress))) {
                    for (j=0;j<(recordLength);j++) {
                        unsigned long data;
                        unsigned char *p;
                        unsigned char *limit;

                        //Record the data from the hex file into the memory allocated
                        //  for that specific memory region.
                        p = (unsigned char*)((totalAddress-(memoryRegions[i].Address * bytesPerAddress)) + j);
                        //TODO <clear> data = hexToInt(dataPayload.substr(j*2,2));
                         hexToInt(substring(dataPayload, j*2, 2), &data);
                        
                        p = (unsigned char*)(pData + (totalAddress-(memoryRegions[i].Address * bytesPerAddress)) + j);
                        limit = (unsigned char*)(pData + ((memoryRegions[i].Size + 1)*bytesPerAddress));
                        if (p>=limit) {
                            break;
                        }

                        *p = (unsigned char)(data);
                    }
                    break;
                }
            }
            break;
        }
        default:
            break;
        }
    }
    
    if (!program()) {
        DEBUG_OUT("FAILED");
        return -10;
    }
    
    return 0;
}
