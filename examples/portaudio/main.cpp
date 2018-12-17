#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <portaudio.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include <haze.h>

// Example: play module using PortAudio

static bool playing;

static int fill_audio(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *userData)
{
    auto hz = static_cast<haze::HazePlayer *>(userData);
    if (!hz->fill(static_cast<int16_t *>(outputBuffer), framesPerBuffer * 4)) {
        playing = false;
    }
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        exit(EXIT_SUCCESS);
    }

    // load module data
    int fd = open(argv[1], O_RDONLY, 0);
    if (fd < 0) {
        std::cerr << "Error: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        std::cerr << "Error: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    uint32_t size = st.st_size;

    // Always map as MAP_PRIVATE! Players can modify module data
    void *data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        std::cerr << "Error: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    // probe module format
    haze::ModuleInfo mi;
    if (!haze::probe(data, size, mi)) {
        std::cerr << "Unrecognized module format" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "Format  : " << mi.description << std::endl;
    std::cout << "Creator : " << mi.creator << std::endl;
    std::cout << "Channels: " << mi.num_channels << std::endl;
    std::cout << "Title   : " << mi.title << std::endl;

    auto hz = haze::HazePlayer(data, size, mi.player_id, mi.format_id);
    haze::PlayerInfo pi;

    PaError err;
    if ((err = Pa_Initialize()) != paNoError) {
        goto error;
    }

    PaStream *stream;
    if ((err = Pa_OpenDefaultStream(&stream,
        0,           // no input channels
        2,           // stereo output
        paInt16,     // 16 bit integer output
        44100,       // sample rate
        paFramesPerBufferUnspecified,
        fill_audio,  // callback function
        &hz)         // pointer to be passed to callback function
    ) != paNoError) {
        goto error;
    }

    // get player info
    hz.player_info(pi);
    std::cout << "Player  : " << pi.name << std::endl;

    playing = true;

    // play module
    if ((err = Pa_StartStream(stream)) != paNoError) {
        goto error;
    }
    haze::FrameInfo fi;
    hz.frame_info(fi);
    char buf[50];

    snprintf(buf, 50, "Duration: %dmin%02ds",
            fi.total_time / (1000 * 60), (fi.total_time / 1000) % 60);
    std::cout << buf << std::endl;

    while (playing) {
        hz.frame_info(fi);
        snprintf(buf, 50, "pos:%3d/%3d  row:%3d/%3d  %02d:%02d:%02d.%d\r",
            fi.pos, mi.length - 1,
            fi.row, fi.num_rows - 1,
            fi.time / (1000 * 60 * 60),
            (fi.time / (1000 * 60)) % 60,
            (fi.time / 1000) % 60,
            (fi.time / 100) % 10);
        std::cout << buf << std::flush;
        Pa_Sleep(100);
    }

    std::cout << std::endl;
    if ((err = Pa_StopStream(stream)) != paNoError) {
        goto error;
    }
    if ((err = Pa_CloseStream(stream)) != paNoError) {
        goto error;
    }
    if ((err = Pa_Terminate()) != paNoError) {
        goto error;
    }

    munmap(data, size);
    close(fd);

    return 0;

error:
    std::cerr << "Error: " << Pa_GetErrorText(err) << std::endl;
    exit(EXIT_FAILURE);
}
