/*Tiertex (GB/GBC) to MIDI converter*/
/*By Will Trowbridge, based on MC2MID*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * mid;
long bank;
long offset;
long tableOffset;
int i, j;
char outfile[1000000];
/*End of frequency table - sequence pointers start immediately after*/
const char MusicFind[10] = { 0xE5, 0x07, 0xE7, 0x07, 0xE8, 0x07, 0xE9, 0x07, 0xEB, 0x07 };

/*Alternate table end + initial pointer for Triple Play 2001*/
const char MusicFindTP[10] = { 0xD9, 0x07, 0xDB, 0x07, 0xDD, 0x07, 0xDF, 0x07, 0x39, 0x50 };
long seqPtrs[4];
long nextPtr;
long endPtr;
long bankAmt;
int songNum;

long midLength;

unsigned static char* romData;
unsigned static char* midData;
unsigned static char* ctrlMidData;

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
unsigned int WriteNoteEvent(unsigned static char* buffer, unsigned int pos, unsigned int note, int length, int delay, int firstNote, int curChan);
int WriteDeltaTime(unsigned static char* buffer, unsigned int pos, unsigned int value);
void song2mid(int songNum, long ptrs[], long nextPtr);

unsigned short ReadLE16(unsigned char* Data)
{
	return (Data[0] << 0) | (Data[1] << 8);
}

static void Write8B(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = value;
}

static void WriteBE32(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF000000) >> 24;
	buffer[0x01] = (value & 0x00FF0000) >> 16;
	buffer[0x02] = (value & 0x0000FF00) >> 8;
	buffer[0x03] = (value & 0x000000FF) >> 0;

	return;
}

static void WriteBE24(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF0000) >> 16;
	buffer[0x01] = (value & 0x00FF00) >> 8;
	buffer[0x02] = (value & 0x0000FF) >> 0;

	return;
}

static void WriteBE16(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = (value & 0xFF00) >> 8;
	buffer[0x01] = (value & 0x00FF) >> 0;

	return;
}

unsigned int WriteNoteEvent(unsigned static char* buffer, unsigned int pos, unsigned int note, int length, int delay, int firstNote, int curChan)
{
	int deltaValue;
	deltaValue = WriteDeltaTime(buffer, pos, delay);
	pos += deltaValue;
	if (firstNote == 1)
	{
		if (curChan != 3)
		{
			Write8B(&buffer[pos], 0x90 | curChan);
		}
		else
		{
			Write8B(&buffer[pos], 0x99);
		}

		pos++;
	}
	Write8B(&buffer[pos], note);
	pos++;
	Write8B(&buffer[pos], 100);
	pos++;

	deltaValue = WriteDeltaTime(buffer, pos, length);
	pos += deltaValue;

	Write8B(&buffer[pos], note);
	pos++;
	Write8B(&buffer[pos], 0);
	pos++;

	return pos;
}

int WriteDeltaTime(unsigned static char* buffer, unsigned int pos, unsigned int value)
{
	unsigned char valSize;
	unsigned char* valData;
	unsigned int tempLen;
	unsigned int curPos;

	valSize = 0;
	tempLen = value;

	while (tempLen != 0)
	{
		tempLen >>= 7;
		valSize++;
	}

	valData = &buffer[pos];
	curPos = valSize;
	tempLen = value;

	while (tempLen != 0)
	{
		curPos--;
		valData[curPos] = 128 | (tempLen & 127);
		tempLen >>= 7;
	}

	valData[valSize - 1] &= 127;

	pos += valSize;

	if (value == 0)
	{
		valSize = 1;
	}
	return valSize;
}

