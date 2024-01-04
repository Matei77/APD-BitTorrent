#include "peer.h"

#include <mpi.h>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <unordered_map>

using namespace std;


void Peer::init() {
	ifstream fin("in" + to_string(rank_) + ".txt");
	int segment_nr;
	string file_name;
	string segment_hash;

	fin >> files_owned_nr_;
	for (int i = 0; i < files_owned_nr_; i++) {
		fin >> file_name >> segment_nr;
		vector<string> segments;
		for (int j = 0; j < segment_nr; j++) {
			fin >> segment_hash;
			segments.emplace_back(segment_hash);
		}

		owned_files_.emplace(file_name, segments);
	}

	fin >> files_wanted_nr_;
	for (int i = 0; i < files_wanted_nr_; i++) {
		fin >> file_name;
		wanted_files_.emplace_back(file_name);
	}
}

void Peer::save_file(string file_name) {}

void Peer::download_thread_func(int rank) {}

void Peer::upload_thread_func(int rank) {}


void Peer::start() {
	thread download_thread(&Peer::download_thread_func, this, rank_);
	thread upload_thread(&Peer::upload_thread_func, this, rank_);

	download_thread.join();
	upload_thread.join();
}
