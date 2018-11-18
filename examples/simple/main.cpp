#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdint>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <haze.h>

#define BUFFER_SIZE 4096

// Example: play module and store a few buffers of data into a wav file.
// The output file will be called out.wav.


void write_le(std::ofstream& out, uint32_t v, int size)
{
    for (int i = 0; i < size; i++, v >>= 8) {
        out.put(v & 0xff);
    }
}


int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        exit(EXIT_SUCCESS);
    }

    char *name = argv[1];

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
    std::cout << "Channels: " << mi.channels << std::endl;
    std::cout << "Title   : " << mi.title << std::endl;

    auto hz = haze::HazePlayer(data, size, mi.player_id, mi.format_id);

    // get player info
    haze::PlayerInfo pi;
    hz.player_info(pi);
    std::cout << "Player  : " << pi.name << std::endl;

    // write wav file
    std::ofstream outfile("out.wav", std::ios::out | std::ios::binary);
    outfile << "RIFF....WAVEfmt ";
    write_le(outfile, 16, 4);         // fmt chunk size
    write_le(outfile, 1, 2);          // data format
    write_le(outfile, 2, 2);          // channels
    write_le(outfile, 44100, 4);      // sampling rate
    write_le(outfile, 44100 * 4, 4);  // byte rate
    write_le(outfile, 4, 2);          // frame size
    write_le(outfile, 16, 2);         // sample size
    outfile << "data....";

    // play module
    std::cout << "Render buffer " << std::flush;
    int16_t *buffer = new int16_t[BUFFER_SIZE];
    for (int i = 0; i < 400; i++) {
        hz.fill(buffer, BUFFER_SIZE);
        outfile.write(reinterpret_cast<char *>(buffer), BUFFER_SIZE);
        std::cout << "." << std::flush;
    }
    std::cout << std::endl;

    uint32_t len = outfile.tellp();
    outfile.seekp(4);
    write_le(outfile, len - 8, 4);
    outfile.seekp(40);
    write_le(outfile, len - 32, 4);

    outfile.close();

    delete [] buffer;

    munmap(data, size);
    close(fd);

    return 0;
}
