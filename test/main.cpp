#include <stdio.h>
#include <string.h>
#include <stdlib.h>



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


	fpBinary = fopen("C:/Users/Nanami/Documents/REAPER Media/���炫�琯.mid", "rb");
	if (fpBinary == NULL) {
		return 0;
	}
	fpBms = fopen("bmsTest.txt","a");
	if (fpBms == NULL) {
		return 0;
	}
	

	bool lpflag = true;
	while (lpflag) {
		fread(chunk, sizeof(char), 4, fpBinary);		// �`�����N�^�C�v�擾
		chunk[4] = '\0';
		fread(tmp, sizeof(char), 4, fpBinary);			// �f�[�^���擾
		length = ConvertBYTEtoINT(tmp, 4);

		// �w�b�_�`�����N�̓X�L�b�v
		if (strncmp(chunk, "MThd", 4) == 0) {
			// ����\�̎擾
			fseek(fpBinary, 4, SEEK_CUR);				// ���ԒP�ʂ��L�^����Ă���Ƃ���܂ňړ�
			fread(&tmp, sizeof(char), 2, fpBinary);
			resolution = ConvertBYTEtoINT(tmp, 2);

			if (resolution >> 13) {
				// �^�C�~���O�̃t�H�[�}�b�g���Ή����Ă��Ȃ��ꍇ�͏I��
				return 1;
			}

			continue;
		}



		// �g���b�N�`�����N�̉��
		int remainTime = 0;
		bool flag;
		while (length > 0) {

			//// 1���ߓ��ɑ��݂���S�Ẳ������A�����{�ŕ\���ł��鉹�������߂� ////
			int byteCnt = 0;
			int concurrentEvt = 0;
			int needLaneCnt = 1;	// �K�v�ȃm�[�g���[���̖{���@(�����ɗ���m�[�g���̍ő�l)
			int timeSum = 0;
			int minUnit = resolution;
			while (true) {

				// �f���^�^�C���̎擾
				int cnt = 0;
				int t = ReadVariableLengthNumber(fpBinary, &cnt);
				byteCnt += cnt;
				if (timeSum == 0) {
					// ���ߓ���1��ڂɓo�ꂷ��m�[�g�͂��̏��߂���̃f���^�^�C���ɕύX
					t -= remainTime;
				}
				if (timeSum+t >= resolution) {
					// 1���߂𒴂�����I��
					remainTime = resolution - timeSum;
					byteCnt-=cnt;
					fseek(fpBinary, -cnt, SEEK_CUR);
					break;
				}

				timeSum += t;


				///// �C�x���g���̃X�L�b�v���� /////

				// �X�L�b�v����f�[�^�̃o�C�g�������߂�
				fread(&tmp, sizeof(char), 1, fpBinary);	// �X�e�[�^�X�o�C�g�̎擾
				byteCnt++;
				int status = ConvertBYTEtoINT(tmp, 1);
				unsigned char type = 0;

				int skipLen = 0;
				if (status == 0xf0 || status == 0xf7) {	// �G�N�X�N���[�V�u���b�Z�[�W
					// �f�[�^���擾
					int cnt = 0;
					skipLen = ReadVariableLengthNumber(fpBinary, &cnt);
					byteCnt += cnt;
				}
				else if (status == 0xff) {	// ���^�C�x���g
					fread(&type, 1, 1, fpBinary);	//�C�x���g�^�C�v�擾
					byteCnt++;

					// �f�[�^���擾
					int cnt = 0;
					skipLen = ReadVariableLengthNumber(fpBinary, &cnt);
					byteCnt += cnt;
				}
				else if ((0x80 <= status && status <= 0x8f) ||		// �m�[�g�I�t
					(0x90 <= status && status <= 0x9f) ||		// �m�[�g�I��
					(0xb0 <= status && status <= 0xbf) ||		// �R���g���[���`�F���W
					status <= 0x7f) {							// �����j���O�X�e�[�^�X�K����
					skipLen = 2;

					if (status <= 0x7f)
						skipLen--;
					else if (!(0x90 <= status && status <= 0x9f))
						flag = false;
					else
						flag = true;

					if (flag) {		// �m�[�g�I�� �������͂��̃����j���O�X�e�[�^�X�K����
						if (t > 0) {
							minUnit = CalcGCD(minUnit, t);	// �v����ɁA�����̒����̍ŏ����{�������߂�

							if (concurrentEvt > needLaneCnt)
								needLaneCnt = concurrentEvt;

							concurrentEvt = 0;
						}
						else {
							// �����ɔ�������C�x���g
							concurrentEvt++;
						}
					}
				}
				else {
					return 1;
				}
				

				// �X�L�b�v
				fseek(fpBinary, skipLen, SEEK_CUR);
				byteCnt+=skipLen;

				if (type == 0x2F) {		// �g���b�N�`�����N�̏I���
					break;
				}
			}

			length -= byteCnt;

			//// �f���^�^�C�����Abms�`���ɕϊ� ////
			fseek(fpBinary, -byteCnt, SEEK_CUR);	// ���߂̎n�߂܂Ŗ߂�

			int writeByte = 0;
			int bar = totalTime/resolution;
			
			for (int i=0; i<needLaneCnt; i++) {
				fprintf(fpBms, "#%03d%02d:", bar, i);
			}


			while (byteCnt > 0) {
				// �ēx�f���^�^�C���̎擾
				int cnt = 0;
				int t = ReadVariableLengthNumber(fpBinary, &cnt);
				byteCnt -= cnt;
				totalTime += t;


				///// �C�x���g���̃X�L�b�v���� /////

				// �X�L�b�v����f�[�^�̃o�C�g�������߂�
				fread(&tmp, sizeof(char), 1, fpBinary);	// �X�e�[�^�X�o�C�g�̎擾
				byteCnt--;
				int status = ConvertBYTEtoINT(tmp, 1);
				unsigned char type = 0;

				int skipLen = 0;
				if (status == 0xf0 || status == 0xf7) {	// �G�N�X�N���[�V�u���b�Z�[�W
														// �f�[�^���擾
					int cnt = 0;
					skipLen = ReadVariableLengthNumber(fpBinary, &cnt);
					byteCnt -= cnt;
				}
				else if (status == 0xff) {	// ���^�C�x���g
					
					fread(&type, 1, 1, fpBinary);	//�C�x���g�^�C�v�擾
					byteCnt--;


					// �f�[�^���擾
					int cnt = 0;
					skipLen = ReadVariableLengthNumber(fpBinary, &cnt);
					byteCnt -= cnt;
				}
				else if ((0x80 <= status && status <= 0x8f) ||		// �m�[�g�I�t
					(0x90 <= status && status <= 0x9f) ||		// �m�[�g�I��
					(0xb0 <= status && status <= 0xbf) ||		// �R���g���[���`�F���W
					status <= 0x7f) {							// �����j���O�X�e�[�^�X�K����
					skipLen = 2;

					if (status <= 0x7f)
						skipLen--;
					else if (!(0x90 <= status && status <= 0x9f))
						flag = false;
					else
						flag = true;

					if (flag) {		// �m�[�g�I�� �������͂��̃����j���O�X�e�[�^�X�K����
						// bms�t�@�C���ɏ�������
						int offset = t/minUnit;

						// �m�[�g��z�u���郌�[�������߂�


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

				// �X�L�b�v
				fseek(fpBinary, skipLen, SEEK_CUR);
				byteCnt-=skipLen;

				if (type == 0x2F) {		// �g���b�N�`�����N�̏I���
					lpflag = false;
					break;
				}
			}

			// ���߂̎c��̗̈��00�Ŗ��߂�
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