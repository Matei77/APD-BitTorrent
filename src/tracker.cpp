// Copyright: Ionescu Matei-Stefan - 333CA - 2023-2024
#include "tracker.h"

#include <mpi.h>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>

using namespace std;

void Tracker::start() {
	// wait for file lists from clients
	for (int i = 1; i < numtasks_; i++) {
		peer_update(i);
	}

	// when received all lists, send OK to all clients
	for (int i = 1; i < numtasks_; i++) {
		MPI_Send(&OK, 1, MPI_INT, i, TRACKER_TAG, MPI_COMM_WORLD);
	}
	
	// wait for all clients to finish downloading
	while (downloads_completed_nr_ < numtasks_ - 1) {
		MPI_Status status;
		int type;
		MPI_Recv(&type, 1, MPI_INT, MPI_ANY_SOURCE, TRACKER_TAG, MPI_COMM_WORLD, &status);

		// check the type of message received from client
		switch(type) {
			case FILE_REQUEST:
				file_request(status.MPI_SOURCE);
				break;
			case PEER_UPDATE:
				peer_update(status.MPI_SOURCE);
				break;
			case DOWNLOAD_COMPLETED:
				download_completed(status.MPI_SOURCE);
				break;
			case ALL_DOWNLOADS_COMPLETED:
				all_downloads_completed(status.MPI_SOURCE);
				break;
		}
	}

	// when all clients finished downloading, notify them to close the upload thread by sending a poison value
	for (int i = 1; i < numtasks_; i++) {
		MPI_Send(POISON_HASH.c_str(), HASH_SIZE, MPI_CHAR, i, DOWNLOAD_TAG, MPI_COMM_WORLD);
	}

}

void Tracker::file_request(int source) {
	char buf[MAX_FILENAME];
	int size;

	// get the name of the requested file
	MPI_Recv(&size, 1, MPI_INT, source, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(buf, size, MPI_CHAR, source, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	string file_name(buf);

	string file_owners;
	string response;
	
	// if the file is in the list of files, build the response string with the list of seeds/peers
	if (swarms.find(file_name) != swarms.end()) {
		unordered_map<Segment, vector<int>> segment_owners = swarms[file_name];
		response += to_string(segment_owners.size()) + "\n";

		for (auto segment : segment_owners) {
			response += to_string(segment.first.nr) + " ";
			response += segment.first.hash + "\n";
			response += to_string(segment.second.size()) + "\n";
			for (auto client : segment.second) {
				response += to_string(client) + "\n";
			}
		}
	} else {
		response += "0\n";
	}

	// send the response to the client
	size = response.size();
	MPI_Send(&size, 1, MPI_INT, source, TRACKER_TAG, MPI_COMM_WORLD);
	MPI_Send(response.c_str(), response.size(), MPI_CHAR, source, TRACKER_TAG, MPI_COMM_WORLD);
}

void Tracker::peer_update(int source) {
	// get the list of owned files from the client
	int size;
	MPI_Recv(&size, 1, MPI_INT, source, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	char buf[size];
	MPI_Recv(buf, size, MPI_CHAR, source, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	
	// parse the list of owned files
	string file_list(buf);
	parse_peer_file_list(file_list, source);
}

void Tracker::download_completed(int source) {
	// get the name of the downloaded file that the client finished downloading
	int size;
	MPI_Recv(&size, 1, MPI_INT, source, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	char buf[size];
	MPI_Recv(buf, size, MPI_CHAR, source, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	string file_name(buf);

	// update the list of peers that have the file
	for (auto pair : swarms[file_name]) {
		// if the peer is not in the list of peers that have the segment, add it
		if (find(pair.second.begin(), pair.second.end(), source) == pair.second.end()) {
			pair.second.push_back(source);
		}
	}
}

void Tracker::all_downloads_completed(int source) {
	// update the number of clients that finished downloading all their files
	downloads_completed_nr_++;
}

void Tracker::parse_peer_file_list(string file_list, int rank) {
	stringstream ss(file_list);

	string file_name;
	string segment_hash;
	int files_nr;
	int segment_nr;

	// parse the list of owned files
	ss >> files_nr;
	for (int i = 0; i < files_nr; i++) {
		ss >> file_name >> segment_nr;
		if (swarms.find(file_name) == swarms.end()) {
			swarms.emplace(file_name, unordered_map<Segment, vector<int>>());
		}
		unordered_map<Segment, vector<int>> segment_owners = swarms[file_name];

		for (int j = 1; j <= segment_nr; j++) {
			ss >> segment_hash;
			Segment segment(j, segment_hash);
			if (segment_owners.find(segment) == segment_owners.end()) {
				segment_owners.emplace(segment, vector<int>());
			}

			segment_owners.at(segment).push_back(rank);
		}

		swarms[file_name] = segment_owners;
	}
}
