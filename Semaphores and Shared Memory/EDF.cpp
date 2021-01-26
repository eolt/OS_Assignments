//
//  main.cpp
//  HW_2
//
//  Created by Edgar Olvera on 11/5/20.
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <sstream>
#include <signal.h>
#include <fstream>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <cstring>
#include <sys/wait.h>

using namespace::std;

void readAgentInfo(ifstream&, int);
void scheduleTasks();
void executeTasks(int, char*);
int convertLetterToInt(char);
bool isOccupied(vector<string>, int, int);
char convertIntToLetter(int num);

void makeReserve(int, char*, string);
void makeWait(int, char*, string);
void makeWaitany(int, char*, string);
void makeTicket(int, char*, string);
void makeCancel(int, char*, string);
void makeCheck_Passenger(int, char*, string);


struct sembuf sb_wait;
struct sembuf sb_signal;

int processTime;
string agentName;
int reserve_time, wait_time, waitany_time;
int ticket_time, cancel_time, check_time;
vector<string> tasks;
vector<string> waitList;
vector<string> waitAnyList;
string** flights;
int numOfFlights;

int main(int argc, const char * argv[])
{
    if (argc != 2)
    {
        perror("input error");
        exit(0);
    }
    
    int numOfAgents;
    string buff;
    key_t key;
    int sid;
    int shmid;
    char* shmTime;
    
    key = 1425331;
    
    //  Create shared memory
    shmid = shmget(key, 26,  0666 | IPC_CREAT);
    
    shmTime = (char*)shmat(shmid, (char*) 0, 0);
    
    //  Create the semaphore and initialize it
    sid = semget(key, 1, 0666 | IPC_CREAT);
    
    //  initialize wait operation
    sb_wait.sem_num = 0;
    sb_wait.sem_op = -1;
    sb_wait.sem_flg = 0;
    
    //  initialize signal operatoin
    sb_signal.sem_num = 0;
    sb_signal.sem_op = 1;
    sb_signal.sem_flg = 0;
    semop(sid, &sb_signal, 1);

    ifstream myFile(argv[1]);
    
    //  We will read the first line of file that gives number of flights
    getline(myFile, buff);
    numOfFlights = stoi(buff);
    
    flights = new string*[numOfFlights];
    
    for (int i = 0; i < numOfFlights; i++)
    {
        flights[i] = new string[3];
    }
    
    //  We will read each of the flights and store in a 2d array
    for(int i = 0; i < numOfFlights; i++)
    {
        getline(myFile, buff);
        string str = "";
        int r = 0;
        for (int j = 0; j < buff.length(); j++)
        {
            if (buff.at(j) == ' ' || buff.at(j) == '\t')
            {
                flights[i][r] = str;
                str = "";
                r++;
            }
            else
                str += buff.at(j);
        }
        flights[i][r] = str;
    }
    
   
    //  We will read the number of agents in the file
    getline(myFile, buff);
    numOfAgents = stoi(buff);
    
    
    //  Now that we have the number of agents we can know the number of processes
    readAgentInfo(myFile, sid);
    int pid = 1;
    
    //  Loop to create a processor for each agent
    for (int i = 1; i < numOfAgents; i++) {
        pid = fork();                      //  fork to create a separate processor
        if (pid == 0)
        {
            readAgentInfo(myFile, sid);
        }
        if (pid > 0) {
            break;
        }
    }
    
    
    if (pid == 0)
    {
        scheduleTasks();
        executeTasks(sid, shmTime);
    }
    else
    {
        scheduleTasks();
        executeTasks(sid, shmTime);

        wait(NULL);     //  We wait that child process to finished
        
        myFile.close();     //  close file
        
        //  Remove and detach shared memory and semaphore
        shmdt(shmTime);
        shmctl(shmid, IPC_RMID, 0);
        semctl(sid, 0, IPC_RMID, 0);
    }
    
    return 0;
}

void readAgentInfo(ifstream& myfile, int sid)
{
    semop(sid, &sb_wait, 1);
    string line;
    
    //  Get agent name
    getline(myfile, line);
    agentName = line;
    
    //  Get agent times
    getline(myfile, line);
    ticket_time = stoi(line.substr(line.length() - 1, line.length()));
    getline(myfile, line);
    reserve_time = stoi(line.substr(line.length() - 1, line.length()));
    getline(myfile, line);
    wait_time = stoi(line.substr(line.length() - 1, line.length()));
    getline(myfile, line);
    waitany_time = stoi(line.substr(line.length() - 1, line.length()));
    getline(myfile, line);
    cancel_time = stoi(line.substr(line.length() - 1, line.length()));
    getline(myfile, line);
    check_time = stoi(line.substr(line.length() - 1, line.length()));
    
    tasks.clear();
    getline(myfile, line);
    while (line != "end.") {
        tasks.push_back(line);
        getline(myfile, line);
    }
    
    semop(sid, &sb_signal, 1);
}

