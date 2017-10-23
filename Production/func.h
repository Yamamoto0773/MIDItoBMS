#pragma once

#include <vector>
#include <string>
#include <stdio.h>

#define MAXNOTELAME (36*36-36)
#define MAXNOTETYPE (36*36)
#define MAXNOTESOUND (36*36)

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
	string instName;	// 楽器
	int interval;		// 音程
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
	union {					// データ内容
		struct {				// ノート情報
			int noteNum;
			int velocity;
			int length;
		} NOTE;

		struct {				// テンポ情報
			float tempo;
		} TEMPO;

		LENGTH BEAT;			// 拍子
	} CONTENT;
} EVENT;



// 最大公約数を求める関数
int CalcGCD(int a, int b);


// 可変長数値をファイルから取得
// 戻り値は固定長数値
int ReadVariableLengthNumber(FILE *fp, int *byteCnt=NULL);


// ビッグエンディアン形式で格納されたbyte(unsigned char)型文字列をint型に変換
int ConvertBYTEtoINT(unsigned char *str, int elementOffset, int elementCnt);

// int型をビッグエンディアン形式でbyte(unsigned char)型文字列に変換
// 戻り値は変換したbyte数
int ConvertINTtoBYTE(int num, unsigned char *str);


// int型を36進数文字列に変換
int ConvertINTtoSTR36(int num, char *buffer);


// int型整数を音階に変換
int GetInterval(int num, char *interval);


// 時間単位を小節を基準とした長さに変換
LENGTH GetBarLength(long startTime, long count, long resolution, const vector<EVENT> &beat);


// ノート番号の検索
int FindNoteType(const NOTETYPE &obj, const vector<NOTETYPE> &aggregate, bool divideFlag);


// キー音の検索
int FindNoteSound(const NOTESOUND &obj, const vector<NOTESOUND> &aggregate, bool divideFlag);


// ノートレーンの検索
int FindLaneType(const LANETYPE &obj, const vector<LANETYPE> &aggregate, bool divideFlag)