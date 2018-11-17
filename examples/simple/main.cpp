#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include <haze.h>

#define BUFFER_SIZE 4096


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

    auto hz = haze::HazePlayer(data, size);

    // get player info
    haze::PlayerInfo pi;
    hz.player_info(pi);
    std::cout << "Player  : " << pi.name << std::endl;


    // play module
    int16_t *buffer = new int16_t[BUFFER_SIZE];

    std::ofstream outfile("out.raw", std::ios::out | std::ios::binary);
    for (int i = 0; i < 400; i++) {
        hz.fill(buffer, BUFFER_SIZE);
        outfile.write(reinterpret_cast<char *>(buffer), BUFFER_SIZE);
    }
    outfile.close();

    delete [] buffer;

    munmap(data, size);
    close(fd);

    return 0;
}