void scheduleTasks()
{
    //  Sort tasks in vector to be scheduled in EDF form, uses bubble sort.
    
    for (int i = 0; i < tasks.size(); i++)
    {
        string line = tasks.at(i);
        if (line[line.length()-1] == ' ') {
            line.pop_back();
        }
        stringstream check(line);
        while (getline(check, line, ' ')){}
        int prevDeadline = stoi(line);
        
        for (int j = i + 1; j < tasks.size(); j++)
        {
            string line = tasks.at(j);
            if (line[line.length()-1] == ' ')
            {
                line.pop_back();
            }
            stringstream check(line);
            while (getline(check, line, ' ')){}
            int currDeadline = stoi(line);
            
            if (prevDeadline > currDeadline)
            {
                string tempTask = tasks.at(i);
                tasks.at(i) = tasks.at(j);
                tasks.at(j) = tempTask;
            }
        }
    }
}

void executeTasks(int sid, char* shmTime)
{
    string commands = "";
    while (!tasks.empty())
    {
        string line = tasks.front();
        stringstream check(line);
        getline(check, commands, ' ');
        if (commands == "reserve")
        {
            getline(check, commands);
            makeReserve(sid, shmTime, commands);
        }
        if(commands == "wait")
        {
            getline(check, commands);
            makeWait(sid, shmTime, commands);
        }
        if (commands == "waitany")
        {
            getline(check, commands);
            makeWaitany(sid, shmTime, commands);
        }
        
        if (commands == "ticket")
        {
            getline(check, commands);
            makeTicket(sid, shmTime, commands);
        }
        if (commands == "cancel")
        {
            getline(check, commands);
            makeCancel(sid, shmTime, commands);
        }
        if (commands == "check_passenger")
        {
            getline(check, commands);
            makeCheck_Passenger(sid, shmTime, commands);
        }
        tasks.erase(tasks.begin());
    }
    
    
    if (!waitList.empty()) {
        string line;
        string nextStr;
        string s;
        for (auto wait : waitList) {
            bool isreserved = false;
            
            stringstream checkW(wait);
            getline(checkW, line, ' ');
            string person_name = line;
            getline(checkW, line, ' ');
            string flight_number = line;
            getline(checkW, line, ' ');
            string flight_seat = line;
            
            stringstream check2(shmTime);
            string line2;
            getline(check2, line, ' ');
            processTime = stoi(line);
            
            while(getline(check2, line, ' ')){
                
                if (flight_number == line)
                {
                    getline(check2, line, ' ');
                    if (flight_seat == line) {
                        cout << person_name << " fails to reserve " << flight_number << " " << flight_seat << " because it is occupied." << endl;
                        isreserved = true;
                    }
                    nextStr += " " + flight_number + " " + line;
                }
                else
                    nextStr += " " + line;
            }
            
            if (!isreserved)
            {
                cout << person_name << " has reserved " << flight_number << " " << flight_seat << " at time " << processTime << "(automatically resulting from wait.)" << endl;
                
                s = to_string(processTime) + nextStr + " " + flight_number + " " + flight_seat + " ";
                
            }
            else
                s = to_string(processTime) + nextStr + " ";
            
        }
        char const *ms = s.c_str();
        memcpy(shmTime, ms, 256);
    }
    
    if(!waitAnyList.empty()){
        string nextStr;
        string line;
        string s;
        vector<string> seatsReserved;
        for (auto waitany : waitAnyList) {
            bool isreserved = false;
            
            stringstream checkW(waitany);
            getline(checkW, line, ' ');
            string person_name = line;
            getline(checkW, line, ' ');
            string flight_number = line;
            getline(checkW, line, ' ');
            string flight_seat = line;
            
            stringstream check2(shmTime);
            string line2;
            getline(check2, line, ' ');
            processTime = stoi(line);
            
            while(getline(check2, line, ' ')){
                
                if (flight_number == line)
                {
                    getline(check2, line, ' ');
                    if (flight_seat == line) {
                        isreserved = true;
                    }
                    nextStr += " " + flight_number + " " + line;
                    seatsReserved.push_back(flight_seat);
                }
                else
                    nextStr += " " + line;
            }
            
           
            if (!isreserved) {
                cout << person_name << " has reserved " << flight_number << " " << flight_seat << " at time " << processTime << "(automatically resulting from waitany.)" << endl;
                
                s = to_string(processTime) + nextStr + " " + flight_number + " " + flight_seat + " ";
            }
            else{
                bool foundSeat = false;
                int assignedRow = 0;
                int assignedSeat = 0;
                
                for (int i=0; i < numOfFlights; i++)
                {
                    if (flights[i][0] == flight_number)
                    {
                        foundSeat = true;
                        int number_of_rows = stoi(flights[i][1]);
                        int number_of_seats = stoi(flights[i][2]);
                        
                        
                        assignedRow = number_of_rows;
                        assignedSeat = number_of_seats;
                        
                        while (isOccupied(seatsReserved, assignedRow, assignedSeat))
                        {
                            if (assignedRow == 0 || assignedSeat == 0)
                            {
                                foundSeat = false;
                                break;
                            }
                            assignedRow--;
                            assignedSeat--;
                        }
                        break;
                    }
                }
                
                if (foundSeat)
                {
                    cout << person_name << " has reserved " << flight_number << " " << to_string(assignedRow) + convertIntToLetter(assignedSeat) << " at time " << processTime << "(automatically resulting from waitany.)" << endl;
                    s = to_string(processTime) + nextStr + " " + flight_number + " " + to_string(assignedRow) + convertIntToLetter(assignedSeat) + " ";
                }
                else{
                    cout << person_name << " fails to reserve " << flight_number << " " << flight_seat << " because it is occupied." << endl;
                }
                
            }
        }
        char const *ms = s.c_str();
        memcpy(shmTime, ms, 256);
    }
    
    cout << agentName << " exits";

    cout << endl;
}