int main(int args, char* argv[])
{
	printf("Tiertex (GB/GBC) to MIDI converter\n");
	if (args != 3)
	{
		printf("Usage: TT2MID <rom> <bank>");
		return -1;
	}
	else
	{
		if ((rom = fopen(argv[1], "rb")) == NULL)
		{
			printf("ERROR: Unable to open file %s!\n", argv[1]);
			exit(1);
		}
		else
		{
			bank = strtol(argv[2], NULL, 16);
			if (bank != 1)
			{
				bankAmt = bankSize;
			}
			else
			{
				bankAmt = 0;
			}
		}
		fseek(rom, ((bank - 1) * bankSize), SEEK_SET);
		romData = (unsigned char*)malloc(bankSize);
		fread(romData, 1, bankSize, rom);
		fclose(rom);

		/*Try to search the bank for song table loader*/
		for (i = 0; i < bankSize; i++)
		{
			if (!memcmp(&romData[i], MusicFind, 10))
			{
				tableOffset = bankAmt + i + 10;
				printf("Song table found at address 0x%04x!\n", tableOffset);
				break;
			}
			if (!memcmp(&romData[i], MusicFindTP, 10))
			{
				tableOffset = bankAmt + i + 8;
				printf("Song table found at address 0x%04x!\n", tableOffset);
				break;
			}
		}
		if (tableOffset != NULL)
		{
			songNum = 1;
			i = tableOffset;
			while ((nextPtr = ReadLE16(&romData[i + 8 - bankAmt])) != 65535)
			{
				seqPtrs[0] = ReadLE16(&romData[i - bankAmt]);
				printf("Song %i channel 1: 0x%04x\n", songNum, seqPtrs[0]);
				seqPtrs[1] = ReadLE16(&romData[i + 2 - bankAmt]);
				printf("Song %i channel 2: 0x%04x\n", songNum, seqPtrs[1]);
				seqPtrs[2] = ReadLE16(&romData[i + 4 - bankAmt]);
				printf("Song %i channel 3: 0x%04x\n", songNum, seqPtrs[2]);
				seqPtrs[3] = ReadLE16(&romData[i + 6 - bankAmt]);
				printf("Song %i channel 4: 0x%04x\n", songNum, seqPtrs[3]);
				endPtr = nextPtr - bankAmt;
				song2mid(songNum, seqPtrs, nextPtr);

				i += 8;
				songNum++;
			}

			seqPtrs[0] = ReadLE16(&romData[i - bankAmt]);
			printf("Song %i channel 1: 0x%04x\n", songNum, seqPtrs[0]);
			seqPtrs[1] = ReadLE16(&romData[i + 2 - bankAmt]);
			printf("Song %i channel 2: 0x%04x\n", songNum, seqPtrs[1]);
			seqPtrs[2] = ReadLE16(&romData[i + 4 - bankAmt]);
			printf("Song %i channel 3: 0x%04x\n", songNum, seqPtrs[2]);
			seqPtrs[3] = ReadLE16(&romData[i + 6 - bankAmt]);
			printf("Song %i channel 4: 0x%04x\n", songNum, seqPtrs[3]);
			endPtr = bankSize;
			song2mid(songNum, seqPtrs, nextPtr);
		}
		else
		{
			printf("ERROR: Magic bytes not found!\n");
			exit(-1);
		}
		printf("The operation was successfully completed!\n");
	}
	return 0;
}

