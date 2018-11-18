#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct{
    char     sGroupID[4];  // "RIFF"
    uint32_t dwFileLength; // total file size
    char     sRiffType[4]; // "WAVE"
} WaveHeader_t;

typedef struct{
	char     sGroupID[4];     // "fmt "
	uint32_t dwChunkSize;     // length of this chunk *after* this line
	uint16_t wFormatTag;      // always 1, for PCM
	uint16_t wChannels;       // channels of audio
	uint32_t dwSamplesPerSec; // sample rate
	uint32_t dwAvgBytesPerSec;// sampleRate * blockAlign
	uint16_t wBlockAlign;     // wChannels * (dwBitsPerSample / 8)
	uint16_t dwBitsPerSample; // 24
} WaveFormat_t;

typedef struct{
	char     sGroupID[4];  // "data"
	uint32_t dwChunkSize;  // length of the below array
	void*    sampleData_p; // pointer to the audio data buffer
} WaveData_t;

typedef struct{
	WaveHeader_t* h;
	WaveFormat_t* f;
	WaveData_t*   d;
	FILE*         file;
} WavFile_t;

WavFile_t* wav_new( uint16_t channels
                  , uint32_t samplerate
                  , uint16_t bitdepth
                  , FILE*    file
                  );

WavFile_t* wav_append( WavFile_t* wf
                     , void*      audio
                     , uint32_t   sample_count
                     );
void wav_close( WavFile_t* wf );
