#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <haze.h>


int main(int argc, char** argv)
{
    std::cout << "Registered formats:" << std::endl;
    for (auto item : haze::list_formats()) {
        std::cout << "[" << item.id << "] " << item.name << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Registered players:" << std::endl;
    for (auto item : haze::list_players()) {
        std::cout << "[" << item.id << "] " << item.name << " - " << item.description << std::endl;
    }

    if (argc < 2) {
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
    std::cout << std::endl;
    std::cout << "Module info:" << std::endl;
    std::cout << "Title: " << mi.title << std::endl;
    std::cout << "Format: " << mi.description << std::endl;
    std::cout << "Creator: " << mi.creator << std::endl;
    std::cout << "Number of channels: " << mi.num_channels << std::endl;
    std::cout << "Number of instruments: " << mi.num_instruments << std::endl;
    std::cout << "Instruments:" << std::endl;
    int i = 1;
    for (auto name : mi.instruments) {
        std::cout << i++ << ": " << name << std::endl;
    }

    munmap(data, size);
    close(fd);

    return 0;
}
