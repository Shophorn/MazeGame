/*
Leo Tamminen

Audio interface for win32 platform.
*/


/// ***********************************************************************************
/// API

// Todo(Leo): Do this when better control of audio is required
// using WinApiAudio = PlatformAudio
// struct PlatformAudio
struct WinApiAudio
{
	IMMDeviceEnumerator *	pEnumerator;
	IMMDevice *				pDevice;
	IAudioClient *			pClient;
	IAudioRenderClient * 	pRenderClient;

	WAVEFORMATEX * 			pFormat;
	u32					bufferFrameCount;

    bool32                  isPlaying;
};

internal WinApiAudio fswin32_create_audio();          
internal void fswin32_release_audio (WinApiAudio * audio);
internal void fswin32_start_playing(WinApiAudio * audio);
internal void fswin32_stop_playing(WinApiAudio * audio);
internal void fswin32_get_audio_buffer(WinApiAudio * audio, int * sampleCount, PlatformStereoSoundSample ** samples);
internal void fswin32_release_audio_buffer(WinApiAudio * audio, int sampleCount);

/// ***********************************************************************************
/// IMPLEMENTATION

// Note(Leo): REFERENCE_TIME are in units of [100 nanoseconds], use these for conversions
constexpr s64 REFERENCE_TIMES_PER_SECOND = 10000000;     // seconds to REFERENCE_TIME
constexpr s64 REFERENCE_TIMES_PER_MILLISECOND = 10000;   // milliseconds to REFERENCE_TIME

/* Todo(Leo): copied from web, forgot where, thats why it is UNKNOWN.
Extracts actual format tag if it is hidden behind WAVEFORMATEXTENSIBLE struct */
inline u32
UNKNOWN_GetFormatTag( const WAVEFORMATEX* wfx )
{
    if ( wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
    {
        if ( wfx->cbSize < ( sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX) ) )
            return 0;

        static const GUID s_wfexBase = {0x00000000, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};

        auto wfex = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>( wfx );

        if ( memcmp( reinterpret_cast<const BYTE*>(&wfex->SubFormat) + sizeof(DWORD),
                     reinterpret_cast<const BYTE*>(&s_wfexBase) + sizeof(DWORD), sizeof(GUID) - sizeof(DWORD) ) != 0 )
        {
            return 0;
        }

        return wfex->SubFormat.Data1;
    }
    else
    {
        return wfx->wFormatTag;
    }
}

internal WinApiAudio fswin32_create_audio ()
{
    const CLSID CLSID_MMDeviceEnumerator =   __uuidof(MMDeviceEnumerator);
    const IID IID_IMMDeviceEnumerator =      __uuidof(IMMDeviceEnumerator);
    const IID IID_IAudioClient =             __uuidof(IAudioClient);
    const IID IID_IAudioRenderClient =       __uuidof(IAudioRenderClient);

    WinApiAudio audio = {};    

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    /*
    WinApiLog(  "Create audio device enumerator",
                CoCreateInstance(   CLSID_MMDeviceEnumerator, nullptr,
                                        CLSCTX_ALL, IID_IMMDeviceEnumerator ,
                                        (void**)&audio.pEnumerator));
    WinApiLog(  "Get audio endpoint device",
                audio.pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &audio.pDevice));

    WinApiLog(  "Activate device to get audio client",
                audio.pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void**)&audio.pClient));

    WinApiLog("Get audio format", audio.pClient->GetMixFormat(&audio.pFormat));
    */

    WIN32_CHECK(CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&audio.pEnumerator));
    WIN32_CHECK(audio.pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &audio.pDevice));
    WIN32_CHECK(audio.pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void**)&audio.pClient));
    WIN32_CHECK(audio.pClient->GetMixFormat(&audio.pFormat));

    #if 1
    // Todo(Leo): this ugly comma formatting is result from past, fix pls
    log_audio(1
        , "|======================================|\n"
        , "Selected device format:\n"
        , "\tformat: "             , WinApiWaveFormatTagString(UNKNOWN_GetFormatTag(audio.pFormat)) , "\n"
        , "\tchannels: "           , audio.pFormat->nChannels , "\n"
        , "\tsamples per sec: "    , audio.pFormat->nSamplesPerSec , "\n"
        , "\tavg bytes per sec: "  , audio.pFormat->nAvgBytesPerSec , "\n"
        , "\tblock align: "        , audio.pFormat->nBlockAlign , "\n"
        , "\tbits per sample: "    , audio.pFormat->wBitsPerSample , "\n"
        , "|======================================|\n");
    #endif

    REFERENCE_TIME requestedDuration = 1 * REFERENCE_TIMES_PER_SECOND;
    /*
    WinApiLog(  "Initialize audio client",
                audio.pClient->Initialize(  AUDCLNT_SHAREMODE_SHARED, 0,
                                            requestedDuration, 0, audio.pFormat, nullptr));

    WinApiLog("Get buffer size", audio.pClient->GetBufferSize(&audio.bufferFrameCount));
    WinApiLog("Get audio renderer", audio.pClient->GetService(IID_IAudioRenderClient, (void**)&audio.pRenderClient));
    */

    WIN32_CHECK(audio.pClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, requestedDuration, 0, audio.pFormat, nullptr));
    WIN32_CHECK(audio.pClient->GetBufferSize(&audio.bufferFrameCount));
    WIN32_CHECK(audio.pClient->GetService(IID_IAudioRenderClient, (void**)&audio.pRenderClient));

    return audio;
}

internal void fswin32_release_audio(WinApiAudio * audio)
{
    CoTaskMemFree(audio->pFormat);

    audio->pEnumerator->Release();
    audio->pEnumerator = nullptr;

    audio->pDevice->Release();
    audio->pEnumerator = nullptr;

    audio->pClient->Release();
    audio->pClient = nullptr;

    audio->pRenderClient->Release();
    audio->pRenderClient = nullptr;

    CoUninitialize();
}

// Todo(Leo): this seems unnecessary function
internal void fswin32_start_playing(WinApiAudio * audio)
{
    // WinApiLog("Start playing audio", audio->pClient->Start());
    WIN32_CHECK(audio->pClient->Start());
}

// Todo(Leo): this seems unnecessary function
internal void fswin32_stop_playing(WinApiAudio * audio)
{
    // WinApiLog("Stop playing audio", audio->pClient->Stop());
    WIN32_CHECK(audio->pClient->Stop());
}

internal void fswin32_get_audio_buffer(WinApiAudio * audio, int * frameCount, PlatformStereoSoundSample ** samples)
{
    u32 currentPadding;
    // WinApiLog("Get audio padding", audio->pClient->GetCurrentPadding(&currentPadding));
    WIN32_CHECK(audio->pClient->GetCurrentPadding(&currentPadding));
    u32 framesAvailable = audio->bufferFrameCount - currentPadding;

    u8 ** data = reinterpret_cast<u8 **>(samples);
    // WinApiLog("Get audio buffer", audio->pRenderClient->GetBuffer(framesAvailable, data));
    WIN32_CHECK(audio->pRenderClient->GetBuffer(framesAvailable, data));

    *frameCount = framesAvailable;
}

internal void fswin32_release_audio_buffer(WinApiAudio * audio, s32 frameCount)
{
    // WinApiLog("Release audio buffer", audio->pRenderClient->ReleaseBuffer(frameCount, 0));
    WIN32_CHECK(audio->pRenderClient->ReleaseBuffer(frameCount, 0));
}