void makeReserve(int sid, char* shmTime, string line)
{
    semop(sid, &sb_wait, 1);
    
    bool reserved = false;
    
    stringstream check(line);
    string buffer;
    getline(check, buffer, ' ');
    string flight_number = buffer;
    getline(check, buffer, ' ');
    string seat_number = buffer;
    getline(check, buffer, ' ');
    string person_name = buffer;
    getline(check, buffer, ' ');
    getline(check, buffer, ' ');
    int deadline = stoi(buffer);
    
    for (int i = 0; i < numOfFlights; i++)
    {
        if (flights[i][0] == flight_number)
        {
            string row = flights[i][1];
            string seats = flights[i][2];
            
            if (stoi(row) > stoi(&seat_number.at(0)) && stoi(seats) > convertLetterToInt(seat_number[1]))
            {
                reserved = true;
            }
            break;
        }
    }
        
    sleep(reserve_time);
    string s;
    if (*shmTime == '\0')
    {
        processTime += reserve_time;
        s = to_string(processTime) + " " + flight_number + " " + seat_number + " ";
    }
    else
    {
        stringstream check(shmTime);
        string line;
        getline(check, line, ' ');
        
        processTime = stoi(line);
        processTime += reserve_time;
        
        string nextStr = "";
        while (getline(check, line, ' ')) {
            if (flight_number == line) {
                getline(check, line, ' ');
                if (seat_number == line)
                {
                    reserved = false;
                }
                nextStr += + " " + flight_number + " " + line;
            }
            else
                nextStr += " " + line;
        }
        
        if(reserved)
            s = to_string(processTime) + nextStr + " " + flight_number + " " + seat_number + " ";
        else
            s = to_string(processTime) + nextStr + " ";
    }
    
    
    char const *ms = s.c_str();
    memcpy(shmTime, ms, 256);
    
    if(reserved)
    {
        cout << person_name << " reserves " << flight_number << " " << seat_number << " successfully at time " << processTime;
        
        if (processTime < deadline)
        {
            cout << ", deadline met." << endl;
        }
        else
        {
            cout << ", deadline missed." << endl;
            
        }
    }
    else
        cout << person_name << " fails to reserve " << flight_number << " " << seat_number << " because it is occupied." << endl;
    
    
    
    semop(sid, &sb_signal, 1);
}

