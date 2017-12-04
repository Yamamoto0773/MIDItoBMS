#include <stdio.h>
#include <string.h>
#include <stdlib.h>



// 最大公約数を求める関数
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


// 可変長数値をファイルから取得
// 戻り値は固定長数値
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
			// ビット7が0なら取得終了
			break;
		}
	}

	return n;
}


int ConvertBYTEtoINT(unsigned char *str, int elementCnt) {
	int i,j;
	int n = 0;

	i = 0;
	while (i < elementCnt) {
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



int main(void) {
	FILE *fpBinary = NULL;
	FILE *fpBms	= NULL;
	long totalTime = 0;
	int resolution;
	unsigned char tmp[5];
	char chunk[4+1];
	int length;


	fpBinary = fopen("C:/Users/Nanami/Documents/REAPER Media/きらきら星.mid", "rb");
	if (fpBinary == NULL) {
		return 0;
	}
	fpBms = fopen("bmsTest.txt","a");
	if (fpBms == NULL) {
		return 0;
	}
	

	bool lpflag = true;
	while (lpflag) {
		fread(chunk, sizeof(char), 4, fpBinary);		// チャンクタイプ取得
		chunk[4] = '\0';
		fread(tmp, sizeof(char), 4, fpBinary);			// データ長取得
		length = ConvertBYTEtoINT(tmp, 4);

		// ヘッダチャンクはスキップ
		if (strncmp(chunk, "MThd", 4) == 0) {
			// 分解能の取得
			fseek(fpBinary, 4, SEEK_CUR);				// 時間単位が記録されているところまで移動
			fread(&tmp, sizeof(char), 2, fpBinary);
			resolution = ConvertBYTEtoINT(tmp, 2);

			if (resolution >> 13) {
				// タイミングのフォーマットが対応していない場合は終了
				return 1;
			}

			continue;
		}



		// トラックチャンクの解析
		int remainTime = 0;
		bool flag;
		while (length > 0) {

			//// 1小節内に存在する全ての音符を、整数倍で表現できる音符を求める ////
			int byteCnt = 0;
			int concurrentEvt = 0;
			int needLaneCnt = 1;	// 必要なノートレーンの本数　(同時に来るノート数の最大値)
			int timeSum = 0;
			int minUnit = resolution;
			while (true) {

				// デルタタイムの取得
				int cnt = 0;
				int t = ReadVariableLengthNumber(fpBinary, &cnt);
				byteCnt += cnt;
				if (timeSum == 0) {
					// 小節内に1回目に登場するノートはその小節からのデルタタイムに変更
					t -= remainTime;
				}
				if (timeSum+t >= resolution) {
					// 1小節を超えたら終了
					remainTime = resolution - timeSum;
					byteCnt-=cnt;
					fseek(fpBinary, -cnt, SEEK_CUR);
					break;
				}

				timeSum += t;


				///// イベント部のスキップ処理 /////

				// スキップするデータのバイト数を求める
				fread(&tmp, sizeof(char), 1, fpBinary);	// ステータスバイトの取得
				byteCnt++;
				int status = ConvertBYTEtoINT(tmp, 1);
				unsigned char type = 0;

				int skipLen = 0;
				if (status == 0xf0 || status == 0xf7) {	// エクスクルーシブメッセージ
					// データ長取得
					int cnt = 0;
					skipLen = ReadVariableLengthNumber(fpBinary, &cnt);
					byteCnt += cnt;
				}
				else if (status == 0xff) {	// メタイベント
					fread(&type, 1, 1, fpBinary);	//イベントタイプ取得
					byteCnt++;

					// データ長取得
					int cnt = 0;
					skipLen = ReadVariableLengthNumber(fpBinary, &cnt);
					byteCnt += cnt;
				}
				else if ((0x80 <= status && status <= 0x8f) ||		// ノートオフ
					(0x90 <= status && status <= 0x9f) ||		// ノートオン
					(0xb0 <= status && status <= 0xbf) ||		// コントロールチェンジ
					status <= 0x7f) {							// ランニングステータス適応時
					skipLen = 2;

					if (status <= 0x7f)
						skipLen--;
					else if (!(0x90 <= status && status <= 0x9f))
						flag = false;
					else
						flag = true;

					if (flag) {		// ノートオン もしくはそのランニングステータス適応時
						if (t > 0) {
							minUnit = CalcGCD(minUnit, t);	// 要するに、音符の長さの最小公倍数を求める

							if (concurrentEvt > needLaneCnt)
								needLaneCnt = concurrentEvt;

							concurrentEvt = 0;
						}
						else {
							// 同時に発生するイベント
							concurrentEvt++;
						}
					}
				}
				else {
					return 1;
				}
				

				// スキップ
				fseek(fpBinary, skipLen, SEEK_CUR);
				byteCnt+=skipLen;

				if (type == 0x2F) {		// トラックチャンクの終わり
					break;
				}
			}

			length -= byteCnt;

			//// デルタタイムを、bms形式に変換 ////
			fseek(fpBinary, -byteCnt, SEEK_CUR);	// 小節の始めまで戻る

			int writeByte = 0;
			int bar = totalTime/resolution;
			
			for (int i=0; i<needLaneCnt; i++) {
				fprintf(fpBms, "#%03d%02d:", bar, i);
			}


			while (byteCnt > 0) {
				// 再度デルタタイムの取得
				int cnt = 0;
				int t = ReadVariableLengthNumber(fpBinary, &cnt);
				byteCnt -= cnt;
				totalTime += t;


				///// イベント部のスキップ処理 /////

				// スキップするデータのバイト数を求める
				fread(&tmp, sizeof(char), 1, fpBinary);	// ステータスバイトの取得
				byteCnt--;
				int status = ConvertBYTEtoINT(tmp, 1);
				unsigned char type = 0;

				int skipLen = 0;
				if (status == 0xf0 || status == 0xf7) {	// エクスクルーシブメッセージ
														// データ長取得
					int cnt = 0;
					skipLen = ReadVariableLengthNumber(fpBinary, &cnt);
					byteCnt -= cnt;
				}
				else if (status == 0xff) {	// メタイベント
					
					fread(&type, 1, 1, fpBinary);	//イベントタイプ取得
					byteCnt--;


					// データ長取得
					int cnt = 0;
					skipLen = ReadVariableLengthNumber(fpBinary, &cnt);
					byteCnt -= cnt;
				}
				else if ((0x80 <= status && status <= 0x8f) ||		// ノートオフ
					(0x90 <= status && status <= 0x9f) ||		// ノートオン
					(0xb0 <= status && status <= 0xbf) ||		// コントロールチェンジ
					status <= 0x7f) {							// ランニングステータス適応時
					skipLen = 2;

					if (status <= 0x7f)
						skipLen--;
					else if (!(0x90 <= status && status <= 0x9f))
						flag = false;
					else
						flag = true;

					if (flag) {		// ノートオン もしくはそのランニングステータス適応時
						// bmsファイルに書き込み
						int offset = t/minUnit;

						// ノートを配置するレーンを求める


						while (offset > 0) {
							fprintf(fpBms, "00");
							writeByte++;
							offset--;
						}
						fprintf(fpBms, "01");
						writeByte++;
					}

				}
				else {
					return 1;
				}

				// スキップ
				fseek(fpBinary, skipLen, SEEK_CUR);
				byteCnt-=skipLen;

				if (type == 0x2F) {		// トラックチャンクの終わり
					lpflag = false;
					break;
				}
			}

			// 小節の残りの領域を00で埋める
			while (writeByte < resolution/minUnit) {
				fprintf(fpBms, "00");
				writeByte++;
			}
			
			fprintf(fpBms, "\n");
		}

	}


	fclose(fpBinary);
	fclose(fpBms);


	return 0;
}