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

void Tracker::start() {
	// TODO: wait for file lists from clients
	// std::cout << "Tracker printing" << std::endl;
	for (int i = 1; i < numtasks_; i++) {
		peer_update(i);
	}

	// for (auto file : swarms) {
	// 	std::cout << file.first << std::endl;
	// 	for (auto segment : file.second) {
	// 		std::cout << segment.first.nr << " " << segment.first.hash << " ";
	// 		for (auto client : segment.second) {
	// 			std::cout << client << " ";
	// 		}
	// 		std::cout << std::endl;
	// 	}
	// 	std::cout << std::endl;
	// }

	// TODO: when received all lists, send ACK to all clients
	for (int i = 1; i < numtasks_; i++) {
		MPI_Send(&OK, 1, MPI_INT, i, TRACKER_TAG, MPI_COMM_WORLD);
	}
	
	while (downloads_completed_nr_ < numtasks_ - 1) {
		MPI_Status status;
		int type;
		MPI_Recv(&type, 1, MPI_INT, MPI_ANY_SOURCE, TRACKER_TAG, MPI_COMM_WORLD, &status);

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
	// TODO: check messages from clients:
		// TODO: 1. file request 
		// TODO: 2. client update 
		// TODO: 3. file downloaded 
		// TODO: 4. all files downloaded 

	// TODO: send stop to all clients when all files are downloaded by all clients
	for (int i = 1; i < numtasks_; i++) {
		// MPI_Send(&STOP_PEER, 1, MPI_INT, i, TRACKER_TAG, MPI_COMM_WORLD);
		MPI_Send(POISON_HASH.c_str(), HASH_SIZE, MPI_CHAR, i, DOWNLOAD_TAG, MPI_COMM_WORLD);
	}

}

void Tracker::file_request(int source) {
	char buf[MAX_FILENAME];
	int size;
	MPI_Recv(&size, 1, MPI_INT, source, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(buf, size, MPI_CHAR, source, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	std::string file_name(buf);

	std::string file_owners;
	std::string response;

	if (swarms.find(file_name) != swarms.end()) {
		std::unordered_map<Segment, std::vector<int>> segment_owners = swarms[file_name];
		response += std::to_string(segment_owners.size()) + "\n";

		for (auto segment : segment_owners) {
			response += std::to_string(segment.first.nr) + " ";
			response += segment.first.hash + "\n";
			response += std::to_string(segment.second.size()) + "\n";
			for (auto client : segment.second) {
				response += std::to_string(client) + "\n";
			}
		}
	}

	size = response.size();
	MPI_Send(&size, 1, MPI_INT, source, TRACKER_TAG, MPI_COMM_WORLD);
	MPI_Send(response.c_str(), response.size(), MPI_CHAR, source, TRACKER_TAG, MPI_COMM_WORLD);
}

void Tracker::peer_update(int source) {
	int size;
	MPI_Recv(&size, 1, MPI_INT, source, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	char buf[size + 1];
	MPI_Recv(buf, size, MPI_CHAR, source, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	std::string file_list(buf);
	parse_peer_file_list(file_list, source);
}

void Tracker::download_completed(int source) {
	int size;
	MPI_Recv(&size, 1, MPI_INT, source, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	char buf[size + 1];
	MPI_Recv(buf, size, MPI_CHAR, source, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	std::string file_name(buf);

	for (auto pair : swarms[file_name]) {
		// if the peer is not in the list of peers that have the segment, add it
		if (std::find(pair.second.begin(), pair.second.end(), source) == pair.second.end()) {
			pair.second.push_back(source);
		}
	}
}

void Tracker::all_downloads_completed(int source) {
	downloads_completed_nr_++;
}

void Tracker::parse_peer_file_list(std::string file_list, int rank) {
	std::stringstream ss(file_list);

	std::string file_name;
	std::string segment_hash;
	int files_nr;
	int segment_nr;


	ss >> files_nr;
	for (int i = 0; i < files_nr; i++) {
		ss >> file_name >> segment_nr;
		if (swarms.find(file_name) == swarms.end()) {
			swarms.emplace(file_name, std::unordered_map<Segment, std::vector<int>>());
		}
		std::unordered_map<Segment, std::vector<int>> segment_owners = swarms[file_name];

		for (int j = 1; j <= segment_nr; j++) {
			ss >> segment_hash;
			Segment segment(j, segment_hash);
			if (segment_owners.find(segment) == segment_owners.end()) {
				segment_owners.emplace(segment, std::vector<int>());
			}

			segment_owners.at(segment).push_back(rank);
		}

		swarms[file_name] = segment_owners;
	}
}
