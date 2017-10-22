#include <vector>
#include <stdio.h>

#define MAXNOTELAME (36*36-36)
#define MAXNOTETYPE (36*36)
#define MAXNOTESOUND (36*36)

using namespace std;

// �ő���񐔂����߂�֐�
int CalcGCD(int a, int b) {
	if (a == 0 || b == 0)
		return 0;

	if (a < b) {
		int tmp = a;
		a = b;
		b = tmp;
	}

	int r = a%b;
	while (r != 0) {
		int tmp = b%r;
		b = r;
		r = tmp;
	}

	return b;
}


// �ϒ����l���t�@�C������擾
// �߂�l�͌Œ蒷���l
int ReadVariableLengthNumber(FILE *fp, int *byteCnt=NULL) {
	if (!fp)
		return 0;

	if (byteCnt) {
		*byteCnt = 0;
	}

	int n = 0;
	while (true) {
		unsigned char tmp = 0;
		fread(&tmp, 1, 1, fp);
		int tmp2 = (int)tmp;

		if (byteCnt) {
			(*byteCnt)++;
		}

		n <<= 7;
		n |= (tmp2&0b01111111);

		if (!(tmp2 >> 7)) {
			// �r�b�g7��0�Ȃ�擾�I��
			break;
		}
	}

	return n;
}


// �r�b�O�G���f�B�A���`���Ŋi�[���ꂽbyte(unsigned char)�^�������int�^�ɕϊ�
int ConvertBYTEtoINT(unsigned char *str, int elementOffset, int elementCnt) {
	int i, j;
	int n = 0;

	i = elementOffset;
	while (i-elementOffset < elementCnt) {
		int code = (int)str[i];

		j = 0;
		while (j < 2) {
			int tmp = (code << 4*j) & 0b11110000;
			tmp >>= 4;

			if (!(0x0 <= tmp && tmp <= 0xf)) {
				return -1;
			}

			n <<= 4;
			n |= tmp;
			j++;
		}

		i++;
	}

	return n;
}


// int�^���r�b�O�G���f�B�A���`����byte(unsigned char)�^������ɕϊ�
// �߂�l�͕ϊ�����byte��
int ConvertINTtoBYTE(int num, unsigned char *str) {
	int i;
	int tmp1, tmp2;

	i = 0;
	tmp1 = num;
	while (true) {
		num >>= 8;
		tmp2 = tmp1 - (num << 8);	// ����8�r�b�g���o��

		str[i] &= 0x00;
		str[i] |= tmp2;
		i++;

		if (num == 0)
			break;

		tmp1 = num;
	}

	// ������𔽓]
	for (int k=0; k < i/2; k++) {
		int t = str[k];
		str[k] = str[i-k-1];
		str[i-k-1] = t;
	}
	return i;
}


// int�^��36�i��������ɕϊ�
int ConvertINTtoSTR36(int num, char *buffer) {
	int cnt = 1;

	while (cnt >= 0) {
		int tmp = num%36;
		if (tmp < 10) {
			buffer[cnt] = (char)(tmp+'0');
		}
		else {
			buffer[cnt] = (char)(tmp-10+'A');
		}

		num /= 36;

		cnt--;
	}
	buffer[2] = '\0';

	return 1;
}


// int�^���������K�ɕϊ�
int ConvertINTtoInterval(int num, char *interval) {
	if (interval == NULL) return 0;

	int cnt = 0;
	for (int i=0; i<5; i++) {
		interval[i] = ' ';
	}

	switch (num%12) {
		case 1: interval[1] = '#'; cnt++;
		case 0: interval[0] = 'C'; break;
		case 3: interval[1] = '#'; cnt++;
		case 2: interval[0] = 'D'; break;
		case 4: interval[0] = 'E'; break;
		case 6: interval[1] = '#'; cnt++;
		case 5: interval[0] = 'F'; break;
		case 8: interval[1] = '#'; cnt++;
		case 7: interval[0] = 'G'; break;
		case 10: interval[1] = '#'; cnt++;
		case 9: interval[0] = 'A'; break;
		case 11: interval[0] = 'B'; break;
	}
	cnt++;

	if (num < 12) {
		interval[cnt++] = '-';
		interval[cnt++] = '1';
	}
	else {
		interval[cnt++] = '0'+ num/12-1;
	}

	interval[cnt++] = '\0';

	return 1;
}



