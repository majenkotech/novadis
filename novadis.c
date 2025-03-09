#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>

unsigned long address = 0;
int byteswap = 0;
int number_format = 0;

const char *devnames[] = {
	"DEV0",
	"MDV", 	// Multiply/Divide 
	"MMU",
	"MAP1",
	"PAR",
	"DEV5",
	"MCAT",
	"MCAR",
	"TTI",
	"TTO",
	"PTR",
	"PTP",
	"RTC",
	"CDR",
	"LPT",
	"DSK",
	"ADCV",
	"MTA",
	"DACV",
	"DCM",
	"DEV20",
	"DEV21",
	"DEV22",
	"QTY",
	"SLA",
	"IBM1",
	"IBM2",
	"DKP",
	"CAS",
	"CRC",
	"IBP",
	"IVT",
	"DPI",
	"DPO",
	"DIO",
	"DIOT",
	"MXM",
	"DEV37",
	"MCAT1",
	"MCAR1",
	"TTI1",
	"TTO0",
	"PTR1",
	"PTP1",
	"RTC1",
	"PLT1",
	"CDR1",
	"LPT1",
	"DSK1",
	"ADCV1",
	"MTA1",
	"DACV1",
	"DEV52",
	"DEV53",
	"DEV54",
	"DEV55",
	"QTY1",
	"DEV57",
	"DEV58",
	"DKP1",
	"FPU1",
	"FPU2",
	"DEV62",
	"CPU"
};

struct opcode {
	uint16_t bits;
	uint16_t mask;
	void (*func)(uint16_t, char *);
};

int extract(uint16_t val, int offset, int len) {
	int x = val >> offset;
	x &= ((1 << len) - 1);
	return x;
}

void eaddr(uint16_t ins, char *buf) {
	if (ins & 0b0000010000000000) {
		strcat(buf, "@");
	}
	char temp[50];
	int index = extract(ins, 8, 2);
	uint8_t displace = extract(ins, 0, 8);
	int8_t sdisplace = (int8_t)displace;

	switch (index) {
		case 0b00:
			switch (number_format) {
				case 0: sprintf(temp, "%d", displace); break;
				case 1: sprintf(temp, "0x%x", displace); break;
				case 2: sprintf(temp, "0%o", displace); break;
			}
			break;
		case 0b01:
			switch (number_format) {
				case 0: sprintf(temp, "%c%d [%d]", sdisplace >= 0 ? '+' : '-', abs(sdisplace), address + sdisplace); break;
				case 1: sprintf(temp, "%c0x%x [0x%x]", sdisplace >= 0 ? '+' : '-', abs(sdisplace), address + sdisplace); break;
				case 2: sprintf(temp, "%c0%o [0%o]", sdisplace >= 0 ? '+' : '-', abs(sdisplace), address + sdisplace); break;
			}
			break;
		case 0b10:
			switch (number_format) {
				case 0: sprintf(temp, "AC2 %c %d", sdisplace >= 0 ? '+' : '-', abs(sdisplace)); break;
				case 1: sprintf(temp, "AC2 %c 0x%x", sdisplace >= 0 ? '+' : '-', abs(sdisplace)); break;
				case 2: sprintf(temp, "AC2 %c 0%o", sdisplace >= 0 ? '+' : '-', abs(sdisplace)); break;
			}	
			break;
		case 0b11:
			switch (number_format) {
				case 0: sprintf(temp, "AC3 %c %d", sdisplace >= 0 ? '+' : '-', abs(sdisplace)); break;
				case 1: sprintf(temp, "AC3 %c 0x%x", sdisplace >= 0 ? '+' : '-', abs(sdisplace)); break;
				case 2: sprintf(temp, "AC3 %c 0%o", sdisplace >= 0 ? '+' : '-', abs(sdisplace)); break;
			}
			break;
	}
	strcat(buf, temp);
}