void makeWait(int sid, char* shmTime, string line)
{
    semop(sid, &sb_wait, 1);
        
    stringstream check(line);
    string buffer;
    
    getline(check, buffer, ' ');
    string flight_number = buffer;
    getline(check, buffer, ' ');
    string seat_number = buffer;
    getline(check, buffer, ' ');
    string person_name = buffer;
    getline(check, buffer, ' ');
    getline(check, buffer, ' ');
    int deadline = stoi(buffer);
    
    for (int i = 0; i < numOfFlights; i++)
    {
        if (flights[i][0] == flight_number)
        {
            string row = flights[i][1];
            string seats = flights[i][2];
            
            if (stoi(row) > stoi(&seat_number.at(0)) && stoi(seats) > convertLetterToInt(seat_number[1]))
            {
                string str = person_name + " " + flight_number + " " + seat_number;
                waitList.push_back(str);
            }
            break;
        }
    }
    
    sleep(wait_time);
    
    string s;
    
    if (*shmTime == '\0')
    {
        processTime += wait_time;
        
        s = to_string(processTime) + " ";
    }
    else{
        
        stringstream check(shmTime);
        string line;
        getline(check, line, ' ');
        
        processTime = stoi(line);
        processTime += wait_time;
        
        string nextStr = "";
        while (getline(check, line)) {
            nextStr += line;
        }
        
        s = to_string(processTime) + " " + nextStr;
    }
    
    char const *ms = s.c_str();
    memcpy(shmTime, ms, 256);
    
    cout << person_name << " waits on " << flight_number << " " << seat_number << " successfully at time " << processTime;
        
    if (processTime < deadline)
    {
        cout << ", deadline met." << endl;
    }
    else
    {
        cout << ", deadline missed." << endl;
    }

    semop(sid, &sb_signal, 1);
}

void makeWaitany(int sid, char* shmTime, string line)
{
    semop(sid, &sb_wait, 1);
    
    stringstream check(line);
    string buffer;
    
    getline(check, buffer, ' ');
    string flight_number = buffer;
    getline(check, buffer, ' ');
    string seat_number = buffer;
    getline(check, buffer, ' ');
    string person_name = buffer;
    getline(check, buffer, ' ');
    getline(check, buffer, ' ');
    int deadline = stoi(buffer);
            
    for (int i = 0; i < numOfFlights; i++)
    {
        if (flights[i][0] == flight_number)
        {
            string row = flights[i][1];
            string seats = flights[i][2];
            
            if (stoi(row) > stoi(&seat_number.at(0)) && stoi(seats) > convertLetterToInt(seat_number[1]))
            {
                string str = person_name + " " + flight_number + " " + seat_number;
                waitAnyList.push_back(str);
            }
            break;
        }
    }
    
    sleep(waitany_time);
    string s;
    
    if (*shmTime == '\0')
    {
        processTime += waitany_time;
        s = to_string(processTime) + " ";
    }
    else
    {
        stringstream check(shmTime);
        string line;
        getline(check, line, ' ');
        
        processTime = stoi(line);
        processTime += waitany_time;
        
        string nextStr = "";
        while (getline(check, line)) {
            nextStr += line;
        }
        
        s = to_string(processTime) + " " + nextStr;
    }
    
    char const *ms = s.c_str();
    memcpy(shmTime, ms, 256);
    
    cout << person_name << " waitany on " << flight_number << " " << seat_number << " at time " << processTime;
    if (processTime < deadline)
    {
        cout << ", deadline met." << endl;
    }
    else
    {
        cout << ", deadline missed." << endl;
        
    }
    
    semop(sid, &sb_signal, 1);
}

void makeTicket(int sid, char* shmTime, string line)
{
    semop(sid, &sb_wait, 1);
        
    bool ticket = false;
    
    stringstream check(line);
    string buffer;
    
    getline(check, buffer, ' ');
    string flight_number = buffer;
    getline(check, buffer, ' ');
    string seat_number = buffer;
    getline(check, buffer, ' ');
    string person_name = buffer;
    getline(check, buffer, ' ');
    getline(check, buffer, ' ');
    int deadline = stoi(buffer);
    
    for (int i = 0; i < numOfFlights; i++)
    {
        if (flights[i][0] == flight_number)
        {
            string row = flights[i][1];
            string seats = flights[i][2];
            
            if (stoi(row) > stoi(&seat_number.at(0)) && stoi(seats) > convertLetterToInt(seat_number[1]))
            {
                ticket = true;
            }
            break;
        }
    }
    
    sleep(ticket_time);
    string s;
    
    if (*shmTime == '\0')
    {
        processTime += ticket_time;
        s = to_string(processTime) + " " + flight_number + " " + seat_number + " ";
    }
    else
    {
        stringstream check(shmTime);
        string line;
        getline(check, line, ' ');
        
        processTime = stoi(line);
        processTime += ticket_time;
        
        string nextStr = "";
        while (getline(check, line, ' ')) {
            if (flight_number == line) {
                getline(check, line, ' ');
                if (seat_number == line)
                {
                    ticket = false;
                }
                nextStr += + " " + flight_number + " " + line;
            }
            else
                nextStr += " " + line;
        }
        
        if(ticket)
            s = to_string(processTime) + nextStr + " " + flight_number + " " + seat_number + " ";
        else
            s = to_string(processTime) + nextStr + " ";
    }
    
    char const *ms = s.c_str();
    memcpy(shmTime, ms, 256);
    
    if (ticket)
    {
        cout << person_name << " tickets " << flight_number << " " << seat_number << " successfully at time " << processTime;
        
        if (processTime < deadline)
        {
            cout << ", deadline met." << endl;
        }
        else
        {
            cout << ", deadline missed." << endl;
        }
    }
    else
        cout << person_name << " fail to ticket " << flight_number << " " << seat_number << " because it is occupied." << endl;
   
    semop(sid, &sb_signal, 1);
}

