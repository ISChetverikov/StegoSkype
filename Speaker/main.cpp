#define _USE_MATH_DEFINES

#include <math.h>
#include <iostream>
#include <windows.h>
#include <mmsystem.h>
#include <fstream>
#include <ctime>

#include "fft.h"

#pragma comment(lib,"winmm.lib") 

//#include "Stego.h"

using namespace std;

//Частота дискретизации
int sampleRate = 44100;

/*Ключ системы*/
int a = 35;
int b = 347;
unsigned int x0 = 121;
int modulo = (int)pow(2, 16);

//Параметры стегосистемы
int amplitude = 70;
const int BLOCK_SIZE = 16384;
int message_length = 12;

unsigned int current_sign = x0;
int is_end = 0;

short int waveBuf1[BLOCK_SIZE];
short int waveBuf2[BLOCK_SIZE];
short int waveBuf3[BLOCK_SIZE];

// Преобразование строки бит в символьную строку
string bits_to_message(string bits)
{
	string result = "";

	unsigned char symbol;
	 
	for (unsigned int i = 0; i < bits.length(); i+=8)
	{
		symbol = 0;

		for (int j = 0; j < 8; j++)
		{
			if (bits[i + j] == '1') symbol += (unsigned char)pow(2, j);
		}

		result += symbol;
	}

	
	return bits;
}

//Коэффициент корреляции
double correlation(short * arr1, short * arr2, int size)
{
	double corr = 0;

	double sum1 = 0;
	double sum2 = 0;

	double av1 = 0;
	double av2 = 0;

	for (int i = 0; i < size; i++)
	{
		av1 += arr1[i];
		av2 += arr2[i];
	}
	av1 /= size;
	av2 /= size;

	for (int i = 0; i < size; i++)
	{
		corr += (arr1[i] - av1) * (arr2[i] - av2);

		sum1 += (arr1[i] - av1) * (arr1[i] - av1);
		sum2 += (arr2[i] - av2) * (arr2[i] - av2);
	}

	corr /= sqrt(sum1*sum2);

	return corr;
}

//Функция максимальной корреляции
double max_correlation(short * arr1, short * arr2, int step, int count, int size)
{
	double max_corr = 0;
	double corr = 0;
	short * temp = new short[size];

	for (int shift = 0; shift < count; shift++)
	{
		for (int i = 0; i < step; i++)
		{
			temp[i] = arr1[i];
		}

		for (int i = 0; i < BLOCK_SIZE / 2 - step; i++)
		{
			arr1[i] = arr1[i + step];
		}

		for (int i = BLOCK_SIZE / 2 - step; i < BLOCK_SIZE / 2; i++)
		{
			arr1[i] = temp[i - (BLOCK_SIZE / 2 - step)];
		}

		corr = correlation(arr1, arr2, size);

		if (abs(corr) > abs(max_corr)) max_corr = corr;
	}

	//printf_s("%f\n", max_corr);
	return max_corr;
}

//Генерация двоичной последовательности
string get_pseudo_sequence()
{
	string result = "";

	current_sign = (a * current_sign + b) % modulo;

	unsigned int dec_number = current_sign;

	for (int j = 0; j < 16; j++)
	{
		result += ('0' + dec_number % 2);

		dec_number /= 2;
	}

	return result;
}

string check_sequences(string s1, string s2)
{
	string result = "";
	string s2_copy = s2;

	int max_count = 0;

	for (int i = 0; i < s1.length(); i++)
	{
		int count = 0;
		for (int j = 0; j < s1.length(); j++)
		{
			if (s1[j] == s2_copy[j]) count++;
		}

		if (count > max_count) max_count = count;

		int temp = s2_copy[0];
		for (int j = 0; j < s2.length() - 1; j++)
		{
			s2_copy[j] = s2_copy[j + 1];
		}
		s2_copy[s1.length()] = s2_copy[0];
	}

	if (max_count > 8) for (int i = 0; i < s1.length(); i++)
	{
		if (s2[i] == '0') result += '1';
		else result += '0';
	}
	else result = s2;

	return result;
}


//Корреляция между сегментом
double noise_correlation(short *data, string pseudo_sequence)
{
	short * random_signal = new short[BLOCK_SIZE / 2];

	int rel = BLOCK_SIZE / 2 / pseudo_sequence.length();

	for (unsigned int i = 0; i < pseudo_sequence.length(); i++)
	{
		for (int j = 0; j < rel; j++)
		{
			random_signal[rel*i + j] = (pseudo_sequence[i] == (int)'1') ? amplitude : -amplitude;
		}
	}

	for (int i = 0; i < BLOCK_SIZE / 2; i++)
	{
		random_signal[i] = random_signal[i] * (2.16 - 1.84 * cos(2 * M_PI * i / (BLOCK_SIZE / 2)));
	}

	short * channel1 = new short[BLOCK_SIZE / 2];

	for (int current_sample = 0; current_sample < BLOCK_SIZE / 2; current_sample++)
	{
		channel1[current_sample] = data[current_sample * 2];
	}
	
	int shift_count = pseudo_sequence.length() * 8;
	int shift_size = BLOCK_SIZE / 2 / shift_count;

	double max_corr = max_correlation(channel1, random_signal, shift_size, shift_count, BLOCK_SIZE/2);

	delete[] channel1;
	delete[] random_signal;

	return max_corr;
}

