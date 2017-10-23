#include "func.h"

using namespace std;

int main(void) {
	FILE *fpMidi = NULL;
	FILE *fpBms	= NULL;

	char midiFileName[_MAX_PATH];

	int RESOLUTION = 0;
	int BAR_RESOLUTION = 1;		// 初期値は1で
	int TRACKS = 1;
	int HOLDSTDLEN_ID = 0;
	int HOLDSTDLEN = 0;

	bool laneDivideFlag = false;
	bool noteDivideFlag = false;

	int endBar = 0;

	int noteCnt = 0;
	int holdCnt = 0;

	vector<vector<EVENT>> writeBuffer(MAXNOTELAME);
	vector<NOTETYPE> noteTypeMem;
	vector<LANETYPE> laneTypeMem;
	vector<NOTESOUND> noteSoundMem;
	vector<unsigned char> buffer(1024);
	vector<int> noteStartTime(MAXNOTELAME, -1);
	vector<EVENT> tempoMem;
	vector<EVENT> beatMem;
	bool tempoSetFlag = false;
	bool beatSetFlag = false;

	int cnt;
	vector<unsigned char> tmp(5);

	char str36[3];
	char intvl[5];



	// ユーザー入力
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
			noteDivideFlag = false;
		}
		else if (tmp == 1) {
			noteDivideFlag = true;
		}
		else {
			printf("[input error] please type 0-1 in the integer.\n");
			continue;
		}

		break;
	}
	puts("");

	// ファイルオープン
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


	// デフォルト値設定
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
	while (++trackCnt <= TRACKS+1) {		// MIDIファイルの解析ループ
		unsigned long totalTime = 0;
		string trackName;

		// チャンクのフォーマットを取得
		char chunk[5];
		fread(&chunk, sizeof(char), 4, fpMidi);		// チャンクタイプ取得
		chunk[4] = '\0';
		fread(tmp.data(), sizeof(char), 4, fpMidi);			// データ長取得
		int chunkLength = ConvertBYTEtoINT(tmp.data(), 0, 4);


		// midiファイルのフォーマットを確認
		if (strncmp(chunk, "MThd", 4) == 0) {
			fread(tmp.data(), 1, 2, fpMidi);		// フォーマット取得
			tmp[2] = '\0';
			if (ConvertBYTEtoINT(tmp.data(), 0, 2) == 2) {
				printf("[SMF error] SMF FORMAT \"2\" is unsupported.");
				return 1;
			}

			fread(tmp.data(), 1, 2, fpMidi);		// トラック数取得
			tmp[2] = '\0';

			TRACKS = ConvertBYTEtoINT(tmp.data(), 0, 2);
			printf("Track(s) : %d\n", TRACKS);
			fread(tmp.data(), sizeof(char), 2, fpMidi);	// 分解能取得
			RESOLUTION = ConvertBYTEtoINT(tmp.data(), 0, 2);
			if (RESOLUTION>>15) {
				printf("[SMF error] this TIME UNIT FORMAT is unsupported.");
				return 1;
			}

			printf("completed analysis > Header Track\n");

			continue;
		}




		// トラックチャンクの解析
		for (int i=0; i<writeBuffer.size(); i++) writeBuffer[i].clear();

		int remainTime = 0;
		int appliedEventKind = 0;
		int barCnt = 1;
		int timeInBar = 0;
		bool loopFlag = true;
		while (loopFlag) {		// 1トラック解析ループ

			for (int i=0; i<buffer.size(); i++) buffer[i] = '\0';
			int bufferByte = 0;
			int eventKind = 0;


			// デルタタイムから小節内の時間、曲の始めからの時間、小節数を求める
			cnt = 0;
			int t = ReadVariableLengthNumber(fpMidi, &cnt);		// デルタタイム取得
			bufferByte+=cnt;

			while (true) {
				for (int i=beatMem.size()-1; i>=0; i--) {	// 1小節の長さの分解能を取得
					if (beatMem[i].totalTime <= totalTime) {
						BAR_RESOLUTION = RESOLUTION*4*beatMem[i].CONTENT.BEAT.numer/beatMem[i].CONTENT.BEAT.denom;
						break;
					}
				}

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


				if (timeInBar+t >= BAR_RESOLUTION) {	// 1小節を超えたら
					remainTime = BAR_RESOLUTION - timeInBar;
					t -= remainTime;
					totalTime += remainTime;

					barCnt++;
					timeInBar = 0;
				}
				else {
					break;
				}

			}
			timeInBar += t;
			totalTime += t;


			fread(tmp.data(), 1, 1, fpMidi);	// イベント取得
			bufferByte++;
			tmp[1] = '\0';
			eventKind = ConvertBYTEtoINT(tmp.data(), 0, 1);

			// ランニングステータスルール対応処理
			if ((eventKind>>4) <= 0x7) {
				if ((appliedEventKind>>4) == 0x8 || (appliedEventKind>>4) == 0x9 || (appliedEventKind>>4) == 0xb) {		//　midiイベント
					eventKind = appliedEventKind;
					fseek(fpMidi, -1, SEEK_CUR);
					bufferByte--;
				}
			}
			else {
				appliedEventKind = eventKind;
			}


			// イベントの種類に応じて処理
			if ((eventKind>>4) == 0x8 || (eventKind>>4) == 0x9 || (eventKind>>4) == 0xb) {		//　midiイベント

				fread(tmp.data(), 1, 1, fpMidi);	// ノートナンバー取得
				bufferByte++;
				tmp[1] = '\0';
				int noteNum = ConvertBYTEtoINT(tmp.data(), 0, 1);
				fread(tmp.data(), 1, 1, fpMidi);	// ベロシティ取得
				bufferByte++;
				tmp[1] = '\0';
				int velocity = ConvertBYTEtoINT(tmp.data(), 0, 1);


				// レーン番号の取得
				LANETYPE addLane ={ trackCnt, trackName, noteNum };
				int laneNum = FindLaneType(addLane, laneTypeMem, laneDivideFlag);
		
				if (laneNum == -1) {
					if (laneTypeMem.size() >= MAXNOTELAME) {
						printf("[MIDI data error] NOTE LANE is needed more than %d kinds.   Bar Cnt:%d\n", MAXNOTELAME, barCnt);
						return 1;
					}
					laneTypeMem.push_back(addLane);
					laneNum = laneTypeMem.size()-1;
				}


				// ノート情報を処理
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

						// キー音の追加
						addSnd.instName = trackName;
						addSnd.interval = noteNum;
						addSnd.barLen	= GetBarLength(noteStartTime[laneNum], length, RESOLUTION, beatMem);
						if (noteDivideFlag) {
							addSnd.velocity = writeBuffer[laneNum].back().CONTENT.NOTE.velocity;
						}
						else {
							addSnd.velocity = 0x7F;
						}

						sndIndex = FindNoteSound(addSnd, noteSoundMem, noteDivideFlag);

						if (sndIndex == -1) {
							if (noteSoundMem.size() >= MAXNOTESOUND-1) {
								printf("[MIDI data error] NOTE SOUND is needed more than %d kinds.   Bar Cnt:%d\n", MAXNOTESOUND, barCnt);
								return 1;
							}
							noteSoundMem.push_back(addSnd);
							sndIndex = noteSoundMem.size()-1;
						}


						// ノート番号の更新
						if (noteDivideFlag) {
							addNote ={ trackName, 0x7F, sndIndex };
						}
						else {
							addNote ={ trackName, writeBuffer[laneNum].back().CONTENT.NOTE.velocity, sndIndex };
						}

						noteIndex = FindNoteType(addNote, noteTypeMem, noteDivideFlag);

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


				if (addFlag) {	// 書き込みバッファへ追加
					if (writeBuffer[laneNum].size() > 0 && writeBuffer[laneNum].back().totalTime == totalTime) {
						printf("[MIDI data error]  NOTE event occured simultaneously.  Bar Cnt:%d\n", barCnt);
						//return 1;
					}

					EVENT addEvt;
					addEvt.totalTime = totalTime;
					addEvt.timeInBar = timeInBar;
					addEvt.bar = barCnt;
					addEvt.eventKind = eventKind;
					addEvt.CONTENT.NOTE.noteNum = 0;
					addEvt.CONTENT.NOTE.length = 0;
					addEvt.CONTENT.NOTE.velocity = velocity;
					writeBuffer[laneNum].push_back(addEvt);

					noteCnt++;
				}
			}
			else if (eventKind == 0xf0 || eventKind == 0xf7) {	// エクスクルーシブメッセージ

			}
			else if (eventKind == 0xff) {						// メタイベント
				fread(tmp.data(), 1, 1, fpMidi);	// イベントタイプ取得
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
					case 0x03:	// トラック名
						trackName.clear();
						for (int i=0; i<buffer[i] != '\0'; i++) {
							trackName += buffer[i];
						}
						trackName += '\0';
						break;
					case 0x2f:	// トラック終端
						loopFlag = false;
						break;
					case 0x51:	// テンポ
						if (!tempoSetFlag) {
							// まだテンポ情報が登録されていないなら
							tempoMem.clear();
							tempoSetFlag = true;
						}

						addEvt.totalTime = totalTime;
						addEvt.timeInBar = timeInBar;
						addEvt.bar = barCnt;
						addEvt.eventKind = 0x51;
						musec = ConvertBYTEtoINT(buffer.data(), 0, 3);
						addEvt.CONTENT.TEMPO.tempo = (float)60*1000000/musec;
						tempoMem.push_back(addEvt);
						break;
					case 0x58:	// 拍子
						if (!beatSetFlag) {
							beatMem.clear();
							beatSetFlag = true;
						}

						addEvt.totalTime = totalTime;
						addEvt.timeInBar = timeInBar;
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

		}	// 1トラック解析ループ




		// BMSファイルへ書き込み
		vector<int> startNum(MAXNOTELAME, 0);
		int crtBar = 0;
		while (++crtBar <= barCnt) {	//　1トラック書き込みループ

			for (int i=0; i<MAXNOTELAME; i++) {		// 全ノートレーン書き込みループ
				if (writeBuffer[i].size() == 0) {
					continue;
				}

				int begin = startNum[i];	// 書き込むデータの範囲を算出
				int end = -1;

				for (int j=begin; j<writeBuffer[i].size(); j++) {
					if (writeBuffer[i][j].bar == crtBar) end = j+1;
					else break;
				}
				if (end < 0) continue;

				int writeByte = 0;
				int prevTime = 0;
				int minUnit = BAR_RESOLUTION;

				for (int j=begin; j<end; j++) {		// 音符の長さの最小公約数を算出
					int deltaTime = writeBuffer[i][j].timeInBar - prevTime;
					if (deltaTime > 0) {
						minUnit = CalcGCD(minUnit, deltaTime);
					};
					prevTime = writeBuffer[i][j].timeInBar;
				}


				ConvertINTtoSTR36(i+36, str36);
				fprintf(fpBms, "#%03d%s:", crtBar, str36);
				for (int j=begin; j<end; j++) {		// bmsファイルに書き込み
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

				while (writeByte < (BAR_RESOLUTION/minUnit)) {	// 余りを0で埋める
					fprintf(fpBms, "00");
					writeByte++;
				}
				fprintf(fpBms, "\n");

				startNum[i] = end;
			}
		}

		printf("completed analysis > Track no.%03d\n", trackCnt);

	} // MIDIファイルの解析ループ

	printf("completed MIDI file analysis.\n");


	// ログ出力
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
			GetInterval(laneTypeMem[i].interval, intvl);
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
		GetInterval(noteSoundMem[i].interval, intvl);
		if (noteDivideFlag) {
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