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



//string message = "Disney";
//string message = "11111000001111100000111110000011111000001111100000";

/*Ключ системы*/
int a = 35;
int b = 347;
unsigned int x0 = 121;
int modulo = (int)pow(2, 16);

//Параметры стегосистемы
int amplitude = 70;
// Количество сэмплов!!!! для блока данных
const int BLOCK_SIZE = 16384;

unsigned int current_sign = x0;

short int waveBuf1[BLOCK_SIZE];
short int waveBuf2[BLOCK_SIZE];
short int waveBuf3[BLOCK_SIZE];

//Перевод сообщения в двоичный вид
string message_to_bits(string message)
{
	string result = "";

	for (unsigned int i = 0; i < message.length(); i++)
	{
		unsigned char dec_number = (unsigned char)message[i];
		//printf("%d\n", dec_number);

		for (int j = 0; j < 8; j++)
		{
			result += ('0' + dec_number % 2);

			dec_number /= 2;
		}

	}


	//printf("%s\n", result.c_str());
	return result;
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

//Встраивание двоичной последовательности
int noise_encode(short * data, WAVEFORMATEX header, string pseudo_sequence)
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

	// Окно Хэмминга
	for (int i = 0; i < BLOCK_SIZE / 2; i++)
	{
		random_signal[i] = random_signal[i] * (2.16 - 1.84 * cos(2 * M_PI * i / (BLOCK_SIZE / 2)));
	}

	for (int i = 0; i < BLOCK_SIZE / 2; i++)
	{
		data[i * 2] += random_signal[i];
		data[i * 2 + 1] += random_signal[i];
	}

	delete[] random_signal;

	return 0;
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
	waveInOpen(&hWaveIn, WAVE_MAPPER, &pFormat, 0L, 0L, WAVE_FORMAT_DIRECT);

	HWAVEOUT hWaveOut;
	waveOutOpen(&hWaveOut, 3, &pFormat, 0L, 0L, WAVE_FORMAT_DIRECT);
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

	while (true)
	{
		// 1 такт внедрения ------------------------------------------------------------------------
		// Пауза без сообщения
		int t = 0;
		while (t < 3)
		{
			t++;

			do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

			waveInPrepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

			waveInAddBuffer(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

			waveOutPrepareHeader(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));

			waveOutWrite(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));



			do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

			waveInPrepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

			waveInAddBuffer(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

			waveOutPrepareHeader(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));

			waveOutWrite(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));



			do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

			waveInPrepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

			waveInAddBuffer(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

			waveOutPrepareHeader(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));

			waveOutWrite(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));


		}


		// Запускаем последовательность без скрытой информации для определения начала передачи
		t = 0;
		while (t < 1)
		{
			t++;

			do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

			noise_encode(waveBuf1, pFormat, "1010101010101010");
			
			waveInPrepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

			waveInAddBuffer(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

			waveOutPrepareHeader(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));

			waveOutWrite(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));



			do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

			noise_encode(waveBuf2, pFormat, "1010101010101010");
		
			waveInPrepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

			waveInAddBuffer(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

			waveOutPrepareHeader(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));

			waveOutWrite(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));



			do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

			noise_encode(waveBuf3, pFormat, "1010101010101010");
		
			waveInPrepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

			waveInAddBuffer(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

			waveOutPrepareHeader(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));

			waveOutWrite(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));


		}

		// Собственно само сокрытие
		string bits = "";//= message;
		for (int i = 0; i < 96; i++)
		{
			int dec_number = rand() % 2;
			bits += ('0' + dec_number % 2);
		}
		printf_s("%s\n", bits.c_str());
		
		t = 0;
		int length = bits.length();
		
		while (t < length)
		{

			string pseudo_sequence1 = get_pseudo_sequence();
			string pseudo_sequence2 = get_pseudo_sequence();// check_sequences(pseudo_sequence1, get_pseudo_sequence());

			string pseudo_sequence = (bits[t] == '0') ? pseudo_sequence1 : pseudo_sequence2;
			
			t++;

			do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

			noise_encode(waveBuf1, pFormat, pseudo_sequence);

			waveInPrepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

			waveInAddBuffer(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

			waveOutPrepareHeader(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));

			waveOutWrite(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));



			do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

			noise_encode(waveBuf2, pFormat, pseudo_sequence);

			waveInPrepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

			waveInAddBuffer(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

			waveOutPrepareHeader(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));

			waveOutWrite(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));



			do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

			noise_encode(waveBuf3, pFormat, pseudo_sequence);

			waveInPrepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

			waveInAddBuffer(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

			waveOutPrepareHeader(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));

			waveOutWrite(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));

			
		}

		current_sign = x0;

		// Пауза после внедрения
		t = 0;
		while (t < 4)
		{
			t++;
			do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

			waveInPrepareHeader(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

			waveInAddBuffer(hWaveIn, &waveInHdr1, sizeof(WAVEHDR));

			waveOutPrepareHeader(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));

			waveOutWrite(hWaveOut, &waveOutHdr1, sizeof(WAVEHDR));



			do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

			waveInPrepareHeader(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

			waveInAddBuffer(hWaveIn, &waveInHdr2, sizeof(WAVEHDR));

			waveOutPrepareHeader(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));

			waveOutWrite(hWaveOut, &waveOutHdr2, sizeof(WAVEHDR));


			do{} while (waveInUnprepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

			waveInPrepareHeader(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

			waveInAddBuffer(hWaveIn, &waveInHdr3, sizeof(WAVEHDR));

			waveOutPrepareHeader(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));

			waveOutWrite(hWaveOut, &waveOutHdr3, sizeof(WAVEHDR));

		}

		
		// 1 такт внедрения: конец ------------------------------------------------------------------------
	}
	waveInStop(hWaveIn);
	waveInClose(hWaveIn);
	waveOutClose(hWaveOut);

	system("pause");
}