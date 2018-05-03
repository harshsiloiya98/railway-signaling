/* 
 * Copyright (C) 2012-2014 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define _SVID_SOURCE
#define __STDC_WANT_LIB_EXT2__ 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <makestuff.h>
#include <libfpgalink.h>
#include <libbuffer.h>
#include <liberror.h>
#include <libdump.h>
#include <argtable2.h>
#include <readline/readline.h>
#include <readline/history.h>
#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#define _POSIX_C_SOURCE 1
/*
   GLOBAL VARIABLES-------------------------------------------
   */
int xc=-1,yc=-1;//global coordinates
int inp_from_board,channel;
int to_send[8];
int h2f_enc[32];
int current_to_send;
int change=0;
int ack1[4]={153,153,153,153};///////////////////value of ack1. convert each of 8 bits to decimal
int ack2[4]={170,170,170,170};////////////////value of ack2."""""
int in_bin[8];
void to_bin(int x)
{
    //printf("Asked to convert to binary : %d \n",x);
    for(int i=7;i>=0;--i)
    {
        in_bin[i]=x%2;
        x/=2;
    }
    /*
    for(int i=0;i<8;++i)
    {
        printf("in_bin[%d] = %d\n",i,in_bin[i]);
    }
    */
}
int to_decimal(int *bin)
{
    int x=0;
    int po2=1;
    for(int i=7;i>=0;--i)
    {
        x+=bin[i]*po2;
        po2=po2<<1;
    }
    return x;
}
bool check(int *rec_ack);
void enc(int *pl,int *res);
void dec(int* pl,int *res);
void search_coord(int *message);
bool sigIsRaised(void);
void sigRegisterHandler(void);
void modify_file(int *message_from_fpga);
static const char *ptr;
static bool enableBenchmarking = false;

static bool isHexDigit(char ch) {
	return
		(ch >= '0' && ch <= '9') ||
		(ch >= 'a' && ch <= 'f') ||
		(ch >= 'A' && ch <= 'F');
}

static uint16 calcChecksum(const uint8 *data, size_t length) {
	uint16 cksum = 0x0000;
	while ( length-- ) {
		cksum = (uint16)(cksum + *data++);
	}
	return cksum;
}

static bool getHexNibble(char hexDigit, uint8 *nibble) {
	if ( hexDigit >= '0' && hexDigit <= '9' ) {
		*nibble = (uint8)(hexDigit - '0');
		return false;
	} else if ( hexDigit >= 'a' && hexDigit <= 'f' ) {
		*nibble = (uint8)(hexDigit - 'a' + 10);
		return false;
	} else if ( hexDigit >= 'A' && hexDigit <= 'F' ) {
		*nibble = (uint8)(hexDigit - 'A' + 10);
		return false;
	} else {
		return true;
	}
}

static int getHexByte(uint8 *byte) {
	uint8 upperNibble;
	uint8 lowerNibble;
	if ( !getHexNibble(ptr[0], &upperNibble) && !getHexNibble(ptr[1], &lowerNibble) ) {
		*byte = (uint8)((upperNibble << 4) | lowerNibble);
		byte += 2;
		return 0;
	} else {
		return 1;
	}
}

static const char *const errMessages[] = {
	NULL,
	NULL,
	"Unparseable hex number",
	"Channel out of range",
	"Conduit out of range",
	"Illegal character",
	"Unterminated string",
	"No memory",
	"Empty string",
	"Odd number of digits",
	"Cannot load file",
	"Cannot save file",
	"Bad arguments"
};

typedef enum {
	FLP_SUCCESS,
	FLP_LIBERR,
	FLP_BAD_HEX,
	FLP_CHAN_RANGE,
	FLP_CONDUIT_RANGE,
	FLP_ILL_CHAR,
	FLP_UNTERM_STRING,
	FLP_NO_MEMORY,
	FLP_EMPTY_STRING,
	FLP_ODD_DIGITS,
	FLP_CANNOT_LOAD,
	FLP_CANNOT_SAVE,
	FLP_ARGS
} ReturnCode;

