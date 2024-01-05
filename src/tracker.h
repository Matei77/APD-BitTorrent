#pragma once

#include <mpi.h>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <unordered_map>

#include "utils.h"

class Tracker {
private:
	int numtasks_;
	int rank_;
	int downloads_completed_nr_ = 0;
	// for each file store a list of segments, and for each segment store a list of clients that have that segment
	// unordered_map<file, unordered_map<segment, vector<client>>>
	std::unordered_map<std::string, std::unordered_map<Segment, std::vector<int>>> swarms;
	
	void parse_peer_file_list(std::string file_list, int rank);

public:
	Tracker(int numtasks, int rank) : numtasks_(numtasks), rank_(rank) {}

	void start();
	void file_request(int source);
	void peer_update(int source);
	void download_completed(int source);
	void all_downloads_completed(int source);
};