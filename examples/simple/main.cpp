#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <haze.h>


size_t getFilesize(const char* filename) {
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}

int main(int argc, char** argv)
{
    char *name = argv[1];

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

    auto hz = haze::HazePlayer(data, size);
    haze::ModuleInfo info;
    hz.info(info);
    std::cout << "Title: " << info.title << std::endl;

    munmap(data, size);
    close(fd);
}
