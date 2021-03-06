#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include <haze.h>

// Example: play module using SDL audio.

static bool playing;

void fill_audio(void *udata, Uint8 *stream, int len)
{
    auto hz = static_cast<haze::HazePlayer *>(udata);
    if (!hz->fill(reinterpret_cast<int16_t *>(stream), len)) {
        playing = false;
    }
}

int sdl_init(haze::HazePlayer *hz)
{
    SDL_AudioSpec a;

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        return -1;
    }

    a.freq = 44100;
    a.format = AUDIO_S16;
    a.channels = 2;
    a.samples = 2048;
    a.callback = fill_audio;
    a.userdata = hz;

    if (SDL_OpenAudio(&a, NULL) < 0) {
        return -1;
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

    if (sdl_init(&hz) < 0) {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }

    // get player info
    haze::PlayerInfo pi;
    hz.player_info(pi);
    std::cout << "Player  : " << pi.name << std::endl;

    playing = true;

    // play module
    SDL_PauseAudio(0);
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
        SDL_Delay(10);
    }

    std::cout << std::endl;
    SDL_CloseAudio();

    munmap(data, size);
    close(fd);

    return 0;
}