void lda(uint16_t ins, char *buf) {
	sprintf(buf, "LDA AC%d,", extract(ins, 11, 2));
	eaddr(ins, buf);
}

void sta(uint16_t ins, char *buf) {
	sprintf(buf, "STA AC%d,", extract(ins, 11, 2));
	eaddr(ins, buf);
}

void csh(uint8_t ins, char *buf) {
	int sh = extract(ins, 6, 2);
	int c = extract(ins, 4, 2);

	switch (c) {
		case 0b00:
			break;
		case 0b01:
			strcat(buf, "Z");
			break;
		case 0b10:
			strcat(buf, "O");
			break;
		case 0b11:
			strcat(buf, "C");
			break;
	}

	switch (sh) {
		case 0b00:
			break;
		case 0b01:
			strcat(buf, "L");
			break;
		case 0b10:
			strcat(buf, "R");
			break;
		case 0b11:
			strcat(buf, "S");
			break;
	}

}

void skip(uint16_t ins, char *buf) {
	int sk = extract(ins, 0, 3);
	switch (sk) {
		case 0b000:
			break;
		case 0b001:
			strcat(buf, ",SKP");
			break;
		case 0b010:
			strcat(buf, ",SZC");
			break;
		case 0b011:
			strcat(buf, ",SNC");
			break;
		case 0b100:
			strcat(buf, ",SZR");
			break;
		case 0b101:
			strcat(buf, ",SNR");
			break;
		case 0b110:
			strcat(buf, ",SEZ");
			break;
		case 0b111:
			strcat(buf, ",SBN");
			break;
	}
}

void multi2ac(uint16_t ins, char *buf, const char *op) {
	strcat(buf, op);
	csh(ins, buf);
	char temp[100];
	sprintf(temp, " AC%d,AC%d", extract(ins, 13, 2), extract(ins, 11, 2));
	strcat(buf, temp);
	skip(ins, buf);
}

void stack(uint16_t ins, char *buf, const char *op) {
	sprintf(buf, "%s AC%d", op, extract(ins, 11, 2));
}

void jump(uint16_t ins, char *buf, const char *op) {
	sprintf(buf, "%s ", op);
	eaddr(ins, buf);
}

void iof(uint16_t ins, char *buf, const char *op) {
	strcat(buf, op);

	int flags = extract(ins, 6, 2);
	switch (flags) {
		case 0b00:
			break;
		case 0b01:
			strcat(buf, "S");
			break;
		case 0b10:
			strcat(buf, "C");
			break;
		case 0b11:
			strcat(buf, "P");
			break;
	}

	char temp[50];
	int devno = extract(ins, 0, 6);
	if (devno == 0b111111) {
		strcat(buf, " CPU");
	} else {
		sprintf(temp, " AC%d,%s", extract(ins, 11, 2), devnames[devno]);
		strcat(buf, temp);
	}
}

void iot(uint16_t ins, char *buf, const char *op) {
	strcat(buf, op);

	int flags = extract(ins, 6, 2);
	switch (flags) {
		case 0b00:
			strcat(buf, "BN");
			break;
		case 0b01:
			strcat(buf, "BZ");
			break;
		case 0b10:
			strcat(buf, "DN");
			break;
		case 0b11:
			strcat(buf, "DZ");
			break;
	}

	char temp[50];
	int devno = extract(ins, 0, 6);
	if (devno == 0b111111) {
		strcat(buf, " CPU");
	} else {
		sprintf(temp, " %s", devnames[extract(ins, 0, 6)]);
		strcat(buf, temp);
	}
}


void add(uint16_t ins, char *buf) { multi2ac(ins, buf, "ADD"); }
void sub(uint16_t ins, char *buf) { multi2ac(ins, buf, "SUB"); }
void neg(uint16_t ins, char *buf) { multi2ac(ins, buf, "NEG"); }
void adc(uint16_t ins, char *buf) { multi2ac(ins, buf, "ADC"); }
void mov(uint16_t ins, char *buf) { multi2ac(ins, buf, "MOV"); }
void inc(uint16_t ins, char *buf) { multi2ac(ins, buf, "INC"); }
void com(uint16_t ins, char *buf) { multi2ac(ins, buf, "COM"); }
void and(uint16_t ins, char *buf) { multi2ac(ins, buf, "AND"); }

