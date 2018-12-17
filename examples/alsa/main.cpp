#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include <haze.h>

// Example: play module using ALSA.

static snd_pcm_t *pcm_handle;

int sound_init()
{
    snd_pcm_hw_params_t *hwparams;
    int ret;
    char const* card_name = "default";

    if ((ret = snd_pcm_open(&pcm_handle, card_name, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        std::cerr << "Error: " << card_name << ": " << snd_strerror(ret) << std::endl;
        return -1;
    }

    unsigned int channels = 2;
    unsigned int btime = 250000;   // 250ms
    unsigned int ptime = 50000;    // 50ms

    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_hw_params_any(pcm_handle, hwparams);
    snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16);
    snd_pcm_hw_params_set_rate(pcm_handle, hwparams, 44100, 0);
    snd_pcm_hw_params_set_channels_near(pcm_handle, hwparams, &channels);
    snd_pcm_hw_params_set_buffer_time_near(pcm_handle, hwparams, &btime, 0);
    snd_pcm_hw_params_set_period_time_near(pcm_handle, hwparams, &ptime, 0);
    snd_pcm_nonblock(pcm_handle, 0);

    if ((ret = snd_pcm_hw_params(pcm_handle, hwparams)) < 0) {
        std::cerr << "Can't set parameters: " << snd_strerror(ret) << std::endl;
        return -1;
    }

    if ((ret = snd_pcm_prepare(pcm_handle)) < 0) {
        std::cerr << "Can't prepare: " << snd_strerror(ret) << std::endl;
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

    if (sound_init() < 0) {
        std::cerr << "Error: can't initialize sound" << std::endl;
        exit(EXIT_FAILURE);
    }

    // get player info
    haze::PlayerInfo pi;
    hz.player_info(pi);
    std::cout << "Player  : " << pi.name << std::endl;

    // play module
    int16_t audio_buffer[256];
    haze::FrameInfo fi;
    hz.frame_info(fi);
    char buf[50];

    snprintf(buf, 50, "Duration: %dmin%02ds",
            fi.total_time / (1000 * 60), (fi.total_time / 1000) % 60);
    std::cout << buf << std::endl;

    while (true) {
        if (!hz.fill(audio_buffer, 256 * 2)) {
            break;
        }

        int frames = snd_pcm_bytes_to_frames(pcm_handle, 256 * 2);
        if (snd_pcm_writei(pcm_handle, audio_buffer, frames) < 0) {
                snd_pcm_prepare(pcm_handle);
        }
        hz.frame_info(fi);
        snprintf(buf, 50, "pos:%3d/%3d  row:%3d/%3d  %02d:%02d:%02d.%d\r",
            fi.pos, mi.length - 1,
            fi.row, fi.num_rows - 1,
            fi.time / (1000 * 60 * 60),
            (fi.time / (1000 * 60)) % 60,
            (fi.time / 1000) % 60,
            (fi.time / 100) % 10);
        std::cout << buf << std::flush;
    }

    std::cout << std::endl;
    snd_pcm_close(pcm_handle);

    munmap(data, size);
    close(fd);

    return 0;
}
