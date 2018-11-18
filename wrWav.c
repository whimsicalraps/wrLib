#include "wav.h"

#include <stdlib.h> // malloc
#include <string.h> // memcpy

static void wav_updatelength( WavFile_t* wf
                            , uint32_t   byte_count
                            );


WavFile_t* wav_new( uint16_t channels
                  , uint32_t samplerate
                  , uint16_t bitdepth
                  , FILE*    file_p
                  )
{
	WavFile_t* wav = malloc(sizeof(WavFile_t));
	wav->h = malloc(sizeof(WaveHeader_t));
	wav->f = malloc(sizeof(WaveFormat_t));
	wav->d = malloc(sizeof(WaveData_t));
	wav->file = file_p;

	memcpy( wav->h->sGroupID,  "RIFF", 4 );
	memcpy( wav->h->sRiffType, "WAVE", 4 );

	memcpy( wav->f->sGroupID, "fmt ", 4 );
	wav->f->dwChunkSize      = sizeof(WaveFormat_t) - 8;
	wav->f->wFormatTag       = 1;
	wav->f->wChannels        = channels;
	wav->f->dwSamplesPerSec  = samplerate;
	wav->f->dwBitsPerSample  = bitdepth;
	wav->f->wBlockAlign      = wav->f->wChannels
                              * (wav->f->dwBitsPerSample >> 3);
	wav->f->dwAvgBytesPerSec = wav->f->dwSamplesPerSec
                              * wav->f->wBlockAlign;

	// data block is empty
	memcpy( wav->d->sGroupID, "data", 4 );
	wav->d->dwChunkSize  = 0;
	wav->h->dwFileLength = 4 + sizeof(WaveFormat_t) + 8;

	fwrite( wav->h, sizeof(WaveHeader_t), 1, wav->file );
	fwrite( wav->f, sizeof(WaveFormat_t), 1, wav->file );
	fwrite( wav->d, 8,                    1, wav->file );
	return wav;
}

WavFile_t* wav_append( WavFile_t* wf
                     , void*      audio
                     , uint32_t   sample_count
                     )
{
	uint32_t byte_count = sample_count * wf->f->wBlockAlign;
	fwrite( audio, byte_count, 1, wf->file );
	wav_updatelength( wf, byte_count);
	return wf;
}

void wav_close( WavFile_t* wf )
{
	fclose( wf->file );
	// free() the inner & outer struct
}

static void wav_updatelength( WavFile_t* wf
                            , uint32_t   byte_count
                            )
{
	// save current FILE* location
	long int tell = ftell(wf->file);

	wf->d->dwChunkSize  += byte_count;
	wf->h->dwFileLength += byte_count;
	// TODO how to abstract this based on struct layout?
	fseek(wf->file, sizeof(WaveHeader_t)+sizeof(WaveFormat_t)+4, SEEK_SET);
	fwrite( &(wf->d->dwChunkSize), sizeof(uint32_t), 1, wf->file );
	fseek(wf->file, 4, SEEK_SET);
	fwrite( &(wf->h->dwFileLength), sizeof(uint32_t), 1, wf->file );

	// restore FILE* location
	fseek(wf->file, tell, SEEK_SET);
}
