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
    void print_file_content(std::fstream &file)
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

    /**
     * @brief calculates the file size of a file 
     * @param a file handle 
     * @return the size of the file
    */
    int get_file_size(std::fstream &file)
    {
        // Read and print the content
        file.seekg(0, std::ios::end);
        std::streamoff end_postion = file.tellg();

        return end_postion;
    }
}