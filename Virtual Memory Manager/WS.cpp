//
//  Created by Edgar Olvera on 11/25/20.
//
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <math.h>
#include <algorithm>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>

using namespace std;

struct sembuf sb_wait;
struct sembuf sb_signal;

int process_id;
int total_number_of_frames_on_disk;
int page_faults{};
int total_number_of_page_frames{};
int number_of_page_frames_per_process{};
vector<string> disk;
vector<string> requests;
vector<string> working_set;
int min_free_pool_size{};
int max_free_pool_size{};

char* pmem;

void readInfo(ifstream&);
void readRequests(ifstream&);
void serviceRequests(int, int);
void serviceFaults(int, string, int);
void startDisk(int, string, string);

int main(int argc, const char * argv[]) {
    
    if (argc != 2)
    {
        perror("Input error");
        exit(0);
    }
    
    
    int max_segment_length{};
    int page_size{};
    int lookahead_window_or_X{};
    
    int total_number_of_processes;
    
    ifstream myFile(argv[1]);
    
    //  initialize shared memory
    int shmid = shmget(1425331, 200, IPC_CREAT | 0666);
    
    pmem = (char*)shmat(shmid, 0, 0);
    
    //  initialize semaphore
    int sid = semget(1425331, 1, 0666 | IPC_CREAT);
    
    
    //  initialize wait operation
    sb_wait.sem_num = 0;
    sb_wait.sem_op = -1;
    sb_wait.sem_flg = 0;
    
    //  initialize signal operatoin
    sb_signal.sem_num = 0;
    sb_signal.sem_op = 1;
    sb_signal.sem_flg = 0;
    semop(sid, &sb_signal, 1);
    
    string line{};
    
    //  We read the first lines of the file that mention info such as: number of processors, frame size, etc.
    getline(myFile, line);
    total_number_of_page_frames = stoi(line);
    getline(myFile, line);
    max_segment_length = stoi(line);
    getline(myFile, line);
    page_size = stoi(line);
    getline(myFile, line);
    number_of_page_frames_per_process = stoi(line);
    getline(myFile, line);
    lookahead_window_or_X = stoi(line);
    getline(myFile, line);
    min_free_pool_size = stoi(line);
    getline(myFile, line);
    max_free_pool_size = stoi(line);
    getline(myFile, line);
    total_number_of_processes = stoi(line);
    
    //  The file provides the page size. We know the page offset n is  2^n = pagesize or log(pagesize) = n;
    //  The file also provides the segment length in bits. which is: x (page number) + y (page offset)
    //  So to find the page number length it is: segment length - log2(pagesize)
    int page_number_length =  max_segment_length - (int)log2(page_size);
    
    
    cout << "Number of page faults for each process: " << endl;
    readInfo(myFile);
    
    int pid{1};
    // We will use fork to create the separate processes used in this program
    for (int i{1}; i < total_number_of_processes; i++)
    {
        pid = fork();   // fork to create a separate processor
        if (pid == 0)
        {
            readInfo(myFile);   // Each processor will read a line of the file that gives Id and disk space
        }
        if (pid > 0) {
            
            break;
        }
    }
    
    if(pid == 0 || pid == 1)
    {
        readRequests(myFile);
        serviceRequests(sid, page_number_length);
        cout << endl;
        cout << "Total number of page faults:" << endl;
        cout << pmem[2] << "." << endl;
        cout << endl;
    }
    else
    {
        
        readRequests(myFile);
        serviceRequests(sid, page_number_length);
        
        wait(NULL); //  We wait that child process to finished,
    }
    
    
    myFile.close(); //  close file
    
    //  Remove and detach shared memory and semaphore
    shmdt(pmem);
    semctl(sid, 0, IPC_RMID, 0);
    shmctl(shmid, IPC_RMID, 0);
    return 0;
}

void readInfo(ifstream& myFile)
{
    //  Reads processor id and disk size of individual processor
    
    string line{};
    getline(myFile, line, ' ');
    process_id = stoi(line);
    getline(myFile, line);
    total_number_of_frames_on_disk = stoi(line);
}

void readRequests(ifstream& myFile)
{
    //  This function will read all the requests from the file (exclusive to a specific processor)
    //  and store them in lists. That way we can more efficiently access and use requests without being
    //  constrained by keeping the information in a file.
    
    string line{};
    while(getline(myFile, line))
    {
        stringstream check(line);
        getline(check, line, ' ');
        if (stoi(line) == process_id)
        {
            getline(check, line, ' ');
            
            if(line == "-1")
                break;
            
            requests.push_back(line);
        }
    }
}


void serviceRequests(int sid, int page_number_length)
{
    cout << "Process " << process_id << ": ";
    string line{};
    
    while (!requests.empty())
    {
        line = requests.front();
        requests.erase(requests.begin());
        string switch_page = "";
        
        string page_number = line.substr(0, page_number_length);
        
        //  If page is not in our working set we must invoke page fault
        if (find(working_set.begin(), working_set.end(), page_number) == working_set.end())
        {
            page_faults++;
        }
        
        //  We handle the working set below, making sure we have an overhead delta of frames
        //  that have been requested by processor
        working_set.push_back(page_number);
        
        if (working_set.size() > number_of_page_frames_per_process)
        {
            switch_page = working_set.front();
            working_set.erase(working_set.begin());
        }
        
        //  We will swap out pages that are not in our working set and send back to the disk
        //  and swap pages in being requested from disk to the main memory
        startDisk(sid, page_number, switch_page);
    }
    
    cout << page_faults << ";" << endl;
    string page_faults_total;
    
    //  As the processer is finished, we must remove any pages still in the main memory.
    //  Our start disk function will be used to swap the pages out of memory and
    //  back to disk.
    for(auto w : working_set)
    {
        startDisk(sid, "", w);
    }
    
    stringstream check(pmem);
    getline(check, line);
    int n = stoi(line);
    string s = "";
    
    for (int i{}; i < n; ++i)
    {
        getline(check, line);
        s += line + "\n";
    }
    
    //  We will attach the total page faults to our shared memory that
    //  way other processors can access and sum up their number of page faults
    //  to have the total number of page faults overall.
    getline(check, line);
    if (line == "")
    {
        page_faults_total = to_string(page_faults);
    }
    else
    {
        page_faults_total = to_string(stoi(line) + page_faults);
    }
    
    s = to_string(n) + "\n" + s + page_faults_total;
    
    
    memcpy(pmem, s.c_str(), 256);
}

void startDisk(int sid, string main_addr, string disk_addr)
{
    semop(sid, &sb_wait, 1);        //  semaphore wait
    
    //  This function will mimic the process of page swapping. It will either swap a page from disk to
    //  the main memory or swap a page from main memory to the disk.
    
    if (disk_addr != "" && find(disk.begin(), disk.end(), disk_addr) != disk.end())
    {
        disk.push_back(disk_addr);
    }
    
    string s{};
    if (*pmem == '\0')
    {
        s = "1\n" + main_addr + "\n";
    }
    else
    {
        stringstream check(pmem);
        string line;
        getline(check, line);
        int n = stoi(line);
        while (getline(check, line))
        {
            if (disk_addr != line && line != main_addr)
            {
                s += line + "\n";
            }
            else
                n--;
        }
        
        if (main_addr != "") {
            s = to_string(n+1) + "\n" + s + main_addr + "\n";
        }
        else
            s = to_string(n) + "\n" + s;
    }
    
    memcpy(pmem, s.c_str(), 500);
    
    semop(sid, &sb_signal, 1);  //  semaphore signal
}