void song2mid(int songNum, long ptrs[], long nextPtr)
{
	static const char* TRK_NAMES[4] = { "Square 1", "Square 2", "Wave", "Noise" };
	long romPos = 0;
	unsigned int midPos = 0;
	long seqPos = 0;
	int trackCnt = 4;
	int curTrack = 0;
	long midTrackBase = 0;
	unsigned int curDelay = 0;
	int midChan = 0;
	int trackEnd = 0;
	int noteTrans = 0;
	int ticks = 120;

	unsigned int ctrlMidPos = 0;
	long ctrlMidTrackBase = 0;

	int valSize = 0;

	long trackSize = 0;

	unsigned int curNote = 0;
	int curVol = 0;
	int curNoteLen = 0;
	int lastNote = 0;

	long tempPos = 0;

	long tempo = 0;

	unsigned int macReturn = 0;
	unsigned long macroBase = 0;
	signed int macTranspose = 0;
	unsigned short macCount = 0;

	unsigned char command[4];
	unsigned char lowNibble;
	unsigned char highNibble;

	int firstNote = 1;
	int repeat = 0;

	unsigned long repeatBase = 0;

	midPos = 0;
	ctrlMidPos = 0;

	midLength = 0x10000;
	midData = (unsigned char*)malloc(midLength);

	ctrlMidData = (unsigned char*)malloc(midLength);

	for (j = 0; j < midLength; j++)
	{
		midData[j] = 0;
		ctrlMidData[j] = 0;
	}

	sprintf(outfile, "song%d.mid", songNum);
	if ((mid = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%d.mid!\n", songNum);
		exit(2);
	}
	else
	{
		/*Write MIDI header with "MThd"*/
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x4D546864);
		WriteBE32(&ctrlMidData[ctrlMidPos + 4], 0x00000006);
		ctrlMidPos += 8;

		WriteBE16(&ctrlMidData[ctrlMidPos], 0x0001);
		WriteBE16(&ctrlMidData[ctrlMidPos + 2], trackCnt + 1);
		WriteBE16(&ctrlMidData[ctrlMidPos + 4], ticks);
		ctrlMidPos += 6;

		/*Get the initial tempo*/
		tempo = 150;


		/*Write initial MIDI information for "control" track*/
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x4D54726B);
		ctrlMidPos += 8;
		ctrlMidTrackBase = ctrlMidPos;

		/*Set channel name (blank)*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE16(&ctrlMidData[ctrlMidPos], 0xFF03);
		Write8B(&ctrlMidData[ctrlMidPos + 2], 0);
		ctrlMidPos += 2;

		/*Set initial tempo*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0xFF5103);
		ctrlMidPos += 4;

		WriteBE24(&ctrlMidData[ctrlMidPos], 60000000 / tempo);
		ctrlMidPos += 3;

		/*Set time signature*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5804);
		ctrlMidPos += 3;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x04021808);
		ctrlMidPos += 4;

		/*Set key signature*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5902);
		ctrlMidPos += 4;

		for (curTrack = 0; curTrack < trackCnt; curTrack++)
		{
			firstNote = 1;
			/*Write MIDI chunk header with "MTrk"*/
			WriteBE32(&midData[midPos], 0x4D54726B);
			midPos += 8;
			midTrackBase = midPos;

			curDelay = 0;
			trackEnd = 0;

			curNote = 0;
			lastNote = 0;
			curNoteLen = 0;

			/*Add track header*/
			valSize = WriteDeltaTime(midData, midPos, 0);
			midPos += valSize;
			WriteBE16(&midData[midPos], 0xFF03);
			midPos += 2;
			Write8B(&midData[midPos], strlen(TRK_NAMES[curTrack]));
			midPos++;
			sprintf((char*)&midData[midPos], TRK_NAMES[curTrack]);
			midPos += strlen(TRK_NAMES[curTrack]);

			romPos = ptrs[curTrack] - bankAmt;

			if (curTrack != 3 && romData[romPos] > 0x7F)
			{
				romPos++;
			}

			command[0] = romData[romPos];
			command[1] = romData[romPos + 1];
			command[2] = romData[romPos + 2];
			command[3] = romData[romPos + 3];

			while (romPos < bankSize && trackEnd == 0)
			{
				command[0] = romData[romPos];
				command[1] = romData[romPos + 1];
				command[2] = romData[romPos + 2];
				command[3] = romData[romPos + 3];
				switch (command[0])
				{
					/*End of macro*/
				case 0xF8:
					if (macCount > 1)
					{
						romPos = macroBase - bankAmt;
						macCount--;
					}
					else
					{
						/*Workaround for Men in Black*/
						if (romPos + bankAmt == 0x5A1D && ptrs[0] == 0x59FA && ptrs[1] == 0x5A0C && ptrs[2] == 0x5A1E && ptrs[3] == 0x5A2F)
						{
							trackEnd = 1;
						}
						if (romPos + bankAmt == 0x5A2E && ptrs[0] == 0x59FA && ptrs[1] == 0x5A0C && ptrs[2] == 0x5A1E && ptrs[3] == 0x5A2F)
						{
							trackEnd = 1;
						}
						if (romPos + bankAmt == 0x5AD7 && ptrs[0] == 0x59FA && ptrs[1] == 0x5A0C && ptrs[2] == 0x5A1E && ptrs[3] == 0x5A2F)
						{
							trackEnd = 1;
						}
						romPos = macReturn;
					}
					break;
					/*Call macro*/
				case 0xF9:
					macCount = command[1];
					macTranspose = (signed char)command[2];
					macroBase = ReadLE16(&romData[romPos + 3]);
					if (macroBase == ptrs[curTrack])
					{
						trackEnd = 1;
					}
					macReturn = romPos + 5;
					romPos = macroBase - bankAmt;
					break;

					/*Loop point*/
				case 0xFC:
					/*Fix for Hercules empty channel 4 (fanfare)*/
					if (command[1] == 0xFF && command[2] == 0xF9)
					{
						trackEnd = 1;
					}
					repeatBase = romPos + 2;
					repeat = command[1];
					romPos = repeatBase;
					break;

					/*End of loop*/
				case 0xFD:
					/*Workaround for A Bug's Life*/
					if (repeat == 8 && ptrs[0] == 0x4F0D && ptrs[1] == 0x4DA9 && ptrs[2] == 0x4E89 && ptrs[3] == 0x4F30)
					{
						repeat = 1;
					}

					/*Workaround for Olympic Summer Games*/
					if (repeat == 200 || repeat == 100)
					{
						repeat = 1;
					}

					/*Workaround for Olympic Summer Games*/
					if (repeat == 20 && ptrs[0] == 0x6EBE && ptrs[1] == 0x6ECD && ptrs[2] == 0x43B4 && ptrs[3] == 0x6EEB)
					{
						repeat = 1;
					}
					if (repeat > 1)
					{
						romPos = repeatBase;
						repeat--;
					}
					else
					{
						romPos++;
					}
					break;

					/*Stop song (for non-looping tunes)*/
				case 0xFE:
					trackEnd = 1;
					break;

					/*Restart song*/
				case 0xFF:
					trackEnd = 1;
					break;

				default:
					if (command[0] < 128)
					{
						curNote = command[0];
						curNoteLen = command[1] * 5;
						if (curNote == 0)
						{
							curDelay += curNoteLen;
						}
						else
						{
							/*Re-map percussion to general MIDI*/
							if (curTrack == 3)
							{
								if (command[0] == 1)
								{
									curNote = 35;
								}
								else if (command[0] == 2)
								{
									curNote = 38;
								}
								else if (command[0] == 3)
								{
									curNote = 42;
								}
								else if (command[0] == 4)
								{
									curNote = 46;
								}
								else if (command[0] == 5)
								{
									curNote = 49;
								}
								else if (command[0] == 6)
								{
									curNote = 40;
								}
								else if (command[0] == 7)
								{
									curNote = 41;
								}
								else if (command[0] == 8)
								{
									curNote = 43;
								}
								else if (command[0] == 9)
								{
									curNote = 45;
								}
								else if (command[0] == 10)
								{
									curNote = 47;
								}
								else if (command[0] == 11)
								{
									curNote = 48;
								}
								else if (command[0] == 12)
								{
									curNote = 50;
								}

							}

							/*Workaround for key change effect in A Bug's Life bonus music*/
							if (ptrs[0] == 0x5C71 && ptrs[1] == 0x5CB4 && ptrs[2] == 0x5CF7 && ptrs[3] == 0x5D3A)
							{
								if (macTranspose >= -6 && curTrack != 3)
								{
									curNote += macTranspose + 6;
								}
							}
							/*Workaround for key change effect in Mulan story music*/
							if (ptrs[0] == 0x5FB6 && ptrs[1] == 0x6019 && ptrs[2] == 0x6067 && ptrs[3] == 0x6082)
							{
								if (macTranspose == -2 && curTrack != 3)
								{
									curNote += 5;
								}
							}
							tempPos = WriteNoteEvent(midData, midPos, curNote, curNoteLen, curDelay, firstNote, curTrack);
							firstNote = 0;
							midPos = tempPos;
							curDelay = 0;
						}
						romPos += 2;
					}
					else
					{
						romPos++;
					}


					break;
				}

			}
			/*End of track*/
			WriteBE32(&midData[midPos], 0xFF2F00);
			midPos += 4;

			/*Calculate MIDI channel size*/
			trackSize = midPos - midTrackBase;
			WriteBE16(&midData[midTrackBase - 2], trackSize);
		}

		/*End of control track*/
		ctrlMidPos++;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0xFF2F00);
		ctrlMidPos += 4;

		/*Calculate MIDI channel size*/
		trackSize = ctrlMidPos - ctrlMidTrackBase;
		WriteBE16(&ctrlMidData[ctrlMidTrackBase - 2], trackSize);

		sprintf(outfile, "song%d.mid", songNum);
		fwrite(ctrlMidData, ctrlMidPos, 1, mid);
		fwrite(midData, midPos, 1, mid);
		fclose(mid);


	}

}