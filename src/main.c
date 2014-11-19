/*
 * main.c - Silicon Laboratories SI4010 programmer/debugger. 
 *
 * si4010prog specific code:
 * Copyright (c) 2014, David Imhoff <dimhoff_devel@xs4all.nl>
 *
 * Based on cycfx2prog.cc:
 * Copyright (c) 2006--2009 by Wolfgang Wieser ] wwieser (a) gmx <*> de [ 
 * 
 * This file may be distributed and/or modified under the terms of the 
 * GNU General Public License version 2 as published by the Free Software 
 * Foundation. (See COPYING.GPL for details.)
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#define  _XOPEN_SOURCE 500 
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include "dehexify.h"
#include "si4010.h"

#define VERSION "0.1"

#define MAXARGS 16 //< Maximum number of subarguments for a command argument 

void usage(const char *name)
{
	fprintf(stderr,
		"SI-4010 ISP Programmer tool %s\n"
		"Usage: %s [options] <commands...>\n"
		"Options:\n"
		"  -b             Use binary output when dumping data\n"
		"  -d <uri>       Programmer device to use. Use 'help' for help.\n"
		"  -q             Quiet\n"
		"  -h             Print this help message\n"
		"Commands:\n"
		"  identify       Get Device ID and Revision ID\n"
		"  reset          Reset 8051\n"
		"  halt           Stop 8051 execution\n"
		"  run            Resume 8051 execution\n"
		"  prg:FILE       program 8051; FILE is an Intel hex file (.ihx); will\n"
		"                 reset the 8051 before download; use \"run\" afterwards\n"
		"  wram:ADR,VAL   Write VAL to RAM starting at address ADR: VAL is a hexadecimal\n"
		"                 string\n"
		"  lram:ADR,FILE  Load content of binary FILE into RAM at address ADR\n"
		"  dram:ADR,LEN   Dump RAM content: LEN bytes starting at ADR\n"
		"  wxram:ADR,VAL  Write VAL to XRAM starting at address ADR: VAL is a hexadecimal\n"
		"                 string\n"
		"  lxram:ADR,FILE Load content of binary FILE into XRAM at address ADR\n"
		"  dxram:ADR,LEN  Dump XRAM content: LEN bytes starting at ADR\n"
		"  wsfr:ADR,VAL   Write VAL to SFR byte at address ADR\n"
		"  dsfr:ADR,LEN   Dump LEN bytes from the SFR memory starting at address ADR\n"
		"  getpc          Get the value of the PC\n"
		"  setpc:VAL      Set the value of the PC to VAL: VAL is a 16-bit hexadecimal\n"
		"                 number\n"
		"  break:ADR,NR   Set breakpoint number NR at address ADR: NR selects one of the\n"
		"                 8 breakpoints 0 being the first\n"
		"  cbreak:NR      Clear breakpoint number NR\n"
		"  waitbreak      Resume 8051, and wait for breakpoint\n"
		"  delay:NN       make a delay for NN msec\n"
		"\n"
		"Note: Currently the program has no way to detect if the MCU is running or\n"
		"      halted. If MCU is not halted when running or vice versa the protocol will\n"
		"      break and can only be recovered with a reset.\n"
		, VERSION, name);
}

void usage_dev_uri()
{
	printf( "To specify a type and location of the C2 bus hardware interface a universal\n"
		"resource identifier is used.\n"
		"The URI format is: '<interface type>://<interface specific path>'\n"
		"Below is a list of valid interface type names with a description.\n"
		"\n"
		"> c2drv\n"
		"  This interface uses a standard LPT printer port and the c2drv Linux kernel\n"
		"  module. The path specifies the device file of the kernel module.\n"
		"> fx2\n"
		"  Cypress EZ-USB FX2 based bus interface. This interface requires a special\n"
		"  firmware to be loaded into the device. If no path is specified the first FX2\n"
		"  device found is used. To use a specific USB device the path must be in the\n"
		"  format 'fx2://BBB/DDD', where 'BBB' is a 3 digit bus number and 'DDD' is the 3\n"
		"  digit device number as can be obtained with lsusb.\n"
		"> ft232 (Default)\n"
		"  FTDI FT232R based bus interface. Although easy to use, this interface is\n"
		"  rather slow. As path string the libftdi description strings are used.\n"
		"  These strings can have one of the following formats:\n"
                "   - d:<devicenode>\n"
		"     path of bus and device-node (e.g. \"003/001\") within usb device tree\n"
		"     (usually at /proc/bus/usb/)\n"
		"   - i:<vendor>:<product>\n"
		"     First device with given vendor and product id, ids can be decimal, octal\n"
		"     (preceded by \"0\") or hex (preceded by \"0x\")\n"
		"   - i:<vendor>:<product>:<index>\n"
		"     As above with index being the number of the device (starting with 0) if\n"
		"     there are more than one\n"
		"   - s:<vendor>:<product>:<serial>\n"
		"     First device with given vendor id, product id and serial string\n"
		"  By default the path \"ftdi://d:0x0403:0x6001\" is used\n"
		);
}

static void *CheckMalloc(void *ptr)
{
	if(!ptr)
	{
		fprintf(stderr,"malloc failed\n");
		exit(1);
	}
	return(ptr);
}

static inline void _HexdumpPutChar(FILE *out,unsigned char x)
{
	if(isprint(x))
	{  fprintf(out,"%c",x);  }
	else
	{  fprintf(out,".");  }
}

static void HexDumpBuffer(FILE *out,const unsigned char *data,size_t size,
	int with_ascii)
{
	size_t i;
	//ssize_t skip_start=-1;
	for(i=0; i<size; )
	{
		size_t j=0;
#if 0
		// Do not dump lines which are only ffffff....
		int do_skip=1;
		for(size_t ii=i; j<16 && ii<size; j++,ii++)
		{
			if(data[ii]!=0xffU)
			{  do_skip=0;  break;  }
		}
		if(do_skip)
		{
			if(skip_start<0)  skip_start=i;
			i+=16;
			continue;
		}
		else if(skip_start>=0)
		{
			printf("         [skipping 0x%04x..0x%04x: "
				"%u words 0xffff]\n",
				size_t(skip_start),i-1,i-size_t(skip_start));
			skip_start=-1;
		}
#endif
		
		printf("  0x%04zx ",i);
		size_t oldi=i;
		for(j=0; j<32 && i<size; j++,i++)
		{
			if(j && !(j%8))  printf(" ");
			printf("%02x",(unsigned int)data[i]);
		}
		// This adds a plaintext column (printable chars only): 
		if(with_ascii)
		{
			for(; j<32; j++)
			{  printf((j && !(j%8)) ? "   " : "  ");  }
			
			printf("    ");
			i=oldi;
			for(j=0; j<32 && i<size; j++,i++)
			{  _HexdumpPutChar(out,data[i]);  }
		}
		printf("\n");
	}
#if 0
	if(skip_start>=0 && size>0)
	{
		printf("         [skipping 0x%04x..0x%04x: "
			"%u words 0xffff]\n",
			size_t(skip_start),size-1,size-size_t(skip_start));
		skip_start=-1;
	}
#endif
	fflush(out);
}

int _ProgramIHexLine(const char *buf, const char *path, int line)
{
	const char *s = buf;
	if(*s!=':') {
		fprintf(stderr, "%s:%d: format violation (1)\n",path,line);
		return 1 ;
	}
	++s;
	
	unsigned int nbytes=0, addr=0, type=0;
	if(sscanf(s, "%02x%04x%02x", &nbytes, &addr, &type) != 3) {
		fprintf(stderr, "%s:%d: format violation (2)\n", path, line);
		return 1;
	}
	s += 8;
	
	if (type == 0) {
		unsigned char data[nbytes];
		unsigned char cksum = nbytes + addr + (addr>>8) + type;
		unsigned int i;
		unsigned int file_cksum = 0;
		//printf("  Writing nbytes=%d at addr=0x%04x\n",nbytes,addr);
		assert(nbytes<256);
		for(i=0; i < nbytes; i++)
		{
			unsigned int d=0;
			if (sscanf(s, "%02x", &d) != 1) {
				fprintf(stderr, "%s:%d: format violation (3)\n", path, line);
				return 1;
			}
			s += 2;
			data[i] = d;
			cksum += d;
		}
		if (sscanf(s, "%02x", &file_cksum) != 1) {
			fprintf(stderr, "%s:%d: format violation (4)\n", path, line);
			return 1;
		}
		if ((cksum+file_cksum) & 0xff) {
			fprintf(stderr,"%s:%d: checksum mismatch (%u/%u)\n", path,line,cksum,file_cksum);
			return 1;
		}
		if (si4010_xram_write(addr, nbytes, data) != 0) {
			return 1;
		}
	} else if (type == 1) {
		// EOF marker. Oh well, trust it. 
		return -1;
	} else {
		fprintf(stderr, "%s:%d: Unknown entry type %d\n", path, line, type);
		return 1;
	}

	return 0;
}

int ProgramIHexFile(const char *path)
{
	FILE *fp = fopen(path, "rb");
	if (fp == NULL) {
		fprintf(stderr,"Failed to open %s: %s\n", path, strerror(errno));
		return 2;
	}
	
	int n_errors=0;
	
	char buf[1024];
	int line=1;
	for(;;++line)
	{
		*buf='\0';
		if (!fgets(buf, sizeof(buf), fp))
		{
			if (feof(fp)) {
				break;
			}
			fprintf(stderr,"Reading %s (line %d): %s\n",path,line,
				strerror(ferror(fp)));
			fclose(fp); fp = NULL;

			return 3 ;
		}
		
		int rv=_ProgramIHexLine(buf, path, line);
		if (rv<0)  break;
		if (rv) {  ++n_errors;  }
	}
	
	if (fp != NULL) {
		fclose(fp);
	}
	
	return (n_errors ? -1 : 0);
}

int main(int argc, char *argv[])
{
	int opt;
	char *c2_bus_type = "ft232";
	char *c2_bus_path = "";

	int errors = 0;
	bool ignore_errors = false;
	struct c2_bus c2_bus_handle;

	while ((opt = getopt(argc, argv, "d:h")) != -1) {
		switch (opt) {
		case 'd':
			c2_bus_type = optarg;
			if (strcmp(c2_bus_type, "help") == 0) {
				usage_dev_uri();
				exit(EXIT_SUCCESS);
			} else {
				char *sep;
				sep = strstr(c2_bus_type, "://");
				if (sep == NULL) {
					fprintf(stderr, "C2 bus device uri "
							"incorrect format\n");
					exit(EXIT_FAILURE);
				}
				*sep = '\0';
				c2_bus_path = sep + 3;
			}
			break;
		case 'h':
			usage(argv[0]);
			exit(EXIT_SUCCESS);
			break;
		default: /* '?' */
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "No command specified\n");
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	// Open C2 bus device
	if (c2_bus_open(&c2_bus_handle, c2_bus_type, c2_bus_path) != 0) {
		fprintf(stderr, "Failed to open C2 bus: %s\n",
					c2_bus_get_error(&c2_bus_handle));
		c2_bus_destroy(&c2_bus_handle);
		return -1;
	}

	if (si4010_init(&c2_bus_handle) != 0) {
		c2_bus_destroy(&c2_bus_handle);
		exit(EXIT_FAILURE);
	}

	// execute commands
	while (optind < argc && (ignore_errors || errors == 0)) {
		// Copy command and args so that we can manipulate it. 
		char *cmd = strdup(argv[optind]);
		
		// Cut into command and arguments: 
		char *args[MAXARGS] = { NULL };
		char *first_arg = NULL;
		int nargs=0;

		first_arg = strchr(cmd,':');
		if(first_arg)
		{
			do {
				*(first_arg++)='\0';
				if(nargs>=MAXARGS)
				{
					fprintf(stderr,"Too many arguments for command \"%s\" "
						"(further args ignored)\n",cmd);
					++errors;
					break;
				}
				args[nargs++]=first_arg;
				first_arg=strchr(first_arg,',');
			} while(first_arg);
		}
		
#if 0
		// Debug: 
		printf("Command: <%s>",cmd);
		for(int j=0; j<nargs; j++)
		{  printf(" <%s>",args[j]);  }
		printf("\n");
#endif
		
		if (!strcmp(cmd, "identify")) {
			uint16_t id;
			id = c2_get_chip_version();
			fprintf(stderr, "Device ID: 0x%.2x; Revision ID: 0x%.2x\n", id >> 8, id & 0xff);
		} else if (!strcmp(cmd, "reset")) {
			fprintf(stderr,"Resetting SI4010\n");
			if (si4010_reset() != 0) {
				fprintf(stderr, "Command Failed\n");
				++errors;
			}
		} else if (!strcmp(cmd, "halt")) {
			fprintf(stderr,"Halting SI4010 MCU\n");
			if (si4010_halt() != 0) {
				fprintf(stderr, "Command Failed\n");
				++errors;
			}
		} else if (!strcmp(cmd, "run")) {
			fprintf(stderr,"Resuming SI4010 MCU\n");
			if (si4010_resume() != 0) {
				fprintf(stderr, "Command Failed\n");
				++errors;
			}
		} else if (!strcmp(cmd, "prg")) {
			const char *file=args[0];
			if (!file) {
				fprintf(stderr,"Command 'prg': requires file to download\n");
				++errors;
			} else {
				fprintf(stderr, "Programming CODE memory using \"%s\"\n", file);
				if (ProgramIHexFile(file) != 0) {
					++errors;
				}
			}
		} else if (!strcmp(cmd, "wram")) {
//TODO: writing to address 0 and 1 doesn't work/crash the device
			int adr = -1;
			int len = -1;
			if (args[0] && *args[0])  {  adr = strtol(args[0],NULL,0);  }
			if (args[1] && *args[1])  {  len = strlen(args[1]); }
			if (adr < 0 || adr > 0xffff) { 
				fprintf(stderr, "Command 'wram': Address out-of-range(0-0xffff)\n");
				++errors; 
			} else if (len < 0) {
				fprintf(stderr, "Command 'wram': Missing VAL\n");
				++errors;
			} else if (len & 0x01) {
				fprintf(stderr, "Command 'wram': VAL must be an even number of characters\n");
				++errors;
			} else {
				len /= 2;
				unsigned char *buf = (unsigned char*) CheckMalloc(malloc(len));
				if (dehexify(args[1], len, buf) != 0) {
					fprintf(stderr, "Command 'wram': VAL contains non hexadecimal characters\n");
					++errors;
				} else {
					fprintf(stderr, "Setting RAM bytes at 0x%.2x-0x%.2x\n", adr, adr + len);
					if (si4010_ram_write(adr, len, buf) != 0) {
						fprintf(stderr, "Command Failed\n");
						++errors;
					}
				}
				free(buf);
			}
		} else if (!strcmp(cmd, "lram")) {
//TODO: load data from file to ram at address
			fprintf(stderr, "TODO:\n");
		} else if (!strcmp(cmd, "dram")) {
			int adr = 0;
			int len = 1;
			if (args[0] && *args[0]) { adr = strtol(args[0],NULL,0); }
			if (adr < 0)  adr=0;
			if (args[1] && *args[1]) { len = strtol(args[1],NULL,0); }
			if (len < 1)  len=1;
			if (len > 1024 * 1024)  len=1024*1024;
			
			fprintf(stderr, "Dumping %u bytes of RAM at 0x%.2x:\n", len, adr);
			unsigned char *buf = (unsigned char*) CheckMalloc(malloc(len));
			memset(buf, 0, len);
			if (si4010_ram_read(adr, len, buf) == 0) {
				HexDumpBuffer(stdout, buf, len, /*with_ascii=*/1);
			} else {
				++errors;
			}
			free(buf);
		} else if (!strcmp(cmd, "wxram")) {
			int adr = -1;
			int len = -1;
			if (args[0] && *args[0])  {  adr = strtol(args[0],NULL,0);  }
			if (args[1] && *args[1])  {  len = strlen(args[1]); }
			if (adr < 0 || adr > 0xffff) { //TODO: use correct memory range for si4010
				fprintf(stderr, "Command 'wxram': Address out-of-range(0-0xffff)\n");
				++errors; 
			} else if (len < 0) {
				fprintf(stderr, "Command 'wxram': Missing VAL\n");
				++errors;
			} else if (len & 0x01) {
				fprintf(stderr, "Command 'wxram': VAL must be an even number of characters\n");
				++errors;
			} else {
				len /= 2;
				unsigned char *buf = (unsigned char*) CheckMalloc(malloc(len));
				if (dehexify(args[1], len, buf) != 0) {
					fprintf(stderr, "Command 'wxram': VAL contains non hexadecimal characters\n");
					++errors;
				} else {
					fprintf(stderr, "Setting XRAM bytes at 0x%.2x-0x%.2x\n", adr, adr + len);
					if (si4010_xram_write(adr, len, buf) != 0) {
						fprintf(stderr, "Command Failed\n");
						++errors;
					}
				}
				free(buf);
			}
		} else if (!strcmp(cmd, "lxram")) {
//TODO: load data from file to xram at address
			fprintf(stderr, "TODO:\n");
		} else if (!strcmp(cmd, "dxram")) {
			int adr = 0;
			int len = 1;
			if (args[0] && *args[0]) { adr = strtol(args[0],NULL,0); }
			if (adr < 0)  adr=0;
			if (args[1] && *args[1]) { len = strtol(args[1],NULL,0); }
			if (len < 1)  len=1;
			if (len > 1024 * 1024)  len=1024*1024;
			
			fprintf(stderr, "Dumping %u bytes of XRAM at 0x%.2x:\n", len, adr);
			unsigned char *buf = (unsigned char*) CheckMalloc(malloc(len));
			memset(buf, 0, len);
			if (si4010_xram_read(adr, len, buf) == 0) {
				HexDumpBuffer(stdout, buf, len, /*with_ascii=*/1);
			} else {
				++errors;
			}
			free(buf);
		} else if(!strcmp(cmd, "wsfr")) {
			int adr=-1;
			int val=-1;
			if (args[0] && *args[0])  {  adr=strtol(args[0],NULL,0);  }
			if (args[1] && *args[1])  {  val=strtol(args[1],NULL,0);  }
			if (adr < 0x80 || adr > 0xff || val < 0 || val > 0xff) { 
				fprintf(stderr,"Command 'wsfr': Illegal/missing address "
						"and/or value.\n");
				++errors; 
			} else {
				uint8_t bval = val;
				fprintf(stderr, "Setting SFR at 0x%.2x to 0x%.2x\n", adr, val);
				if (si4010_sfr_write(adr, 1, &bval) != 0) {
					fprintf(stderr, "Command Failed\n");
					++errors;
				}
			}
		} else if(!strcmp(cmd, "dsfr")) {
			int adr=-1;
			int len=1;
			if (args[0] && *args[0]) {  adr=strtol(args[0],NULL,0);  }
			if (args[1] && *args[1]) {  len=strtol(args[1],NULL,0);  }
			if (len<1)  len=1;
			if (len > 1024*1024)  len=1024*1024;
			if (adr < 0x80 || adr > 0xff) { 
				fprintf(stderr,"Command 'dsfr': Address out of range(0x80-0xff).\n");
				++errors; 
			} else {
				if (adr + len > 0x100) { len = 0x100 - adr; };
			
				fprintf(stderr, "Dumping SFR registers 0x%.2x-0x%.2x:\n", adr, adr + len);
				unsigned char buf[128] = { 0 };
				if (si4010_sfr_read(adr, len, buf) == 0) {
					HexDumpBuffer(stdout, buf, len, /*with_ascii=*/1);
				} else {
					fprintf(stderr, "Command Failed\n");
					++errors;
				}
			}
		} else if(!strcmp(cmd, "getpc")) {
			uint16_t pc;
			fprintf(stderr, "Getting program counter:\n");
			if (si4010_pc_get(&pc) != 0) {
				fprintf(stderr, "Command Failed\n");
				++errors;
			}
			printf("0x%.4hx\n", pc);
		} else if(!strcmp(cmd, "setpc")) {
			int pc=-1;
			if (args[0] && *args[0]) {  pc = strtol(args[0],NULL,0);  }
			if (pc < 0 || pc > 0xffff) {
				fprintf(stderr,"Command 'setpc': Value out of range(0x0-0xffff).\n");
				++errors; 
			} else {
				fprintf(stderr, "Setting program counter to 0x%.4hx\n", pc);
				if (si4010_pc_set(pc) != 0) {
					fprintf(stderr, "Command Failed\n");
					++errors;
				}
			}
		} else if(!strcmp(cmd, "break")) {
			int adr=-1;
			int bp=0;
			if (args[0] && *args[0])  {  adr = strtol(args[0],NULL,0);  }
			if (args[1] && *args[1])  {  bp = strtol(args[1],NULL,0);  }
			if (adr < 0 || adr > 0xffff) {
				fprintf(stderr,"Command 'break': Address out of range(0-0xffff).\n");
				++errors; 
			} else if (bp < 0 || bp > 7) { 
				fprintf(stderr,"Command 'break': NR out of range(0-7).\n");
				++errors; 
			} else {
				si4010_bp_set(bp, adr);
			}
		} else if(!strcmp(cmd, "cbreak")) {
			int bp=0;
			if (args[0] && *args[0]) {
				bp = strtol(args[0], NULL, 0);
				if (bp < 0 || bp > 7) { 
					fprintf(stderr,"Command 'break': NR out of range(0-7).\n");
					++errors; 
				} else {
					si4010_bp_clear(bp);
				}
			} else {
				si4010_bp_clear_all();
			}
		} else if(!strcmp(cmd, "waitbreak")) {
			fprintf(stderr, "TODO:\n");
		} else if(!strcmp(cmd, "delay")) {
			long delay=-1;
			if(args[0] && *args[0]) {  delay=strtol(args[0], NULL, 0);  }
			if(delay<0)  delay=250;
			fprintf(stderr,"Delay: %ld msec\n", delay);
			usleep(delay * 1000);
		} else {
			fprintf(stderr, "Command '%s': Unknown command\n", cmd);
			++errors; 
		}
		optind++;
	}

	c2_bus_destroy(&c2_bus_handle);
	return 0;
}
