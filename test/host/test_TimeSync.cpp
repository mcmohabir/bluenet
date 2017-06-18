/*
 * Author: Bart van Vliet
 * Copyright: Crownstone B.V. (https://crownstone.rocks)
 * Date: Jun 16, 2017
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "processing/cs_TimeSync.h"

#include <iostream>
#include <string.h> // Required for memset, memcpy, memcmp

using namespace std;

#define NUM_NODES 4

TimeSync timesync;

//! Print all the things! (requires all private stuff to be public)
void printNodes() {
	for (auto i=0; i<timesync._nodeListSize; ++i) {
		cout << "node [";
		for (auto j=0; j<sizeof(node_id_t); ++j) {
			cout << (int)timesync._nodeList[i].id.addr[j] << " ";
		}
		cout << "]";
		cout << " val=" << timesync._nodeList[i].timestampDiff;
		cout << " outlier=" << timesync._nodeList[i].isOutlier;
		cout << endl;
	}
}

//! Calculates mean, std, and marks an outlier.
//! Requires all private stuff to be public)
bool verboseStep() {
	timesync.calcMean();
	timesync.calcStd();

	bool foundOutlier = timesync.markLargestOutlier();
	cout << "mean=" << timesync._mean << " std=" << timesync._std << " tau=" << timesync._tau << endl;
	cout << "val=" << timesync._printVal << endl;
	printNodes();
	return foundOutlier;
}

int main() {
	cout << "Test TimeSync implementation" << endl;

	//! ids of other nodes
	node_id_t ids[NUM_NODES-1];
	for (auto i=0; i<NUM_NODES-1; ++i) {
		memset(&(ids[i]), i+1, sizeof(node_id_t));
		cout << "id " << i << ": [";
		for (auto j=0; j<sizeof(node_id_t); ++j) {
			cout << (int)ids[i].addr[j] << " ";
		}
		cout << "]" << endl;
	}

	timesync.updateNodeTime(&(ids[0]), 1);
	timesync.updateNodeTime(&(ids[1]), 2);
	timesync.updateNodeTime(&(ids[2]), 8);

//	printNodes();
//	while (verboseStep()) {
//
//	}



	int64_t adjustment = timesync.syncTime();

	printNodes();

	cout << "Adjustment: " << adjustment << endl;

}