void makeCancel(int sid, char* shmTime, string line)
{
    semop(sid, &sb_wait, 1);
    
    stringstream check(line);
    string buffer;
    
    getline(check, buffer, ' ');
    string flight_number = buffer;
    getline(check, buffer, ' ');
    string seat_number = buffer;
    getline(check, buffer, ' ');
    string person_name = buffer;
    getline(check, buffer, ' ');
    getline(check, buffer, ' ');
    int deadline = stoi(buffer);
    
    string seat;
  
    sleep(cancel_time);
    string s;
    
    if (*shmTime == '\0')
    {
        processTime += cancel_time;
        s = to_string(processTime) + " ";
    }
    else{
        stringstream check(shmTime);
        string line;
        getline(check, line, ' ');
        
        processTime = stoi(line);
        processTime += cancel_time;
        
        string nextStr = "";
        while (getline(check, line, ' ')) {
            if (flight_number == line) {
                getline(check, line, ' ');
                if (seat_number == line)
                {
                    continue;
                }
                nextStr += + " " + flight_number + " " + line;
            }
            else
                nextStr += " " + line;
        }
        
        s = to_string(processTime) + nextStr;
    }
    
    char const *ms = s.c_str();
    memcpy(shmTime, ms, 256);
    
    cout << person_name << " cancels " << flight_number << " " << seat_number << " at time " << processTime;
    
    if (processTime < deadline)
    {
        cout << ", deadline met." << endl;
    }
    else
    {
        cout << ", deadline missed." << endl;
        
    }
    
    semop(sid, &sb_signal, 1);
}

void makeCheck_Passenger(int sid, char* shmTime, string line)
{
    semop(sid, &sb_wait, 1);
    
    stringstream check(line);
    string buffer;
    
    getline(check, buffer, ' ');
    string person_name = buffer;
    getline(check, buffer, ' ');
    getline(check, buffer, ' ');
    int deadline = stoi(buffer);
    
    sleep(check_time);
    
    if (*shmTime == '\0')
    {
        processTime += check_time;
    }
    else
    {
        stringstream check(shmTime);
        string line;
        getline(check, line, ' ');
        
        processTime = stoi(line);
        processTime += check_time;
    }
    
    cout << person_name << " does check at time " << processTime;
    
    if (processTime < deadline)
    {
        cout << ", deadline met." << endl;
    }
    else
    {
        cout << ", deadline missed." << endl;
        
    }
    
    semop(sid, &sb_signal, 1);
}

int convertLetterToInt(char letter){
    switch (letter) {
        case 'A':
            return 1;
            break;
        case 'B':
            return 2;
            break;
        case 'C':
            return 3;
            break;
        case 'D':
            return 4;
            break;
        case 'E':
            return 5;
            break;
        case 'F':
            return 6;
            break;
        case 'G':
            return 7;
            break;
        case 'H':
            return 8;
            break;
        case 'I':
            return 9;
            break;
        case 'J':
            return 10;
            break;
        default:
            break;
    }
    return 0;
}

char convertIntToLetter(int num)
{
    switch (num) {
        case 1:
            return 'A';
            break;
        case 2:
            return 'B';
            break;
        case 3:
            return 'C';
            break;
        case 4:
            return 'D';
            break;
        case 5:
            return 'E';
            break;
        case 6:
            return 'F';
            break;
        case 7:
            return 'G';
            break;
        case 8:
            return 'H';
            break;
        case 9:
            return 'I';
            break;
        case 10:
            return 'J';
            break;
        default:
            break;
    }
    return '\0';
}


bool isOccupied(vector<string> seats, int assignedRow, int assignedSeat)
{
    for (auto s: seats)
    {
        if (assignedRow == stoi(&s.at(0)) && assignedSeat == convertLetterToInt(s.at(1)) ) {
            return true;
        }
    }
    
    return false;
}



