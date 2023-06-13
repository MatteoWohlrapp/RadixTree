/**
 * @file    File.h
 *
 * @author  Matteo Wohlrapp
 * @date    23.05.2023
 */

#pragma once

#include <fstream>
#include <iostream>

/**
 * @brief namespace contains helper functions for file access
 */
namespace File
{
    /**
     * @brief prints the content of a binary file
     * @param data_fs a file handle
     */
    inline void print_file_content(std::fstream &file)
    {

        // Read and print the content
        file.seekg(0, std::ios::end);
        std::streamoff endPosition = file.tellg();
        std::streamoff fileSize = endPosition;
        std::cout << "FileSize: " << std::dec << fileSize << std::endl;
        // Reset the file pointer to the beginning of the file
        file.seekg(0, std::ios::beg);

        char ch;
        while (file.get(ch))
        {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ch & 0xFF) << " ";
        }
        file.clear();

        std::cout << std::endl;
    }

    inline void print_headers_in_file(std::string path, std::fstream *data_fs, std::shared_ptr<spdlog::logger> logger)
    {
        data_fs->flush();
        std::ifstream file(path, std::ios::binary);

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        logger->info("File content: ");

        Header *header = (Header *)malloc(96);
        while (file.read(reinterpret_cast<char *>(header), 96))
        {
            std::stringstream ss;
            ss << "Page ID: " << header->page_id
               << ", Inner: " << std::boolalpha << header->inner;

            std::string result = ss.str();
            logger->info(result);
        }
        free(header);

        file.close();
    }

    /**
     * @brief calculates the file size of a file in byte
     * @param a file handle
     * @return the size of the file
     */
    inline int get_file_size(std::fstream *file)
    {
        // Read and print the content
        file->seekg(0, std::ios::end);
        std::streamoff end_postion = file->tellg();

        return end_postion;
    }
}