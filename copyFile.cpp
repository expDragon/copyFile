#include <cctype>
#include <cstddef>
#include <cerrno>
#include <cstring>
#include <string.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <iomanip>

struct fileStreams
{
    std::ifstream source;
    std::ofstream dest;
    bool cont;
};

//copies file from source to destination
//if destination is a directory copies it into the directory with original name
int copy_file(const std::string&, std::string&);
//settup file streams, checks for errors with file streams
//if dest is a directory appends the source filename to the end to copy into directory
fileStreams setup_streams(const std::string& _source , std::string& _dest);

std::string get_file_name(const std::string& filePath);
std::size_t get_file_size(std::ifstream * filePath);

//returns true if path ends in "." or "/." or "/.."
int has_directory_terminator(std::string filePath);
void progress_bar();
void update_progress_counter(int progress);
void print_progress(int progress);

//global variable to hold percentage of file copied
std::atomic_int progressPercentage(0);

//print how many percent have been copied and a progress bar
void print_progress(int progress)
{
    if(progress > 100)
        progress = 100;

    std::cout << "\r" << std::right << std::setfill('0') << std::setw(3) << progress << "% [";
    //print a # for every percent that has been copied
    for(int block = 0; block < progress; block++)
    {  
        std::cout << '#';
    }
    //fill the rest of the bar with blank spaces
    for(int blank = progress; blank < 100; blank++)
    { 
        std::cout << ' ';
    }
    std::cout << "]";
    std::cout.flush();
}

void update_progress_counter(int progress)
{
    progressPercentage = progress;
}

void progress_bar()
{
    int last = 0;

    //continue untill all data is copied
    while(1)
    {
        //if percentage has changed
        if(progressPercentage > last)
        {
            last = progressPercentage;
            print_progress(last);

            if(last >= 100)
                return; 
        }

        //sleep for 500 milliseconds
        std::this_thread::sleep_for(std::chrono::milliseconds(500));    
    }

}


fileStreams setup_streams(const std::string& _source , std::string& _dest)
{
    fileStreams fs; 

    fs.source.open(_source , std::ios_base::binary);
    fs.dest.open(_dest, std::ios_base::binary | std::ios_base::trunc);    

    if(!fs.source.good())
    {
        std::perror(("unable to open file " + _source).c_str());        
        fs.cont = false;
        return fs; 
    }

    if(!fs.dest.good())
    {
        //if dest is a directory
        if(errno == 21)
        {
            //append source file name to directory and continue copying 
            if(!has_directory_terminator(_dest))
            {
                _dest.append("/");
            }

            _dest.append(get_file_name(_source));
            fs.dest.open(_dest, std::ios_base::binary | std::ios_base::trunc);

            if(!fs.dest.good())
            {
                std::perror(("unable to copy file to " + _dest).c_str());
                fs.cont = false;
            } 

        }
        else
        {
            std::cout << errno << std::endl;
            std::perror(("unable to open " + _dest).c_str());
            fs.cont = false;
            return fs;
        }
    }

    fs.cont = true;
    return fs;
}

int has_directory_terminator(std::string filePath)
{
    std::size_t size = filePath.size();//used for index
    std::size_t index = size-1; 
    bool isDirectory = false; 

    //filePath = "*." 
    if(size > 0 && filePath[index] == '.')
    {
        //filePath = "*.." 
        if(size > 1 && (filePath[index-1] == '.' ))
        {
            //filePath = "*/.."
            if(size > 2 && filePath[index-2] == '/')
            {
                isDirectory = true; 
            }
        }
        //filePath = "*/."
        else if(size > 1 && filePath[index-1] == '/')
        {
            isDirectory = true; 
        }
    }
    //filePath = "*/"
    else if(size > 0 && filePath[size] == '/')
    {
        isDirectory = true;
    } 

    return isDirectory;
}

//takes ifstream or ofstream to determine size of file
std::size_t get_file_size(std::ifstream& filePath)
{
    filePath.seekg(0, filePath.end);
    std::size_t length = filePath.tellg();
    filePath.seekg(0, filePath.beg);

    return length; 
}

int copy_file(std::string&& source, std::string&& destination)
{
    fileStreams fs = setup_streams(source, destination); 

    //something failed exit
    if(fs.cont == false)
        return -1;

    std::size_t fileSize = get_file_size(fs.source);

    //size of one percent of the file
    int onePercent = fileSize / 100; 
    //bytes copied 
    std::size_t copied = 0;
    //chunks copied at a time
    std::size_t copySize = 4096;

    char buff[copySize];

    std::thread thread1(progress_bar);

    while(copied < fileSize)
    {
        fs.source.read(buff, copySize);
        copied += fs.source.gcount();
        //some error occured whilst reading file. 
        if(fs.source.bad())
        {
            perror(("\nerror while reading file" + source ).c_str()); 
            std::cout << "copy failed... exiting.\n";
            return -1; 
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        fs.dest.write(buff, fs.source.gcount());
        //some error occured whilst writing to file.
        if(fs.dest.bad()) 
        {
            perror(("\nerror while writing to file " + destination).c_str()); 
            std::cout << "copy failed... exiting.\n";  
            return -1; 
        }
        update_progress_counter(copied / onePercent);
    } 

    fs.dest.close(); 
    fs.source.close();

    thread1.join(); 
    std::cout << std::endl;
    return 1; 
} 

std::string get_file_name(const std::string& filePath)
{
    std::size_t found = filePath.find_last_of('/'); 

    if(found != filePath.npos)
        return filePath.substr(found+1);
    else
        return filePath;
}



bool yesOrNo()
{
    char answer = 'q';  

    do
    {
        std::cout << "Continue anyway?(Y/N) ";

        std::cin >> answer; 
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << '\r';

    }
    while(toupper(answer) != 'N' && toupper(answer) != 'Y');  

    if(toupper(answer) == 'N')
        return false;
    else
        return true;

}

int main(int argc, char * argv[])
{
    if(argc < 3)
    {
        std::cout << "Usage: copyFile Source Destination\n";
        return 0; 
    }

    copy_file((std::string)argv[1], (std::string)argv[2]);

    return 0;
}
