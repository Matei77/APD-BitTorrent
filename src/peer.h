// Copyright: Ionescu Matei-Stefan - 333CA - 2023-2024
#pragma once

#include <mpi.h>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>

#include "utils.h"

class Peer {
private:
	int numtasks_;
	int rank_;
	int files_owned_nr_;
	int files_wanted_nr_;
	std::unordered_map<std::string, std::vector<Segment>> owned_files_;
	std::vector<std::string> wanted_files_;
	int downloaded_segments_nr_;

	void download_thread_func();

    void upload_thread_func();
	
	void init();

	std::string send_file_request_to_tracker(std::string file_name);
	
	void send_peer_update_to_tracker();

	void send_download_completed_to_tracker(std::string file_name);

	void send_all_downloads_completed_to_tracker();
	
	void send_owned_files_to_tracker();
	
	std::unordered_map<Segment, std::vector<int>> parse_file_owners(std::string file_owners);
	
	void save_file(std::string file_name);

public:
	Peer(int numtasks, int rank) : numtasks_(numtasks),
								   rank_(rank),
								   downloaded_segments_nr_(0) {}

	void start();

};
