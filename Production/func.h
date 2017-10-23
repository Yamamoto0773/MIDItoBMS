#pragma once

#include <vector>
#include <stdio.h>

#define MAXNOTELAME (36*36-36)
#define MAXNOTETYPE (36*36)
#define MAXNOTESOUND (36*36)

typedef struct _LENGTH {
	int denom;
	int numer;
} LENGTH;


typedef struct _NOTETYPE {
	std::string instName;
	int velocity;
	int soundNum;
} NOTETYPE;


typedef struct _LANETYPE {
	int trackNum;
	std::string instName;	// �y��
	int interval;		// ����
} LANETYPE;


typedef struct _NOTESOUND {
	std::string instName;
	int interval;
	int velocity;
	LENGTH barLen;
}NOTESOUND;


typedef struct _EVENT {
	unsigned long totalTime;
	int timeInBar;
	int bar;
	int eventKind;
	union {					// �f�[�^��e
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



// �ő���񐔂�߂�֐�
int CalcGCD(int a, int b);


// �ϒ����l��t�@�C������擾
// �߂�l�͌Œ蒷���l
int ReadVariableLengthNumber(FILE *fp, int *byteCnt=NULL);


// �r�b�O�G���f�B�A���`���Ŋi�[���ꂽbyte(unsigned char)�^�������int�^�ɕϊ�
int ConvertBYTEtoINT(unsigned char *str, int elementOffset, int elementCnt);

// int�^��r�b�O�G���f�B�A���`����byte(unsigned char)�^������ɕϊ�
// �߂�l�͕ϊ�����byte��
int ConvertINTtoBYTE(int num, unsigned char *str);


// int�^��36�i��������ɕϊ�
int ConvertINTtoSTR36(int num, char *buffer);


// int�^������K�ɕϊ�
int GetInterval(int num, char *interval);


// ���ԒP�ʂ�߂��Ƃ��������ɕϊ�
LENGTH GetBarLength(long startTime, long count, long resolution, const std::vector<EVENT> &beat);


// �m�[�g�ԍ��̌���
int FindNoteType(const NOTETYPE &obj, const std::vector<NOTETYPE> &aggregate, bool divideFlag);


// �L�[���̌���
int FindNoteSound(const NOTESOUND &obj, const std::vector<NOTESOUND> &aggregate, bool divideFlag);


// �m�[�g���[���̌���
int FindLaneType(const LANETYPE &obj, const std::vector<LANETYPE> &aggregate, bool divideFlag)