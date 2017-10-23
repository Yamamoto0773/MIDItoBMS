#include "func.h"

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
int ReadVariableLengthNumber(FILE *fp, int *byteCnt) {
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
int GetInterval(int num, char *interval) {
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


// ���ԒP�ʂ����߂���Ƃ��������ɕϊ�
LENGTH GetBarLength(long startTime, long count, long resolution, const std::vector<EVENT> &beat) {
	long remain = count;
	LENGTH ans;
	ans.denom = 0, ans.numer = 0;

	for (int i=beat.size()-1; i>=0; i--) {
		if (remain == 0) break;

		if (beat[i].totalTime < startTime + remain) {
			int len = (startTime+remain) - beat[i].totalTime;
			if (beat[i].totalTime < startTime) {
				len = remain;
			}

			int barRsltn = resolution*4*beat[i].CONTENT.BEAT.numer/beat[i].CONTENT.BEAT.denom;
			int unit = CalcGCD(len, barRsltn);
			ans.numer += len/unit;
			ans.denom += barRsltn/unit;

			remain -= len;
		}
	}

	return ans;
}


// �m�[�g�ԍ��̌���
int FindNoteType(const NOTETYPE &obj, const std::vector<NOTETYPE> &aggregate, bool divideFlag) {
	int index = -1;

	if (divideFlag) {
		for (int i=0; i<aggregate.size(); i++) {
			if (aggregate[i].instName == obj.instName &&
				aggregate[i].soundNum == obj.soundNum) {
				index = i;
				break;
			}
		}
	}
	else {
		for (int i=0; i<aggregate.size(); i++) {
			if (aggregate[i].instName == obj.instName &&
				aggregate[i].velocity == obj.velocity &&
				aggregate[i].soundNum == obj.soundNum) {
				index = i;
				break;
			}
		}
	}

	return index;
}


// �L�[���̌���
int FindNoteSound(const NOTESOUND &obj, const std::vector<NOTESOUND> &aggregate, bool divideFlag) {
	int index = -1;

	if (divideFlag) {
		for (int i=0; i<aggregate.size(); i++) {
			if (aggregate[i].instName == obj.instName &&
				aggregate[i].interval == obj.interval &&
				aggregate[i].velocity == obj.velocity &&
				aggregate[i].barLen.denom == obj.barLen.denom &&
				aggregate[i].barLen.numer == obj.barLen.numer) {
				index = i;
				break;
			}
		}
	}
	else {
		for (int i=0; i<aggregate.size(); i++) {
			if (aggregate[i].instName == obj.instName &&
				aggregate[i].interval == obj.interval &&
				aggregate[i].barLen.denom == obj.barLen.denom &&
				aggregate[i].barLen.numer == obj.barLen.numer) {
				index = i;
				break;
			}
		}
	}

	return index;
}


// �m�[�g���[���̌���
int FindLaneType(const LANETYPE &obj, const std::vector<LANETYPE> &aggregate, bool divideFlag) {
	int index = -1;

	if (divideFlag) {
		for (int i=0; i<aggregate.size(); i++) {
			if (aggregate[i].trackNum == obj.trackNum &&
				aggregate[i].instName == obj.instName &&
				aggregate[i].interval == obj.interval) {
				index = i;
				break;
			}

		}
	}
	else {
		for (int i=0; i<aggregate.size(); i++) {

			if (aggregate[i].trackNum == obj.trackNum &&
				aggregate[i].instName == obj.instName) {
				index = i;
				break;
			}

		}
	}

	return index;
}