void psha(uint16_t ins, char *buf) { stack(ins, buf, "PSHA"); }
void popa(uint16_t ins, char *buf) { stack(ins, buf, "POPA"); }
void sav(uint16_t ins, char *buf) { strcat(buf, "SAV"); }
void mtsp(uint16_t ins, char *buf) { stack(ins, buf, "MTSP"); }
void mtfp(uint16_t ins, char *buf) { stack(ins, buf, "MTFP"); }
void mfsp(uint16_t ins, char *buf) { stack(ins, buf, "MFSP"); }
void mffp(uint16_t ins, char *buf) { stack(ins, buf, "MFFP"); }
void jmp(uint16_t ins, char *buf) { jump(ins, buf, "JMP"); }
void jsr(uint16_t ins, char *buf) { jump(ins, buf, "JSR"); }
void isz(uint16_t ins, char *buf) { jump(ins, buf, "ISZ"); }
void dsz(uint16_t ins, char *buf) { jump(ins, buf, "DSZ"); }

void ret(uint16_t ins, char *buf) { strcat(buf, "RET"); }

void trap(uint16_t ins, char *buf) {
	switch (number_format) {
		case 0: sprintf(buf, "TRAP AC$d,AC%d,%d", extract(ins, 13, 2), extract(ins, 11, 2), extract(ins, 4, 7)); break;
		case 1: sprintf(buf, "TRAP AC$d,AC%d,0x%x", extract(ins, 13, 2), extract(ins, 11, 2), extract(ins, 4, 7)); break;
		case 2: sprintf(buf, "TRAP AC$d,AC%d,0%o", extract(ins, 13, 2), extract(ins, 11, 2), extract(ins, 4, 7)); break;
	}
}

void dia(uint16_t ins, char *buf) { iof(ins, buf, "DIA"); }
void dib(uint16_t ins, char *buf) { iof(ins, buf, "DIB"); }
void dic(uint16_t ins, char *buf) { iof(ins, buf, "DIC"); }
void doa(uint16_t ins, char *buf) { iof(ins, buf, "DOA"); }
void dob(uint16_t ins, char *buf) { iof(ins, buf, "DOB"); }
void doc(uint16_t ins, char *buf) { iof(ins, buf, "DOC"); }
void skp(uint16_t ins, char *buf) { iot(ins, buf, "SKP"); }
void nio(uint16_t ins, char *buf) { iof(ins, buf, "NIO"); }

void mul(uint16_t ins, char *buf) { strcat(buf, "MUL"); }
void xdiv(uint16_t ins, char *buf) { strcat(buf, "DIV"); }


struct opcode opcodes[] = {
	0b0110010110000001, 0b1111111111111111, &ret,
	0b0111011011000001, 0b1111111111111111, &mul,
	0b0111011001000001, 0b1111111111111111, &xdiv,
	0b0110010100000001, 0b1111111111111111, &sav,

	0b0110011100000000, 0b1111111100000000, &skp,
	0b0110000000000000, 0b1111111100000000, &nio,

	0b0000000000000000, 0b1111100000000000, &jmp,
	0b0000100000000000, 0b1111100000000000, &jsr,
	0b0001000000000000, 0b1111100000000000, &isz,
	0b0001100000000000, 0b1111100000000000, &dsz,