//Детектирование стартовой последовательности
int start_decode(short *data, WAVEFORMATEX header)
{

	double max_corr = noise_correlation(data, "1010101010101010");

	//printf_s("%f\n", max_corr);
	/*short * channel1 = new short[BLOCK_SIZE / 2];

	for (int current_sample = 0; current_sample < BLOCK_SIZE / 2; current_sample++)
	{
		channel1[current_sample] = data[current_sample * 2];
	}

	double max_corr = max_correlation(channel1, sinus, 2, 45, BLOCK_SIZE/2);

	delete[] channel1;*/
	return (abs(max_corr) > 0.2) ? 1 : 0;
}

//Извлечение бита информации
char noise_decode(short *data)
{
	string pseudo_sequence1 = get_pseudo_sequence();
	string pseudo_sequence2 = get_pseudo_sequence();//check_sequences(pseudo_sequence1, get_pseudo_sequence());
	/*string pseudo_sequence2 = pseudo_sequence1;
	for (int i = 0; i < pseudo_sequence2.length(); i++)
	{
		if (pseudo_sequence2[i] == '1') pseudo_sequence2[i] = '0';
		else pseudo_sequence2[i] = '1';
	}*/

	/*printf_s("\n%s\n", pseudo_sequence1.c_str());
	printf_s("%s\n", pseudo_sequence2.c_str());*/

	double corr0 = noise_correlation(data, pseudo_sequence1);
	double corr1 = noise_correlation(data, pseudo_sequence2);
	
	return (abs(corr0) > abs(corr1)) ? '0': '1';
}