typedef struct _LENGTH {
	int denom;
	int numer;
} LENGTH;


typedef struct _NOTETYPE {
	string instName;
	int velocity;
	int soundNum;
} NOTETYPE;


typedef struct _LANETYPE {
	int trackNum;
	string instName;	// �y��
	int interval;		// ����
} LANETYPE;


typedef struct _NOTESOUND {
	string instName;
	int interval;
	int velocity;
	LENGTH barLen;
}NOTESOUND;


typedef struct _EVENT {
	unsigned long totalTime;
	int timeInBar;
	int bar;
	int eventKind;
	union {					// �f�[�^���e
		struct {				// �m�[�g���
			int noteNum;
			int velocity;
			int length;
		} NOTE;

		struct {				// �e���|���
			float tempo;
		} TEMPO;

		LENGTH BEAT;			// ���q
	} CONTENT;
} EVENT;


int main(void) {
	FILE *fpMidi = NULL;
	FILE *fpBms	= NULL;

	char midiFileName[_MAX_PATH];

	int RESOLUTION = 0;
	int BAR_RESOLUTION = 1;		// �����l��1��
	int TRACKS = 1;
	int HOLDSTDLEN_ID = 0;
	int HOLDSTDLEN = 0;

	bool laneDivideFlag = false;
	bool noteDividFlag = false;

	int endBar = 0;

	int noteCnt = 0;
	int holdCnt = 0;

	vector<vector<EVENT>> writeBuffer(MAXNOTELAME);
	vector<NOTETYPE> noteTypeMem;
	vector<LANETYPE> laneTypeMem;
	vector<_NOTESOUND> noteSoundMem;
	vector<unsigned char> buffer(1024);
	vector<int>noteStartTime(MAXNOTELAME, -1);
	vector<EVENT> tempoMem;
	vector<EVENT> beatMem;
	bool tempoSetFlag = false;
	bool beatSetFlag = false;

	int cnt;
	vector<unsigned char> tmp(5);

	char str36[3];
	char intvl[5];



	// ���[�U�[����
	printf("MIDI FILE path >");
	fgets(midiFileName, _MAX_PATH, stdin);
	midiFileName[strlen(midiFileName)-1] = '\0';
	while (true) {
		printf("HOLD standard length\n");
		printf("0:none			1:sixteenth-note\n"
			"2:eighth-note		3:quarter note\n"
			"4:half note		5:whole note\n"
			"6:whole note*1.5	7:whole note*2.0\n"
			"8:whole note*2.5	9:whole note*3.0	>");
		scanf("%d", &HOLDSTDLEN_ID);
		if (0 > HOLDSTDLEN_ID || HOLDSTDLEN_ID > 7) {
			printf("[input error] please type 0-4 in the integer.\n");
			continue;
		}

		printf("Divide lane by interval?	1:yes  0:no	>");
		int tmp;
		scanf("%d", &tmp);
		if (tmp == 0) {
			laneDivideFlag = false;
		}
		else if (tmp == 1) {
			laneDivideFlag = true;
		}
		else {
			printf("[input error] please type 0-1 in the integer.\n");
			continue;
		}

		printf("Divide note sound by velocity?	1:yes  0:no	>");
		scanf("%d", &tmp);
		if (tmp == 0) {
			noteDividFlag = false;
		}
		else if (tmp == 1) {
			noteDividFlag = true;
		}
		else {
			printf("[input error] please type 0-1 in the integer.\n");
			continue;
		}

		break;
	}
	puts("");

	// �t�@�C���I�[�v��
	fpMidi = fopen(midiFileName, "rb");
	if (fpMidi == NULL) {
		printf("[file error] MIDI file could not open.\n");
		return 1;
	}
	fpBms = fopen("BMS.txt", "w");
	if (fpBms == NULL) {
		printf("[file error] BMS file could not open.\n");
		return 1;
	}


	// �f�t�H���g�l�ݒ�
	EVENT addEvt;
	addEvt.totalTime = 0;
	addEvt.timeInBar = 0;
	addEvt.bar = 1;
	addEvt.eventKind = 0x51;
	addEvt.CONTENT.TEMPO.tempo = 120;
	tempoMem.push_back(addEvt);
	addEvt.eventKind = 0x58;
	addEvt.CONTENT.BEAT.numer = 4;
	addEvt.CONTENT.BEAT.denom = 4;
	beatMem.push_back(addEvt);
	NOTETYPE addNote ={ "", 0, 0 };
	noteTypeMem.push_back(addNote);

	
	puts("------------------- LOG --------------------");

	int trackCnt = 0;
	while (++trackCnt <= TRACKS+1) {		// MIDI�t�@�C���̉�̓��[�v
		unsigned long totalTime = 0;
		string trackName;

		// �`�����N�̃t�H�[�}�b�g���擾
		char chunk[5];
		fread(&chunk, sizeof(char), 4, fpMidi);		// �`�����N�^�C�v�擾
		chunk[4] = '\0';
		fread(tmp.data(), sizeof(char), 4, fpMidi);			// �f�[�^���擾
		int chunkLength = ConvertBYTEtoINT(tmp.data(), 0, 4);


		// midi�t�@�C���̃t�H�[�}�b�g���m�F
		if (strncmp(chunk, "MThd", 4) == 0) {
			fread(tmp.data(), 1, 2, fpMidi);		// �t�H�[�}�b�g�擾
			tmp[2] = '\0';
			if (ConvertBYTEtoINT(tmp.data(), 0, 2) == 2) {
				printf("[SMF error] SMF FORMAT \"2\" is unsupported.");
				return 1;
			}

			fread(tmp.data(), 1, 2, fpMidi);		// �g���b�N���擾
			tmp[2] = '\0';

			TRACKS = ConvertBYTEtoINT(tmp.data(), 0, 2);
			printf("Track(s) : %d\n", TRACKS);
			fread(tmp.data(), sizeof(char), 2, fpMidi);	// ����\�擾
			RESOLUTION = ConvertBYTEtoINT(tmp.data(), 0, 2);
			if (RESOLUTION>>15) {
				printf("[SMF error] this TIME UNIT FORMAT is unsupported.");
				return 1;
			}

			printf("completed analysis > Header Track\n");

			continue;
		}


		// �g���b�N�`�����N�̉��
		for (int i=0; i<writeBuffer.size(); i++) writeBuffer[i].clear();

		int remainTime = 0;
		int appliedEventKind = 0;
		int barCnt = 1;
		int timeSum = 0;
		bool loopFlag = true;
		while (loopFlag) {		// 1�g���b�N��̓��[�v

			for (int i=0; i<buffer.size(); i++) buffer[i] = '\0';
			int bufferByte = 0;
			int eventKind = 0;

			for (int i=beatMem.size()-1; i>=0; i--) {	// 1���߂̒����̕���\���擾
				if (beatMem[i].totalTime <= totalTime) {
					BAR_RESOLUTION = RESOLUTION*4*beatMem[i].CONTENT.BEAT.numer/beatMem[i].CONTENT.BEAT.denom;
					switch (HOLDSTDLEN_ID) {
						case 0: HOLDSTDLEN = INT16_MAX; break;
						case 1: HOLDSTDLEN = BAR_RESOLUTION/16; break;
						case 2: HOLDSTDLEN = BAR_RESOLUTION/8; break;
						case 3: HOLDSTDLEN = BAR_RESOLUTION/4; break;
						case 4: HOLDSTDLEN = BAR_RESOLUTION/2; break;
						case 5: HOLDSTDLEN = BAR_RESOLUTION; break;
						case 6: HOLDSTDLEN = (int)(BAR_RESOLUTION*1.5); break;
						case 7: HOLDSTDLEN = (int)(BAR_RESOLUTION*2.0); break;
						case 8: HOLDSTDLEN = (int)(BAR_RESOLUTION*2.5); break;
						case 9: HOLDSTDLEN = (int)(BAR_RESOLUTION*3.0); break;
					}
					break;
				}
			}

			cnt = 0;
			int t = ReadVariableLengthNumber(fpMidi, &cnt);		// �f���^�^�C���擾
			bufferByte+=cnt;

			// t�����̏��߂̐擪����̃f���^�^�C���ɕϊ�
			if (timeSum == 0) {
				if (remainTime > BAR_RESOLUTION) {
					t -= BAR_RESOLUTION;
					totalTime += BAR_RESOLUTION;
					remainTime -= BAR_RESOLUTION;
				}
				else {
					t -= remainTime;
					totalTime += remainTime;
					remainTime = 0;
				}
			}

			if (timeSum+t >= BAR_RESOLUTION) {	// 1���߂𒴂�����
				remainTime = BAR_RESOLUTION - timeSum;
				fseek(fpMidi, -bufferByte, SEEK_CUR);
				barCnt++;
				timeSum = 0;

				continue;
			}
			timeSum += t;
			totalTime += t;

			fread(tmp.data(), 1, 1, fpMidi);	// �C�x���g�擾
			bufferByte++;
			tmp[1] = '\0';
			eventKind = ConvertBYTEtoINT(tmp.data(), 0, 1);

			// �����j���O�X�e�[�^�X���[���Ή�����
			if ((eventKind>>4) <= 0x7) {
				if ((appliedEventKind>>4) == 0x8 || (appliedEventKind>>4) == 0x9 || (appliedEventKind>>4) == 0xb) {		//�@midi�C�x���g
					eventKind = appliedEventKind;
					fseek(fpMidi, -1, SEEK_CUR);
					bufferByte--;
				}
			}
			else {
				appliedEventKind = eventKind;
			}


			// �C�x���g�̎�ނɉ����ď���
			if ((eventKind>>4) == 0x8 || (eventKind>>4) == 0x9 || (eventKind>>4) == 0xb) {		//�@midi�C�x���g

				fread(tmp.data(), 1, 1, fpMidi);	// �m�[�g�i���o�[�擾
				bufferByte++;
				tmp[1] = '\0';
				int noteNum = ConvertBYTEtoINT(tmp.data(), 0, 1);
				fread(tmp.data(), 1, 1, fpMidi);	// �x���V�e�B�擾
				bufferByte++;
				tmp[1] = '\0';
				int velocity = ConvertBYTEtoINT(tmp.data(), 0, 1);

				// ���[���ԍ��̎擾
				LANETYPE addLane ={ trackCnt, trackName, noteNum };
				int laneNum = -1;
				for (int i=0; i<laneTypeMem.size(); i++) {
					if (laneDivideFlag) {
						if (laneTypeMem[i].trackNum == addLane.trackNum &&
							laneTypeMem[i].instName == addLane.instName &&
							laneTypeMem[i].interval == addLane.interval) {
							laneNum = i;
							break;
						}
					}
					else {
						if (laneTypeMem[i].trackNum == addLane.trackNum &&
							laneTypeMem[i].instName == addLane.instName) {
							laneNum = i;
							break;
						}
					}
				}
				if (laneNum == -1) {
					if (laneTypeMem.size() >= MAXNOTELAME) {
						printf("[MIDI data error] NOTE LANE is needed more than %d kinds.   Bar Cnt:%d\n", MAXNOTELAME, barCnt);
						return 1;
					}
					laneTypeMem.push_back(addLane);
					laneNum = laneTypeMem.size()-1;
				}


				// �m�[�g��������
				bool addFlag = false;
				int noteIndex = -1;
				int sndIndex = -1;
				int length = 0;
				int d=0, n=0;
				int remain = 0;
				NOTESOUND addSnd;
				switch (eventKind>>4) {
					case 0x8:
						length = totalTime-noteStartTime[laneNum];
						if (length >= HOLDSTDLEN) {
							addFlag = true;
							holdCnt++;
						}

						// ���ԒP�ʂ����߂���Ƃ��������ɕϊ�
						remain = length;
						for (int i=beatMem.size()-1; i>=0; i--) {
							if (remain == 0) break;

							if (beatMem[i].totalTime < noteStartTime[laneNum] + remain) {
								int len = (noteStartTime[laneNum]+remain) - beatMem[i].totalTime;
								if (beatMem[i].totalTime < noteStartTime[laneNum]) {
									len = remain;
								}

								int barRsltn = RESOLUTION*4*beatMem[i].CONTENT.BEAT.numer/beatMem[i].CONTENT.BEAT.denom;
								int unit = CalcGCD(len, barRsltn);
								n += len/unit;
								d += barRsltn/unit;

								remain -= len;
							}
						}

						// �L�[���̒ǉ�
						addSnd.instName = trackName;
						addSnd.interval = noteNum;
						addSnd.barLen.numer = n;
						addSnd.barLen.denom = d;
						if (noteDividFlag) {
							addSnd.velocity = writeBuffer[laneNum].back().CONTENT.NOTE.velocity;
							for (int i=0; i<noteSoundMem.size(); i++) {
								if (noteSoundMem[i].instName == addSnd.instName &&
									noteSoundMem[i].interval == addSnd.interval &&
									noteSoundMem[i].velocity == addSnd.velocity &&
									noteSoundMem[i].barLen.denom == addSnd.barLen.denom &&
									noteSoundMem[i].barLen.numer == addSnd.barLen.numer) {
									sndIndex = i;
									break;
								}
							}
						}
						else {
							addSnd.velocity = 0x7F;
							for (int i=0; i<noteSoundMem.size(); i++) {
								if (noteSoundMem[i].instName == addSnd.instName &&
									noteSoundMem[i].interval == addSnd.interval &&
									noteSoundMem[i].barLen.denom == addSnd.barLen.denom &&
									noteSoundMem[i].barLen.numer == addSnd.barLen.numer) {
									sndIndex = i;
									break;
								}
							}
						}

						
						if (sndIndex == -1) {
							if (noteSoundMem.size() >= MAXNOTESOUND-1) {
								printf("[MIDI data error] NOTE SOUND is needed more than %d kinds.   Bar Cnt:%d\n", MAXNOTESOUND, barCnt);
								return 1;
							}
							noteSoundMem.push_back(addSnd);
							sndIndex = noteSoundMem.size()-1;
						}

						// �m�[�g�ԍ��̍X�V
						if (noteDividFlag) {
							addNote ={ trackName, 0x7F, sndIndex };
							for (int i=0; i<noteTypeMem.size(); i++) {
								if (noteTypeMem[i].instName == addNote.instName &&
									noteTypeMem[i].soundNum == addNote.soundNum) {
									noteIndex = i;
									break;
								}
							}
						}
						else {
							addNote ={ trackName, writeBuffer[laneNum].back().CONTENT.NOTE.velocity, sndIndex };
							for (int i=0; i<noteTypeMem.size(); i++) {
								if (noteTypeMem[i].instName == addNote.instName &&
									noteTypeMem[i].velocity == addNote.velocity &&
									noteTypeMem[i].soundNum == addNote.soundNum) {
									noteIndex = i;
									break;
								}
							}
						}
						if (noteIndex == -1) {
							if (noteTypeMem.size() >= MAXNOTETYPE-1) {
								printf("[MIDI data error] NOTE TYPE is needed more than %d kinds.   Bar Cnt:%d\n", MAXNOTETYPE, barCnt);
								return 1;
							}
							noteTypeMem.push_back(addNote);
							noteIndex = noteTypeMem.size()-1;
						}
						writeBuffer[laneNum].back().CONTENT.NOTE.length = length;
						writeBuffer[laneNum].back().CONTENT.NOTE.noteNum = noteIndex;
						break;
					case 0x9:
						noteStartTime[laneNum] = totalTime;
						addFlag = true;
						break;
				}


				if (addFlag) {	// �������݃o�b�t�@�֒ǉ�
					if (writeBuffer[laneNum].size() > 0 && writeBuffer[laneNum].back().totalTime == totalTime) {
						printf("[MIDI data error]  NOTE event occured simultaneously.  Bar Cnt:%d\n", barCnt);
						//return 1;
					}

					EVENT addEvt;
					addEvt.totalTime = totalTime;
					addEvt.timeInBar = timeSum;
					addEvt.bar = barCnt;
					addEvt.eventKind = eventKind;
					addEvt.CONTENT.NOTE.noteNum = 0;
					addEvt.CONTENT.NOTE.length = 0;
					addEvt.CONTENT.NOTE.velocity = velocity;
					writeBuffer[laneNum].push_back(addEvt);

					noteCnt++;
				}
			}
			else if (eventKind == 0xf0 || eventKind == 0xf7) {	// �G�N�X�N���[�V�u���b�Z�[�W

			}
			else if (eventKind == 0xff) {						// ���^�C�x���g
				fread(tmp.data(), 1, 1, fpMidi);	// �C�x���g�^�C�v�擾
				bufferByte++;
				tmp[1] = '\0';
				int eventType = ConvertBYTEtoINT(tmp.data(), 0, 1);
				cnt = 0;
				int eventLen = ReadVariableLengthNumber(fpMidi, &cnt);
				bufferByte++;
				fread(buffer.data(), 1, eventLen, fpMidi);
				bufferByte+=eventLen;

				int musec;
				switch (eventType) {
					case 0x03:	// �g���b�N��
						trackName.clear();
						for (int i=0; i<buffer[i] != '\0'; i++) {
							trackName += buffer[i];
						}
						trackName += '\0';
						break;
					case 0x2f:	// �g���b�N�I�[
						loopFlag = false;
						break;
					case 0x51:	// �e���|
						if (!tempoSetFlag) {
							// �܂��e���|��񂪓o�^����Ă��Ȃ��Ȃ�
							tempoMem.clear();
							tempoSetFlag = true;
						}

						addEvt.totalTime = totalTime;
						addEvt.timeInBar = timeSum;
						addEvt.bar = barCnt;
						addEvt.eventKind = 0x51;
						musec = ConvertBYTEtoINT(buffer.data(), 0, 3);
						addEvt.CONTENT.TEMPO.tempo = (float)60*1000000/musec;
						tempoMem.push_back(addEvt);
						break;
					case 0x58:	// ���q
						if (!beatSetFlag) {
							beatMem.clear();
							beatSetFlag = true;
						}

						addEvt.totalTime = totalTime;
						addEvt.timeInBar = timeSum;
						addEvt.eventKind = 0x58;
						addEvt.CONTENT.BEAT.numer = ConvertBYTEtoINT(buffer.data(), 0, 1);
						addEvt.CONTENT.BEAT.denom = (int)pow(2, ConvertBYTEtoINT(buffer.data(), 1, 1));
						addEvt.bar = barCnt;
						beatMem.push_back(addEvt);
						break;
					default:
						break;
				}

			}
			else {
				printf("[SMF error] this SMF data is no consistency.   Bar Cnt:%d\n", barCnt);
				return 1;
			}


			if (endBar < barCnt) {
				endBar = barCnt;
			}

		}	// 1�g���b�N��̓��[�v




		// BMS�t�@�C���֏�������
		vector<int> startNum(MAXNOTELAME, 0);
		int crtBar = 0;
		while (++crtBar <= barCnt) {	//�@1�g���b�N�������݃��[�v

			for (int i=0; i<MAXNOTELAME; i++) {		// �S�m�[�g���[���������݃��[�v
				if (writeBuffer[i].size() == 0) {
					continue;
				}

				int begin = startNum[i];	// �������ރf�[�^�͈̔͂��Z�o
				int end = -1;

				for (int j=begin; j<writeBuffer[i].size(); j++) {
					if (writeBuffer[i][j].bar == crtBar) end = j+1;
					else break;
				}
				if (end < 0) continue;

				int writeByte = 0;
				int prevTime = 0;
				int minUnit = BAR_RESOLUTION;

				for (int j=begin; j<end; j++) {		// �����̒����̍ŏ����񐔂��Z�o
					int deltaTime = writeBuffer[i][j].timeInBar - prevTime;
					if (deltaTime > 0) {
						minUnit = CalcGCD(minUnit, deltaTime);
					};
					prevTime = writeBuffer[i][j].timeInBar;
				}


				ConvertINTtoSTR36(i+36, str36);
				fprintf(fpBms, "#%03d%s:", crtBar, str36);
				for (int j=begin; j<end; j++) {		// bms�t�@�C���ɏ�������
					int offset = writeBuffer[i][j].timeInBar/minUnit;

					for (int k=writeByte; k<offset; k++) {
						fprintf(fpBms, "00");
						writeByte++;
					}
					switch (writeBuffer[i][j].eventKind>>4) {
						case 0x8:
							fprintf(fpBms, "ZZ");
							writeByte++;
							break;
						case 0x9:
							ConvertINTtoSTR36(writeBuffer[i][j].CONTENT.NOTE.noteNum, str36);
							fprintf(fpBms, "%s", str36);
							writeByte++;
							break;
					}
				}

				while (writeByte < (BAR_RESOLUTION/minUnit)) {	// �]���0�Ŗ��߂�
					fprintf(fpBms, "00");
					writeByte++;
				}
				fprintf(fpBms, "\n");

				startNum[i] = end;
			}
		}

		printf("completed analysis > Track no.%03d\n", trackCnt);

	} // MIDI�t�@�C���̉�̓��[�v

	printf("completed MIDI file analysis.\n");


	// ���O�o��
	puts("------------------ RESULT ------------------");
	printf("all NOTE cnt	%4d\n", noteCnt);
	printf("HOLD NOTE cnt	%4d\n\n", holdCnt);
	puts("*BEAT");
	fprintf(fpBms, "\n");
	fprintf(fpBms, "#DIFINESTART HEADER\n");
	fprintf(fpBms, "//BEAT\n");
	for (int i=0; i<beatMem.size(); i++) {
		printf("BAR:%03d-  beat:%d/%d\n", beatMem[i].bar, beatMem[i].CONTENT.BEAT.numer, beatMem[i].CONTENT.BEAT.denom);
	}
	float beatTmp;
	cnt = 0;
	for (int i=1; i<=endBar; i++) {
		if (cnt < beatMem.size() && i == beatMem[cnt].bar) {
			beatTmp = (float)beatMem[cnt].CONTENT.BEAT.numer/beatMem[cnt].CONTENT.BEAT.denom;
			cnt++;
		}
		fprintf(fpBms, "#%03d02:%f\n", i, beatTmp);
	}
	printf("\n*TEMPO\n");
	fprintf(fpBms, "\n");
	fprintf(fpBms, "//TEMPO\n");
	fprintf(fpBms, "#BPM %f\n", tempoMem[0].CONTENT.TEMPO.tempo);
	printf("BAR:%03d-  bpm:%f\n", tempoMem[0].bar, tempoMem[0].CONTENT.TEMPO.tempo);
	for (int i=1; i<tempoMem.size(); i++) {
		printf("BAR:%03d-  bpm:%f\n", tempoMem[i].bar, tempoMem[i].CONTENT.TEMPO.tempo);
		ConvertINTtoSTR36(i, str36);
		fprintf(fpBms, "#BPM%s %f\n", str36, tempoMem[i].CONTENT.TEMPO.tempo);
	}
	fprintf(fpBms, "\n");
	for (int i=1; i<tempoMem.size(); i++) {
		ConvertINTtoSTR36(i, str36);
		fprintf(fpBms, "#%03d08:%s\n", tempoMem[i].bar, str36);
	}
	fprintf(fpBms, "#DEFINEEND HEADER\n");
	fprintf(fpBms, "\n");
	fprintf(fpBms, "//NOTELANE\n");
	printf("\nNOTE LANE	%4d kind[s]\n", laneTypeMem.size());
	for (int i=0; i<laneTypeMem.size(); i++) {
		ConvertINTtoSTR36(i, str36);
		if (laneDivideFlag) {
			ConvertINTtoInterval(laneTypeMem[i].interval, intvl);
			fprintf(fpBms, "#LANENAME%s:TRACK %d %s  INTERVAL:%s\n", str36, laneTypeMem[i].trackNum, laneTypeMem[i].instName.c_str(), intvl);
		}
		else {
			fprintf(fpBms, "#LANENAME%s:TRACK %d %s\n", str36, laneTypeMem[i].trackNum, laneTypeMem[i].instName.c_str());
		}
		fprintf(fpBms, "#LANEPOS%s:\n", str36);
	}
	fprintf(fpBms, "\n");
	fprintf(fpBms, "//WAV\n");
	printf("NOTE SOUND	%4d kind[s]\n", noteSoundMem.size());
	for (int i=0; i<noteSoundMem.size(); i++) {
		ConvertINTtoSTR36(i, str36);
		ConvertINTtoInterval(noteSoundMem[i].interval, intvl);
		if (noteDividFlag) {
			fprintf(fpBms, "#WAV%s %s interval:%s velocity:%d len:%d/%d bar\n", str36, noteSoundMem[i].instName.c_str(), intvl, noteSoundMem[i].velocity, noteSoundMem[i].barLen.numer, noteSoundMem[i].barLen.denom);
		}
		else {
			fprintf(fpBms, "#WAV%s %s interval:%s len:%d/%d bar\n", str36, noteSoundMem[i].instName.c_str(), intvl, noteSoundMem[i].barLen.numer, noteSoundMem[i].barLen.denom);
		}
	}
	fprintf(fpBms, "\n");
	fprintf(fpBms, "//BACK MUSIC\n");
	ConvertINTtoSTR36(MAXNOTESOUND-1, str36);
	fprintf(fpBms, "#WAV%s back music\n", str36);
	fprintf(fpBms, "#00101:%s\n", str36);
	fprintf(fpBms, "\n");
	fprintf(fpBms, "//NOTESOUND & NOTEVOL\n");
	printf("NOTE TYPE	%4d kind[s]\n", noteTypeMem.size()-1);
	char str36a[3];
	for (int i=1; i<noteTypeMem.size(); i++) {
		ConvertINTtoSTR36(i, str36);
		ConvertINTtoSTR36(noteTypeMem[i].soundNum, str36a);
		fprintf(fpBms, "#NOTESOUND%s %s\n", str36, str36a);
		fprintf(fpBms, "#NOTEVOL%s %f\n", str36, sqrtf((float)noteTypeMem[i].velocity/0x7f));
	}


	fclose(fpMidi);
	fclose(fpBms);


	return 0;
}