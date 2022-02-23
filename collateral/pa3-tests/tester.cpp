#include <getopt.h>
#include <stdlib.h>

#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "BoundedBuffer.h"

#define CAP 5
#define SIZE 16
#define NUM 1

using namespace std;

// fill char buffer with random values
void make_word (char* buf, int size) {
    for (int i = 0; i < size; i++) {
        buf[i] = (rand() % 256) - 128;
    }
}

// mutex for synchronization of vector
mutex mtx;

// add element to vector
void add_word (vector<char*>* words, char* wrd) {
    mtx.lock();
    words->push_back(wrd);
    mtx.unlock();
}

// thread to push char buffer to BoundedBuffer
void push_thread_function (char* wrd, int size, BoundedBuffer* bb) {
    bb->push(wrd, size);
}

// thread to pop char buffer from BoundedBuffer
void pop_thread_function (int size, BoundedBuffer* bb, vector<char*>* words) {
    char* wrd = new char[size];
    int read = bb->pop(wrd, size);
    if (read != size) {
        delete[] wrd;
        return;
    }

    mtx.lock();
    for (vector<char*>::iterator iter = words->begin(); iter != words->end(); ++iter) {
        char* cur = *iter;
        bool equal = true;
        for (int i = 0; i < size; i++) {
            if (wrd[i] != cur[i]) {
                equal = false;
                break;
            }
        }
        if (equal) {
            words->erase(iter);
            delete[] cur;
            break;
        }
    }
    mtx.unlock();

    delete[] wrd;
}

int main (int argc, char** argv) {
    int bbcap = CAP;
    int wsize = SIZE;
    int nthrd = NUM;

    // CLI option to change BoundedBuffer capacity, word size, and number of threads
    int opt;
    static struct option long_options[] = {
        {"bbcap", required_argument, nullptr, 'b'},
        {"wsize", required_argument, nullptr, 's'},
        {"nthrd", required_argument, nullptr, 'n'},
        {0, 0, 0, 0}
    };
    while ((opt = getopt_long(argc, argv, "b:s:n:", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'b':
                bbcap = atoi(optarg);
                break;
            case 's':
                wsize = atoi(optarg);
                break;
            case 'n':
                nthrd = atoi(optarg);
                break;
        }
    }

    // initialize overhead
    srand(time(nullptr));

    BoundedBuffer bb(bbcap);

    thread** push_thrds = new thread*[nthrd];
    thread** pop_thrds = new thread*[nthrd];
    for (int i = 0; i < nthrd; i++) {
        push_thrds[i] = nullptr;
        pop_thrds[i] = nullptr;
    }

    vector<char*> words;
    size_t count = 0;

    // process commands to test
    string type;
    int thrd;
    while (cin >> type >> thrd) {
        if (type == "push") {
            char* wrd = new char[wsize];
            make_word(wrd, wsize);
            add_word(&words, wrd);
            push_thrds[thrd] = new thread(push_thread_function, wrd, wsize, &bb);
            count++;
        }
        else if (type == "pop") {
            pop_thrds[thrd] = new thread(pop_thread_function, wsize, &bb, &words);
            count--;
        }
    }

    // joining all threads
    for (int i = 0; i < nthrd; i++) {
        if (push_thrds[i] != nullptr) {
            push_thrds[i]->join();
        }
        delete push_thrds[i];
        
        if (pop_thrds[i] != nullptr) {
            pop_thrds[i]->join();
        }
        delete pop_thrds[i];
    }

    cerr << count << " " << words.size() << " " << bb.size() << endl;
    // determining exit status
    int status = 0;
    if (count != words.size() || count != bb.size()) {
        status = 1;
    }

    // clean-up head allocated memory
    for (auto wrd : words) {
        delete[] wrd;
    }
    delete[] push_thrds;
    delete[] pop_thrds;

    return status;
}
