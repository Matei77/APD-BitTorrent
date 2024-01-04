#pragma once

#include <mpi.h>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <unordered_map>

using namespace std;

class Peer {
private:
	int numtasks_;
	int rank_;
	int files_owned_nr_;
	int files_wanted_nr_;
	unordered_map<string, vector<string>> owned_files_;
	vector<string> wanted_files_;
	int upload_load_;


	void init();
	
	void save_file(string file_name);
	
	void download_thread_func(int rank);

    void upload_thread_func(int rank);


public:
	Peer(int numtasks, int rank) : numtasks_(numtasks), rank_(rank), upload_load_(0) { init(); }

	void start();
};