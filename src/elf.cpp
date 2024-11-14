#include <cstdio>
#include "elf.hpp"

elf::elf(const char *filename, unsigned char *memory, int &pc)
{
    file = fopen(filename, "rb");
    if(!file)
        throw std::runtime_error("Error: cannot open file " + std::string(filename) + "\n");

    fread(&ehdr, sizeof(ehdr), 1, file);
    pc = ehdr.e_entry;

    // section headers
    shdr.resize(ehdr.e_shnum);
    fseek(file, ehdr.e_shoff, SEEK_SET);
    fread(shdr.data(), sizeof(Elf64_Shdr), ehdr.e_shnum, file);

    // program headers
    phdr.resize(ehdr.e_phnum);
    fseek(file, ehdr.e_phoff, SEEK_SET);
    fread(phdr.data(), sizeof(Elf64_Phdr), ehdr.e_phnum, file);

    // .text
    for(const auto &i : phdr)
    {
        fseek(file, i.p_offset, SEEK_SET);
        fread(memory + i.p_vaddr, sizeof(char), i.p_filesz, file);
    }

    return;
}

elf::~elf(void)
{
    fclose(file);
    return;
}