	0b0110001100000001, 0b1110011111111111, &psha,
	0b0110001110000001, 0b1110011111111111, &popa,
	0b0110001000000001, 0b1110011111111111, &mtsp,
	0b0110000000000001, 0b1110011111111111, &mtfp,
	0b0110001010000001, 0b1110011111111111, &mfsp,
	0b0110000010000001, 0b1110011111111111, &mffp,
	0b0110000100000000, 0b1110011100000000, &dia,
	0b0110001100000000, 0b1110011100000000, &dib,
	0b0110010100000000, 0b1110011100000000, &dic,
	0b0110001000000000, 0b1110011100000000, &doa,
	0b0110010000000000, 0b1110011100000000, &dob,
	0b0110011000000000, 0b1110011100000000, &doc,
	0b0010000000000000, 0b1110000000000000, &lda,
	0b0100000000000000, 0b1110000000000000, &sta,

	0b1000011000000000, 0b1000011100000000, &add,
	0b1000010100000000, 0b1000011100000000, &sub,
	0b1000000100000000, 0b1000011100000000, &neg,
	0b1000001000000000, 0b1000011100000000, &mov,
	0b1000001100000000, 0b1000011100000000, &inc,
	0b1000000000000000, 0b1000011100000000, &com,
	0b1000011100000000, 0b1000011100000000, &and,
	0b1000010000000000, 0b1000011100000000, &adc,

	0b1000000000001000, 0b1000000000001111, &trap,

	0, 0, 0
};


void decode(uint16_t ins, char *buf) {
	buf[0] = 0;

	for (int i = 0; opcodes[i].mask != 0; i++) {
		if ((ins & opcodes[i].mask) == opcodes[i].bits) {
			opcodes[i].func(ins, buf);
			return;
		}
	}
	strcat(buf, "[UNKNOWN]");
}

void usage() {
	printf("Usage: novadis [options] <filename>\n");
	printf("    Options:\n");
	printf("        -a <address>: Start address\n");
	printf("        -s: Swap bytes before decoding\n");
	printf("        -d: Use decimal for numbers\n");
	printf("        -x: Use hexadecimal for numbers\n");
	printf("        -o: Use octal for numbers\n");
}

int main(int argc, char **argv) {


	int opt;

	while ((opt = getopt(argc, argv, "shoxda:")) != -1) {
		switch (opt) {
			case 'a':
				address = strtoul(optarg, NULL, 0);
				break;
			case 's': 
				byteswap = 1;
				break;
			case 'd':
				number_format = 0;
				break;
			case 'x':
				number_format = 1;
				break;
			case 'o':
				number_format = 2;
				break;
			case 'h':
				usage();
				return -1;
		}
	}

	const char *filename = argv[optind];

	if (argc <= optind) {
		usage();
		return -1;
	}

	if (access(filename, R_OK) != 0) {
		fprintf(stderr, "Error: cannot access %s\n", filename);
		return -1;
	}

	int fd = open(filename, O_RDONLY);

	uint16_t wrd;


	while (read(fd, &wrd, 2) == 2) {
		char tmp[1024];

		uint8_t bh = (wrd >> 8);
		uint8_t bl = (wrd & 0xFF);

		if (byteswap == 1) {
			int t = bh;
			bh = bl;
			bl = t;
			wrd = bh << 8 | bl;
		}

		decode(wrd, tmp);

		switch (number_format) {
			case 0:
				printf("%5d : '%c%c' : %5d : %s\n", address, (bh >= 32 && bh < 127) ? bh : ' ', (bl >= 32 && bl < 127) ? bl : ' ', wrd, tmp);
				break;
			case 1:
				printf("%4x : '%c%c' : %04x : %s\n", address, (bh >= 32 && bh < 127) ? bh : ' ', (bl >= 32 && bl < 127) ? bl : ' ', wrd, tmp);
				break;
			case 2:
				printf("%6o : '%c%c' : %6o : %s\n", address, (bh >= 32 && bh < 127) ? bh : ' ', (bl >= 32 && bl < 127) ? bl : ' ', wrd, tmp);
				break;
		}
		address++;
	}

	close(fd);

	return 0;
}