void main()
{
	setlocale(LC_ALL, "Russian");
	//srand(unsigned(time(0)));

	//Формат аудио данных
	WAVEFORMATEX pFormat;
	pFormat.wFormatTag = WAVE_FORMAT_PCM;     // simple, uncompressed format
	pFormat.nChannels = 2;                    //  1=mono, 2=stereo
	pFormat.wBitsPerSample = 16;              //  16 for high quality, 8 for telephone-grade
	pFormat.nSamplesPerSec = sampleRate;
	pFormat.nAvgBytesPerSec = sampleRate * pFormat.nChannels * pFormat.wBitsPerSample / 8;
	pFormat.nBlockAlign = pFormat.nChannels * pFormat.wBitsPerSample / 8;
	pFormat.cbSize = 0;

	//Открытие устройств
	HWAVEIN hWaveIn;
	waveInOpen(&hWaveIn, 2, &pFormat, 0L, 0L, WAVE_FORMAT_DIRECT);

	HWAVEOUT hWaveOut;
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &pFormat, 0L, 0L, WAVE_FORMAT_DIRECT);
	waveOutSetVolume(hWaveOut, 0xFFFFFFFF);

	// Подготовка структур для заголовков блоков данных
	WAVEHDR waveInHdr1;
	waveInHdr1.lpData = (LPSTR)waveBuf1;
	waveInHdr1.dwBufferLength = BLOCK_SIZE * 2;
	waveInHdr1.dwBytesRecorded = 0;
	waveInHdr1.dwUser = 0L;
	waveInHdr1.dwFlags = 0L;
	waveInHdr1.dwLoops = 0L;

	WAVEHDR waveInHdr2;
	waveInHdr2.lpData = (LPSTR)waveBuf2;
	waveInHdr2.dwBufferLength = BLOCK_SIZE * 2;
	waveInHdr2.dwBytesRecorded = 0;
	waveInHdr2.dwUser = 0L;
	waveInHdr2.dwFlags = 0L;
	waveInHdr2.dwLoops = 0L;

	WAVEHDR waveInHdr3;
	waveInHdr3.lpData = (LPSTR)waveBuf3;
	waveInHdr3.dwBufferLength = BLOCK_SIZE * 2;
	waveInHdr3.dwBytesRecorded = 0;
	waveInHdr3.dwUser = 0L;
	waveInHdr3.dwFlags = 0L;
	waveInHdr3.dwLoops = 0L;

	WAVEHDR waveOutHdr1;
	waveOutHdr1.lpData = (LPSTR)waveBuf1;
	waveOutHdr1.dwBufferLength = BLOCK_SIZE * 2;
	waveOutHdr1.dwBytesRecorded = 0;
	waveOutHdr1.dwUser = 0L;
	waveOutHdr1.dwFlags = 0L;
	waveOutHdr1.dwLoops = 1L;

	WAVEHDR waveOutHdr2;
	waveOutHdr2.lpData = (LPSTR)waveBuf2;
	waveOutHdr2.dwBufferLength = BLOCK_SIZE * 2;
	waveOutHdr2.dwBytesRecorded = 0;
	waveOutHdr2.dwUser = 0L;
	waveOutHdr2.dwFlags = 0L;
	waveOutHdr2.dwLoops = 1L;

	WAVEHDR waveOutHdr3;
	waveOutHdr3.lpData = (LPSTR)waveBuf3;
	waveOutHdr3.dwBufferLength = BLOCK_SIZE * 2;
	waveOutHdr3.dwBytesRecorded = 0;
	waveOutHdr3.dwUser = 0L;
	waveOutHdr3.dwFlags = 0L;
	waveOutHdr3.dwLoops = 1L;

	// Используем два буфера
	waveInStart(hWaveIn);

	waveInPrepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

	waveInAddBuffer(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

	waveInPrepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

	waveInAddBuffer(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

	waveInPrepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

	waveInAddBuffer(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

	char bit_ridden;
	int is_began = 0;

	string check = "";//= message;
	for (int i = 0; i < 1000; i++)
	{
		int dec_number = rand() % 2;
		check += ('0' + dec_number % 2);
	}
	printf_s("%s\n", check.c_str());

	while (true)
	{

		do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

		if (start_decode(waveBuf1, pFormat) == 1) is_began++;
		else if (is_began > 0) is_began--;
		//printf_s("%d\n", is_began);

		waveInPrepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

		waveInAddBuffer(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

		waveOutPrepareHeader(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));

		waveOutWrite(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));

		// Читаем ================================================================================================
		if (is_began == 3)
		{
			int t = 0;
			
			string message = "";
			while (t < message_length * 8)
			{
				
				do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

				waveInPrepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

				waveInAddBuffer(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

				waveOutPrepareHeader(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));

				waveOutWrite(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));



				do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

				bit_ridden = noise_decode(waveBuf3);

				//printf_s("%c\n", bit_ridden);

				message += bit_ridden;

				waveInPrepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

				waveInAddBuffer(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

				waveOutPrepareHeader(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));

				waveOutWrite(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));


				
				do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

				waveInPrepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

				waveInAddBuffer(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

				waveOutPrepareHeader(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));

				waveOutWrite(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));

				t++;
			}
		
			printf_s("%s\n", bits_to_message(message).c_str());
			current_sign = x0;
			is_began = 0;
			is_end = 0;
		}
		//============================================================================================




		do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

		if (start_decode((short *)waveBuf2, pFormat) == 1) is_began++;
		else if (is_began > 0) is_began--;
		//printf_s("%d\n", is_began);

		waveInPrepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

		waveInAddBuffer(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

		waveOutPrepareHeader(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));

		waveOutWrite(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));

		// Читаем ================================================================================================
		if (is_began == 3)
		{
			int t = 0;
			string message;
			
			while (t <message_length*8)
			{
				do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);
				
				waveInPrepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

				waveInAddBuffer(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

				waveOutPrepareHeader(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));

				waveOutWrite(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));



				do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

				bit_ridden = noise_decode(waveBuf1);

				//printf_s("%c\n", bit_ridden);

				message += bit_ridden;

				waveInPrepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

				waveInAddBuffer(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

				waveOutPrepareHeader(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));

				waveOutWrite(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));



				do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

				waveInPrepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

				waveInAddBuffer(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

				waveOutPrepareHeader(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));

				waveOutWrite(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));

				t++;
			}

			printf_s("%s\n", bits_to_message(message).c_str());
			current_sign = x0;
			is_began = 0;
			is_end = 0;
		}
		//============================================================================================




		do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

		if (start_decode(waveBuf3, pFormat) == 1) is_began++;
		else if (is_began > 0) is_began--;
		//printf_s("%d\n", is_began);
		
		waveInPrepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

		waveInAddBuffer(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

		waveOutPrepareHeader(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));

		waveOutWrite(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));

		// Читаем ================================================================================================
		if (is_began == 3)
		{
			int t = 0;
			
			string message;

			while (t < message_length*8)
			{
				do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

				waveInPrepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

				waveInAddBuffer(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

				waveOutPrepareHeader(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));

				waveOutWrite(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));



				do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

				bit_ridden = noise_decode(waveBuf2);

				//printf_s("%c\n", bit_ridden);

				message += bit_ridden;

				waveInPrepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

				waveInAddBuffer(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

				waveOutPrepareHeader(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));

				waveOutWrite(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));



				do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

				waveInPrepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

				waveInAddBuffer(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

				waveOutPrepareHeader(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));

				waveOutWrite(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));

				t++;
			}

			printf_s("%s\n", bits_to_message(message).c_str());
			current_sign = x0;
			is_began = 0;
			is_end = 0;
		}
		//============================================================================================

	}

	waveInStop(hWaveIn);
	waveInClose(hWaveIn);
	waveOutClose(hWaveOut);

	system("pause");
}