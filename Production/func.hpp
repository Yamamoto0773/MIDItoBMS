#pragma once

#include <vector>
#include <stdio.h>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>

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
	std::string instName;	
	int interval;		
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
	union {					
		struct {			
			int noteNum;
			int velocity;
			int length;
		} NOTE;

		struct {				
			float tempo;
		} TEMPO;

		LENGTH BEAT;		
	} CONTENT;
} EVENT;



// 最大公約数を求める関数
int CalcGCD(int a, int b);


// 可変長数値をファイルから取得
// 戻り値は固定長数値
int ReadVariableLengthNumber(std::ifstream &ifs, int *byteCnt=NULL);



// ビッグエンディアン形式で格納されたchar型文字列をint型に変換
int ConvertCHARtoINT(const char *str, int elementOffset, int elementCnt);

// int型をビッグエンディアン形式でbyte(unsigned char)型文字列に変換
// 戻り値は変換したbyte数
int ConvertINTtoBYTE(int num, unsigned char *str);


// int型を36進数文字列に変換
int ConvertINTtoSTR36(int num, char *buffer);

// int型整数を音階に変換
int GetInterval(int num, char *interval);

// 時間単位を小節を基準とした長さに変換
LENGTH GetBarLength(long startTime, long count, long resolution, const std::vector<EVENT> &beat);

// ノート番号の検索
int FindNoteType(const NOTETYPE &obj, const std::vector<NOTETYPE> &aggregate, bool divideFlag);


// キー音の検索
int FindNoteSound(const NOTESOUND &obj, const std::vector<NOTESOUND> &aggregate, bool divideFlag);

// ノートレーンの検索
int FindLaneType(const LANETYPE &obj, const std::vector<LANETYPE> &aggregate, bool divideFlag);