static ReturnCode doRead(
	struct FLContext *handle, uint8 chan, uint32 length, FILE *destFile, uint16 *checksum,
	const char **error)
{
	ReturnCode retVal = FLP_SUCCESS;
	uint32 bytesWritten;
	FLStatus fStatus;
	uint32 chunkSize;
	const uint8 *recvData;
	uint32 actualLength;
	const uint8 *ptr;
	uint16 csVal = 0x0000;
	#define READ_MAX 65536

	// Read first chunk
	chunkSize = length >= READ_MAX ? READ_MAX : length;
	fStatus = flReadChannelAsyncSubmit(handle, chan, chunkSize, NULL, error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");
	length = length - chunkSize;

	while ( length ) {
		// Read chunk N
		chunkSize = length >= READ_MAX ? READ_MAX : length;
		fStatus = flReadChannelAsyncSubmit(handle, chan, chunkSize, NULL, error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");
		length = length - chunkSize;
		
		// Await chunk N-1
		fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");

		// Write chunk N-1 to file
		bytesWritten = (uint32)fwrite(recvData, 1, actualLength, destFile);
		CHECK_STATUS(bytesWritten != actualLength, FLP_CANNOT_SAVE, cleanup, "doRead()");

		// Checksum chunk N-1
		chunkSize = actualLength;
		ptr = recvData;
		while ( chunkSize-- ) {
			csVal = (uint16)(csVal + *ptr++);
		}
	}

	// Await last chunk
	fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");
	
	// Write last chunk to file
	bytesWritten = (uint32)fwrite(recvData, 1, actualLength, destFile);
	CHECK_STATUS(bytesWritten != actualLength, FLP_CANNOT_SAVE, cleanup, "doRead()");

	// Checksum last chunk
	chunkSize = actualLength;
	ptr = recvData;
	while ( chunkSize-- ) {
		csVal = (uint16)(csVal + *ptr++);
	}
	
	// Return checksum to caller
	*checksum = csVal;
cleanup:
	return retVal;
}

static ReturnCode doWrite(
	struct FLContext *handle, uint8 chan, FILE *srcFile, size_t *length, uint16 *checksum,
	const char **error)
{
	ReturnCode retVal = FLP_SUCCESS;
	size_t bytesRead, i;
	FLStatus fStatus;
	const uint8 *ptr;
	uint16 csVal = 0x0000;
	size_t lenVal = 0;
	#define WRITE_MAX (65536 - 5)
	uint8 buffer[WRITE_MAX];

	do {
		// Read Nth chunk
		bytesRead = fread(buffer, 1, WRITE_MAX, srcFile);
		if ( bytesRead ) {
			// Update running total
			lenVal = lenVal + bytesRead;

			// Submit Nth chunk
			fStatus = flWriteChannelAsync(handle, chan, bytesRead, buffer, error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doWrite()");

			// Checksum Nth chunk
			i = bytesRead;
			ptr = buffer;
			while ( i-- ) {
				csVal = (uint16)(csVal + *ptr++);
			}
		}
	} while ( bytesRead == WRITE_MAX );

	// Wait for writes to be received. This is optional, but it's only fair if we're benchmarking to
	// actually wait for the work to be completed.
	fStatus = flAwaitAsyncWrites(handle, error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doWrite()");

	// Return checksum & length to caller
	*checksum = csVal;
	*length = lenVal;
cleanup:
	return retVal;
}

static int parseLine(struct FLContext *handle, const char *line, const char **error) {
	ReturnCode retVal = FLP_SUCCESS, status;
	FLStatus fStatus;
	struct Buffer dataFromFPGA = {0,};
	BufferStatus bStatus;
	uint8 *data = NULL;
	char *fileName = NULL;
	FILE *file = NULL;
	double totalTime, speed;
	#ifdef WIN32
		LARGE_INTEGER tvStart, tvEnd, freq;
		DWORD_PTR mask = 1;
		SetThreadAffinityMask(GetCurrentThread(), mask);
		QueryPerformanceFrequency(&freq);
	#else
		struct timeval tvStart, tvEnd;
		long long startTime, endTime;
	#endif
	bStatus = bufInitialise(&dataFromFPGA, 1024, 0x00, error);
	CHECK_STATUS(bStatus, FLP_LIBERR, cleanup);
	ptr = line;
	do {
		while ( *ptr == ';' ) {
			ptr++;
		}
		switch ( *ptr ) {
		case 'r':{
			uint32 chan;
			uint32 length = 1;
			char *end;
			ptr++;
			
			// Get the channel to be read:
			errno = 0;
			chan = (uint32)strtoul(ptr, &end, 16);
			CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
			CHECK_STATUS(chan > 127, FLP_CHAN_RANGE, cleanup);
			ptr = end;
            //--------------------------------change channel manually
            chan=(uint32)(channel*2);
			// Only three valid chars at this point:
			CHECK_STATUS(*ptr != '\0' && *ptr != ';' && *ptr != ' ', FLP_ILL_CHAR, cleanup);

			if ( *ptr == ' ' ) {
				ptr++;

				// Get the read count:
				errno = 0;
				length = (uint32)strtoul(ptr, &end, 16);
				CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);
				ptr = end;
				
				// Only three valid chars at this point:
				CHECK_STATUS(*ptr != '\0' && *ptr != ';' && *ptr != ' ', FLP_ILL_CHAR, cleanup);
				if ( *ptr == ' ' ) {
					const char *p;
					const char quoteChar = *++ptr;
					CHECK_STATUS(
						(quoteChar != '"' && quoteChar != '\''),
						FLP_ILL_CHAR, cleanup);
					
					// Get the file to write bytes to:
					ptr++;
					p = ptr;
					while ( *p != quoteChar && *p != '\0' ) {
						p++;
					}
					CHECK_STATUS(*p == '\0', FLP_UNTERM_STRING, cleanup);
					fileName = malloc((size_t)(p - ptr + 1));
					CHECK_STATUS(!fileName, FLP_NO_MEMORY, cleanup);
					CHECK_STATUS(p - ptr == 0, FLP_EMPTY_STRING, cleanup);
					strncpy(fileName, ptr, (size_t)(p - ptr));
					fileName[p - ptr] = '\0';
					ptr = p + 1;
				}
			}
			if ( fileName ) {
				uint16 checksum = 0x0000;

				// Open file for writing
				file = fopen(fileName, "wb");
				CHECK_STATUS(!file, FLP_CANNOT_SAVE, cleanup);
				free(fileName);
				fileName = NULL;

				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					status = doRead(handle, (uint8)chan, length, file, &checksum, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					status = doRead(handle, (uint8)chan, length, file, &checksum, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Read %d bytes (checksum 0x%04X) from channel %d at %f MiB/s\n",
						length, checksum, chan, speed);
				}
				CHECK_STATUS(status, status, cleanup);

				// Close the file
				fclose(file);
				file = NULL;
			} else {
				size_t oldLength = dataFromFPGA.length;
				bStatus = bufAppendConst(&dataFromFPGA, 0x00, length, error);
				CHECK_STATUS(bStatus, FLP_LIBERR, cleanup);
				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					fStatus = flReadChannel(handle, (uint8)chan, length, dataFromFPGA.data + oldLength, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					fStatus = flReadChannel(handle, (uint8)chan, length, dataFromFPGA.data + oldLength, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Read %d bytes (checksum 0x%04X) from channel %d at %f MiB/s\n",
						length, calcChecksum(dataFromFPGA.data + oldLength, length), chan, speed);
				}
				CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			}
			break;
		}
		case 'w':{
			unsigned long int chan;
			size_t length = 1, i;
			char *end, ch;
			const char *p;
			ptr++;
			
			// Get the channel to be written:
			errno = 0;
			chan = strtoul(ptr, &end, 16);
			CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
			CHECK_STATUS(chan > 127, FLP_CHAN_RANGE, cleanup);
			ptr = end;
            //------------------------------ change channel manually
            chan=(uint32)(2*channel+1);

			// There must be a space now:
			CHECK_STATUS(*ptr != ' ', FLP_ILL_CHAR, cleanup);

			// Now either a quote or a hex digit
		   ch = *++ptr;
			if ( ch == '"' || ch == '\'' ) {
				uint16 checksum = 0x0000;

				// Get the file to read bytes from:
				ptr++;
				p = ptr;
				while ( *p != ch && *p != '\0' ) {
					p++;
				}
				CHECK_STATUS(*p == '\0', FLP_UNTERM_STRING, cleanup);
				fileName = malloc((size_t)(p - ptr + 1));
				CHECK_STATUS(!fileName, FLP_NO_MEMORY, cleanup);
				CHECK_STATUS(p - ptr == 0, FLP_EMPTY_STRING, cleanup);
				strncpy(fileName, ptr, (size_t)(p - ptr));
				fileName[p - ptr] = '\0';
				ptr = p + 1;  // skip over closing quote

				// Open file for reading
				file = fopen(fileName, "rb");
				CHECK_STATUS(!file, FLP_CANNOT_LOAD, cleanup);
				free(fileName);
				fileName = NULL;
				
				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					status = doWrite(handle, (uint8)chan, file, &length, &checksum, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					status = doWrite(handle, (uint8)chan, file, &length, &checksum, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Wrote "PFSZD" bytes (checksum 0x%04X) to channel %lu at %f MiB/s\n",
						length, checksum, chan, speed);
				}
				CHECK_STATUS(status, status, cleanup);

				// Close the file
				fclose(file);
				file = NULL;
			} else if ( isHexDigit(ch) ) {
				// Read a sequence of hex bytes to write
				uint8 *dataPtr;
				p = ptr + 1;
				while ( isHexDigit(*p) ) {
					p++;
				}
				CHECK_STATUS((p - ptr) & 1, FLP_ODD_DIGITS, cleanup);
				length = (size_t)(p - ptr) / 2;
				data = malloc(length);
				dataPtr = data;
				for ( i = 0; i < length; i++ ) {
					getHexByte(dataPtr++);
					ptr += 2;
				}
				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					fStatus = flWriteChannel(handle, (uint8)chan, length, data, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
                    if(change)
                    {
                        *(data) = current_to_send;
                    }
					fStatus = flWriteChannel(handle, (uint8)chan, length, data, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Wrote "PFSZD" bytes (checksum 0x%04X) to channel %lu at %f MiB/s\n",
						length, calcChecksum(data, length), chan, speed);
				}
				CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
				free(data);
				data = NULL;
			} else {
				FAIL(FLP_ILL_CHAR, cleanup);
			}
			break;
		}
		case '+':{
			uint32 conduit;
			char *end;
			ptr++;

			// Get the conduit
			errno = 0;
			conduit = (uint32)strtoul(ptr, &end, 16);
			CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
			CHECK_STATUS(conduit > 255, FLP_CONDUIT_RANGE, cleanup);
			ptr = end;

			// Only two valid chars at this point:
			CHECK_STATUS(*ptr != '\0' && *ptr != ';', FLP_ILL_CHAR, cleanup);

			fStatus = flSelectConduit(handle, (uint8)conduit, error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			break;
		}
		default:
			FAIL(FLP_ILL_CHAR, cleanup);
		}
	} while ( *ptr == ';' );
	CHECK_STATUS(*ptr != '\0', FLP_ILL_CHAR, cleanup);


    inp_from_board = *(dataFromFPGA.data);
	dump(0x00000000, dataFromFPGA.data, dataFromFPGA.length);

cleanup:
	bufDestroy(&dataFromFPGA);
	if ( file ) {
		fclose(file);
	}
	free(fileName);
	free(data);
	if ( retVal > FLP_LIBERR ) {
		const int column = (int)(ptr - line);
		int i;
		fprintf(stderr, "%s at column %d\n  %s\n  ", errMessages[retVal], column, line);
		for ( i = 0; i < column; i++ ) {
			fprintf(stderr, " ");
		}
		fprintf(stderr, "^\n");
	}
	return retVal;
}

static const char *nibbles[] = {
	"0000",  // '0'
	"0001",  // '1'
	"0010",  // '2'
	"0011",  // '3'
	"0100",  // '4'
	"0101",  // '5'
	"0110",  // '6'
	"0111",  // '7'
	"1000",  // '8'
	"1001",  // '9'

	"XXXX",  // ':'
	"XXXX",  // ';'
	"XXXX",  // '<'
	"XXXX",  // '='
	"XXXX",  // '>'
	"XXXX",  // '?'
	"XXXX",  // '@'

	"1010",  // 'A'
	"1011",  // 'B'
	"1100",  // 'C'
	"1101",  // 'D'
	"1110",  // 'E'
	"1111"   // 'F'
};

int main(int argc, char *argv[]) {
	ReturnCode retVal = FLP_SUCCESS, pStatus;
	struct arg_str *ivpOpt = arg_str0("i", "ivp", "<VID:PID>", "            vendor ID and product ID (e.g 04B4:8613)");
	struct arg_str *vpOpt = arg_str1("v", "vp", "<VID:PID[:DID]>", "       VID, PID and opt. dev ID (e.g 1D50:602B:0001)");
	struct arg_str *fwOpt = arg_str0("f", "fw", "<firmware.hex>", "        firmware to RAM-load (or use std fw)");
	struct arg_str *portOpt = arg_str0("d", "ports", "<bitCfg[,bitCfg]*>", " read/write digital ports (e.g B13+,C1-,B2?)");
	struct arg_str *queryOpt = arg_str0("q", "query", "<jtagBits>", "         query the JTAG chain");
	struct arg_str *progOpt = arg_str0("p", "program", "<config>", "         program a device");
	struct arg_uint *conOpt = arg_uint0("c", "conduit", "<conduit>", "        which comm conduit to choose (default 0x01)");
	struct arg_str *actOpt = arg_str0("a", "action", "<actionString>", "    a series of CommFPGA actions");
	struct arg_lit *shellOpt  = arg_lit0("s", "shell", "                    start up an interactive CommFPGA session");
	struct arg_lit *benOpt  = arg_lit0("b", "benchmark", "                enable benchmarking & checksumming");
	struct arg_lit *rstOpt  = arg_lit0("r", "reset", "                    reset the bulk endpoints");
	struct arg_str *dumpOpt = arg_str0("l", "dumploop", "<ch:file.bin>", "   write data from channel ch to file");
	struct arg_lit *helpOpt  = arg_lit0("h", "help", "                     print this help and exit");
	struct arg_str *eepromOpt  = arg_str0(NULL, "eeprom", "<std|fw.hex|fw.iic>", "   write firmware to FX2's EEPROM (!!)");
	struct arg_str *backupOpt  = arg_str0(NULL, "backup", "<kbitSize:fw.iic>", "     backup FX2's EEPROM (e.g 128:fw.iic)\n");
	struct arg_end *endOpt   = arg_end(20);
	void *argTable[] = {
		ivpOpt, vpOpt, fwOpt, portOpt, queryOpt, progOpt, conOpt, actOpt,
		shellOpt, benOpt, rstOpt, dumpOpt, helpOpt, eepromOpt, backupOpt, endOpt
	};
	const char *progName = "flcli";
	int numErrors;
	struct FLContext *handle = NULL;
	FLStatus fStatus;
	const char *error = NULL;
	const char *ivp = NULL;
	const char *vp = NULL;
	bool isNeroCapable, isCommCapable;
	uint32 numDevices, scanChain[16], i;
	const char *line = NULL;
	uint8 conduit = 0x01;

	if ( arg_nullcheck(argTable) != 0 ) {
		fprintf(stderr, "%s: insufficient memory\n", progName);
		FAIL(1, cleanup);
	}

	numErrors = arg_parse(argc, argv, argTable);

	if ( helpOpt->count > 0 ) {
		printf("FPGALink Command-Line Interface Copyright (C) 2012-2014 Chris McClelland\n\nUsage: %s", progName);
		arg_print_syntax(stdout, argTable, "\n");
		printf("\nInteract with an FPGALink device.\n\n");
		arg_print_glossary(stdout, argTable,"  %-10s %s\n");
		FAIL(FLP_SUCCESS, cleanup);
	}

	if ( numErrors > 0 ) {
		arg_print_errors(stdout, endOpt, progName);
		fprintf(stderr, "Try '%s --help' for more information.\n", progName);
		FAIL(FLP_ARGS, cleanup);
	}

	fStatus = flInitialise(0, &error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);

	vp = vpOpt->sval[0];

	printf("Attempting to open connection to FPGALink device %s...\n", vp);
	fStatus = flOpen(vp, &handle, NULL);
	if ( fStatus ) {
		if ( ivpOpt->count ) {
			int count = 60;
			uint8 flag;
			ivp = ivpOpt->sval[0];
			printf("Loading firmware into %s...\n", ivp);
			if ( fwOpt->count ) {
				fStatus = flLoadCustomFirmware(ivp, fwOpt->sval[0], &error);
			} else {
				fStatus = flLoadStandardFirmware(ivp, vp, &error);
			}
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			
			printf("Awaiting renumeration");
			flSleep(1000);
			do {
				printf(".");
				fflush(stdout);
				fStatus = flIsDeviceAvailable(vp, &flag, &error);
				CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
				flSleep(250);
				count--;
			} while ( !flag && count );
			printf("\n");
			if ( !flag ) {
				fprintf(stderr, "FPGALink device did not renumerate properly as %s\n", vp);
				FAIL(FLP_LIBERR, cleanup);
			}

			printf("Attempting to open connection to FPGLink device %s again...\n", vp);
			fStatus = flOpen(vp, &handle, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		} else {
			fprintf(stderr, "Could not open FPGALink device at %s and no initial VID:PID was supplied\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	printf(
		"Connected to FPGALink device %s (firmwareID: 0x%04X, firmwareVersion: 0x%08X)\n",
		vp, flGetFirmwareID(handle), flGetFirmwareVersion(handle)
	);

	if ( eepromOpt->count ) {
		if ( !strcmp("std", eepromOpt->sval[0]) ) {
			printf("Writing the standard FPGALink firmware to the FX2's EEPROM...\n");
			fStatus = flFlashStandardFirmware(handle, vp, &error);
		} else {
			printf("Writing custom FPGALink firmware from %s to the FX2's EEPROM...\n", eepromOpt->sval[0]);
			fStatus = flFlashCustomFirmware(handle, eepromOpt->sval[0], &error);
		}
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
	}

	if ( backupOpt->count ) {
		const char *fileName;
		const uint32 kbitSize = strtoul(backupOpt->sval[0], (char**)&fileName, 0);
		if ( *fileName != ':' ) {
			fprintf(stderr, "%s: invalid argument to option --backup=<kbitSize:fw.iic>\n", progName);
			FAIL(FLP_ARGS, cleanup);
		}
		fileName++;
		printf("Saving a backup of %d kbit from the FX2's EEPROM to %s...\n", kbitSize, fileName);
		fStatus = flSaveFirmware(handle, kbitSize, fileName, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
	}

	if ( rstOpt->count ) {
		// Reset the bulk endpoints (only needed in some virtualised environments)
		fStatus = flResetToggle(handle, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
	}

	if ( conOpt->count ) {
		conduit = (uint8)conOpt->ival[0];
	}

	isNeroCapable = flIsNeroCapable(handle);
	isCommCapable = flIsCommCapable(handle, conduit);

	if ( portOpt->count ) {
		uint32 readState;
		char hex[9];
		const uint8 *p = (const uint8 *)hex;
		printf("Configuring ports...\n");
		fStatus = flMultiBitPortAccess(handle, portOpt->sval[0], &readState, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		sprintf(hex, "%08X", readState);
		printf("Readback:   28   24   20   16    12    8    4    0\n          %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf("  %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s\n", nibbles[*p++ - '0']);
		flSleep(100);
	}

	if ( queryOpt->count ) {
		if ( isNeroCapable ) {
			fStatus = flSelectConduit(handle, 0x00, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = jtagScanChain(handle, queryOpt->sval[0], &numDevices, scanChain, 16, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( numDevices ) {
				printf("The FPGALink device at %s scanned its JTAG chain, yielding:\n", vp);
				for ( i = 0; i < numDevices; i++ ) {
					printf("  0x%08X\n", scanChain[i]);
				}
			} else {
				printf("The FPGALink device at %s scanned its JTAG chain but did not find any attached devices\n", vp);
			}
		} else {
			fprintf(stderr, "JTAG chain scan requested but FPGALink device at %s does not support NeroProg\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( progOpt->count ) {
		printf("Programming device...\n");
		if ( isNeroCapable ) {
			fStatus = flSelectConduit(handle, 0x00, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flProgram(handle, progOpt->sval[0], NULL, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		} else {
			fprintf(stderr, "Program operation requested but device at %s does not support NeroProg\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( benOpt->count ) {
		enableBenchmarking = true;
	}
	
	if ( actOpt->count ) {
		printf("Executing CommFPGA actions on FPGALink device %s...\n", vp);
		if ( isCommCapable ) {
			uint8 isRunning;
			fStatus = flSelectConduit(handle, conduit, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flIsFPGARunning(handle, &isRunning, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( isRunning ) {
				pStatus = parseLine(handle, actOpt->sval[0], &error);
				CHECK_STATUS(pStatus, pStatus, cleanup);
			} else {
				fprintf(stderr, "The FPGALink device at %s is not ready to talk - did you forget --program?\n", vp);
				FAIL(FLP_ARGS, cleanup);
			}
		} else {
			fprintf(stderr, "Action requested but device at %s does not support CommFPGA\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( dumpOpt->count ) {
		const char *fileName;
		unsigned long chan = strtoul(dumpOpt->sval[0], (char**)&fileName, 10);
		FILE *file = NULL;
		const uint8 *recvData;
		uint32 actualLength;
		if ( *fileName != ':' ) {
			fprintf(stderr, "%s: invalid argument to option -l|--dumploop=<ch:file.bin>\n", progName);
			FAIL(FLP_ARGS, cleanup);
		}
		fileName++;
		printf("Copying from channel %lu to %s", chan, fileName);
		file = fopen(fileName, "wb");
		CHECK_STATUS(!file, FLP_CANNOT_SAVE, cleanup);
		sigRegisterHandler();
		fStatus = flSelectConduit(handle, conduit, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		fStatus = flReadChannelAsyncSubmit(handle, (uint8)chan, 22528, NULL, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		do {
			fStatus = flReadChannelAsyncSubmit(handle, (uint8)chan, 22528, NULL, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fwrite(recvData, 1, actualLength, file);
			printf(".");
		} while ( !sigIsRaised() );
		printf("\nCaught SIGINT, quitting...\n");
		fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		fwrite(recvData, 1, actualLength, file);
		fclose(file);
	}

	if ( shellOpt->count ) {
		printf("\nEntering CommFPGA command-line mode:\n");
		if ( isCommCapable ) {
		   uint8 isRunning;
			fStatus = flSelectConduit(handle, conduit, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flIsFPGARunning(handle, &isRunning, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( isRunning ) {
				do {
					do {
						line = readline("> ");
					} while ( line && !line[0] );
					if ( line && line[0] && line[0] != 'q' ) {
						add_history(line);
                        while(1)
                        {
                            
                                int rec_ack1[32];
                                int message[32];
H2:
							printf("In Reset State\n");
                            channel=0;
                            while(1)
                            {
                                /*
                                if(channel>1)
                                {
                                    printf("MANUAL BREAKING. REMOVE BREAK STATEMENT AT START OF WHILE LOOP\n");
                                    break;
                                }
                                */
                                sprintf(line,"r0 1");
                                int coord[4];
                                int cipher[32];
                                for(int i=0;i<4;++i)
                                {
                                    printf("Reading on channel %d",channel);
                                    fflush(stdout);
                                    pStatus = parseLine(handle, line, &error);
                                    coord[i]=inp_from_board;
                                    printf("inp_from_board : %d",inp_from_board);
                                    printf("\n");
                                    to_bin(coord[i]);
                                    for(int j=0;j<8;++j)
                                    {
                                        cipher[8*i+j]=in_bin[j];
                                        //printf("i j 8*i+j cipher[8*i+j] in_bin[j] : %d %d %d %d %d\n",i,j,8*i+j,cipher[8*i+j],in_bin[j]);
                                    }
                                }

                                printf("Receieved cipher : ");
                                for(int i=0;i<32;++i)
                                {
                                    printf("%d",cipher[i]);
                                }
                                printf("\n");
                                printf("Now Starting Decryption\n");
                                dec(cipher,message);

                                printf("Receieved message : ");
                                for(int i=0;i<32;++i)
                                {
                                    printf("%d",message[i]);
                                }
                                printf("\n");

                                int re_enc[32];//stores re_encrpyted message
                                printf("Starting Re-encryption\n");
                                enc(message,re_enc);
                                for(int i=0;i<4;++i)
                                {
                                    printf("Now sending Re-encrypted message\n");
                                    int val=0;
                                    val=to_decimal(re_enc+8*i);
                                    current_to_send=val;
                                    sprintf(line,"w1 99");
                                    change=1;
                                    pStatus = parseLine(handle, line, &error);//////sending re-encrypted message
                                    change=0;
        
                                }
                                //sleep(0.2);
                                //--------waiting for ack1
                                sprintf(line,"r0 1");
                                for(int i=0;i<4;++i)
                                {
                                    printf("Now reading ack1\n");
                                    pStatus = parseLine(handle, line, &error);
                                    to_bin(inp_from_board);
                                    for(int j=0;j<8;++j)
                                    {
                                        rec_ack1[8*i+j]=in_bin[j];
                                    }
                                }

                                printf("Received ack1 : ");
                                for(int i=0;i<32;++i)
                                {
                                    printf("%d ",rec_ack1[i]);
                                }
                                printf("\n");

                                if(check(rec_ack1))
                                {
                                    printf("Ack1 received successfully.  i=%d\n",channel);
                                    break;
                                }
                                else
                                {
                                    printf("Errorr in receiving Ack1. Trying again after 5 seconds    i=%d\n",channel) ;
                                    sleep(5);
                                    sprintf(line,"r0 1");
                                    for(int i=0;i<4;++i)
                                    {
                                        printf("Now reading ack1\n");
                                        pStatus = parseLine(handle, line, &error);
                                        to_bin(inp_from_board);
                                        for(int j=0;j<8;++j)
                                        {
                                            rec_ack1[8*i+j]=in_bin[j];
                                        }
                                    }

                                    printf("Received ack1 : ");
                                    for(int i=0;i<32;++i)
                                    {
                                        printf("%d ",rec_ack1[i]);
                                    }
                                    printf("\n");

                                    if(check(rec_ack1))
                                    {
                                        printf("Ack1 received successfully.  i=%d\n",channel);
                                        break;
                                    }
                                    else
                                    {
                                        printf("Errorr in receiving Ack1. Will try next channel.  i=%d\n",channel) ;
                                    }
                                }
                                channel=(channel+1)%64;
                            }
                            int bin_ack2[32],enc_ack2[32];
                            for(int i=0;i<4;++i)
                            {
                                int num=ack2[i];
                                to_bin(num);
                                for(int j=0;j<8;++j)
                                {
                                    bin_ack2[8*i+j]=in_bin[j];
                                }
                            }
                            enc(bin_ack2,enc_ack2);//encrypting ack2
                            //-----sending ack2
                            for(int i=0;i<4;++i)
                            {
                                printf("Now sending ack2 :");
                                //current_to_send=to_decimal( ack2+i*8);
                                current_to_send=to_decimal(enc_ack2+8*i);
                                printf("%d\n",current_to_send);
                                sprintf(line,"w1 99"); 
                                change=1;
                                pStatus = parseLine(handle, line, &error);
                                change=0;
                            }




                            printf("Now looking up coordinates in networks.txt\n");
                            //printf("SKIPPING search_coord. REMOVE IF CONDITION\n");
                            if(1)
                            {
                                search_coord(message);// lookup networks.txt for the coordinates
                            }

                            int h2f[32];
                            for(int i=0;i<4;++i)
                            {
                                printf("Tabulating data into 4 bytes\n");
                                int num=to_send[i];
                                to_bin(num);
                                for(int j=7;j>=0;--j)
                                {
                                    h2f[8*i+j]=in_bin[j];
                                }
                            }

                            printf("Unencrypted 4 bytes of data : ");
                            for(int i=0;i<32;++i)
                            {
                                printf("%d",h2f[i]);
                            }
                            printf("\n");

                            printf("Encrypting data\n");
                            enc(h2f,h2f_enc);
 
                            printf("Encrypted 4 bytes of data : ");
                            for(int i=0;i<32;++i)
                            {
                                printf("%d",h2f_enc[i]);
                            }
                            printf("\n");

                            for(int i=0;i<4;++i)
                            {
                                printf("Sending 4 bytes\n");
                                int val=0;
                                val=to_decimal(h2f_enc+8*i);
                                current_to_send=val;
                                sprintf(line,"w1 99");
                                change=1;
                                pStatus = parseLine(handle, line, &error);
                                change=0;
                            }
    
                            sleep(0.1);
                            int time1=0;
                            while(1)
                            {
                                //--------waiting for ack1
                                sprintf(line,"r0 1");
                                for(int i=0;i<4;++i)
                                {
                                    printf("Reading ack1\n");
                                    pStatus = parseLine(handle, line, &error);
                                    to_bin(inp_from_board);
                                    for(int j=0;j<8;++j)
                                    {
                                        rec_ack1[8*i+j]=in_bin[j];
                                    }
                                }

                                printf("Received ack1 : ");
                                for(int i=0;i<32;++i)
                                {
                                    printf("%d ",rec_ack1[i]);
                                }
                                printf("\n");

                                if(check(rec_ack1))
                                {
                                    printf("Ack1 received Successfully\n");
                                    break;
                                }
                                else printf("Ack1 not received Successfully\n");
                                time1+=4;
                                if(time1>256)
                                {
                                    printf("Ack1 not received in 256 seconds. Timing out\n");
                                    goto H2;
                                }
                                sleep(4);
                            }

                            for(int i=0;i<4;++i)
                            {
                                printf("Tabulating next data into 4 bytes\n");
                                int num=to_send[i+4];
                                to_bin(num);
                                for(int j=7;j>=0;--j)
                                {
                                    h2f[8*i+j]=in_bin[j];
                                }
                            }

                            printf("Unencrypted next 4 bytes of data : ");
                            for(int i=0;i<32;++i)
                            {
                                printf("%d",h2f[i]);
                            }
                            printf("\n");

                            printf("Encrypting data\n");
                            enc(h2f,h2f_enc);
 
                            printf("Encrypted next 4 bytes of data : ");
                            for(int i=0;i<32;++i)
                            {
                                printf("%d",h2f_enc[i]);
                            }
                            printf("\n");   

                            for(int i=0;i<4;++i)
                            {
                                int val=0;
                                val=to_decimal(h2f_enc+8*i);
                                current_to_send=val;
                                printf("sending next 4 bytes\n");
                                sprintf(line,"w1 99");
                                change=1;
                                pStatus = parseLine(handle, line, &error);
                                change=0;
                            }

                            sleep(0.1);
                            time1=0;
                            while(1)
                            {
                                //--------waiting for ack1
                                sprintf(line,"r0 1");
                                for(int i=0;i<4;++i)
                                {
                                    printf("Reading ack1\n");
                                    pStatus = parseLine(handle, line, &error);
                                    to_bin(inp_from_board);
                                    for(int j=0;j<8;++j)
                                    {
                                        rec_ack1[8*i+j]=in_bin[j];
                                    }
                                }

                                printf("Received ack1 : ");
                                for(int i=0;i<32;++i)
                                {
                                    printf("%d ",rec_ack1[i]);
                                }
                                printf("\n");

                                if(check(rec_ack1))
                                {
                                    printf("Ack1 received successfully\n");
                                    break;
                                }
                                else printf("Ack1 not received successfully\n");
                                time1+=4;
                                if(time1>256)
                                {
                                    printf("Ack1 not received in 256 seconds. Timing out.\n");
                                    goto H2;
                                }
                                sleep(4);
                            } 
                            //-----sending ack2
                            for(int i=0;i<4;++i)
                            {
                                printf("Now sending ack2 :");
                                //current_to_send=to_decimal( ack2+i*8);
                                current_to_send=to_decimal(enc_ack2+8*i);
                                printf("%d\n",current_to_send);
                                sprintf(line,"w1 99"); 
                                change=1;
                                pStatus = parseLine(handle, line, &error);
                                change=0;
                            }  
                            /*
                            printf("Process Over. Restarting after 32 seconds\n");
                            sleep(32);
                            //commented for project
                            */
                            int message_from_fpga[32];
                            sleep(0.5);
                            time1=0;
                            while(1)
                            {
                                if(time1>20)
                                {
                                    printf("Ack1 not found. Timing out. Resetting after 5 seconds\n");
                                    sleep(5);
                                    goto H2;
                                }
                                //--------waiting for ack1
                                sprintf(line,"r0 1");
                                for(int i=0;i<4;++i)
                                {
                                    printf("Reading ack1\n");
                                    pStatus = parseLine(handle, line, &error);
                                    to_bin(inp_from_board);
                                    for(int j=0;j<8;++j)
                                    {
                                        rec_ack1[8*i+j]=in_bin[j];
                                    }
                                }

                                printf("Received ack1 : ");
                                for(int i=0;i<32;++i)
                                {
                                    printf("%d ",rec_ack1[i]);
                                }
                                printf("\n");

                                if(check(rec_ack1))
                                {
                                    printf("Ack1 received successfully\n");
                                    sprintf(line,"r0 1");
                                    printf("hello\n");
                                    for(int i=0;i<4;++i)
                                    {
                                        printf("Reading 4 encrypted bytes\n");
                                        pStatus = parseLine(handle, line, &error);
                                        to_bin(inp_from_board);
                                        for(int j=0;j<8;++j)
                                        {
                                            message_from_fpga[8*i+j]=in_bin[j];
                                        }
                                    }
                                    printf("Now modifying backend data\n");
                                    sleep(2);
                                    modify_file(message_from_fpga);
                                    break;
                                }
                                else
				{
					 printf("Ack1 not received successfully. Timing out\n");
					 sleep(15);
					goto H2;
				}
                                printf("Trying again after 2 seconds\n");
                                sleep(2);
                                time1+=2;
                            }
                                printf("Data received from FPGA and backend modified. Resetting after 20 seconds\n");
                                sleep(20);
                        	}
    
    						CHECK_STATUS(pStatus, pStatus, cleanup);
    						free((void*)line);
    					}
                } while ( line && line[0] != 'q' );
			} else {
				fprintf(stderr, "The FPGALink device at %s is not ready to talk - did you forget --xsvf?\n", vp);
				FAIL(FLP_ARGS, cleanup);
			}
		} else {
			fprintf(stderr, "Shell requested but device at %s does not support CommFPGA\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

cleanup:
	free((void*)line);
	flClose(handle);
	if ( error ) {
		fprintf(stderr, "%s\n", error);
		flFreeError(error);
	}
	return retVal;
}






bool check(int *rec_ack)
{
    int ret=1;
    for(int i=0;i<4;++i)
    {
        if(to_decimal(rec_ack+8*i)!=ack1[i])
        {
            ret=0;
        }
    }
    return ret;
}









char key[32]="11001100110011001100110011000001";
int k[32],p[32],t[32],fif[32];
void enc(int *pl,int *res)
{
    int n1=0;
    for(int i=0;i<32;++i)
    {
        k[31-i]=key[i]-'0';
        p[31-i]=pl[i];
        if(key[i]=='1')
        {
            n1++;
        }
    }
    t[0]=k[28]^k[24]^k[20]^k[16]^k[12]^k[8]^k[4]^k[0]; 
    t[1]=k[29]^k[25]^k[21]^k[17]^k[13]^k[9]^k[5]^k[1]; 
    t[2]=k[30]^k[26]^k[22]^k[18]^k[14]^k[10]^k[6]^k[2];    
    t[3]=k[31]^k[27]^k[23]^k[19]^k[15]^k[11]^k[7]^k[3];
    int t1=t[0]+2*t[1]+4*t[2]+8*t[3];
    for(int i=4;i<32;++i)
    {
        t[i]=t[i-4];
    }
    for(int i=0;i<n1;++i)
    {
        for(int i=0;i<32;++i)
        {
            p[i]=p[i]^t[i];
        }
        t1++;
        for(int i=0;i<4;++i)
        {
            t[i]  =    (t1& (1<<i) )>0;
        }
        for(int i=4;i<32;++i)
        {
            t[i]=t[i-4];
        }
    }
    for(int i=31;i>=0;--i)
    {
        res[31-i]=0;
        if(p[i]==1)
        {
            res[31-i]=1;
        }
    }
}



void dec(int* pl,int *res)
{
    fif[0]=1;fif[1]=1;fif[2]=1;fif[3]=1;
    int n0=0;
    for(int i=0;i<32;++i)
    {   
        k[31-i]=key[i]-'0';
        p[31-i]=pl[i];
        if(key[i]=='0')
        {
            n0++;
        }
    }

    t[0]=k[28]^k[24]^k[20]^k[16]^k[12]^k[8]^k[4]^k[0];
    t[1]=k[29]^k[25]^k[21]^k[17]^k[13]^k[9]^k[5]^k[1];
    t[2]=k[30]^k[26]^k[22]^k[18]^k[14]^k[10]^k[6]^k[2];
    t[3]=k[31]^k[27]^k[23]^k[19]^k[15]^k[11]^k[7]^k[3];
    int t1=t[0]+2*t[1]+4*t[2]+8*t[3];
    t1+=15;
    for(int i=0;i<4;++i)
    {
        t[i] =   (t1&(1<<i) ) >0;
    }
    for(int i=4;i<32;++i)
    {
        t[i]=t[i-4];
    }
    //ui t1=0;
    for(int i=0;i<32;++i)
    {
        //t1+=t[i]*(ui)(1<<i);
    }
    //ui two31= (  (1<<30)-1) * 2  +1 ;
    //ui two32= two31*2 +1;
    for(int i=0;i<n0;++i)                                                                                                                                                               
    {
        for(int i=0;i<32;++i)
        {
            p[i]=p[i]^t[i];
        }
        t1+=15;
        for(int i=0;i<4;++i)
        {
            t[i] =   (t1&(1<<i) ) >0;
        }
        for(int i=4;i<32;++i)
        {
            t[i]=t[i-4];
        }
    }
    for(int i=31;i>=0;--i)
    {
        res[31-i]=0;
        if(p[i]==1)
        {
            res[31-i]=1;
        }
    }
}

// -----------taken from https://stackoverflow.com/questions/12911299/read-csv-file-in-c
const char* getfield(char* line, int num)
{
    const char* tok;
    for (tok = strtok(line, ",");
            tok && *tok;
            tok = strtok(NULL, ",\n"))
    {
        if (!--num)
            return tok;
    }
    return NULL;
}

void search_coord(int *message)
{
    int val=0;
    int po2=1;
    printf("Message : ");
    for(int i=0;i<32;++i)
    {
        printf("%d",message[i]);
    }
    printf("\n");
    for(int i=31;i>=24;--i)
    {
        val+=po2*message[i];
        po2*=2;
    }
    printf("val=%d ",val);
    int y=val%16;
    val/=16;
    int x=val%16;
    xc=x;yc=y;
    printf("Coordinates : x = %d, y = %d\n",x,y);
    FILE* stream = fopen("networks.txt", "r");
    char line[100];
    for(int i=0;i<8;++i)to_send[i]=0;
    while (fgets(line,100,stream)!=NULL)
    {
        char* tmp = strdup(line);
        if(atoi(getfield(tmp,1))==x)
        {
            tmp=strdup(line);
            if(atoi(getfield(tmp,2))==y)
            {
                tmp=strdup(line);
                int direction=atoi(getfield(tmp,3));
                tmp=strdup(line);
                int track_ok=atoi(getfield(tmp,4));
                tmp=strdup(line);
                int next_signal=atoi(getfield(tmp,5));
                int data=(1<<7)+(1<<6)*track_ok+(1<<3)*direction+next_signal;
                //int data=(1<<5)*direction+(1<<4) +(1<<3)*track_ok+next_signal;
                printf("x=%d y=%d direction=%d track_ok=%d next_signal=%d\n",x,y,direction,track_ok,next_signal);
                to_send[direction]=data;
            }
        }
        free(tmp);
    }
    fclose(stream);
    printf("File opened\n");
    for(int i=0;i<8;++i)
    {
        if(to_send[i]==0)
        {
            //to_send[i]=(1<<3)*i;//track doesn't exist
            //to_send[i]=(1<<7)+(1<<6)+(1<<3)*i+3;
            to_send[i]=(1<<3)*i;
        }
    }
}
//exists ok direction next
void modify_file(int *message_from_fpga)
{
    int message[32];
    dec(message_from_fpga,message);
    printf("Encrypted data from FPGA:\n");
    for(int i=0;i<32;++i)
    {
        printf("%d",message_from_fpga[i]);
    }
    printf("\nDecrypted Data from FPGA:\n");
    for(int i=0;i<32;++i)
    {
        printf("%d",message[i]);
    }
    printf("\n");
    int val=to_decimal(message+24);
    to_bin(val);
    //01234567
    //ex ok dir next
    int dire1=in_bin[4]+2*in_bin[3]+4*in_bin[2];
    int t_exists=in_bin[0];
    int t_ok=in_bin[1];
    int n_signal=in_bin[7]+2*in_bin[6]+4*in_bin[5];
    printf("Modified entries with coordinates (%d,%d)\n",xc,yc);
    FILE* stream = fopen("networks.txt", "r+");
    printf("Received from FPGA: direction=%d,next signal=%d,track_ok=%d,track_exists=%d\n",dire1,n_signal,t_ok,t_exists);
    FILE* temp = fopen("temp.txt", "w+");
    char line[100];
    for(int i=0;i<8;++i)to_send[i]=0;
    int changed=0;
    while (fgets(line,100,stream)!=NULL)
    {
        char* tmp = strdup(line);
        int x=atoi(getfield(tmp,1));
        tmp=strdup(line);
        int y=atoi(getfield(tmp,2));
        if(1)
        {
            if(1)
            {
                tmp=strdup(line);
                int direction=atoi(getfield(tmp,3));
                /*
                tmp=strdup(line);
                int track_ok=atoi(getfield(tmp,4));
                tmp=strdup(line);
                int next_signal=atoi(getfield(tmp,5));
                //printf("x=%d y=%d direction=%d track_ok=%d next_signal=%d\n",x,y,direction,track_ok,next_signal);
                */
                if(x==xc&&y==yc&&dire1==direction)
                {
                    if(t_exists==1)
                    {
                        fprintf(temp,"%d,%d,%d,%d,%d\n",x,y,direction,t_ok,n_signal);
                    }
                    changed=1;
                }
                else
                {
                    fprintf(temp,"%s",line);
                }
            }
        }
        free(tmp);
    }
    fprintf(temp,"\n");
    if(changed==0&&t_exists==1)
    {
        fprintf(temp,"%d,%d,%d,%d,%d\n",xc,yc,dire1,t_ok,n_signal);
        printf("Entry for that direction not found. So creating new entry\n");
    }
    int rem_ok=remove("networks.txt");
    if(rem_ok!=0)
    {
        printf("Error deleting networks.txt\n");
    }
    int ren_ok=rename("temp.txt","networks.txt");
    if(ren_ok!=0)
    {
        printf("Error renaming file\n");
    }
    fclose(stream);
    fclose(temp);
}
