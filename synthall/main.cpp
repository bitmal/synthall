#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include <limits>
#include <cmath>

#define KEY_PRESS_EXIT_LATENCY_MAX 0.005f
#define KEY_0  84
#define KEY_1  88
#define KEY_2  91

struct key_press
{
	DWORD timeStamp;
} volatile _keys[127];

void CALLBACK _MidiInProc(
	HMIDIIN   hMidiIn,
	UINT      wMsg,
	DWORD_PTR dwInstance,
	DWORD_PTR dwParam1,
	DWORD_PTR dwParam2
)
{
	if (wMsg & MIM_DATA &&
		((unsigned char *)&dwParam1)[0] == 144 &&
		((unsigned char *)&dwParam1)[2] > 0)
	{
		printf("%u, %u, %u\n", ((unsigned char*)&dwParam1)[0], ((unsigned char*)&dwParam1)[1],
			((unsigned char*)&dwParam1)[2]);
		
		_keys[((unsigned char*)&dwParam1)[1]].timeStamp = dwParam2;
	}
}

void CALLBACK _MidiOutProc(
	HMIDIOUT  hMidiOut,
	UINT      wMsg,
	DWORD_PTR dwInstance,
	DWORD_PTR dwParam1,
	DWORD_PTR dwParam2
)
{
}

int main()
{
	UINT numMidiDevs = midiInGetNumDevs();
	
	bool isDevFound = false;

	UINT midiDevId;
	MIDIINCAPSA midiDevInfo;
	for (midiDevId = 0; midiDevId < numMidiDevs; ++midiDevId)
	{
		midiInGetDevCapsA(midiDevId, &midiDevInfo, sizeof(midiDevInfo));

		if (strcmp(midiDevInfo.szPname, "Alesis") >= 0)
		{
			isDevFound = true;
			break;
		}
	}

	if (!isDevFound)
	{
		fprintf(stderr, "Midi Device Error: Device not found.\n");
		return -1;
	}

	fprintf(stderr, "MIDI DEVICE -- ID(%u): %s\n", midiDevId, midiDevInfo.szPname);

	UINT numOutputDevs = midiOutGetNumDevs();

	isDevFound = false;

	UINT outputDevId;
	MIDIOUTCAPSA outputDevInfo;
	for (outputDevId = 0; outputDevId < numOutputDevs; ++outputDevId)
	{
		midiOutGetDevCapsA(outputDevId, &outputDevInfo, sizeof(outputDevInfo));

		if (strcmp(outputDevInfo.szPname, "Microsoft") >= 0)
		{
			isDevFound = true;
			break;
		}
	}
	
	if (!isDevFound)
	{
		fprintf(stderr, "Output Device Error: Device not found.\n");
		return -1;
	}

	fprintf(stderr, "OUTPUT DEVICE -- ID(%u): %s\n", outputDevId, outputDevInfo.szPname);

	HMIDIIN midiHandle;
	midiInOpen(&midiHandle, midiDevId, (DWORD_PTR)_MidiInProc, NULL, CALLBACK_FUNCTION);

	HMIDIOUT outputHandle;
	midiOutOpen(&outputHandle, outputDevId, (DWORD_PTR)_MidiOutProc, NULL, CALLBACK_FUNCTION);

	midiConnect((HMIDI)midiHandle, outputHandle, NULL);

	fprintf(stderr, "Connected.\n");
	fprintf(stderr, "Hit middle C triad to exit.\n");

	midiInStart(midiHandle);
	midiOutSetVolume(outputHandle, INT_MAX);

	while (true)
	{
		DWORD timeAvg = (_keys[KEY_0].timeStamp + _keys[KEY_1].timeStamp + _keys[KEY_2].timeStamp) / 3;

		if (abs(1.f - (_keys[KEY_0].timeStamp / (float)timeAvg)) <= KEY_PRESS_EXIT_LATENCY_MAX &&
			abs(1.f - (_keys[KEY_1].timeStamp / (float)timeAvg)) <= KEY_PRESS_EXIT_LATENCY_MAX &&
			abs(1.f - (_keys[KEY_2].timeStamp / (float)timeAvg)) <= KEY_PRESS_EXIT_LATENCY_MAX)
		{
			break;
		}
	}

	midiDisconnect((HMIDI)midiHandle, outputHandle, NULL);

	midiOutClose(outputHandle);
	midiInClose(midiHandle);

	fprintf(stderr, "Disconnected.\n");

	return 0;
}